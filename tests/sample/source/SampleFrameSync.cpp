// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/SampleFrameSync.hpp"

namespace QC
{
namespace sample
{

SampleFrameSync::SampleFrameSync() {}
SampleFrameSync::~SampleFrameSync() {}

QCStatus_e SampleFrameSync::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_number = Get( config, "number", 2 );
    if ( m_number < 2 )
    {
        QC_ERROR( "invalid number = %u\n", m_number );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_windowMs = Get( config, "window", 100 );

    for ( uint32_t i = 0; i < m_number; i++ )
    {
        std::string topicName = Get( config, "input_topic" + std::to_string( i ), "" );
        if ( "" == topicName )
        {
            QC_ERROR( "no input_topic%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        m_inputTopicNames.push_back( topicName );
    }

    std::vector<uint32_t> perms;
    m_perms = Get( config, "perms", perms );

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    auto syncModeStr = Get( config, "mode", "window" );

    if ( "window" == syncModeStr )
    {
        m_syncMode = FRAME_SYNC_MODE_WINDOW;
    }
    else
    {
        QC_ERROR( "invalid mode %s\n", syncModeStr.c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e SampleFrameSync::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_subs.resize( m_number );
        for ( uint32_t i = 0; ( i < m_number ) && ( QC_STATUS_OK == ret ); i++ )
        {
            ret = m_subs[i].Init( name + "_" + std::to_string( i ), m_inputTopicNames[i] );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

QCStatus_e SampleFrameSync::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = false;
    if ( FRAME_SYNC_MODE_WINDOW == m_syncMode )
    {
        m_thread = std::thread( &SampleFrameSync::threadWindowMain, this );
    }


    return ret;
}

void SampleFrameSync::threadWindowMain()
{
    QCStatus_e ret;
    while ( false == m_stop )
    {
        uint64_t timeoutMs = (uint64_t) m_windowMs;
        std::vector<DataFrames_t> framesList;
        DataFrames_t frames;
        uint64_t frameId;
        ret = m_subs[0].Receive( frames, timeoutMs );
        if ( QC_STATUS_OK == ret )
        {
            framesList.push_back( frames );
            frameId = frames.FrameId( 0 );
            auto begin = std::chrono::high_resolution_clock::now();
            PROFILER_BEGIN();
            TRACE_BEGIN( frameId );
            QC_DEBUG( "[0]receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
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
                    ret = QC_STATUS_TIMEOUT;
                }

                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "input %u frame not ready in %u ms", i, m_windowMs );
                    break;
                }
                else
                {
                    framesList.push_back( frames );
                    QC_DEBUG( "[%u]receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", i,
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


QCStatus_e SampleFrameSync::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();
    return ret;
}

QCStatus_e SampleFrameSync::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    return ret;
}

REGISTER_SAMPLE( FrameSync, SampleFrameSync );

}   // namespace sample
}   // namespace QC
