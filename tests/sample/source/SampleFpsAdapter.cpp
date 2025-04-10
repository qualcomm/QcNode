// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/sample/SampleFpsAdapter.hpp"


namespace ridehal
{
namespace sample
{

static uint32_t GetNumBitsOfInteger( uint32_t nInteger )
{
    uint32_t nTmp = nInteger;
    uint32_t n = 0;

    while ( 0 != nTmp )
    {
        nTmp = ( nTmp >> 1U );
        ++n;
    }

    return n;
}

SampleFpsAdapter::SampleFpsAdapter() {}
SampleFpsAdapter::~SampleFpsAdapter() {}


RideHalError_e SampleFpsAdapter::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_frameDropPattern = Get( config, "frame_drop_patten", (uint32_t) 0 );
    if ( 0 == m_frameDropPattern )
    {
        RIDEHAL_ERROR( "invalid frame drop pattern" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_frameDropPeriod = GetNumBitsOfInteger( m_frameDropPattern );

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        RIDEHAL_ERROR( "no input topic" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleFpsAdapter::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    TRACE_BEGIN( SYSTRACE_TASK_INIT );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }
    TRACE_END( SYSTRACE_TASK_INIT );

    return ret;
}

RideHalError_e SampleFpsAdapter::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    m_curPeriod = 0;
    m_stop = false;
    m_thread = std::thread( &SampleFpsAdapter::ThreadMain, this );
    TRACE_END( SYSTRACE_TASK_START );

    return ret;
}

void SampleFpsAdapter::ThreadMain()
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
            if ( 0 != ( m_frameDropPattern & ( 1 << m_curPeriod ) ) )
            {
                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                m_pub.Publish( frames );
                PROFILER_END();
                TRACE_END( frames.FrameId( 0 ) );
            }
            m_curPeriod++;
            if ( m_curPeriod >= m_frameDropPeriod )
            {
                m_curPeriod = 0;
            }
        }
    }
}

RideHalError_e SampleFpsAdapter::Stop()
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

RideHalError_e SampleFpsAdapter::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( FpsAdapter, SampleFpsAdapter );

}   // namespace sample
}   // namespace ridehal
