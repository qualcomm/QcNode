// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SampleFpsAdapter.hpp"


namespace QC
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


QCStatus_e SampleFpsAdapter::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_frameDropPattern = Get( config, "frame_drop_patten", (uint32_t) 0 );
    if ( 0 == m_frameDropPattern )
    {
        QC_ERROR( "invalid frame drop pattern" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_frameDropPeriod = GetNumBitsOfInteger( m_frameDropPattern );

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e SampleFpsAdapter::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    TRACE_BEGIN( SYSTRACE_TASK_INIT );
    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }
    TRACE_END( SYSTRACE_TASK_INIT );

    return ret;
}

QCStatus_e SampleFpsAdapter::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    m_curPeriod = 0;
    m_stop = false;
    m_thread = std::thread( &SampleFpsAdapter::ThreadMain, this );
    TRACE_END( SYSTRACE_TASK_START );

    return ret;
}

void SampleFpsAdapter::ThreadMain()
{
    QCStatus_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
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

QCStatus_e SampleFpsAdapter::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

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

QCStatus_e SampleFpsAdapter::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( FpsAdapter, SampleFpsAdapter );

}   // namespace sample
}   // namespace QC
