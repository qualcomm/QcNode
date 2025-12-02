// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/SampleSharedRing.hpp"

namespace QC
{
namespace sample
{

SampleSharedRing::SampleSharedRing() {}
SampleSharedRing::~SampleSharedRing() {}

QCStatus_e SampleSharedRing::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

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
        QC_ERROR( "invalid type <%s>\n", typeStr.c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        QC_ERROR( "no topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_queueDepth = Get( config, "queue_depth", 2 );

    return ret;
}

QCStatus_e SampleSharedRing::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }
    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        if ( m_bIsPub )
        {
            ret = m_sub.Init( name, m_topicName, m_queueDepth );
            if ( QC_STATUS_OK == ret )
            {
                ret = m_sharedPub.Init( name, m_topicName );
            }
        }
        else
        {
            ret = m_pub.Init( name, m_topicName );
            if ( QC_STATUS_OK == ret )
            {
                ret = m_sharedSub.Init( name, m_topicName, m_queueDepth );
            }
        }
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    return ret;
}

QCStatus_e SampleSharedRing::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    if ( m_bIsPub )
    {
        ret = m_sharedPub.Start();
        if ( QC_STATUS_OK == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleSharedRing::ThreadPubMain, this );
        }
    }
    else
    {
        ret = m_sharedSub.Start();
        if ( QC_STATUS_OK == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleSharedRing::ThreadSubMain, this );
        }
    }
    TRACE_END( SYSTRACE_TASK_START );

    return ret;
}

QCStatus_e SampleSharedRing::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

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

QCStatus_e SampleSharedRing::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

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
    QCStatus_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            PROFILER_BEGIN();
            TRACE_BEGIN( frames.FrameId( 0 ) );
            ret = m_sharedPub.Publish( frames );
            if ( QC_STATUS_OK == ret )
            {
                PROFILER_END();
                TRACE_END( frames.FrameId( 0 ) );
            }
            else
            {
                QC_ERROR( "Failed to publish for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
            }
        }
    }
}

void SampleSharedRing::ThreadSubMain()
{
    QCStatus_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sharedSub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            PROFILER_BEGIN();
            TRACE_BEGIN( frames.FrameId( 0 ) );
            ret = m_pub.Publish( frames );
            if ( QC_STATUS_OK == ret )
            {
                PROFILER_END();
                TRACE_END( frames.FrameId( 0 ) );
            }
            else
            {
                QC_ERROR( "Failed to publish for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
            }
        }
    }
}

REGISTER_SAMPLE( SharedRing, SampleSharedRing );

}   // namespace sample
}   // namespace QC
