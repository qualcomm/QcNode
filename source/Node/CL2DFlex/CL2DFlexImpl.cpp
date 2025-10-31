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
    QCStatus_e status = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Start", {} );
    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "CL2DFlex node start failed due to wrong state!" );
        status = QC_STATUS_BAD_STATE;
    }
    QC_TRACE_END( "Start", {} );

    return status;
}

QCStatus_e CL2DFlexImpl::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Stop", {} );
    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "CL2DFlex node stop failed due to wrong state!" );
        status = QC_STATUS_BAD_STATE;
    }
    QC_TRACE_END( "Stop", {} );

    return status;
}

QCStatus_e
CL2DFlexImpl::Initialize( std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e status = QC_STATUS_OK;


    QC_TRACE_INIT( [&]() {
        std::ostringstream oss;
        oss << "{";
        oss << "\"name\": \"" << m_nodeId.name << "\", ";
        oss << "\"processor\": \"" << "gpu" << "\", ";
        oss << "\"coreIds\": [0]";
        oss << "}";
        return oss.str();
    }() );

    QC_TRACE_BEGIN( "Init", {} );
    if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        QC_ERROR( "CL2DFlex not in initial state!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_OpenclSrvObj.Init( "Opencl", LOGGER_LEVEL_ERROR, m_config.params.priority,
                                      m_config.params.deviceId );
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
                                                             &m_config.params, &m_OpenclSrvObj,
                                                             buffers );
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
                    cl_mem bufferCL;
                    status = m_OpenclSrvObj.RegBufferDesc( buffers[bufferId], bufferCL );
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
    QC_TRACE_END( "Init", {} );

    return status;
}

QCStatus_e CL2DFlexImpl::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    QC_TRACE_BEGIN( "DeInit", {} );
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
    QC_TRACE_END( "DeInit", {} );

    return status;
}

QCStatus_e CL2DFlexImpl::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "CL2DFlex node not in running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        if ( QC_STATUS_OK == status )
        {
            uint32_t outputBufferId = m_config.globalBufferIdMap[m_inputNum].globalBufferId;
            ImageDescriptor_t &outputBufDesc =
                    dynamic_cast<ImageDescriptor_t &>( frameDesc.GetBuffer( outputBufferId ) );
            if ( m_config.params.outputFormat != outputBufDesc.format )
            {
                QC_ERROR( "Output image format not match!" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( m_config.params.outputWidth != outputBufDesc.width )
            {
                QC_ERROR( "Output image width not match!" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( m_config.params.outputHeight != outputBufDesc.height )
            {
                QC_ERROR( "Output image height not match!" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                for ( uint32_t inputId = 0; inputId < m_inputNum; inputId++ )
                {
                    uint32_t inputBufferId = m_config.globalBufferIdMap[inputId].globalBufferId;
                    ImageDescriptor_t &inputBufDesc = dynamic_cast<ImageDescriptor_t &>(
                            frameDesc.GetBuffer( inputBufferId ) );
                    if ( nullptr == inputBufDesc.pBuf )
                    {
                        QC_ERROR( "Input buffer data is null for inputId=%d!", inputId );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    else if ( m_config.params.inputFormats[inputId] != inputBufDesc.format )
                    {
                        QC_ERROR( "Input image format not match for inputId=%d!", inputId );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    else if ( m_config.params.inputWidths[inputId] != inputBufDesc.width )
                    {
                        QC_ERROR( "Input image width not match for inputId=%d!", inputId );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    else if ( m_config.params.inputHeights[inputId] != inputBufDesc.height )
                    {
                        QC_ERROR( "Input image height not match for inputId=%d!", inputId );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    else
                    {
                        if ( nullptr == m_pCL2DPipeline[inputId] )
                        {
                            QC_ERROR( "Pipeline not setup for inputId=%d!", inputId );
                            status = QC_STATUS_FAIL;
                        }
                        else
                        {
                            QC_TRACE_BEGIN( "Execute",
                                            { QCNodeTraceArg( "frameId", inputBufDesc.id ) } );
                            status = m_pCL2DPipeline[inputId]->Execute( inputBufDesc,
                                                                        outputBufDesc );
                            QC_TRACE_END( "Execute", {} );
                        }
                    }
                    if ( QC_STATUS_OK != status )
                    {
                        QC_ERROR( "Failed to execute pipeline for inputId=%d!", inputId );
                        break;
                    }
                }
            }
        }
    }

    return status;
}

QCObjectState_e CL2DFlexImpl::GetState()
{
    return m_state;
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
