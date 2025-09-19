// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "FadasRemap.hpp"

/* These are temporary used macro definations for some image formats supported in engineering build
 * fadas library but not exist in formal release header files, will be removed when engineering
 * build merged to mainline*/
#define FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888_RH ( (FadasRemapPipeline_e) 12 )
#define FADAS_REMAP_PIPELINE_UYVY_TO_BGR888_RH ( (FadasRemapPipeline_e) 16 )
#define FADAS_REMAP_PIPELINE_UBWC_NV12_TO_BGR888_RH ( (FadasRemapPipeline_e) 17 )
#define FADAS_REMAP_PIPELINE_MAX_RH ( (FadasRemapPipeline_e) 19 )
#define FADAS_IMAGE_FORMAT_BGR888_RH ( (FadasImageFormat_e) 14 )
#define FADAS_IMAGE_FORMAT_UBWC_NV12_RH ( (FadasImageFormat_e) 15 )

namespace QC
{
namespace libs
{
namespace FadasIface
{

FadasRemap::FadasRemap() {}

FadasRemap::~FadasRemap() {}

QCStatus_e FadasRemap::SetRemapParams( uint32_t numOfInputs, uint32_t outputWidth,
                                       uint32_t outputHeight, QCImageFormat_e outputFormat,
                                       FadasNormlzParams_t normlzR, FadasNormlzParams_t normlzG,
                                       FadasNormlzParams_t normlzB, bool bEnableUndistortion,
                                       bool bEnableNormalize )
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( int i = 0; i < QC_MAX_INPUTS; i++ )
    {
        m_workerPtrsDSP[i] = 0;
        m_remapPtrsDSP[i] = 0;
        m_workerPtrsCPU[i] = nullptr;
        m_remapPtrsCPU[i] = nullptr;
    }

    if ( ( QC_IMAGE_FORMAT_RGB888 != outputFormat ) && ( QC_IMAGE_FORMAT_BGR888 != outputFormat ) )
    {
        QC_ERROR( "Invalid output format!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( QC_MAX_INPUTS <= numOfInputs )
    {
        QC_ERROR( "Invalid number of inputs!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        m_handle64 = GetRemoteHandle64();
        m_numOfInputs = numOfInputs;
        m_outputFormat = outputFormat;
        m_outputWidth = outputWidth;
        m_outputHeight = outputHeight;
        m_normlz[0] = normlzR;
        m_normlz[1] = normlzG;
        m_normlz[2] = normlzB;
        m_bEnableUndistortion = bEnableUndistortion;
        m_bEnableNormalize = bEnableNormalize;
    }

    return ret;
};

FadasRemapPipeline_e FadasRemap::RemapGetPipelineCPU( QCImageFormat_e inputFormat,
                                                      QCImageFormat_e outputFormat,
                                                      bool bEnableNormalize )
{
    FadasRemapPipeline_e pipeline = FADAS_REMAP_PIPELINE_MAX_RH;

    if ( ( QC_IMAGE_FORMAT_UYVY == inputFormat ) && ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
         ( true == bEnableNormalize ) )   // UYVY to RGB normalize pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8;
    }
    else if ( ( QC_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to RGB pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888;
    }
    else if ( ( QC_IMAGE_FORMAT_RGB888 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // RGB to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_3C888;
    }
    else if ( ( QC_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( QC_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to BGR pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_BGR888_RH;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( true == bEnableNormalize ) )   // NV12 to RGB normalize pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to BGR pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888_RH;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12_UBWC == inputFormat ) &&
              ( QC_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 UBWC to BGR pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_UBWC_NV12_TO_BGR888_RH;
    }
    else
    {
        QC_ERROR( "Invalid remap pipeline" );
    }

    return pipeline;
}

FadasIface_FadasRemapPipeline_e FadasRemap::RemapGetPipelineDSP( QCImageFormat_e inputFormat,
                                                                 QCImageFormat_e outputFormat,
                                                                 bool bEnableNormalize )
{
    FadasIface_FadasRemapPipeline_e pipeline = FADAS_REMAP_PIPELINE_MAX_NSP;

    if ( ( QC_IMAGE_FORMAT_UYVY == inputFormat ) && ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
         ( true == bEnableNormalize ) )   // UYVY to RGB normalize pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8_NSP;
    }
    else if ( ( QC_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to RGB pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NSP;
    }
    else if ( ( QC_IMAGE_FORMAT_RGB888 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // RGB to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_3C888_NSP;
    }
    else if ( ( QC_IMAGE_FORMAT_UYVY == inputFormat ) &&
              ( QC_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // UYVY to BGR pipeline
    {
        pipeline = FADAS_REMAP_PIPELINE_UYVY_TO_BGR888_NSP;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to RGB pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NSP;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_RGB888 == outputFormat ) &&
              ( true == bEnableNormalize ) )   // NV12 to RGB normalize pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8_NSP;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == inputFormat ) &&
              ( QC_IMAGE_FORMAT_BGR888 == outputFormat ) &&
              ( false == bEnableNormalize ) )   // NV12 to BGR pipeline
    {

        pipeline = FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888_NSP;
    }
    else
    {
        QC_ERROR( "Invalid remap pipeline for inputformat = %d, outputfprmat = %d, "
                  "bEnableNormalize = %d ",
                  inputFormat, outputFormat, bEnableNormalize );
    }

    return pipeline;
}

QCStatus_e FadasRemap::CreatRemapTable( uint32_t inputId, uint32_t mapWidth, uint32_t mapHeight,
                                        const TensorDescriptor_t &bufDescMapX,
                                        const TensorDescriptor_t &bufDescMapY )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_mapWidths[inputId] = mapWidth;
    m_mapHeights[inputId] = mapHeight;

    if ( ( true == m_bEnableUndistortion ) &&
         ( ( QC_BUFFER_TYPE_TENSOR != bufDescMapX.type ) || ( nullptr == bufDescMapX.pBuf ) ) )
    {
        QC_ERROR( "Invalid mapX buffer desc!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( true == m_bEnableUndistortion ) &&
              ( ( QC_BUFFER_TYPE_TENSOR != bufDescMapY.type ) || ( nullptr == bufDescMapY.pBuf ) ) )
    {
        QC_ERROR( "Invalid mapY buffer desc!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( true == m_bEnableUndistortion ) &&
              ( ( QC_TENSOR_TYPE_FLOAT_32 != bufDescMapX.tensorType ) ||
                ( 2 != bufDescMapX.numDims ) || ( mapWidth != bufDescMapX.dims[0] ) ||
                ( mapHeight != bufDescMapX.dims[1] ) ) )
    {
        QC_ERROR( "mapX buffer desc size not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( true == m_bEnableUndistortion ) &&
              ( ( QC_TENSOR_TYPE_FLOAT_32 != bufDescMapY.tensorType ) ||
                ( 2 != bufDescMapY.numDims ) || ( mapWidth != bufDescMapY.dims[0] ) ||
                ( mapHeight != bufDescMapY.dims[1] ) ) )
    {
        QC_ERROR( "mapY buffer desc size not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
        {
            FadasIface_FadasRemapPipeline_e pipeline = RemapGetPipelineDSP(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX_NSP == pipeline )
            {
                QC_ERROR( "Invalid remap pipelie for DSP!" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                uint64 remapPtr = 0;
                uint64 retVal = 0;
                if ( true == m_bEnableUndistortion )
                {
                    int32_t mapXFd = RegBuf( bufDescMapX, FADAS_BUF_TYPE_IN );
                    int32_t mapYFd = RegBuf( bufDescMapY, FADAS_BUF_TYPE_IN );
                    retVal = FadasIface_FadasRemap_CreateMapFromMap(
                            m_handle64, &remapPtr, m_inputWidths[inputId], m_inputHeights[inputId],
                            m_mapWidths[inputId], m_mapHeights[inputId], mapXFd, mapYFd,
                            m_mapWidths[inputId] * sizeof( float ), pipeline, 0 );
                    DeregBuf( bufDescMapX.pBuf );
                    DeregBuf( bufDescMapY.pBuf );
                }
                else
                {
                    retVal = FadasIface_FadasRemap_CreateMapNoUndistortion(
                            m_handle64, &remapPtr, m_inputWidths[inputId], m_inputHeights[inputId],
                            m_mapWidths[inputId], m_mapHeights[inputId], pipeline, 0 );
                }

                if ( AEE_SUCCESS != retVal )
                {
                    QC_ERROR( "Failed to create a remap map for DSP!" );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    m_remapPtrsDSP[inputId] = remapPtr;
                }
            }
        }
        else if ( QC_PROCESSOR_GPU == m_processor )
        {
            FadasRemapPipeline_e pipeline = RemapGetPipelineCPU(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX_RH == pipeline )
            {
                QC_ERROR( "Invalid remap pipelie for CPU!" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                FadasRemapMap_t *remapPtr = nullptr;
                if ( true == m_bEnableUndistortion )
                {
                    remapPtr = s_FadasRemap_CreateMapFromMapGPU(
                            m_inputWidths[inputId], m_inputHeights[inputId], m_mapWidths[inputId],
                            m_mapHeights[inputId], m_mapWidths[inputId] * sizeof( float ),
                            static_cast<float *>( bufDescMapX.pBuf ),
                            static_cast<float *>( bufDescMapY.pBuf ), pipeline, 0, 1 );
                }
                else
                {
                    remapPtr = s_FadasRemap_CreateMapNoUndistortionGPU(
                            m_inputWidths[inputId], m_inputHeights[inputId], m_mapWidths[inputId],
                            m_mapHeights[inputId], pipeline, 0 );
                }

                if ( remapPtr == nullptr )
                {
                    QC_ERROR( "Failed to create a remap map for CPU!" );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    m_remapPtrsCPU[inputId] = remapPtr;
                }
            }
        }
        else
        {
            FadasRemapPipeline_e pipeline = RemapGetPipelineCPU(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX_RH == pipeline )
            {
                QC_ERROR( "Invalid remap pipelie for GPU!" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                FadasRemapMap_t *remapPtr = nullptr;
                if ( true == m_bEnableUndistortion )
                {
                    remapPtr = FadasRemap_CreateMapFromMap(
                            m_inputWidths[inputId], m_inputHeights[inputId], m_mapWidths[inputId],
                            m_mapHeights[inputId], m_mapWidths[inputId] * sizeof( float ),
                            static_cast<float *>( bufDescMapX.pBuf ),
                            static_cast<float *>( bufDescMapY.pBuf ), pipeline, 0 );
                }
                else
                {
                    remapPtr = FadasRemap_CreateMapNoUndistortion(
                            m_inputWidths[inputId], m_inputHeights[inputId], m_mapWidths[inputId],
                            m_mapHeights[inputId], pipeline, 0 );
                }

                if ( remapPtr == nullptr )
                {
                    QC_ERROR( "Failed to create a remap map for CPU!" );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    m_remapPtrsCPU[inputId] = remapPtr;
                }
            }
        }
    }

    return ret;
}

QCStatus_e FadasRemap::CreateRemapWorker( uint32_t inputId, QCImageFormat_e inputFormat,
                                          uint32_t inputWidth, uint32_t inputHeight,
                                          FadasROI_t ROI )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_inputFormats[inputId] = inputFormat;
    m_inputWidths[inputId] = inputWidth;
    m_inputHeights[inputId] = inputHeight;
    m_ROIs[inputId] = ROI;

    if ( ( QC_IMAGE_FORMAT_UYVY != m_inputFormats[inputId] ) &&
         ( QC_IMAGE_FORMAT_RGB888 != m_inputFormats[inputId] ) &&
         ( QC_IMAGE_FORMAT_NV12 != m_inputFormats[inputId] ) &&
         ( QC_IMAGE_FORMAT_NV12_UBWC != m_inputFormats[inputId] ) )
    {
        QC_ERROR( "Invalid input format for inputId = %d ", inputId );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
        {
            FadasIface_FadasRemapPipeline_e pipeline = RemapGetPipelineDSP(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX_NSP == pipeline )
            {
                QC_ERROR( "Invalid remap pipelie for DSP!" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                uint64 workerPtr = 0;
                auto retVal =
                        FadasIface_FadasRemap_CreateWorkers( m_handle64, &workerPtr, 4, pipeline );
                if ( AEE_SUCCESS != retVal )
                {
                    QC_ERROR( "Failed to create a remap worker for DSP!" );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    m_workerPtrsDSP[inputId] = workerPtr;
                }
            }
        }
        else if ( QC_PROCESSOR_GPU == m_processor )
        {
            /*fadas GPU not support multiple threads, no need to create workers*/
        }
        else
        {
            int32_t pThreadsAffinity[] = { 0, 1, 2, 3 };
            FadasRemapPipeline_e pipeline = RemapGetPipelineCPU(
                    m_inputFormats[inputId], m_outputFormat, m_bEnableNormalize );
            if ( FADAS_REMAP_PIPELINE_MAX_RH == pipeline )
            {
                QC_ERROR( "Invalid remap pipelie for CPU!" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                void *workerPtr = FadasRemap_CreateWorkers( 4, pThreadsAffinity, pipeline );
                if ( workerPtr == nullptr )
                {
                    QC_ERROR( "Failed to create a remap worker for CPU!" );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    m_workerPtrsCPU[inputId] = workerPtr;
                }
            }
        }
    }
    return ret;
}

QCStatus_e FadasRemap::RemapRunCPU( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;

    /*call unified RegBuf functions even the buffers may have been registered*/
    int32_t srcFds[QC_MAX_INPUTS];
    int32_t dstFd;
    ImageDescriptor_t &bufDescOutput =
            dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( m_numOfInputs ) );
    for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
    {
        const ImageDescriptor_t &bufDescInput =
                dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( inputId ) );
        srcFds[inputId] = RegBuf( bufDescInput, FADAS_BUF_TYPE_IN );
        if ( srcFds[inputId] < 0 )
        {
            QC_ERROR( "Input Buffer register failed!" );
            ret = QC_STATUS_INVALID_BUF;
            break;
        }
    }
    if ( QC_STATUS_OK == ret )
    {
        dstFd = RegBuf( bufDescOutput, FADAS_BUF_TYPE_OUT );
        if ( dstFd < 0 )
        {
            QC_ERROR( "Output buffer register failed!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        size_t outputSize = bufDescOutput.size / bufDescOutput.batchSize;
        for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
        {
            const ImageDescriptor_t &bufDescInput =
                    dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( inputId ) );
            uint8_t *pSrc = static_cast<uint8_t *>( bufDescInput.pBuf );
            uint8_t *pDst = static_cast<uint8_t *>( bufDescOutput.pBuf ) + inputId * outputSize;

            FadasImage_t srcImg = { 0 };
            srcImg.props.width = bufDescInput.width;
            srcImg.props.height = bufDescInput.height;
            srcImg.props.numPlanes = bufDescInput.numPlanes;
            for ( int i = 0; i < bufDescInput.numPlanes; i++ )
            {
                srcImg.props.stride[i] = bufDescInput.stride[i];
            }
            srcImg.plane[0] = pSrc;
            if ( QC_IMAGE_FORMAT_UYVY == m_inputFormats[inputId] )
            {
                srcImg.props.format = FADAS_IMAGE_FORMAT_UYVY;
            }
            else if ( QC_IMAGE_FORMAT_RGB888 == m_inputFormats[inputId] )
            {
                srcImg.props.format = FADAS_IMAGE_FORMAT_RGB888;
            }
            else if ( QC_IMAGE_FORMAT_NV12 == m_inputFormats[inputId] )
            {
                srcImg.props.format = FADAS_IMAGE_FORMAT_Y8UV8;
                srcImg.plane[1] = pSrc + bufDescInput.planeBufSize[0];
            }
            else if ( QC_IMAGE_FORMAT_NV12_UBWC == m_inputFormats[inputId] )
            {
                srcImg.props.format = FADAS_IMAGE_FORMAT_UBWC_NV12_RH;
                srcImg.props.numPlanes = 1;
                srcImg.props.stride[0] = bufDescInput.size / bufDescInput.height;
                srcImg.props.stride[1] = 0;
                srcImg.props.stride[2] = 0;
                srcImg.props.stride[3] = 0;
            }
            else
            {
                QC_ERROR( "Invalid input format for inputId = %d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            srcImg.bAllocated = false;

            FadasImage_t rgbImg{ 0 };
            rgbImg.props.width = bufDescOutput.width;
            rgbImg.props.height = bufDescOutput.height;
            if ( QC_IMAGE_FORMAT_RGB888 == m_outputFormat )
            {
                rgbImg.props.format = FADAS_IMAGE_FORMAT_RGB888;
            }
            else if ( QC_IMAGE_FORMAT_BGR888 == m_outputFormat )
            {
                rgbImg.props.format = FADAS_IMAGE_FORMAT_BGR888_RH;
            }
            else
            {
                QC_ERROR( "Invalid output format!" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            rgbImg.props.numPlanes = bufDescOutput.numPlanes;
            for ( int i = 0; i < bufDescOutput.numPlanes; i++ )
            {
                rgbImg.props.stride[i] = bufDescOutput.stride[i];
            }
            rgbImg.plane[0] = pDst;
            rgbImg.bAllocated = false;

            if ( QC_STATUS_OK == ret )
            {
                FadasROI_t roi = m_ROIs[inputId];
                FadasRemapMap_t *remapPtr = m_remapPtrsCPU[inputId];
                void *workerPtr = m_workerPtrsCPU[inputId];
                FadasError_e retFadas;

                if ( QC_PROCESSOR_GPU == m_processor )
                {
                    if ( false == m_bEnableNormalize )
                    {
                        retFadas = s_FadasRemap_RunGPU( remapPtr, &srcImg, &rgbImg, &roi, 1.0,
                                                        nullptr );
                    }
                    else
                    {
                        retFadas = s_FadasRemap_RunGPU( remapPtr, &srcImg, &rgbImg, &roi, 1.0,
                                                        m_normlz );
                    }
                    if ( FADAS_ERROR_NONE != retFadas )
                    {
                        QC_ERROR( "FadasRemap_RunGPU failed for batch %d: ret = 0x%x", inputId,
                                  retFadas );
                        ret = QC_STATUS_FAIL;
                    }
                }
                else
                {
                    if ( false == m_bEnableNormalize )
                    {
                        retFadas = FadasRemap_RunMT( workerPtr, remapPtr, &srcImg, &rgbImg, &roi );
                    }
                    else
                    {
                        retFadas = FadasRemap_RunMT( workerPtr, remapPtr, &srcImg, &rgbImg, &roi,
                                                     1.0, m_normlz );
                    }
                    if ( FADAS_ERROR_NONE != retFadas )
                    {
                        QC_ERROR( "FadasRemap_RunMT failed for batch %d: ret = 0x%x", inputId,
                                  retFadas );
                        ret = QC_STATUS_FAIL;
                    }
                }
            }
            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
    }

    return ret;
}

QCStatus_e FadasRemap::RemapRunDSP( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;

    /*call unified RegBuf functions even the buffers may have been registered*/
    int32_t srcFds[QC_MAX_INPUTS];
    int32_t dstFd;
    ImageDescriptor_t &bufDescOutput =
            dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( m_numOfInputs ) );
    for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
    {
        const ImageDescriptor_t &bufDescInput =
                dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( inputId ) );
        srcFds[inputId] = RegBuf( bufDescInput, FADAS_BUF_TYPE_IN );
        if ( srcFds[inputId] < 0 )
        {
            QC_ERROR( "Input Buffer register failed!" );
            ret = QC_STATUS_INVALID_BUF;
            break;
        }
    }
    if ( QC_STATUS_OK == ret )
    {
        dstFd = RegBuf( bufDescOutput, FADAS_BUF_TYPE_OUT );
        if ( dstFd < 0 )
        {
            QC_ERROR( "Output buffer register failed!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        size_t outputSize = bufDescOutput.size / bufDescOutput.batchSize;
        FadasIface_FadasROI_t ROIs[QC_MAX_INPUTS];
        FadasIface_FadasImgProps_t srcImgProps[QC_MAX_INPUTS];
        uint32_t offsets[QC_MAX_INPUTS];

        for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
        {
            const ImageDescriptor_t &bufDescInput =
                    dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( inputId ) );
            FadasIface_FadasImgProps_t srcImgProp;
            srcImgProp.width = bufDescInput.width;
            srcImgProp.height = bufDescInput.height;
            srcImgProp.numPlanes = bufDescInput.numPlanes;
            for ( int i = 0; i < bufDescInput.numPlanes; i++ )
            {
                srcImgProp.stride[i] = bufDescInput.stride[i];
            }
            for ( int i = 0; i < bufDescInput.numPlanes; i++ )
            {
                srcImgProp.actualHeight[i] = bufDescInput.actualHeight[i];
            }
            if ( QC_IMAGE_FORMAT_UYVY == m_inputFormats[inputId] )
            {
                srcImgProp.format = FADAS_IMAGE_FORMAT_UYVY_NSP;
            }
            else if ( QC_IMAGE_FORMAT_RGB888 == m_inputFormats[inputId] )
            {
                srcImgProp.format = FADAS_IMAGE_FORMAT_RGB888_NSP;
            }
            else if ( QC_IMAGE_FORMAT_NV12 == m_inputFormats[inputId] )
            {
                srcImgProp.format = FADAS_IMAGE_FORMAT_Y8UV8_NSP;
            }
            else
            {
                QC_ERROR( "Invalid input format for inputId = %d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
                break;
            }
            srcImgProps[inputId] = srcImgProp;
            offsets[inputId] = bufDescInput.offset;
            ROIs[inputId].x = m_ROIs[inputId].x;
            ROIs[inputId].y = m_ROIs[inputId].y;
            ROIs[inputId].width = m_ROIs[inputId].width;
            ROIs[inputId].height = m_ROIs[inputId].height;
        }

        FadasIface_FadasImgProps_t dstImgProp;
        dstImgProp.width = bufDescOutput.width;
        dstImgProp.height = bufDescOutput.height;
        dstImgProp.format = FADAS_IMAGE_FORMAT_RGB888_NSP;
        dstImgProp.numPlanes = bufDescOutput.numPlanes;
        for ( int i = 0; i < bufDescOutput.numPlanes; i++ )
        {
            dstImgProp.stride[i] = bufDescOutput.stride[i];
        }

        if ( QC_STATUS_OK == ret )
        {
            AEEResult retV;
            if ( false == m_bEnableNormalize )
            {
                retV = FadasIface_FadasRemap_RunMT(
                        m_handle64, m_workerPtrsDSP, m_numOfInputs, m_remapPtrsDSP, m_numOfInputs,
                        srcFds, m_numOfInputs, offsets, m_numOfInputs, srcImgProps, m_numOfInputs,
                        dstFd, outputSize, &dstImgProp, ROIs, m_numOfInputs, nullptr, 0 );
            }
            else
            {
                FadasIface_FadasNormlzParams_t normlz[3];
                normlz[0].sub = m_normlz[0].sub;
                normlz[0].mul = m_normlz[0].mul;
                normlz[0].add = m_normlz[0].add;
                normlz[1].sub = m_normlz[1].sub;
                normlz[1].mul = m_normlz[1].mul;
                normlz[1].add = m_normlz[1].add;
                normlz[2].sub = m_normlz[2].sub;
                normlz[2].mul = m_normlz[2].mul;
                normlz[2].add = m_normlz[2].add;
                retV = FadasIface_FadasRemap_RunMT(
                        m_handle64, m_workerPtrsDSP, m_numOfInputs, m_remapPtrsDSP, m_numOfInputs,
                        srcFds, m_numOfInputs, offsets, m_numOfInputs, srcImgProps, m_numOfInputs,
                        dstFd, outputSize, &dstImgProp, ROIs, m_numOfInputs, normlz, 3 );
            }
            if ( retV != AEE_SUCCESS )
            {
                QC_ERROR( "Remap888 failed: ret = 0x%x", retV );
                ret = QC_STATUS_FAIL;
            }
        }
    }

    return ret;
}

QCStatus_e FadasRemap::RemapRun( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( uint32_t inputId = 0; inputId < m_numOfInputs; inputId++ )
    {
        ImageDescriptor_t &bufDescInput =
                dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( inputId ) );

        if ( m_inputFormats[inputId] != bufDescInput.format )
        {
            QC_ERROR( "Format in input buffer and config not match!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( m_inputWidths[inputId] != bufDescInput.width )
        {
            QC_ERROR( "Width in input buffer and config not match!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( m_inputHeights[inputId] != bufDescInput.height )
        {
            QC_ERROR( "Height in input buffer and config not match!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( 1 != bufDescInput.batchSize )
        {
            QC_ERROR( "Batch in input buffer must be 1!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            QC_INFO( "Input image property check pass for id=%d!", inputId );
        }
        if ( QC_STATUS_OK != ret )
        {
            break;
        }
    }

    ImageDescriptor_t &bufDescOutput =
            dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( m_numOfInputs ) );
    if ( m_outputFormat != bufDescOutput.format )
    {
        QC_ERROR( "Format in output buffer and config not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_outputWidth != bufDescOutput.width )
    {
        QC_ERROR( "Width in output buffer and config not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_outputHeight != bufDescOutput.height )
    {
        QC_ERROR( "Height in output buffer and config not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_numOfInputs != bufDescOutput.batchSize )
    {
        QC_ERROR( "Batch in output buffer and config not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        QC_INFO( "Output image property check pass!" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
        {
            ret = RemapRunDSP( frameDesc );
        }
        else
        {
            ret = RemapRunCPU( frameDesc );
        }
    }

    return ret;
}

QCStatus_e FadasRemap::DestroyWorkers()
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( int i = 0; i < QC_MAX_INPUTS; i++ )
    {
        if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
        {
            if ( 0 != m_workerPtrsDSP[i] )
            {
                AEEResult retVal =
                        FadasIface_FadasRemap_DestroyWorkers( m_handle64, m_workerPtrsDSP[i] );
                if ( AEE_SUCCESS != retVal )
                {
                    QC_ERROR( "Destroy worker failed!" );
                    ret = QC_STATUS_FAIL;
                }
            }
        }
        else if ( QC_PROCESSOR_GPU == m_processor )
        {
            /*do nothing for fadas GPU */
        }
        else
        {
            if ( nullptr != m_workerPtrsCPU[i] )
            {
                FadasError_e retVal = FadasRemap_DestroyWorkers( m_workerPtrsCPU[i] );
                if ( FADAS_ERROR_NONE != retVal )
                {
                    QC_ERROR( "Destroy worker failed!" );
                    ret = QC_STATUS_FAIL;
                }
            }
        }
    }

    return ret;
}

QCStatus_e FadasRemap::DestroyMap()
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( int i = 0; i < QC_MAX_INPUTS; i++ )
    {
        if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
        {
            if ( 0 != m_remapPtrsDSP[i] )
            {
                AEEResult retVal =
                        FadasIface_FadasRemap_DestroyMap( m_handle64, m_remapPtrsDSP[i] );
                if ( AEE_SUCCESS != retVal )
                {
                    QC_ERROR( "Destroy map failed!" );
                    ret = QC_STATUS_FAIL;
                }
            }
        }
        else if ( QC_PROCESSOR_GPU == m_processor )
        {
            if ( nullptr != m_remapPtrsCPU[i] )
            {
                FadasError_e retVal = s_FadasRemap_DestroyMapGPU( m_remapPtrsCPU[i] );
                if ( FADAS_ERROR_NONE != retVal )
                {
                    QC_ERROR( "Destroy map failed!" );
                    ret = QC_STATUS_FAIL;
                }
            }
        }
        else
        {
            if ( nullptr != m_remapPtrsCPU[i] )
            {
                FadasError_e retVal = FadasRemap_DestroyMap( m_remapPtrsCPU[i] );
                if ( FADAS_ERROR_NONE != retVal )
                {
                    QC_ERROR( "Destroy map failed!" );
                    ret = QC_STATUS_FAIL;
                }
            }
        }
    }

    return ret;
}

}   // namespace FadasIface
}   // namespace libs
}   // namespace QC