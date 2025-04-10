// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleVideoDemuxer.hpp"

namespace ridehal
{
namespace sample
{

SampleVideoDemuxer::SampleVideoDemuxer() {};
SampleVideoDemuxer::~SampleVideoDemuxer() {};

RideHalError_e SampleVideoDemuxer::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_videoFile = Get( config, "input_file", "" );
    m_config.pVideoFileName = m_videoFile.c_str();
    if ( "" == m_config.pVideoFileName )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "input file missed" );
    }

    m_config.startFrameIdx = Get( config, "start_frame_idx", 0 );
    if ( m_config.startFrameIdx < 0 )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "start_frame_idx could not be negative" );
    }

    m_bReplayMode = Get( config, "replay_mode", true );

    m_fps = Get( config, "fps", 30 );
    if ( 0 == m_fps )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "invalid fps = %d\n", m_fps );
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "invalid pool_size = %d", m_poolSize );
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "no input topic" );
    }

    return ret;
}

RideHalError_e SampleVideoDemuxer::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_ON( CPU );
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_vidcDemuxer.Init( &m_config );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_vidcDemuxer.GetVideoInfo( m_videoInfo );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = m_videoInfo.frameWidth;
        imgProps.height = m_videoInfo.frameHeight;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = m_videoInfo.maxFrameSize;
        imgProps.format = m_videoInfo.format;
        ret = m_framePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProps,
                                RIDEHAL_BUFFER_USAGE_DEFAULT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleVideoDemuxer::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    TRACE_END( SYSTRACE_TASK_START );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleVideoDemuxer::ThreadMain, this );
    }

    return ret;
}

void SampleVideoDemuxer::ThreadMain()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    uint64_t frameIdx = 0;
    uint64_t frameCnt = 0;
    VidcDemuxer_FrameInfo_t frameInfo;

    while ( false == m_stop )
    {
        DataFrames_t frames;
        auto start = std::chrono::high_resolution_clock::now();
        PROFILER_BEGIN();

        std::shared_ptr<SharedBuffer_t> buffer = m_framePool.Get();
        if ( nullptr != buffer )
        {
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = m_vidcDemuxer.GetFrame( &buffer->sharedBuffer, frameInfo );
            }

            if ( RIDEHAL_ERROR_NONE == ret )
            {
                DataFrame_t frame;
                struct timespec ts;
                clock_gettime( CLOCK_MONOTONIC, &ts );
                frame.buffer = buffer;
                frame.frameId = frameIdx;
                frame.timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
                frames.Add( frame );
            }
            else if ( RIDEHAL_ERROR_OUT_OF_BOUND == ret )
            {
                if ( m_bReplayMode )
                {
                    m_playBackTime = 0;
                    ret = m_vidcDemuxer.SetPlayTime( m_playBackTime );
                    if ( RIDEHAL_ERROR_NONE == ret )
                    {
                        frameCnt = 0;
                        continue;
                    }
                    else
                    {
                        RIDEHAL_ERROR( "Failed to set play time for frame %lu", frameCnt );
                    }
                }
                else
                {
                    ret = RIDEHAL_ERROR_NONE;
                    break;
                }
            }
            else
            {
                RIDEHAL_ERROR( "Failed to get frame for index %lu", frameCnt );
            }
        }
        else
        {
            ret = RIDEHAL_ERROR_NOMEM;
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            PROFILER_END();
            TRACE_EVENT( frameIdx );
            m_pub.Publish( frames );
            frameIdx++;
            frameCnt++;
        }
        auto end = std::chrono::high_resolution_clock::now();
        uint64_t elapsedMs =
                std::chrono::duration_cast<std::chrono::milliseconds>( end - start ).count();
        RIDEHAL_DEBUG( "Loading frame %" PRIu64 " cost %" PRIu64 "ms", frameIdx, elapsedMs );
        if ( ( 1000 / m_fps ) > elapsedMs )
        {
            std::this_thread::sleep_for(
                    std::chrono::milliseconds( ( 1000 / m_fps ) - elapsedMs ) );
        }
    }
}

RideHalError_e SampleVideoDemuxer::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleVideoDemuxer::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_vidcDemuxer.DeInit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( VideoDemuxer, SampleVideoDemuxer );

}   // namespace sample
}   // namespace ridehal
