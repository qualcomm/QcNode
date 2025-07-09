// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/Camera.hpp"

namespace QC
{
namespace Node
{

QCStatus_e CameraConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<DataTree> streamConfigs;
    std::vector<uint32_t> bufferIds;

    std::string name = dt.Get<std::string>( "name", "" );
    if ( "" == name )
    {
        errors += "the name is empty, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t id = dt.Get<uint32_t>( "id", UINT32_MAX );
        if ( UINT32_MAX == id )
        {
            errors += "the id is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t inputId = dt.Get<uint32_t>( "inputId", UINT32_MAX );
        if ( UINT32_MAX == inputId )
        {
            errors += "the inputId is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t clientId = dt.Get<uint32_t>( "clientId", UINT32_MAX );
        if ( UINT32_MAX == clientId )
        {
            errors += "the clientId is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t srcId = dt.Get<uint32_t>( "srcId", UINT32_MAX );
        if ( UINT32_MAX == srcId )
        {
            errors += "the srcId is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t inputMode = dt.Get<uint32_t>( "inputMode", UINT32_MAX );
        if ( UINT32_MAX == inputMode )
        {
            errors += "the inputMode is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t ispUseCase = dt.Get<uint32_t>( "ispUseCase", UINT32_MAX );
        if ( UINT32_MAX == ispUseCase )
        {
            errors += "the ispUseCase is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t camFrameDropPattern = dt.Get<uint32_t>( "camFrameDropPattern", UINT32_MAX );
        if ( UINT32_MAX == camFrameDropPattern )
        {
            errors += "the camFrameDropPattern is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t camFrameDropPeriod = dt.Get<uint32_t>( "camFrameDropPeriod", UINT32_MAX );
        if ( UINT32_MAX == camFrameDropPeriod )
        {
            errors += "the camFrameDropPeriod is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t opMode = dt.Get<uint32_t>( "opMode", UINT32_MAX );
        if ( UINT32_MAX == opMode )
        {
            errors += "the opMode is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        status = dt.Get( "streamConfigs", streamConfigs );
        if ( QC_STATUS_OK != status )
        {
            errors += "the streamConfigs is invalid";
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t numStream = streamConfigs.size();
        if ( numStream > MAX_CAMERA_STREAM )
        {
            errors += "the numStream is larger than maximum, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        for ( uint32_t i = 0; i < streamConfigs.size(); i++ )
        {
            DataTree streamConfig = streamConfigs[i];

            uint32_t streamId = streamConfig.Get<uint32_t>( "streamId", UINT32_MAX );
            if ( UINT32_MAX == streamId )
            {
                errors += "the streamId for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            uint32_t bufCnt = streamConfig.Get<uint32_t>( "bufCnt", UINT32_MAX );
            if ( UINT32_MAX == bufCnt )
            {
                errors += "the bufCnt for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( streamConfig.Exists( "bufferIds" ) )
            {
                std::vector<uint32_t> bufferIds =
                        streamConfig.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );
                if ( ( 0 == bufferIds.size() ) || ( bufCnt != bufferIds.size() ) )
                {
                    errors += "the bufferIds for stream " + std::to_string( i ) + "is invalid, ";
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
            }

            uint32_t width = streamConfig.Get<uint32_t>( "width", UINT32_MAX );
            if ( UINT32_MAX == width )
            {
                errors += "the width for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            uint32_t height = streamConfig.Get<uint32_t>( "height", UINT32_MAX );
            if ( UINT32_MAX == height )
            {
                errors += "the height for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            uint32_t submitRequestPattern =
                    streamConfig.Get<uint32_t>( "submitRequestPattern", UINT32_MAX );
            if ( UINT32_MAX == submitRequestPattern )
            {
                errors += "the submitRequestPattern for stream " + std::to_string( i ) +
                          " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            QCImageFormat_e format = streamConfig.GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
            if ( QC_IMAGE_FORMAT_MAX == format )
            {
                errors += "the format for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( QC_STATUS_OK != status )
            {
                QC_ERROR( "Config verification failed for stream %u, error: %s", i, errors );
                break;
            }
        }
    }
    else
    {
        QC_ERROR( "Config verification failed for CameraConfigIfs, error: %s", errors );
    }

    return status;
}

QCStatus_e CameraConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<DataTree> streamConfigs;

    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );
        m_config.params.inputId = dt.Get<uint32_t>( "inputId", UINT32_MAX );
        m_config.params.clientId = dt.Get<uint32_t>( "clientId", UINT32_MAX );
        m_config.params.srcId = dt.Get<uint32_t>( "srcId", UINT32_MAX );
        m_config.params.inputMode = dt.Get<uint32_t>( "inputMode", UINT32_MAX );
        m_config.params.ispUserCase = dt.Get<uint32_t>( "ispUseCase", UINT32_MAX );
        m_config.params.camFrameDropPattern = dt.Get<uint32_t>( "camFrameDropPattern", UINT32_MAX );
        m_config.params.camFrameDropPeriod = dt.Get<uint32_t>( "camFrameDropPeriod", UINT32_MAX );
        m_config.params.opMode = dt.Get<uint32_t>( "opMode", UINT32_MAX );
        status = dt.Get( "streamConfigs", streamConfigs );
    }

    if ( QC_STATUS_OK == status )
    {
        m_config.params.bAllocator = true;
        m_config.params.numStream = streamConfigs.size();
        for ( uint32_t i = 0; i < streamConfigs.size(); i++ )
        {
            DataTree streamConfig = streamConfigs[i];

            m_config.params.streamConfig[i].streamId =
                    streamConfig.Get<uint32_t>( "streamId", UINT32_MAX );
            m_config.params.streamConfig[i].bufCnt =
                    streamConfig.Get<uint32_t>( "bufCnt", UINT32_MAX );
            if ( streamConfig.Exists( "bufferIds" ) )
            {
                std::vector<uint32_t> bufferIds =
                        streamConfig.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );
                m_config.streamBufferIds.push_back( bufferIds );
                m_config.params.bAllocator = false;
            }
            m_config.params.streamConfig[i].width =
                    streamConfig.Get<uint32_t>( "width", UINT32_MAX );
            m_config.params.streamConfig[i].height =
                    streamConfig.Get<uint32_t>( "height", UINT32_MAX );
            m_config.params.streamConfig[i].submitRequestPattern =
                    streamConfig.Get<uint32_t>( "submitRequestPattern", UINT32_MAX );
            m_config.params.streamConfig[i].format =
                    streamConfig.GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        m_config.params.bRequestMode = dt.Get<bool>( "requestMode", true );
        m_config.params.bPrimary = dt.Get<bool>( "primary", true );
        m_config.params.bRecovery = dt.Get<bool>( "recovery", true );
    }

    return status;
}

QCStatus_e CameraConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    DataTree dt;

    status = NodeConfigIfs::VerifyAndSet( config, errors );
    if ( QC_STATUS_OK == status )
    {

        status = m_dataTree.Get( "static", dt );
    }
    if ( QC_STATUS_OK == status )
    {
        status = ParseStaticConfig( dt, errors );
    }

    return status;
}

const std::string &CameraConfigIfs::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;
    return m_options;
}

const QCNodeConfigBase_t &CameraConfigIfs::Get()
{
    QCStatus_e status = QC_STATUS_OK;
    return m_config;
}

QCStatus_e Camera::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;

    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const CameraConfig_t *pConfig = dynamic_cast<const CameraConfig_t *>( &cfg );
    Camera_Config_t params;
    bool bNodeBaseInitDone = false;
    bool bCameraInitDone = false;

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
        params = pConfig->params;
        status = (QCStatus_e) m_camera.Init( m_nodeId.name.c_str(), &params );
    }

    if ( QC_STATUS_OK == status )
    {
        bCameraInitDone = true;
        m_bRequestMode = params.bRequestMode;
        m_callback = config.callback;
        if ( nullptr == m_callback )
        {
            status = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "callback is invalid" );
        }
    }
    else
    {
        QC_ERROR( "Failed to initialize camera component" );
    }

    if ( QC_STATUS_OK == status )
    {
        status = (QCStatus_e) m_camera.RegisterCallback( FrameCallback, EventCallback, this );
    }

    if ( QC_STATUS_OK == status )
    {
        if ( false == params.bAllocator )
        {
            for ( uint32_t i = 0; i < params.numStream; i++ )
            {
                uint32_t streamId = params.streamConfig[i].streamId;
                uint32_t bufCnt = params.streamConfig[i].bufCnt;
                QCSharedBuffer_t buffers[bufCnt];

                for ( uint32_t k = 0; k < bufCnt; k++ )
                {
                    uint32_t bufferId = pConfig->streamBufferIds[i][k];
                    if ( bufferId < config.buffers.size() )
                    {
                        const QCBufferDescriptorBase_t &bufDesc = config.buffers[bufferId];
                        const QCSharedBufferDescriptor_t *pSharedBuffer =
                                dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
                        if ( nullptr == pSharedBuffer )
                        {
                            status = QC_STATUS_INVALID_BUF;
                            QC_ERROR( "buffer %" PRIu32 "is invalid", bufferId );
                        }
                        buffers[k] = pSharedBuffer->buffer;
                    }
                    else
                    {
                        status = QC_STATUS_BAD_ARGUMENTS;
                        QC_ERROR( "buffer index out of range for stream %u", streamId );
                    }

                    if ( QC_STATUS_OK != status )
                    {
                        break;
                    }
                }

                if ( QC_STATUS_OK == status )
                {
                    status = (QCStatus_e) m_camera.SetBuffers( buffers, bufCnt, streamId );
                    if ( QC_STATUS_OK != status )
                    {
                        QC_ERROR( "Failed to set buffers for QCarCamera for stream %u", streamId );
                    }
                }
                if ( QC_STATUS_OK != status )
                {
                    break;
                }
            }
        }
    }

    if ( QC_STATUS_OK != status )
    {
        if ( bCameraInitDone )
        {
            (void) m_camera.Deinit();
        }
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e Camera::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    status = (QCStatus_e) m_camera.Start();
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to start camera component" );
    }

    return status;
}

QCStatus_e Camera::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( 0 );
    const QCSharedCameraFrameDescriptor_t *pCamFrameDesc =
            dynamic_cast<const QCSharedCameraFrameDescriptor_t *>( &bufDesc );

    if ( nullptr == pCamFrameDesc )
    {
        status = QC_STATUS_INVALID_BUF;
    }

    if ( QC_STATUS_OK == status )
    {
        status = ProcessFrame( pCamFrameDesc );
    }

    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to process camera frame" );
    }

    return status;
}

QCStatus_e Camera::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = (QCStatus_e) m_camera.Stop();
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to stop camera component" );
    }

    return status;
}

QCStatus_e Camera::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;

    status = (QCStatus_e) m_camera.Deinit();
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to deinitialize camera component" );
    }
    else
    {
        status = NodeBase::DeInitialize();
    }

    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to deinitialize NodeBase" );
    }

    return status;
}

QCStatus_e Camera::ProcessFrame( const QCSharedCameraFrameDescriptor_t *pFrame )
{
    QCStatus_e status = QC_STATUS_OK;
    QC::component::CameraFrame_t camFrame;

    camFrame.sharedBuffer = pFrame->buffer;
    camFrame.timestamp = pFrame->timestamp;
    camFrame.timestampQGPTP = pFrame->timestampQGPTP;
    camFrame.frameIndex = pFrame->frameIndex;
    camFrame.flags = pFrame->flags;
    camFrame.streamId = pFrame->streamId;
    if ( m_bRequestMode )
    {
        status = (QCStatus_e) m_camera.RequestFrame( &camFrame );
    }
    else
    {
        status = (QCStatus_e) m_camera.ReleaseFrame( &camFrame );
    }

    return status;
}

void Camera::FrameCallback( QC::component::CameraFrame_t *pFrame, void *pPrivData )
{
    Camera *self = reinterpret_cast<Camera *>( pPrivData );

    if ( nullptr != self )
    {
        self->FrameCallback( pFrame );
    }
    else
    {
        QC_LOG_ERROR( "Camera: pAppPriv is invalid" );
    }
}

void Camera::FrameCallback( QC::component::CameraFrame_t *pFrame )
{
    QCStatus_e status = QC_STATUS_OK;
    QCSharedFrameDescriptorNode frameDesc( 1 );
    QCSharedCameraFrameDescriptor_t camFrameDesc;

    if ( nullptr == m_callback )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "callback is invalid" );
    }

    if ( QC_STATUS_OK == status )
    {
        camFrameDesc.buffer = pFrame->sharedBuffer;
        camFrameDesc.timestamp = pFrame->timestamp;
        camFrameDesc.timestampQGPTP = pFrame->timestampQGPTP;
        camFrameDesc.frameIndex = pFrame->frameIndex;
        camFrameDesc.streamId = pFrame->streamId;
        camFrameDesc.flags = pFrame->flags;

        status = frameDesc.SetBuffer( 0, camFrameDesc );
    }

    if ( QC_STATUS_OK == status )
    {
        QCNodeEventInfo_t info( frameDesc, m_nodeId, QC_STATUS_OK,
                                static_cast<QCObjectState_e>( m_camera.GetState() ) );
        m_callback( info );
    }
    else
    {
        QC_ERROR( "Failed to set frame buffer descriptor" );
    }
}

void Camera::EventCallback( const uint32_t eventId, const void *pPayload, void *pPrivData )
{
    Camera *self = reinterpret_cast<Camera *>( pPrivData );

    self->EventCallback( eventId, pPayload );
}

void Camera::EventCallback( const uint32_t eventId, const void *pPayload )
{
    QCStatus_e status = QC_STATUS_OK;
    QCSharedFrameDescriptorNode frameDesc( 1 );
    QCBufferDescriptorBase_t eventDesc;
    CameraEvent_t event;

    if ( nullptr == m_callback )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "callback is invalid" );
    }

    if ( QC_STATUS_OK == status )
    {
        event.eventId = eventId;
        event.pPayload = pPayload;
        eventDesc.name = "Camera Event";
        eventDesc.pBuf = &event;
        eventDesc.size = sizeof( event );
        status = frameDesc.SetBuffer( 0, eventDesc );
    }

    if ( QC_STATUS_OK == status )
    {
        QCNodeEventInfo_t info( frameDesc, m_nodeId, QC_STATUS_FAIL,
                                static_cast<QCObjectState_e>( m_camera.GetState() ) );
        m_callback( info );
    }
    else
    {
        QC_ERROR( "Failed to set event buffer descriptor" );
    }
}

}   // namespace Node
}   // namespace QC

