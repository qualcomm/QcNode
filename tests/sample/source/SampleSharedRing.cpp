// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/sample/SampleSharedRing.hpp"

namespace ridehal
{
namespace sample
{

SampleSharedRing::SampleSharedRing() {}
SampleSharedRing::~SampleSharedRing() {}

RideHalError_e SampleSharedRing::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    auto typeStr = Get( config, "type", "pub" );
    if ( "pub" == typeStr )
    {
        m_bIsPub = true;
    }
    else if ( "sub" == typeStr )
    {
        m_bIsPub = false;
    }
    else
    {
        RIDEHAL_ERROR( "invalid type <%s>\n", typeStr.c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_queueDepth = Get( config, "queue_depth", 2 );

    return ret;
}

RideHalError_e SampleSharedRing::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        if ( m_bIsPub )
        {
            ret = m_sub.Init( name, m_topicName, m_queueDepth );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = m_sharedPub.Init( name, m_topicName );
            }
        }
        else
        {
            ret = m_pub.Init( name, m_topicName );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = m_sharedSub.Init( name, m_topicName, m_queueDepth );
            }
        }
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    return ret;
}

RideHalError_e SampleSharedRing::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    if ( m_bIsPub )
    {
        ret = m_sharedPub.Start();
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleSharedRing::ThreadPubMain, this );
        }
    }
    else
    {
        ret = m_sharedSub.Start();
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleSharedRing::ThreadSubMain, this );
        }
    }
    TRACE_END( SYSTRACE_TASK_START );

    return ret;
}

RideHalError_e SampleSharedRing::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    if ( m_bIsPub )
    {
        ret = m_sharedPub.Stop();
    }
    else
    {
        ret = m_sharedSub.Stop();
    }
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleSharedRing::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    if ( m_bIsPub )
    {
        ret = m_sharedPub.Deinit();
    }
    else
    {
        ret = m_sharedSub.Deinit();
    }
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}


void SampleSharedRing::ThreadPubMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            PROFILER_BEGIN();
            TRACE_BEGIN( frames.FrameId( 0 ) );
            ret = m_sharedPub.Publish( frames );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                PROFILER_END();
                TRACE_END( frames.FrameId( 0 ) );
            }
            else
            {
                RIDEHAL_ERROR( "Failed to publish for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
            }
        }
    }
}

void SampleSharedRing::ThreadSubMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sharedSub.Receive( frames );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            PROFILER_BEGIN();
            TRACE_BEGIN( frames.FrameId( 0 ) );
            ret = m_pub.Publish( frames );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                PROFILER_END();
                TRACE_END( frames.FrameId( 0 ) );
            }
            else
            {
                RIDEHAL_ERROR( "Failed to publish for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
            }
        }
    }
}

REGISTER_SAMPLE( SharedRing, SampleSharedRing );

}   // namespace sample
}   // namespace ridehal
