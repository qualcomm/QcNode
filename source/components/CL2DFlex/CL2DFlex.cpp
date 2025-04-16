// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/component/CL2DFlex.hpp"
#include "include/CL2DPipelineBase.hpp"
#include "include/CL2DPipelineConvert.hpp"
#include "include/CL2DPipelineConvertUBWC.hpp"
#include "include/CL2DPipelineLetterbox.hpp"
#include "include/CL2DPipelineLetterboxMultiple.hpp"
#include "include/CL2DPipelineRemap.hpp"
#include "include/CL2DPipelineResize.hpp"
#include "include/CL2DPipelineResizeMultiple.hpp"
#include "kernel/CL2DFlex.cl.h"

namespace QC
{
namespace component
{

CL2DFlex::CL2DFlex() {}

CL2DFlex::~CL2DFlex() {}

QCStatus_e CL2DFlex::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "CL2DFlex component start failed due to wrong state!" );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e CL2DFlex::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "CL2DFlex component stop failed due to wrong state!" );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e CL2DFlex::Init( const char *pName, const CL2DFlex_Config_t *pConfig,
                           Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to init component!" );
    }
    else
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;

        if ( nullptr == pConfig )
        {
            QC_ERROR( "Empty config pointer!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( QC_MAX_INPUTS < pConfig->numOfInputs )
        {
            QC_ERROR( "Invalid inputs number!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            for ( uint32_t inputId = 0; inputId < pConfig->numOfInputs; inputId++ )
            {
                if ( ( 0 != ( pConfig->inputWidths[inputId] % 2 ) ) ||
                     ( 0 == pConfig->inputWidths[inputId] ) )
                {
                    QC_ERROR( "Invalid input width!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( ( 0 != ( pConfig->inputHeights[inputId] % 2 ) ) ||
                          ( 0 == pConfig->inputHeights[inputId] ) )
                {
                    QC_ERROR( "Invalid input height!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( pConfig->inputWidths[inputId] <
                          ( pConfig->ROIs[inputId].x + pConfig->ROIs[inputId].width ) )
                {
                    QC_ERROR( "Invalid ROI values, (ROI.x + ROI.width) > inputWidth!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( pConfig->inputHeights[inputId] <
                          ( pConfig->ROIs[inputId].y + pConfig->ROIs[inputId].height ) )
                {
                    QC_ERROR( "Invalid ROI values, (ROI.y + ROI.height) > inputHeight!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == pConfig->workModes[inputId] ) &&
                          ( nullptr == pConfig->remapTable[inputId].pMapX ) )
                {
                    QC_ERROR( "Invalid map table X!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == pConfig->workModes[inputId] ) &&
                          ( nullptr == pConfig->remapTable[inputId].pMapY ) )
                {
                    QC_ERROR( "Invalid map table Y!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else
                {
                    // empty else block
                }
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_config = *pConfig;
            ret = m_OpenclSrvObj.Init( pName, level, m_config.priority );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Init OpenCL failed!" );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                ret = m_OpenclSrvObj.LoadFromSource( s_pSourceCL2DFlex );
            }
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Load program from source file s_pSourceCL2DFlex failed!" );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
                {
                    m_pCL2DPipeline[inputId] = nullptr;

                    if ( CL2DFLEX_WORK_MODE_CONVERT == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineConvert();
                    }
                    else if ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineResize();
                    }
                    else if ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineLetterbox();
                    }
                    else if ( CL2DFLEX_WORK_MODE_CONVERT_UBWC == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineConvertUBWC();
                    }
                    else if ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE ==
                              m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineLetterboxMultiple();
                    }
                    else if ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE ==
                              m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineResizeMultiple();
                    }
                    else if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineRemap();
                    }
                    else
                    {
                        QC_ERROR( "Invalid CL2DFlex work mode for inputId=%d!", inputId );
                        ret = QC_STATUS_BAD_ARGUMENTS;
                    }

                    if ( nullptr != m_pCL2DPipeline[inputId] )
                    {
                        m_pCL2DPipeline[inputId]->InitLogger( pName, level );
                        ret = m_pCL2DPipeline[inputId]->Init( inputId, &m_kernel[inputId],
                                                              &m_config, &m_OpenclSrvObj );
                    }
                    else
                    {
                        QC_ERROR( "CL2D pipeline create failed for inputId=%d!", inputId );
                        ret = QC_STATUS_BAD_ARGUMENTS;
                    }

                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Setup pipeline failed for inputId=%d!", inputId );
                        break;
                    }
                }
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_state = QC_OBJECT_STATE_READY;
        }
        else
        {
            m_state = QC_OBJECT_STATE_INITIAL;
            QCStatus_e retVal;
            retVal = ComponentIF::Deinit();
            if ( QC_STATUS_OK != retVal )
            {
                QC_ERROR( "Deinit ComponentIF failed!" );
            }
        }
    }

    return ret;
}

QCStatus_e CL2DFlex::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "CL2DFlex component not in ready status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        QCStatus_e retVal;

        retVal = m_OpenclSrvObj.Deinit();
        if ( QC_STATUS_OK != retVal )
        {
            QC_ERROR( "Release CL resources failed!" );
            ret = QC_STATUS_FAIL;
        }

        for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
        {
            if ( nullptr != m_pCL2DPipeline[inputId] )
            {
                m_pCL2DPipeline[inputId]->DeinitLogger();
                retVal = m_pCL2DPipeline[inputId]->Deinit();
                if ( QC_STATUS_OK != retVal )
                {
                    QC_ERROR( "Deinit pipeline failed for inputId=%d!", inputId );
                    ret = QC_STATUS_FAIL;
                }
                delete m_pCL2DPipeline[inputId];
            }
        }

        retVal = ComponentIF::Deinit();
        if ( QC_STATUS_OK != retVal )
        {
            QC_ERROR( "Deinit ComponentIF failed!" );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}


QCStatus_e CL2DFlex::RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "CL2DFlex component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            if ( QC_IMAGE_FORMAT_NV12_UBWC == pBuffers[i].imgProps.format )
            {
                QCStatus_e retVal = QC_STATUS_OK;
                cl_mem bufferSrc;
                cl_mem bufferSrcY;
                cl_mem bufferSrcUV;
                cl_image_format inputImageFormat = { 0 };
                inputImageFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12;
                inputImageFormat.image_channel_data_type = CL_UNORM_INT8;
                cl_image_desc inputImageDesc = { 0 };
                inputImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
                inputImageDesc.image_width = (size_t) pBuffers[i].imgProps.width;
                inputImageDesc.image_height = (size_t) pBuffers[i].imgProps.height;
                retVal = m_OpenclSrvObj.RegImage( pBuffers[i].data(), pBuffers[i].buffer.dmaHandle,
                                                  &bufferSrc, &inputImageFormat, &inputImageDesc );
                if ( QC_STATUS_OK != retVal )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to register input NV12 UBWC image!" );
                }
                else
                {
                    cl_image_format inputYFormat = { 0 };
                    inputYFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_Y;
                    inputYFormat.image_channel_data_type = CL_UNORM_INT8;
                    cl_image_desc inputYDesc = { 0 };
                    inputYDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
                    inputYDesc.image_width = (size_t) m_config.outputWidth;
                    inputYDesc.image_height = (size_t) m_config.outputHeight;
                    inputYDesc.mem_object = bufferSrc;
                    retVal = m_OpenclSrvObj.RegPlane( pBuffers[i].data(), &bufferSrcY,
                                                      &inputYFormat, &inputYDesc );
                }
                if ( QC_STATUS_OK != retVal )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to register input Y plane!" );
                }
                else
                {
                    cl_image_format inputUVFormat = { 0 };
                    inputUVFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_UV;
                    inputUVFormat.image_channel_data_type = CL_UNORM_INT8;
                    cl_image_desc inputUVDesc = { 0 };
                    inputUVDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
                    inputUVDesc.image_width = (size_t) m_config.outputWidth;
                    inputUVDesc.image_height = (size_t) m_config.outputHeight;
                    inputUVDesc.mem_object = bufferSrc;
                    retVal = m_OpenclSrvObj.RegPlane( pBuffers[i].data(), &bufferSrcUV,
                                                      &inputUVFormat, &inputUVDesc );
                }
                if ( QC_STATUS_OK != retVal )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to register input UV plane!" );
                }
            }
            else
            {
                cl_mem bufferCL;
                ret = m_OpenclSrvObj.RegBuf( &( pBuffers[i].buffer ), &bufferCL );
            }

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to register buffer for number %d!", i );
                break;
            }
        }
    }

    return ret;
}

QCStatus_e CL2DFlex::DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "CL2DFlex component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            if ( QC_IMAGE_FORMAT_NV12_UBWC == pBuffers[i].imgProps.format )
            {
                QCStatus_e retVal = QC_STATUS_OK;
                retVal = m_OpenclSrvObj.DeregImage( pBuffers[i].data() );
                if ( QC_STATUS_OK != retVal )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to deregister input NV12 UBWC image!" );
                }
                else
                {
                    cl_image_format inputYFormat = { 0 };
                    inputYFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_Y;
                    retVal = m_OpenclSrvObj.DeregPlane( pBuffers[i].data(), &inputYFormat );
                }
                if ( QC_STATUS_OK != retVal )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to deregister input Y plane!" );
                }
                else
                {
                    cl_image_format inputUVFormat = { 0 };
                    inputUVFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_UV;
                    retVal = m_OpenclSrvObj.DeregPlane( pBuffers[i].data(), &inputUVFormat );
                }
                if ( QC_STATUS_OK != retVal )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to deregister input UV plane!" );
                }
            }
            else
            {
                ret = m_OpenclSrvObj.DeregBuf( &( pBuffers[i].buffer ) );
            }
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to deregister buffer for number %d!", i );
            }
        }
    }

    return ret;
}

QCStatus_e CL2DFlex::Execute( const QCSharedBuffer_t *pInputs, const uint32_t numInputs,
                              const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "CL2DFlex component not initialized!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pOutput )
    {
        QC_ERROR( "Output buffer is null!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == pInputs )
    {
        QC_ERROR( "Input buffer is null!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( numInputs != m_config.numOfInputs )
    {
        QC_ERROR( "number of inputs not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != pOutput->type )
    {
        QC_ERROR( "Output buffer is not image type!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.outputFormat != pOutput->imgProps.format )
    {
        QC_ERROR( "Output image format not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.outputWidth != pOutput->imgProps.width )
    {
        QC_ERROR( "Output image width not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.outputHeight != pOutput->imgProps.height )
    {
        QC_ERROR( "Output image height not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( numInputs != pOutput->imgProps.batchSize )
    {
        QC_ERROR( "Output image batch not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < numInputs; inputId++ )
        {
            if ( nullptr == pInputs[inputId].data() )
            {
                QC_ERROR( "Input buffer data is null for inputId=%d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( QC_BUFFER_TYPE_IMAGE != pInputs[inputId].type )
            {
                QC_ERROR( "Input buffer is not image type for inputId=%d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( m_config.inputFormats[inputId] != pInputs[inputId].imgProps.format )
            {
                QC_ERROR( "Input image format not match for inputId=%d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( m_config.inputWidths[inputId] != pInputs[inputId].imgProps.width )
            {
                QC_ERROR( "Input image width not match for inputId=%d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( m_config.inputHeights[inputId] != pInputs[inputId].imgProps.height )
            {
                QC_ERROR( "Input image height not match for inputId=%d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                if ( nullptr == m_pCL2DPipeline[inputId] )
                {
                    QC_ERROR( "Pipeline not setup for inputId=%d!", inputId );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    ret = m_pCL2DPipeline[inputId]->Execute( &pInputs[inputId], pOutput );
                }
            }
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to execute pipeline for inputId=%d!", inputId );
                break;
            }
        }
    }

    return ret;
}

QCStatus_e CL2DFlex::ExecuteWithROI( const QCSharedBuffer_t *pInput,
                                     const QCSharedBuffer_t *pOutput,
                                     const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "CL2DFlex component not initialized!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pInput )
    {
        QC_ERROR( "Input buffer is null!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == pOutput )
    {
        QC_ERROR( "Output buffer is null!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == pROIs )
    {
        QC_ERROR( "ROI configurations is null!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 1 != m_config.numOfInputs )
    {
        QC_ERROR( "number of inputs not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == pOutput->data() )
    {
        QC_ERROR( "Output buffer data is null" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != pOutput->type )
    {
        QC_ERROR( "Output buffer is not image type!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.outputFormat != pOutput->imgProps.format )
    {
        QC_ERROR( "Output image format not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.outputWidth != pOutput->imgProps.width )
    {
        QC_ERROR( "Output image width not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.outputHeight != pOutput->imgProps.height )
    {
        QC_ERROR( "Output image height not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( numROIs != pOutput->imgProps.batchSize )
    {
        QC_ERROR( "Output image batch not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == pInput->data() )
    {
        QC_ERROR( "Input buffer data is null" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != pInput->type )
    {
        QC_ERROR( "Input buffer is not image type" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.inputFormats[0] != pInput->imgProps.format )
    {
        QC_ERROR( "Input image format not match" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.inputWidths[0] != pInput->imgProps.width )
    {
        QC_ERROR( "Input image width not match" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.inputHeights[0] != pInput->imgProps.height )
    {
        QC_ERROR( "Input image height not match" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        if ( nullptr == m_pCL2DPipeline[0] )
        {
            QC_ERROR( "Pipeline not setup for inputId=0!" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            ret = m_pCL2DPipeline[0]->ExecuteWithROI( pInput, pOutput, pROIs, numROIs );
        }

        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to execute pipeline with ROI!" );
        }
    }

    return ret;
}

}   // namespace component
}   // namespace QC
