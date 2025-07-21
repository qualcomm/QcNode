// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "CL2DFlexImpl.hpp"
#include "kernel/CL2DFlex.cl.h"
#include "pipeline/CL2DPipelineBase.hpp"
#include "pipeline/CL2DPipelineConvert.hpp"
#include "pipeline/CL2DPipelineConvertUBWC.hpp"
#include "pipeline/CL2DPipelineLetterbox.hpp"
#include "pipeline/CL2DPipelineLetterboxMultiple.hpp"
#include "pipeline/CL2DPipelineRemap.hpp"
#include "pipeline/CL2DPipelineResize.hpp"
#include "pipeline/CL2DPipelineResizeMultiple.hpp"

namespace QC
{
namespace Node
{

QCStatus_e CL2DFlexImpl::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "CL2DFlex node start failed due to wrong state!" );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e CL2DFlexImpl::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "CL2DFlex node stop failed due to wrong state!" );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e
CL2DFlexImpl::Initialize( std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        QC_ERROR( "CL2DFlex not in initial state!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < m_config.params.numOfInputs; inputId++ )
        {
            if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == m_config.params.workModes[inputId] )
            {
                uint32_t mapXBufferId = m_mapXBufferIds[inputId];
                QCBufferDescriptorBase_t &mapXBufferDesc = buffers[mapXBufferId];
                QCSharedBufferDescriptor_t *pMapXBufferDesc =
                        dynamic_cast<QCSharedBufferDescriptor_t *>( &mapXBufferDesc );
                m_config.params.remapTable[inputId].pMapX = &( pMapXBufferDesc->buffer );

                uint32_t mapYBufferId = m_mapYBufferIds[inputId];
                QCBufferDescriptorBase_t &mapYBufferDesc = buffers[mapYBufferId];
                QCSharedBufferDescriptor_t *pMapYBufferDesc =
                        dynamic_cast<QCSharedBufferDescriptor_t *>( &mapYBufferDesc );
                m_config.params.remapTable[inputId].pMapY = &( pMapYBufferDesc->buffer );
            }
        }

        status = m_OpenclSrvObj.Init( "Opencl", LOGGER_LEVEL_ERROR, m_config.params.priority );
        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "Init OpenCL failed!" );
            status = QC_STATUS_FAIL;
        }
        else
        {
            status = m_OpenclSrvObj.LoadFromSource( s_pSourceCL2DFlex );
        }

        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "Load program from source file s_pSourceCL2DFlex failed!" );
            status = QC_STATUS_FAIL;
        }
        else
        {
            for ( uint32_t inputId = 0; inputId < m_config.params.numOfInputs; inputId++ )
            {
                m_pCL2DPipeline[inputId] = nullptr;
                CL2DFlex_Work_Mode_e workMode = m_config.params.workModes[inputId];

                if ( CL2DFLEX_WORK_MODE_CONVERT == workMode )
                {
                    m_pCL2DPipeline[inputId] = new CL2DPipelineConvert();
                }
                else if ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST == workMode )
                {
                    m_pCL2DPipeline[inputId] = new CL2DPipelineResize();
                }
                else if ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST == workMode )
                {
                    m_pCL2DPipeline[inputId] = new CL2DPipelineLetterbox();
                }
                else if ( CL2DFLEX_WORK_MODE_CONVERT_UBWC == workMode )
                {
                    m_pCL2DPipeline[inputId] = new CL2DPipelineConvertUBWC();
                }
                else if ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE == workMode )
                {
                    m_pCL2DPipeline[inputId] = new CL2DPipelineLetterboxMultiple();
                }
                else if ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE == workMode )
                {
                    m_pCL2DPipeline[inputId] = new CL2DPipelineResizeMultiple();
                }
                else if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == workMode )
                {
                    m_pCL2DPipeline[inputId] = new CL2DPipelineRemap();
                }
                else
                {
                    QC_ERROR( "Invalid CL2DFlex work mode for inputId=%d!", inputId );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }

                if ( nullptr != m_pCL2DPipeline[inputId] )
                {
                    m_pCL2DPipeline[inputId]->InitLogger( "CL2DPipeline", LOGGER_LEVEL_ERROR );
                    status = m_pCL2DPipeline[inputId]->Init( inputId, &m_kernel[inputId],
                                                             &m_config.params, &m_OpenclSrvObj );
                }
                else
                {
                    QC_ERROR( "CL2D pipeline create failed for inputId=%d!", inputId );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }

                if ( QC_STATUS_OK != status )
                {
                    QC_ERROR( "Setup pipeline failed for inputId=%d!", inputId );
                    break;
                }
            }
        }

        if ( QC_STATUS_OK == status )
        {
            status = SetupGlobalBufferIdMap();
        }

        if ( QC_STATUS_OK == status )
        {   // do buffer register during initialization
            for ( uint32_t bufferId : m_config.bufferIds )
            {
                if ( bufferId < buffers.size() )
                {
                    const QCBufferDescriptorBase_t &bufDesc = buffers[bufferId];
                    const QCSharedBufferDescriptor_t *pSharedBuffer =
                            dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
                    if ( nullptr == pSharedBuffer )
                    {
                        QC_ERROR( "buffer %" PRIu32 "is invalid", bufferId );
                        status = QC_STATUS_INVALID_BUF;
                    }
                    else
                    {
                        status = RegisterBuffers( &( pSharedBuffer->buffer ), 1 );
                    }
                }
                else
                {
                    QC_ERROR( "buffer index out of range" );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }

                if ( status != QC_STATUS_OK )
                {
                    break;
                }
            }
        }

        if ( QC_STATUS_OK == status )
        {
            m_state = QC_OBJECT_STATE_READY;
        }
    }

    return status;
}

QCStatus_e CL2DFlexImpl::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "CL2DFlex node not in ready status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        status2 = m_OpenclSrvObj.Deinit();
        if ( QC_STATUS_OK != status2 )
        {
            QC_ERROR( "Release CL resources failed!" );
            status = status2;
        }

        for ( uint32_t inputId = 0; inputId < m_config.params.numOfInputs; inputId++ )
        {
            if ( nullptr != m_pCL2DPipeline[inputId] )
            {
                m_pCL2DPipeline[inputId]->DeinitLogger();
                status2 = m_pCL2DPipeline[inputId]->Deinit();
                if ( QC_STATUS_OK != status2 )
                {
                    QC_ERROR( "Deinit pipeline failed for inputId=%d!", inputId );
                    status = status2;
                }
                delete m_pCL2DPipeline[inputId];
            }
        }
    }

    return status;
}

QCStatus_e CL2DFlexImpl::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<QCSharedBuffer_t> inputs;
    std::vector<QCSharedBuffer_t> outputs;
    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "CL2DFlex node not in running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        inputs.reserve( m_inputNum );
        for ( uint32_t i = 0; i < m_inputNum; i++ )
        {
            uint32_t globalBufferId = m_config.globalBufferIdMap[i].globalBufferId;
            QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( globalBufferId );
            const QCSharedBufferDescriptor_t *pSharedBuffer =
                    dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
            if ( nullptr == pSharedBuffer )
            {
                status = QC_STATUS_INVALID_BUF;
                break;
            }
            else
            {
                inputs.push_back( pSharedBuffer->buffer );
            }
        }

        if ( QC_STATUS_OK == status )
        {
            outputs.reserve( m_outputNum );
            for ( uint32_t i = m_inputNum; i < m_inputNum + m_outputNum; i++ )
            {
                uint32_t globalBufferId = m_config.globalBufferIdMap[i].globalBufferId;
                QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( globalBufferId );
                const QCSharedBufferDescriptor_t *pSharedBuffer =
                        dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
                if ( nullptr == pSharedBuffer )
                {
                    status = QC_STATUS_INVALID_BUF;
                    break;
                }
                else
                {
                    outputs.push_back( pSharedBuffer->buffer );
                }
            }
        }

        if ( QC_STATUS_OK == status )
        {
            if ( 0 == m_numOfROIs )
            {
                status = Execute( inputs.data(), inputs.size(), outputs.data() );
            }
            else
            {
                status = ExecuteWithROI( inputs.data(), outputs.data(), m_ROIs, m_numOfROIs );
            }
        }
    }

    return status;
}

QCObjectState_e CL2DFlexImpl::GetState()
{
    return m_state;
}

QCStatus_e CL2DFlexImpl::RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
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
                    inputYDesc.image_width = (size_t) m_config.params.outputWidth;
                    inputYDesc.image_height = (size_t) m_config.params.outputHeight;
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
                    inputUVDesc.image_width = (size_t) m_config.params.outputWidth;
                    inputUVDesc.image_height = (size_t) m_config.params.outputHeight;
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

QCStatus_e CL2DFlexImpl::DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
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

QCStatus_e CL2DFlexImpl::Execute( const QCSharedBuffer_t *pInputs, const uint32_t numInputs,
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
    else if ( numInputs != m_config.params.numOfInputs )
    {
        QC_ERROR( "number of inputs not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != pOutput->type )
    {
        QC_ERROR( "Output buffer is not image type!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.params.outputFormat != pOutput->imgProps.format )
    {
        QC_ERROR( "Output image format not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.params.outputWidth != pOutput->imgProps.width )
    {
        QC_ERROR( "Output image width not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.params.outputHeight != pOutput->imgProps.height )
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
            else if ( m_config.params.inputFormats[inputId] != pInputs[inputId].imgProps.format )
            {
                QC_ERROR( "Input image format not match for inputId=%d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( m_config.params.inputWidths[inputId] != pInputs[inputId].imgProps.width )
            {
                QC_ERROR( "Input image width not match for inputId=%d!", inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( m_config.params.inputHeights[inputId] != pInputs[inputId].imgProps.height )
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

QCStatus_e CL2DFlexImpl::ExecuteWithROI( const QCSharedBuffer_t *pInput,
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
    else if ( 1 != m_config.params.numOfInputs )
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
    else if ( m_config.params.outputFormat != pOutput->imgProps.format )
    {
        QC_ERROR( "Output image format not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.params.outputWidth != pOutput->imgProps.width )
    {
        QC_ERROR( "Output image width not match!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.params.outputHeight != pOutput->imgProps.height )
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
    else if ( m_config.params.inputFormats[0] != pInput->imgProps.format )
    {
        QC_ERROR( "Input image format not match" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.params.inputWidths[0] != pInput->imgProps.width )
    {
        QC_ERROR( "Input image width not match" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_config.params.inputHeights[0] != pInput->imgProps.height )
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

QCStatus_e CL2DFlexImpl::SetupGlobalBufferIdMap()
{
    QCStatus_e status = QC_STATUS_OK;

    m_inputNum = m_config.params.numOfInputs;
    m_outputNum = 1;

    if ( 0 < m_config.globalBufferIdMap.size() )
    {
        if ( ( m_inputNum + m_outputNum ) != m_config.globalBufferIdMap.size() )
        {
            QC_ERROR( "global buffer map size is not correct: expect %" PRIu32,
                      m_inputNum + m_outputNum + 1 );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }
    else
    { /* create a default global buffer index map */
        m_config.globalBufferIdMap.resize( m_inputNum + m_outputNum );
        uint32_t globalBufferId = 0;
        for ( uint32_t i = 0; i < m_inputNum; i++ )
        {
            m_config.globalBufferIdMap[globalBufferId].name = "Input" + std::to_string( i );
            m_config.globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
            globalBufferId++;
        }
        for ( uint32_t i = 0; i < m_outputNum; i++ )
        {
            m_config.globalBufferIdMap[globalBufferId].name = "Output";
            m_config.globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
            globalBufferId++;
        }
    }
    return status;
}

}   // namespace Node
}   // namespace QC
