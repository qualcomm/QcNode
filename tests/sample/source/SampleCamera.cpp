// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/SampleCamera.hpp"
#include <time.h>

namespace QC
{
namespace sample
{

SampleCamera::SampleCamera() {}
SampleCamera ::~SampleCamera() {}

void SampleCamera::FrameCallBack( CameraFrame_t *pFrame )
{
    CameraFrame_t camFrame = *pFrame;

    if ( !m_stop )
    {
        std::unique_lock<std::mutex> lck( m_lock );
        m_camFrameQueue.push( camFrame );
        m_condVar.notify_one();
    }
    else
    {
        if ( false == m_camConfig.bRequestMode )
        {
            m_camera.ReleaseFrame( &camFrame );
        }
        else
        {
            m_camera.RequestFrame( &camFrame );
        }
    }
}

void SampleCamera::ProcessFrame( CameraFrame_t *pFrame )
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
        QC_ERROR( "no publisher for stream %u", pFrame->streamId );
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
    QC_INFO( "Received event: %d, pPayload:%p", eventId, pPayload );
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

QCStatus_e SampleCamera::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( CAMERA );

        m_camConfig.numStream = Get( config, "number", 1 );
        m_camConfig.inputId = Get( config, "input_id", -1 );
        if ( -1 == m_camConfig.inputId )
        {
            QC_ERROR( "invalid input id = %d", m_camConfig.inputId );
            ret = QC_STATUS_BAD_ARGUMENTS;
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
                QC_ERROR( "invalid width for stream %u", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].height = Get( config, "height" + suffix, 0 );
            if ( 0 == m_camConfig.streamConfig[i].height )
            {
                QC_ERROR( "invalid height for stream %u", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].format =
                    Get( config, "format" + suffix, QC_IMAGE_FORMAT_NV12 );
            if ( QC_IMAGE_FORMAT_MAX == m_camConfig.streamConfig[i].format )
            {
                QC_ERROR( "invalid format for stream %u", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].bufCnt = Get( config, "pool_size" + suffix, 4 );
            if ( 0 == m_camConfig.streamConfig[i].bufCnt )
            {
                QC_ERROR( "invalid pool_size for stream %u", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].submitRequestPattern =
                    Get( config, "submit_request_pattern" + suffix, 0 );
            if ( m_camConfig.streamConfig[i].submitRequestPattern > 10 )
            {
                QC_ERROR( "invalid submit_request_pattern for stream %u", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_camConfig.streamConfig[i].streamId = Get( config, "stream_id" + suffix, i );

            std::string topicName = Get( config, "topic" + suffix, "" );
            if ( "" == topicName )
            {
                QC_ERROR( "no topic for stream %u", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
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

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_camera.Init( (char *) name.c_str(), &m_camConfig );
        if ( QC_STATUS_OK == ret )
        {
            ret = m_camera.RegisterCallback( SampleCamera::FrameCallBack,
                                             SampleCamera::EventCallBack, (void *) this );
        }
        else
        {
            if ( m_bIgnoreError )
            {
                QC_ERROR( "Init failed: %d, ignore it" );
                ret = QC_STATUS_OK;
            }
        }
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < m_camConfig.numStream; i++ )
        {
            uint32_t streamId = m_camConfig.streamConfig[i].streamId;
            uint32_t bufCnt = m_camConfig.streamConfig[i].bufCnt;
            std::vector<QCSharedBuffer_t> buffers;
            buffers.resize( bufCnt );
            ret = m_camera.GetBuffers( buffers.data(), bufCnt, streamId );
            if ( QC_STATUS_OK == ret )
            {
                ret = SampleIF::RegisterBuffers( name + "." + std::to_string( streamId ),
                                                 buffers.data(), bufCnt );
            }

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "failed to register buffers for stream %u", streamId );
                break;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < m_camConfig.numStream; i++ )
        {
            uint32_t streamId = m_camConfig.streamConfig[i].streamId;
            std::string topicName = m_topicNameMap[streamId];
            m_pubMap[streamId] = std::make_shared<DataPublisher<DataFrames_t>>();
            ret = m_pubMap[streamId]->Init( name + "." + std::to_string( streamId ), topicName );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "create topic %s for stream %u failed: %d", topicName.c_str(), i, ret );
                break;
            }
        }
    }

    return ret;
}

QCStatus_e SampleCamera::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_camera.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( ret != QC_STATUS_OK )
    {
        if ( m_bIgnoreError )
        {
            QC_ERROR( "Start failed: %d, ignore it", ret );
            ret = QC_STATUS_OK;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleCamera::ThreadMain, this );
    }

    return ret;
}

void SampleCamera::ThreadMain()
{
    while ( !m_stop )
    {
        CameraFrame_t frame;
        std::unique_lock<std::mutex> lck( m_lock );
        (void) m_condVar.wait_for( lck, std::chrono::milliseconds( 10 ) );
        if ( !m_camFrameQueue.empty() )
        {
            frame = m_camFrameQueue.front();
            m_camFrameQueue.pop();
            ProcessFrame( &frame );
        }
    }
}

QCStatus_e SampleCamera::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    while ( !m_camFrameQueue.empty() )
    {
        std::unique_lock<std::mutex> lck( m_lock );
        CameraFrame_t frame = m_camFrameQueue.front();
        m_camFrameQueue.pop();
        ProcessFrame( &frame );
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_camera.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleCamera::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_camera.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( Camera, SampleCamera );

}   // namespace sample
}   // namespace QC
