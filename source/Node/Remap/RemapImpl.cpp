// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "RemapImpl.hpp"

namespace QC
{
namespace Node
{

QCStatus_e RemapImpl::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Start", {} );
    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "Remap node start failed due to wrong state!" );
        status = QC_STATUS_BAD_STATE;
    }
    QC_TRACE_END( "Start", {} );

    return status;
}

QCStatus_e RemapImpl::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Stop", {} );
    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "Remap node stop failed due to wrong state!" );
        status = QC_STATUS_BAD_STATE;
    }
    QC_TRACE_END( "Stop", {} );

    return status;
}

QCStatus_e
RemapImpl::Initialize( std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e status = QC_STATUS_OK;


    QC_TRACE_INIT( [&]() {
        std::ostringstream oss;
        std::string processor = "unknown";
        switch ( m_config.params.processor )
        {
            case QC_PROCESSOR_HTP0:
                processor = "htp0";
                break;
            case QC_PROCESSOR_HTP1:
                processor = "htp1";
                break;
            case QC_PROCESSOR_CPU:
                processor = "cpu";
                break;
            case QC_PROCESSOR_GPU:
                processor = "gpu";
                break;
            default:
                break;
        }
        oss << "{";
        oss << "\"name\": \"" << m_nodeId.name << "\", ";
        oss << "\"processor\": \"" << processor << "\", ";
        oss << "\"coreIds\": [0]";
        oss << "}";
        return oss.str();
    }() );

    QC_TRACE_BEGIN( "Init", {} );
    if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        QC_ERROR( "Remap not in initial state!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        QC_INFO( "REMAP node version: %u.%u.%u", QCNODE_REMAP_VERSION_MAJOR,
                 QCNODE_REMAP_VERSION_MINOR, QCNODE_REMAP_VERSION_PATCH );

        status = m_fadasRemapObj.Init( m_config.params.processor, "Remap", LOGGER_LEVEL_ERROR );
        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "Failed to init fadas remap!" );
        }
        else
        {
            status = m_fadasRemapObj.SetRemapParams(
                    m_config.params.numOfInputs, m_config.params.outputWidth,
                    m_config.params.outputHeight, m_config.params.outputFormat,
                    m_config.params.normlzR, m_config.params.normlzG, m_config.params.normlzB,
                    m_config.params.bEnableUndistortion, m_config.params.bEnableNormalize );

            if ( QC_STATUS_OK != status )
            {
                QC_ERROR( "Failed to set parameters!" );
            }
            else
            {
                for ( uint32_t inputId = 0; inputId < m_config.params.numOfInputs; inputId++ )
                {
                    status = m_fadasRemapObj.CreateRemapWorker(
                            inputId, m_config.params.inputConfigs[inputId].inputFormat,
                            m_config.params.inputConfigs[inputId].inputWidth,
                            m_config.params.inputConfigs[inputId].inputHeight,
                            m_config.params.inputConfigs[inputId].ROI );
                    if ( QC_STATUS_OK != status )
                    {
                        QC_ERROR( "Create worker fail at inputId = %d", inputId );
                        break;
                    }

                    TensorDescriptor_t *pMapXTensorDesc = nullptr;
                    TensorDescriptor_t *pMapYTensorDesc = nullptr;
                    if ( true == m_config.params.bEnableUndistortion )
                    {
                        uint32_t mapXBufferId =
                                m_config.params.inputConfigs[inputId].remapTable.mapXBufferId;
                        QCBufferDescriptorBase_t &mapXBufDesc = buffers[mapXBufferId];
                        pMapXTensorDesc = dynamic_cast<TensorDescriptor_t *>( &mapXBufDesc );
                        if ( nullptr == pMapXTensorDesc )
                        {
                            QC_ERROR( "null mapX buffer!" );
                            break;
                        }

                        uint32_t mapYBufferId =
                                m_config.params.inputConfigs[inputId].remapTable.mapYBufferId;
                        QCBufferDescriptorBase_t &mapYBufDesc = buffers[mapYBufferId];
                        pMapYTensorDesc = dynamic_cast<TensorDescriptor_t *>( &mapYBufDesc );
                        if ( nullptr == pMapYTensorDesc )
                        {
                            QC_ERROR( "null mapY buffer!" );
                            break;
                        }
                    }
                    status = m_fadasRemapObj.CreatRemapTable(
                            inputId, m_config.params.inputConfigs[inputId].mapWidth,
                            m_config.params.inputConfigs[inputId].mapHeight, *pMapXTensorDesc,
                            *pMapYTensorDesc );
                    if ( QC_STATUS_OK != status )
                    {
                        QC_ERROR( "Create remap table fail at inputId = %d", inputId );
                        break;
                    }
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
                    int32_t fd = m_fadasRemapObj.RegBuf( bufDesc, FADAS_BUF_TYPE_INOUT );
                    if ( 0 > fd )
                    {
                        QC_ERROR( "Failed to register buffer for bufferId = %d!", bufferId );
                        status = QC_STATUS_FAIL;
                        break;
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
    QC_TRACE_END( "Init", {} );

    return status;
}

QCStatus_e RemapImpl::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2 = QC_STATUS_OK;

    QC_TRACE_BEGIN( "DeInit", {} );
    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Remap node not in ready status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        status2 = m_fadasRemapObj.DestroyMap();
        if ( QC_STATUS_OK != status2 )
        {
            QC_ERROR( "Destroy map failed!" );
            status = QC_STATUS_FAIL;
        }
        status2 = m_fadasRemapObj.DestroyWorkers();
        if ( QC_STATUS_OK != status2 )
        {
            QC_ERROR( "Destroy worker failed!" );
            status = QC_STATUS_FAIL;
        }
        status2 = m_fadasRemapObj.Deinit();
        if ( QC_STATUS_OK != status2 )
        {
            QC_ERROR( "Deinit fadas remap failed!" );
            status = QC_STATUS_FAIL;
        }
    }
    QC_TRACE_END( "DeInit", {} );

    return status;
}

QCStatus_e RemapImpl::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Execute", { QCNodeTraceArg( "frameId", [&]() {
                        uint64_t frameId = 0;
                        const BufferDescriptor_t *pBufDesc =
                                dynamic_cast<BufferDescriptor_t *>( &frameDesc.GetBuffer( 0 ) );
                        frameId = pBufDesc->id;
                        return frameId;
                    }() ) } );
    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Remap node not in running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_fadasRemapObj.RemapRun( frameDesc );
    }
    QC_TRACE_END( "Execute", {} );

    return status;
}

QCObjectState_e RemapImpl::GetState()
{
    return m_state;
}

QCStatus_e RemapImpl::SetupGlobalBufferIdMap()
{
    QCStatus_e status = QC_STATUS_OK;

    if ( 0 < m_config.globalBufferIdMap.size() )
    {
        if ( ( m_config.params.numOfInputs + 1 ) != m_config.globalBufferIdMap.size() )
        {
            QC_ERROR( "global buffer map size is not correct: expect %" PRIu32,
                      m_config.params.numOfInputs + 1 );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }
    else
    { /* create a default global buffer index map */
        m_config.globalBufferIdMap.resize( m_config.params.numOfInputs + 1 );
        uint32_t globalBufferId = 0;
        for ( uint32_t i = 0; i < m_config.params.numOfInputs; i++ )
        {
            m_config.globalBufferIdMap[globalBufferId].name = "Input" + std::to_string( i );
            m_config.globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
            globalBufferId++;
        }
        m_config.globalBufferIdMap[globalBufferId].name = "Output";
        m_config.globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
        globalBufferId++;
    }
    return status;
}

}   // namespace Node
}   // namespace QC
