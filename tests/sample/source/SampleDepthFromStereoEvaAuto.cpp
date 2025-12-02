// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/SampleDepthFromStereoEvaAuto.hpp"


namespace QC
{
namespace sample
{

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

SampleDepthFromStereoEvaAuto::SampleDepthFromStereoEvaAuto() : m_dfs( m_logger ) {}
SampleDepthFromStereoEvaAuto::~SampleDepthFromStereoEvaAuto() {}

QCStatus_e SampleDepthFromStereoEvaAuto::ParseConfig( SampleConfig_t &config )
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
        m_bufferCache = QC_CACHEABLE_NON;
    }
    else
    {
        m_bufferCache = QC_CACHEABLE;
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

QCStatus_e SampleDepthFromStereoEvaAuto::Init( std::string name, SampleConfig_t &config )
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

        ret = m_dispPool.Init( name + ".disp", m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, dispTsProp,
                               QC_MEMORY_ALLOCATOR_DMA_EVA, m_bufferCache );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t confTsProp = { QC_TENSOR_TYPE_UINT_8,
                                       { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                       4 };

        ret = m_confPool.Init( name + ".conf", m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, confTsProp,
                               QC_MEMORY_ALLOCATOR_DMA_EVA, m_bufferCache );
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

QCStatus_e SampleDepthFromStereoEvaAuto::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_dfs.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleDepthFromStereoEvaAuto::ThreadMain, this );
    }

    return ret;
}

void SampleDepthFromStereoEvaAuto::ThreadMain()
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
                QCBufferDescriptorBase_t &priImg = frames.GetBuffer( 0 );
                QCBufferDescriptorBase_t &auxImg = frames.GetBuffer( 1 );
                QCBufferDescriptorBase_t &dispTs = disp->GetBuffer();
                QCBufferDescriptorBase_t &confTs = conf->GetBuffer();

                ImageDescriptor_t *pPriImg = dynamic_cast<ImageDescriptor_t *>( &priImg );
                ImageDescriptor_t *pAuxImg = dynamic_cast<ImageDescriptor_t *>( &auxImg );
                TensorDescriptor_t *pDispTs = dynamic_cast<TensorDescriptor_t *>( &dispTs );
                TensorDescriptor_t *pConfTs = dynamic_cast<TensorDescriptor_t *>( &confTs );

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                ret = m_dfs.Execute( pPriImg, pAuxImg, pDispTs, pConfTs );
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

QCStatus_e SampleDepthFromStereoEvaAuto::Stop()
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

QCStatus_e SampleDepthFromStereoEvaAuto::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_dfs.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( DepthFromStereo, SampleDepthFromStereoEvaAuto );

}   // namespace sample
}   // namespace QC
