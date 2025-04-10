// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/sample/SampleDepthFromStereo.hpp"


namespace ridehal
{
namespace sample
{

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

SampleDepthFromStereo::SampleDepthFromStereo() {}
SampleDepthFromStereo::~SampleDepthFromStereo() {}

RideHalError_e SampleDepthFromStereo::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

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
        RIDEHAL_ERROR( "invalid direction %s\n", dirStr.c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.width = Get( config, "width", 1280u );
    m_config.height = Get( config, "height", 416u );
    m_config.format = Get( config, "format", RIDEHAL_IMAGE_FORMAT_NV12 );
    m_config.frameRate = Get( config, "fps", 30u );

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferFlags = 0;
    }
    else
    {
        m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        RIDEHAL_ERROR( "no input topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleDepthFromStereo::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    uint32_t width = m_config.width;
    uint32_t height = m_config.height;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t dispTsProp = { RIDEHAL_TENSOR_TYPE_UINT_16,
                                             { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                             4 };

        ret = m_dispPool.Init( name + ".disp", LOGGER_LEVEL_INFO, m_poolSize, dispTsProp,
                               RIDEHAL_BUFFER_USAGE_EVA, m_bufferFlags );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t confTsProp = { RIDEHAL_TENSOR_TYPE_UINT_8,
                                             { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                             4 };

        ret = m_confPool.Init( name + ".conf", LOGGER_LEVEL_INFO, m_poolSize, confTsProp,
                               RIDEHAL_BUFFER_USAGE_EVA, m_bufferFlags );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_dfs.Init( name.c_str(), &m_config );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleDepthFromStereo::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_dfs.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleDepthFromStereo::ThreadMain, this );
    }

    return ret;
}

void SampleDepthFromStereo::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( 2 != frames.frames.size() )
        {
            RIDEHAL_ERROR( "DepthFromStereo expect 2 input images" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            std::shared_ptr<SharedBuffer_t> disp = m_dispPool.Get();
            std::shared_ptr<SharedBuffer_t> conf = m_confPool.Get();
            if ( ( nullptr != disp ) && ( nullptr != conf ) )
            {
                RideHal_SharedBuffer_t &priImg = frames.SharedBuffer( 0 );
                RideHal_SharedBuffer_t &auxImg = frames.SharedBuffer( 1 );

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                memset( disp->sharedBuffer.data(), 0, disp->sharedBuffer.size );
                memset( conf->sharedBuffer.data(), 0, conf->sharedBuffer.size );
                ret = m_dfs.Execute( &priImg, &auxImg, &disp->sharedBuffer, &conf->sharedBuffer );
                if ( RIDEHAL_ERROR_NONE == ret )
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
                    RIDEHAL_ERROR( "DepthFromStereo failed for %" PRIu64 " : %d",
                                   frames.FrameId( 0 ), ret );
                }
            }
        }
    }
}

RideHalError_e SampleDepthFromStereo::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

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

RideHalError_e SampleDepthFromStereo::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_dfs.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( DepthFromStereo, SampleDepthFromStereo );

}   // namespace sample
}   // namespace ridehal
