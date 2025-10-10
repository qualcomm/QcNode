// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/Node/Radar.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

QCStatus_e RadarConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    std::string name = dt.Get<std::string>( "name", "" );
    if ( "" == name )
    {
        errors += "the name is empty, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t id = dt.Get<uint32_t>( "id", UINT32_MAX );
    if ( UINT32_MAX == id )
    {
        errors += "the id is empty, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t maxInputBufferSize = dt.Get<uint32_t>( "maxInputBufferSize", 0 );
    if ( 0 == maxInputBufferSize )
    {
        errors += "maxInputBufferSize is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t maxOutputBufferSize = dt.Get<uint32_t>( "maxOutputBufferSize", 0 );
    if ( 0 == maxOutputBufferSize )
    {
        errors += "maxOutputBufferSize is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string serviceName = dt.Get<std::string>( "serviceName", "" );
    if ( "" == serviceName )
    {
        errors += "serviceName is empty, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t timeoutMs = dt.Get<uint32_t>( "timeoutMs", 0 );
    if ( 0 == timeoutMs )
    {
        errors += "timeoutMs is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    // Validate globalBufferIdMap if present
    std::vector<DataTree> globalBufferIdMap;
    QCStatus_e status2 = dt.Get( "globalBufferIdMap", globalBufferIdMap );
    if ( QC_STATUS_OUT_OF_BOUND == status2 )
    {
        /* OK if not configured */
    }
    else if ( QC_STATUS_OK != status2 )
    {
        errors += "the globalBufferIdMap is invalid, ";
        status = status2;
    }
    else
    {
        uint32_t idx = 0;
        for ( DataTree &gbm : globalBufferIdMap )
        {
            std::string name = gbm.Get<std::string>( "name", "" );
            uint32_t index = gbm.Get<uint32_t>( "id", UINT32_MAX );
            if ( "" == name )
            {
                errors += "the globalIdMap " + std::to_string( idx ) + " name is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( UINT32_MAX == index )
            {
                errors += "the globalIdMap " + std::to_string( idx ) + " id is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            idx++;
        }
    }

    return status;
}

QCStatus_e RadarConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );

        m_config.numOfEntries = 2;

        m_config.params.maxInputBufferSize = dt.Get<uint32_t>( "maxInputBufferSize", 0 );
        m_config.params.maxOutputBufferSize = dt.Get<uint32_t>( "maxOutputBufferSize", 0 );

        m_config.params.serviceConfig.serviceName = dt.Get<std::string>( "serviceName", "" );
        m_config.params.serviceConfig.timeoutMs = dt.Get<uint32_t>( "timeoutMs", 5000 );
        m_config.params.serviceConfig.bEnablePerformanceLog =
                dt.Get<bool>( "enablePerformanceLog", false );

        m_config.bufferIds = dt.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );

        std::vector<DataTree> globalBufferIdMap;
        (void) dt.Get( "globalBufferIdMap", globalBufferIdMap );
        m_config.globalBufferIdMap.resize( globalBufferIdMap.size() );
        uint32_t idx = 0;
        for ( DataTree &gbm : globalBufferIdMap )
        {
            m_config.globalBufferIdMap[idx].name = gbm.Get<std::string>( "name", "" );
            m_config.globalBufferIdMap[idx].globalBufferId = gbm.Get<uint32_t>( "id", UINT32_MAX );
            idx++;
        }

        m_config.bDeRegisterAllBuffersWhenStop =
                dt.Get<bool>( "deRegisterAllBuffersWhenStop", false );
    }
    else
    {
        QC_ERROR( "VerifyStaticConfig failed!" );
    }

    return status;
}

QCStatus_e RadarConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    status = NodeConfigIfs::VerifyAndSet( config, errors );
    if ( QC_STATUS_OK == status )
    {
        DataTree dt;
        status = m_dataTree.Get( "static", dt );
        if ( QC_STATUS_OK == status )
        {
            status = ParseStaticConfig( dt, errors );
        }
        else
        {
            QC_ERROR( "Radar only support static config" );
        }
    }

    return status;
}

const std::string &RadarConfigIfs::GetOptions()
{
    // Return empty options for now
    return m_options;
}

QCStatus_e Radar::SetupGlobalBufferIdMap( const RadarConfig_t &cfg )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( cfg.globalBufferIdMap.size() > 0 )
    {
        if ( ( m_inputNum + m_outputNum ) != cfg.globalBufferIdMap.size() )
        {
            QC_ERROR( "global buffer map size is not correct: expect %u",
                      m_inputNum + m_outputNum );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            m_globalBufferIdMap = cfg.globalBufferIdMap;
        }
    }
    else
    {
        // Create a default global buffer index map
        m_globalBufferIdMap.resize( m_inputNum + m_outputNum );
        uint32_t globalBufferId = 0;

        // Input buffer
        m_globalBufferIdMap[globalBufferId].name = "Input";
        m_globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
        globalBufferId++;

        // Output buffer
        m_globalBufferIdMap[globalBufferId].name = "Output";
        m_globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
        globalBufferId++;
    }
    return status;
}

void Radar::NotifyEvent( QCFrameDescriptorNodeIfs &frameDesc, QCStatus_e status )
{
    if ( m_eventCallback )
    {
        QCNodeEventInfo_t eventInfo( frameDesc, m_nodeId, status, GetState() );
        m_eventCallback( eventInfo );
    }
}

QCStatus_e Radar::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const RadarConfig_t *pConfig = dynamic_cast<const RadarConfig_t *>( &cfg );
    bool bNodeBaseInitDone = false;
    bool bRadarInitDone = false;

    m_eventCallback = config.callback;

    status = m_configIfs.VerifyAndSet( config.config, errors );

    if ( QC_STATUS_OK == status )
    {
        status = NodeBase::Init( cfg.nodeId );
    }
    else
    {
        QC_ERROR( "config error: %s", errors.c_str() );
    }

    if ( QC_STATUS_OK == status )
    {
        bNodeBaseInitDone = true;
        status = m_radar.Init( m_nodeId.name.c_str(), &pConfig->params );
    }

    if ( QC_STATUS_OK == status )
    {
        status = SetupGlobalBufferIdMap( *pConfig );
    }

    if ( QC_STATUS_OK == status )
    {
        bRadarInitDone = true;
        m_bDeRegisterAllBuffersWhenStop = pConfig->bDeRegisterAllBuffersWhenStop;
    }

    if ( QC_STATUS_OK == status )
    {
        // Register buffers during initialization if specified and available
        if ( config.buffers.size() > 0 )
        {
            for ( uint32_t bufferId : pConfig->bufferIds )
            {
                if ( bufferId < config.buffers.size() )
                {
                    QCBufferDescriptorBase_t &bufDesc = config.buffers[bufferId].get();
                    const QCSharedBufferDescriptor_t *pSharedBuffer =
                            dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
                    if ( nullptr == pSharedBuffer )
                    {
                        QC_ERROR( "buffer %u is invalid", bufferId );
                        status = QC_STATUS_INVALID_BUF;
                    }
                    else
                    {
                        // Register as input buffer (assuming first buffer is input)
                        status = m_radar.RegisterInputBuffer( &( pSharedBuffer->buffer ) );
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
        else
        {
            // No buffers provided during initialization - this is acceptable
            // Buffers will be provided later via frame descriptors
            QC_DEBUG( "No buffers provided during initialization, will use frame descriptor "
                      "buffers" );
        }
    }

    if ( QC_STATUS_OK != status )
    {
        // Error cleanup
        if ( bRadarInitDone )
        {
            (void) m_radar.Deinit();
        }
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e Radar::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = m_radar.Deinit();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
    }

    status2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
    }

    return status;
}

QCStatus_e Radar::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_radar.Start();

    return status;
}

QCStatus_e Radar::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_radar.Stop();

    return status;
}

QCStatus_e Radar::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    // Ensure we have at least 2 buffers (input and output)
    if ( m_globalBufferIdMap.size() < 2 )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "Insufficient global buffer map entries: %zu", m_globalBufferIdMap.size() );
        NotifyEvent( frameDesc, status );
    }
    else
    {
        // Get input buffer (first entry in global buffer map)
        uint32_t inputGlobalBufferId = m_globalBufferIdMap[0].globalBufferId;
        QCBufferDescriptorBase_t &inputBufDesc = frameDesc.GetBuffer( inputGlobalBufferId );
        const QCSharedBufferDescriptor_t *pInputSharedBuffer =
                dynamic_cast<const QCSharedBufferDescriptor_t *>( &inputBufDesc );
        if ( nullptr == pInputSharedBuffer )
        {
            status = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Input buffer is invalid at global ID %u", inputGlobalBufferId );
            NotifyEvent( frameDesc, status );
        }
        else
        {
            // Get output buffer (second entry in global buffer map)
            uint32_t outputGlobalBufferId = m_globalBufferIdMap[1].globalBufferId;
            QCBufferDescriptorBase_t &outputBufDesc = frameDesc.GetBuffer( outputGlobalBufferId );
            const QCSharedBufferDescriptor_t *pOutputSharedBuffer =
                    dynamic_cast<const QCSharedBufferDescriptor_t *>( &outputBufDesc );
            if ( nullptr == pOutputSharedBuffer )
            {
                status = QC_STATUS_INVALID_BUF;
                QC_ERROR( "Output buffer is invalid at global ID %u", outputGlobalBufferId );
                NotifyEvent( frameDesc, status );
            }
            else
            {
                // Execute radar processing
                status = m_radar.Execute( &pInputSharedBuffer->buffer,
                                          &pOutputSharedBuffer->buffer );

                NotifyEvent( frameDesc, status );
            }
        }
    }


    return status;
}

}   // namespace Node
}   // namespace QC
