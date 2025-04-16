// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SampleDepthFromStereo.hpp"


namespace QC
{
namespace sample
{

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

SampleDepthFromStereo::SampleDepthFromStereo() {}
SampleDepthFromStereo::~SampleDepthFromStereo() {}

QCStatus_e SampleDepthFromStereo::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    std::string dirStr = Get( config, "direction", "l2r" );
    if ( "l2r" == dirStr )
    {
        m_config.dfsSearchDir = EVA_DFS_SEARCH_L2R;
    }
    else if ( "r2l" == dirStr )
    {
        m_config.dfsSearchDir = EVA_DFS_SEARCH_R2L;
    }
    else
    {
        QC_ERROR( "invalid direction %s\n", dirStr.c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.width = Get( config, "width", 1280u );
    m_config.height = Get( config, "height", 416u );
    m_config.format = Get( config, "format", QC_IMAGE_FORMAT_NV12 );
    m_config.frameRate = Get( config, "fps", 30u );

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferFlags = 0;
    }
    else
    {
        m_bufferFlags = QC_BUFFER_FLAGS_CACHE_WB_WA;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e SampleDepthFromStereo::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    uint32_t width = m_config.width;
    uint32_t height = m_config.height;

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t dispTsProp = { QC_TENSOR_TYPE_UINT_16,
                                       { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                       4 };

        ret = m_dispPool.Init( name + ".disp", LOGGER_LEVEL_INFO, m_poolSize, dispTsProp,
                               QC_BUFFER_USAGE_EVA, m_bufferFlags );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t confTsProp = { QC_TENSOR_TYPE_UINT_8,
                                       { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                       4 };

        ret = m_confPool.Init( name + ".conf", LOGGER_LEVEL_INFO, m_poolSize, confTsProp,
                               QC_BUFFER_USAGE_EVA, m_bufferFlags );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_dfs.Init( name.c_str(), &m_config );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

QCStatus_e SampleDepthFromStereo::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_dfs.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleDepthFromStereo::ThreadMain, this );
    }

    return ret;
}

void SampleDepthFromStereo::ThreadMain()
{
    QCStatus_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( 2 != frames.frames.size() )
        {
            QC_ERROR( "DepthFromStereo expect 2 input images" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            std::shared_ptr<SharedBuffer_t> disp = m_dispPool.Get();
            std::shared_ptr<SharedBuffer_t> conf = m_confPool.Get();
            if ( ( nullptr != disp ) && ( nullptr != conf ) )
            {
                QCSharedBuffer_t &priImg = frames.SharedBuffer( 0 );
                QCSharedBuffer_t &auxImg = frames.SharedBuffer( 1 );

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                memset( disp->sharedBuffer.data(), 0, disp->sharedBuffer.size );
                memset( conf->sharedBuffer.data(), 0, conf->sharedBuffer.size );
                ret = m_dfs.Execute( &priImg, &auxImg, &disp->sharedBuffer, &conf->sharedBuffer );
                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_END();
                    TRACE_END( frames.FrameId( 0 ) );
                    DataFrames_t outFrames;
                    DataFrame_t frame;
                    frame.buffer = disp;
                    frame.frameId = frames.FrameId( 0 );
                    frame.timestamp = frames.Timestamp( 0 );
                    outFrames.Add( frame );
                    frame.buffer = conf;
                    outFrames.Add( frame );
                    m_pub.Publish( outFrames );
                }
                else
                {
                    QC_ERROR( "DepthFromStereo failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                              ret );
                }
            }
        }
    }
}

QCStatus_e SampleDepthFromStereo::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_dfs.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );


    return ret;
}

QCStatus_e SampleDepthFromStereo::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_dfs.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( DepthFromStereo, SampleDepthFromStereo );

}   // namespace sample
}   // namespace QC
