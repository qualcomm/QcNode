// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "include/CL2DPipelineResize.hpp"

namespace QC
{
namespace component
{

CL2DPipelineResize::CL2DPipelineResize() {}

CL2DPipelineResize::~CL2DPipelineResize() {}

QCStatus_e CL2DPipelineResize::Init( uint32_t inputId, cl_kernel *pKernel,
                                     CL2DFlex_Config_t *pConfig, OpenclSrv *pOpenclSrvObj )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( QC_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
         ( QC_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ResizeNV12ToRGB" );
    }

    else if ( ( QC_IMAGE_FORMAT_UYVY == m_config.inputFormats[m_inputId] ) &&
              ( QC_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_RGB;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ResizeUYVYToRGB" );
    }

    else if ( ( QC_IMAGE_FORMAT_RGB888 == m_config.inputFormats[m_inputId] ) &&
              ( QC_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_RESIZE_NEAREST_RGB_TO_RGB;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ResizeRGBToRGB" );
    }

    else if ( ( QC_IMAGE_FORMAT_UYVY == m_config.inputFormats[m_inputId] ) &&
              ( QC_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_NV12;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ResizeUYVYToNV12" );
    }

    else if ( ( QC_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
              ( QC_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_NV12;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ResizeNV12ToNV12" );
    }

    else
    {
        QC_ERROR( "Invalid CL2DFlex resize pipeline for inputId=%d!", m_inputId );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    return ret;
}

QCStatus_e CL2DPipelineResize::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    // empty function

    return ret;
}

QCStatus_e CL2DPipelineResize::Execute( const QCSharedBuffer_t *pInput,
                                        const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    cl_mem bufferDst;
    ret = m_pOpenclSrvObj->RegBuf( &( pOutput->buffer ), &bufferDst );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register output buffer!" );
    }
    else
    {
        cl_mem bufferSrc;
        ret = m_pOpenclSrvObj->RegBuf( &( pInput->buffer ), &bufferSrc );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to register input buffer!" );
        }
        else
        {
            uint32_t srcOffset = pInput->offset;
            uint32_t sizeOne = ( uint32_t )( pOutput->size ) / ( pOutput->imgProps.batchSize );
            uint32_t dstOffset = ( uint32_t )( pOutput->offset ) + m_inputId * sizeOne;

            if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB == m_pipeline )
            {
                ret = ResizeFromNV12ToRGB( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                           pOutput );
            }
            else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_RGB == m_pipeline )
            {
                ret = ResizeFromUYVYToRGB( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                           pOutput );
            }
            else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_NV12 == m_pipeline )
            {
                ret = ResizeFromUYVYToNV12( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                            pOutput );
            }
            else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_RGB_TO_RGB == m_pipeline )
            {
                ret = ResizeFromRGBToRGB( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                          pOutput );
            }
            else if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_NV12 == m_pipeline )
            {
                ret = ResizeFromNV12ToNV12( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                            pOutput );
            }
            else
            {
                QC_ERROR( "Invalid CL2DFlex resize pipeline for inputId=%d!", m_inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }

    return ret;
}

QCStatus_e CL2DPipelineResize::ExecuteWithROI( const QCSharedBuffer_t *pInput,
                                               const QCSharedBuffer_t *pOutput,
                                               const CL2DFlex_ROIConfig_t *pROIs,
                                               const uint32_t numROIs )
{
    QCStatus_e ret = QC_STATUS_BAD_ARGUMENTS;

    // empty function

    return ret;
}

QCStatus_e CL2DPipelineResize::ResizeFromNV12ToRGB( cl_mem bufferSrc, uint32_t srcOffset,
                                                    cl_mem bufferDst, uint32_t dstOffset,
                                                    const QCSharedBuffer_t *pInput,
                                                    const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numOfArgs = 14;
    OpenclIfcae_Arg_t OpenclArgs[14];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[m_inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[m_inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( pInput->imgProps.stride[1] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[m_inputId].x / m_config.ROIs[m_inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[m_inputId].y / m_config.ROIs[m_inputId].height * m_config.outputHeight;
    OpenclArgs[12].pArg = (void *) &( kernelROIX );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( kernelROIY );
    OpenclArgs[13].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute resize NV12 to RGB OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e CL2DPipelineResize::ResizeFromUYVYToRGB( cl_mem bufferSrc, uint32_t srcOffset,
                                                    cl_mem bufferDst, uint32_t dstOffset,
                                                    const QCSharedBuffer_t *pInput,
                                                    const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numOfArgs = 12;
    OpenclIfcae_Arg_t OpenclArgs[12];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[m_inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[m_inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[m_inputId].x / m_config.ROIs[m_inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[m_inputId].y / m_config.ROIs[m_inputId].height * m_config.outputHeight;
    OpenclArgs[10].pArg = (void *) &( kernelROIX );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( kernelROIY );
    OpenclArgs[11].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute resize UYVY to RGB OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e CL2DPipelineResize::ResizeFromUYVYToNV12( cl_mem bufferSrc, uint32_t srcOffset,
                                                     cl_mem bufferDst, uint32_t dstOffset,
                                                     const QCSharedBuffer_t *pInput,
                                                     const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numOfArgs = 14;
    OpenclIfcae_Arg_t OpenclArgs[14];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[m_inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[m_inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( pOutput->imgProps.planeBufSize[0] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( pOutput->imgProps.stride[1] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[m_inputId].x / m_config.ROIs[m_inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[m_inputId].y / m_config.ROIs[m_inputId].height * m_config.outputHeight;
    OpenclArgs[12].pArg = (void *) &( kernelROIX );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( kernelROIY );
    OpenclArgs[13].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute resize UYVY to NV12 OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e CL2DPipelineResize::ResizeFromRGBToRGB( cl_mem bufferSrc, uint32_t srcOffset,
                                                   cl_mem bufferDst, uint32_t dstOffset,
                                                   const QCSharedBuffer_t *pInput,
                                                   const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numOfArgs = 12;
    OpenclIfcae_Arg_t OpenclArgs[12];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[m_inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[m_inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[m_inputId].x / m_config.ROIs[m_inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[m_inputId].y / m_config.ROIs[m_inputId].height * m_config.outputHeight;
    OpenclArgs[10].pArg = (void *) &( kernelROIX );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( kernelROIY );
    OpenclArgs[11].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute resize RGB to RGB OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e CL2DPipelineResize::ResizeFromNV12ToNV12( cl_mem bufferSrc, uint32_t srcOffset,
                                                     cl_mem bufferDst, uint32_t dstOffset,
                                                     const QCSharedBuffer_t *pInput,
                                                     const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numOfArgs = 16;
    OpenclIfcae_Arg_t OpenclArgs[16];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[m_inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[m_inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( pInput->imgProps.stride[1] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    OpenclArgs[12].pArg = (void *) &( pOutput->imgProps.planeBufSize[0] );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( pOutput->imgProps.stride[1] );
    OpenclArgs[13].argSize = sizeof( cl_int );
    uint32_t kernelROIX =
            m_config.ROIs[m_inputId].x / m_config.ROIs[m_inputId].width * m_config.outputWidth;
    uint32_t kernelROIY =
            m_config.ROIs[m_inputId].y / m_config.ROIs[m_inputId].height * m_config.outputHeight;
    OpenclArgs[14].pArg = (void *) &( kernelROIX );
    OpenclArgs[14].argSize = sizeof( cl_int );
    OpenclArgs[15].pArg = (void *) &( kernelROIY );
    OpenclArgs[15].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute resize NV12 to NV12 OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

}   // namespace component
}   // namespace QC
