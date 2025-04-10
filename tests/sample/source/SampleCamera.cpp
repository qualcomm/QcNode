// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleCamera.hpp"
#include <time.h>

namespace ridehal
{
namespace sample
{

SampleCamera::SampleCamera() {}
SampleCamera ::~SampleCamera() {}

void SampleCamera::FrameCallBack( CameraFrame_t *pFrame )
{
    DataFrames_t frames;
    DataFrame_t frame;
    SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;
    pSharedBuffer->sharedBuffer = pFrame->sharedBuffer;
    pSharedBuffer->pubHandle =
            (uint64_t) pFrame->frameIndex + ( (uint64_t) pFrame->streamId << 32 );

    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        uint32_t frameIndex = pSharedBuffer->pubHandle & 0xFFFFFFFFul;
        CameraFrame_t camFrame;
        camFrame.sharedBuffer = pSharedBuffer->sharedBuffer;
        camFrame.frameIndex = frameIndex;
        camFrame.streamId = pSharedBuffer->pubHandle >> 32;
        if ( true == m_bImmediateRelease )
        {
            /* do nothing as immediate release in the callback */
        }
        else if ( ( 0 != m_camConfig.clientId ) && ( false == m_camConfig.bPrimary ) )
        {
            /* do nothing for multi-client non-primary session */
        }
        else if ( false == m_camConfig.bRequestMode )
        {
            m_camera.ReleaseFrame( &camFrame );
        }
        else
        {
            m_camera.RequestFrame( &camFrame );
        }
        delete pSharedBuffer;
    } );

    frame.frameId = m_frameId[pFrame->streamId]++;
    frame.buffer = buffer;
    frame.timestamp = pFrame->timestamp;
    frames.Add( frame );

    if ( m_camConfig.streamConfig[0].streamId == pFrame->streamId )
    { /* trace and profiling the first stream */
        PROFILER_BEGIN();
        PROFILER_END();
        TRACE_EVENT( frame.frameId );
    }
    else
    { /* profiling the other streams */
        TRACE_CAMERA_EVENT( pFrame->streamId, frame.frameId );
    }
    auto it = m_pubMap.find( pFrame->streamId );
    if ( m_pubMap.end() != it )
    {
        it->second->Publish( frames );
    }
    else
    {
        RIDEHAL_ERROR( "no publisher for stream %u", pFrame->streamId );
    }

    if ( true == m_bImmediateRelease )
    {
        if ( ( 0 != m_camConfig.clientId ) && ( false == m_camConfig.bPrimary ) )
        {
            /* do nothing for multi-client non-primary session */
        }
        else if ( false == m_camConfig.bRequestMode )
        {
            m_camera.ReleaseFrame( pFrame );
        }
        else
        {
            m_camera.RequestFrame( pFrame );
        }
    }
}

void SampleCamera::EventCallBack( const uint32_t eventId, const void *pPayload )
{
    RIDEHAL_INFO( "Received event: %d, pPayload:%p", eventId, pPayload );
}

void SampleCamera::FrameCallBack( CameraFrame_t *pFrame, void *pPrivData )
{
    SampleCamera *self = (SampleCamera *) pPrivData;

    self->FrameCallBack( pFrame );
}

void SampleCamera::EventCallBack( const uint32_t eventId, const void *pPayload, void *pPrivData )
{
    SampleCamera *self = (SampleCamera *) pPrivData;
    self->EventCallBack( eventId, pPayload );
}

RideHalError_e SampleCamera::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_ON( CAMERA );

        m_camConfig.numStream = Get( config, "number", 1 );
        m_camConfig.inputId = Get( config, "input_id", -1 );
        if ( -1 == m_camConfig.inputId )
        {
            RIDEHAL_ERROR( "invalid input id = %d", m_camConfig.inputId );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_camConfig.srcId = Get( config, "src_id", 0 );
        m_camConfig.inputMode = Get( config, "input_mode", 0 );

        m_camConfig.bRequestMode = Get( config, "request_mode", false );
        m_camConfig.bAllocator = true;
        m_camConfig.ispUserCase = Get( config, "isp_use_case", 3 );

        m_camConfig.clientId = Get( config, "client_id", 0u );
        m_camConfig.bPrimary = Get( config, "is_primary", false );
        m_camConfig.bRecovery = Get( config, "recovery", false );

        for ( uint32_t i = 0; i < m_camConfig.numStream; i++ )
        {
            std::string suffix = "";
            if ( i > 0 )
            {
                suffix = std::to_string( i );
            }
            m_camConfig.streamConfig[i].width = Get( config, "width" + suffix, 0 );
            if ( 0 == m_camConfig.streamConfig[i].width )
            {
                RIDEHAL_ERROR( "invalid width for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].height = Get( config, "height" + suffix, 0 );
            if ( 0 == m_camConfig.streamConfig[i].height )
            {
                RIDEHAL_ERROR( "invalid height for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].format =
                    Get( config, "format" + suffix, RIDEHAL_IMAGE_FORMAT_NV12 );
            if ( RIDEHAL_IMAGE_FORMAT_MAX == m_camConfig.streamConfig[i].format )
            {
                RIDEHAL_ERROR( "invalid format for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].bufCnt = Get( config, "pool_size" + suffix, 4 );
            if ( 0 == m_camConfig.streamConfig[i].bufCnt )
            {
                RIDEHAL_ERROR( "invalid pool_size for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].submitRequestPattern =
                    Get( config, "submit_request_pattern" + suffix, 0 );
            if ( m_camConfig.streamConfig[i].submitRequestPattern > 10 )
            {
                RIDEHAL_ERROR( "invalid submit_request_pattern for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].streamId = Get( config, "stream_id" + suffix, i );

            std::string topicName = Get( config, "topic" + suffix, "" );
            if ( "" == topicName )
            {
                RIDEHAL_ERROR( "no topic for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                m_topicNameMap[m_camConfig.streamConfig[i].streamId] = topicName;
            }

            m_frameId[m_camConfig.streamConfig[i].streamId] = 0;
        }

        m_camConfig.camFrameDropPattern = Get( config, "frame_drop_patten", (uint32_t) 0 );
        m_camConfig.camFrameDropPeriod = Get( config, "frame_drop_period", (uint32_t) 0 );

        m_camConfig.opMode = Get( config, "op_mode", (uint32_t) QCARCAM_OPMODE_OFFLINE_ISP );

        m_bIgnoreError = Get( config, "ignore_error", false );
        m_bImmediateRelease = Get( config, "immediate_release", false );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_camera.Init( (char *) name.c_str(), &m_camConfig );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = m_camera.RegisterCallback( SampleCamera::FrameCallBack,
                                             SampleCamera::EventCallBack, (void *) this );
        }
        else
        {
            if ( m_bIgnoreError )
            {
                RIDEHAL_ERROR( "Init failed: %d, ignore it" );
                ret = RIDEHAL_ERROR_NONE;
            }
        }
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_camConfig.numStream; i++ )
        {
            uint32_t streamId = m_camConfig.streamConfig[i].streamId;
            uint32_t bufCnt = m_camConfig.streamConfig[i].bufCnt;
            std::vector<RideHal_SharedBuffer_t> buffers;
            buffers.resize( bufCnt );
            ret = m_camera.GetBuffers( buffers.data(), bufCnt, streamId );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = SampleIF::RegisterBuffers( name + "." + std::to_string( streamId ),
                                                 buffers.data(), bufCnt );
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "failed to register buffers for stream %u", streamId );
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_camConfig.numStream; i++ )
        {
            uint32_t streamId = m_camConfig.streamConfig[i].streamId;
            std::string topicName = m_topicNameMap[streamId];
            m_pubMap[streamId] = std::make_shared<DataPublisher<DataFrames_t>>();
            ret = m_pubMap[streamId]->Init( name + "." + std::to_string( streamId ), topicName );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "create topic %s for stream %u failed: %d", topicName.c_str(), i,
                               ret );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e SampleCamera::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_camera.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( ret != RIDEHAL_ERROR_NONE )
    {
        if ( m_bIgnoreError )
        {
            RIDEHAL_ERROR( "Start failed: %d, ignore it", ret );
            ret = RIDEHAL_ERROR_NONE;
        }
    }

    return ret;
}

RideHalError_e SampleCamera::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_camera.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleCamera::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_camera.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( Camera, SampleCamera );

}   // namespace sample
}   // namespace ridehal
