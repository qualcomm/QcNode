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

void SampleDepthFromStereo::OnDoneCb( const QCNodeEventInfo_t &eventInfo ) {}

QCStatus_e SampleDepthFromStereo::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_config.Set<std::string>( "name", m_name );

    std::string dirStr = Get( config, "direction", "l2r" );
    m_config.Set<std::string>( "direction", dirStr );

    m_width = Get( config, "width", 1280 );
    m_config.Set<uint32_t>( "width", m_width );

    m_height = Get( config, "height", 416 );
    m_config.Set<uint32_t>( "height", m_height );

    m_config.Set<std::string>( "format", Get( config, "format", "nv12" ) );
    m_config.Set<uint32_t>( "fps", Get( config, "fps", 30 ) );

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferCache = QC_CACHEABLE_NON;
    }
    else
    {
        m_bufferCache = QC_CACHEABLE;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    m_config.Set<uint32_t>( "pool_size", m_poolSize );

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

    m_dataTree.Set( "static", m_config );

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

    uint32_t width = m_width;
    uint32_t height = m_height;

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t dispTsProp = { QC_TENSOR_TYPE_UINT_16,
                                       { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                       4 };

        ret = m_dispPool.Init( name + ".disp", LOGGER_LEVEL_INFO, m_poolSize, dispTsProp,
                               QC_MEMORY_ALLOCATOR_DMA_EVA, m_bufferCache );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t confTsProp = { QC_TENSOR_TYPE_UINT_8,
                                       { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                       4 };

        ret = m_confPool.Init( name + ".conf", LOGGER_LEVEL_INFO, m_poolSize, confTsProp,
                               QC_MEMORY_ALLOCATOR_DMA_EVA, m_bufferCache );
    }

    if ( QC_STATUS_OK == ret )
    {
        using std::placeholders::_1;

        QCNodeInit_t config = { .config = m_dataTree.Dump(),
                                .callback =
                                        std::bind( &SampleDepthFromStereo::OnDoneCb, this, _1 ) };

        ret = m_dfs.Initialize( config );
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
    ret = static_cast<QCStatus_e>( m_dfs.Start() );
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
    QCFrameDescriptorNodeIfs *frameDescriptor =
            new QCSharedFrameDescriptorNode( QC_NODE_DFS_LAST_BUFF_ID );
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
                QCSharedBufferDescriptor_t buffPriImg;
                QCSharedBufferDescriptor_t buffAuxImg;
                QCSharedBufferDescriptor_t buffDisp;
                QCSharedBufferDescriptor_t buffConf;

                buffPriImg.buffer = frames.SharedBuffer( 0 );
                buffAuxImg.buffer = frames.SharedBuffer( 1 );
                buffDisp.buffer = disp->sharedBuffer;
                buffConf.buffer = conf->sharedBuffer;

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                memset( buffDisp.buffer.data(), 0, disp->sharedBuffer.size );
                memset( buffConf.buffer.data(), 0, conf->sharedBuffer.size );

                // TODO
                // Add check of error code below
                QCStatus_e status = frameDescriptor->SetBuffer(
                        static_cast<uint32_t>( QC_NODE_DFS_PRIMARY_IMAGE_BUFF_ID ), buffPriImg );
                status = frameDescriptor->SetBuffer(
                        static_cast<uint32_t>( QC_NODE_DFS_AUXILARY_IMAGE_BUFF_ID ), buffAuxImg );
                status = frameDescriptor->SetBuffer(
                        static_cast<uint32_t>( QC_NODE_DFS_DISPARITY_MAP_BUFF_ID ), buffDisp );
                status = frameDescriptor->SetBuffer(
                        static_cast<uint32_t>( QC_NODE_DFS_DISPARITY_CONFIDANCE_MAP_BUFF_ID ),
                        buffConf );

                ret = static_cast<QCStatus_e>( m_dfs.ProcessFrameDescriptor( *frameDescriptor ) );

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
    reinterpret_cast<QCSharedFrameDescriptorNode *>( frameDescriptor )
            ->~QCSharedFrameDescriptorNode();
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
    ret = static_cast<QCStatus_e>( m_dfs.Stop() );
    TRACE_END( SYSTRACE_TASK_STOP );


    return ret;
}

QCStatus_e SampleDepthFromStereo::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = static_cast<QCStatus_e>( m_dfs.DeInitialize() );
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( DepthFromStereo, SampleDepthFromStereo );

}   // namespace sample
}   // namespace QC
