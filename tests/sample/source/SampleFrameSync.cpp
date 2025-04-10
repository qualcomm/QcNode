// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleFrameSync.hpp"

namespace ridehal
{
namespace sample
{

SampleFrameSync::SampleFrameSync() {}
SampleFrameSync::~SampleFrameSync() {}

RideHalError_e SampleFrameSync::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_number = Get( config, "number", 2 );
    if ( m_number < 2 )
    {
        RIDEHAL_ERROR( "invalid number = %u\n", m_number );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_windowMs = Get( config, "window", 100 );

    for ( uint32_t i = 0; i < m_number; i++ )
    {
        std::string topicName = Get( config, "input_topic" + std::to_string( i ), "" );
        if ( "" == topicName )
        {
            RIDEHAL_ERROR( "no input_topic%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        m_inputTopicNames.push_back( topicName );
    }

    std::vector<uint32_t> perms;
    m_perms = Get( config, "perms", perms );

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    auto syncModeStr = Get( config, "mode", "window" );

    if ( "window" == syncModeStr )
    {
        m_syncMode = FRAME_SYNC_MODE_WINDOW;
    }
    else
    {
        RIDEHAL_ERROR( "invalid mode %s\n", syncModeStr.c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleFrameSync::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_subs.resize( m_number );
        for ( uint32_t i = 0; ( i < m_number ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
        {
            ret = m_subs[i].Init( name + "_" + std::to_string( i ), m_inputTopicNames[i] );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleFrameSync::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = false;
    if ( FRAME_SYNC_MODE_WINDOW == m_syncMode )
    {
        m_thread = std::thread( &SampleFrameSync::threadWindowMain, this );
    }


    return ret;
}

void SampleFrameSync::threadWindowMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        uint64_t timeoutMs = (uint64_t) m_windowMs;
        std::vector<DataFrames_t> framesList;
        DataFrames_t frames;
        uint64_t frameId;
        ret = m_subs[0].Receive( frames, timeoutMs );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            framesList.push_back( frames );
            frameId = frames.FrameId( 0 );
            auto begin = std::chrono::high_resolution_clock::now();
            PROFILER_BEGIN();
            TRACE_BEGIN( frameId );
            RIDEHAL_DEBUG( "[0]receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            for ( uint32_t i = 1; i < m_number; i++ )
            {
                auto now = std::chrono::high_resolution_clock::now();
                uint64_t elapsedMs =
                        std::chrono::duration_cast<std::chrono::milliseconds>( now - begin )
                                .count();
                if ( (uint64_t) m_windowMs > elapsedMs )
                {
                    timeoutMs = (uint64_t) m_windowMs - elapsedMs;
                    ret = m_subs[i].Receive( frames, timeoutMs );
                }
                else
                {
                    ret = RIDEHAL_ERROR_TIMEOUT;
                }

                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "input %u frame not ready in %u ms", i, m_windowMs );
                    break;
                }
                else
                {
                    framesList.push_back( frames );
                    RIDEHAL_DEBUG( "[%u]receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", i,
                                   frames.FrameId( 0 ), frames.Timestamp( 0 ) );
                }
            }
            if ( framesList.size() == (size_t) m_number )
            {
                DataFrames_t outFrames;
                for ( auto &frames : framesList )
                {
                    for ( auto &frame : frames.frames )
                    {
                        outFrames.Add( frame );
                    }
                }
                if ( m_perms.size() == outFrames.frames.size() )
                {
                    DataFrames_t newFrames;
                    for ( auto i : m_perms )
                    {
                        newFrames.Add( outFrames.frames[i] );
                    }
                    m_pub.Publish( newFrames );
                }
                else
                {
                    m_pub.Publish( outFrames );
                }
                PROFILER_END();
                TRACE_END( frameId );
            }
        }
    }
}


RideHalError_e SampleFrameSync::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();
    return ret;
}

RideHalError_e SampleFrameSync::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    return ret;
}

REGISTER_SAMPLE( FrameSync, SampleFrameSync );

}   // namespace sample
}   // namespace ridehal
