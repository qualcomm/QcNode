
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SampleOpticalFlow.hpp"

namespace QC
{
namespace sample
{

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

SampleOpticalFlow::SampleOpticalFlow() {}
SampleOpticalFlow::~SampleOpticalFlow() {}


QCStatus_e SampleOpticalFlow::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_config.Set<std::string>( "name", m_name );

    m_width = Get( config, "width", 1920 );
    m_config.Set<uint32_t>( "width", m_width );

    m_height = Get( config, "height", 1024 );
    m_config.Set<uint32_t>( "height", m_height );

    m_config.Set<std::string>( "format", Get( config, "format", "nv12" ) );
    m_config.Set<uint32_t>( "fps", Get( config, "fps", 30 ) );

    m_nStepSize = Get( config, "step_size", 1 );
    if ( m_nStepSize == 1 )
    {
        m_config.Set<uint8_t>( "motionMapStepSize", MOTION_MAP_STEP_SIZE_1 );
    }
    else if ( m_nStepSize == 2 )
    {
        m_config.Set<uint8_t>( "motionMapStepSize", MOTION_MAP_STEP_SIZE_2 );
    }
    else if ( m_nStepSize == 4 )
    {
        m_config.Set<uint8_t>( "motionMapStepSize", MOTION_MAP_STEP_SIZE_4 );
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( m_poolSize )
    {
        m_config.Set<uint32_t>( "pool_size", m_poolSize );
    }
    else
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.Set<uint32_t>( "edgeAlignMetric", 0 );
    m_config.Set<bool>( "isFirstRequest", true );

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

QCStatus_e SampleOpticalFlow::Init( std::string name, SampleConfig_t &config )
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
        QCTensorProps_t mvMapTsProp = { QC_TENSOR_TYPE_UINT_16,
                                        { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 },
                                        4 };

        ret = m_mvPool.Init( name + ".mv", m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, mvMapTsProp,
                             QC_MEMORY_ALLOCATOR_DMA_EVA );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t mvConfTsProp = { QC_TENSOR_TYPE_UINT_8,
                                         { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 },
                                         4 };

        ret = m_mvConfPool.Init( name + ".mvConf", m_nodeId, LOGGER_LEVEL_INFO, m_poolSize,
                                 mvConfTsProp, QC_MEMORY_ALLOCATOR_DMA_EVA );
    }


    if ( QC_STATUS_OK == ret )
    {
        using std::placeholders::_1;

        QCNodeInit_t config = { m_dataTree.Dump() };

        ret = m_of.Initialize( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    m_LastFrames.frames.clear();

    return ret;
}

QCStatus_e SampleOpticalFlow::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = static_cast<QCStatus_e>( m_of.Start() );
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleOpticalFlow::ThreadMain, this );
    }

    return ret;
}

void SampleOpticalFlow::ThreadMain()
{
    QCStatus_e ret;
    QCFrameDescriptorNodeIfs *frameDescriptor =
            new QCSharedFrameDescriptorNode( QC_NODE_OF_LAST_BUFF_ID );
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            if ( 0 == m_LastFrames.frames.size() )
            {
                m_LastFrames = frames;
                continue;
            }
            std::shared_ptr<SharedBuffer_t> mv = m_mvPool.Get();
            std::shared_ptr<SharedBuffer_t> mvConf = m_mvConfPool.Get();
            if ( ( nullptr != mv ) && ( nullptr != mvConf ) )
            {

                QCBufferDescriptorBase_t &buffRefImg = m_LastFrames.GetBuffer( 0 );
                QCBufferDescriptorBase_t &buffCurImg = frames.GetBuffer( 0 );
                QCBufferDescriptorBase_t &buffMV = mv->buffer;
                QCBufferDescriptorBase_t &buffConf = mvConf->buffer;

                ImageDescriptor_t &buffRefImgDesc = dynamic_cast<ImageDescriptor_t &>( buffRefImg );
                ImageDescriptor_t &buffCurImgDesc = dynamic_cast<ImageDescriptor_t &>( buffCurImg );
                TensorDescriptor_t &buffMVDesc = dynamic_cast<TensorDescriptor_t &>( buffMV );
                TensorDescriptor_t &buffConfDesc = dynamic_cast<TensorDescriptor_t &>( buffConf );

                QCStatus_e status = frameDescriptor->SetBuffer(
                        static_cast<uint32_t>( QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID ),
                        buffRefImgDesc );
                if ( QC_STATUS_OK == status )
                {
                    status = frameDescriptor->SetBuffer(
                            static_cast<uint32_t>( QC_NODE_OF_CURRENT_IMAGE_BUFF_ID ),
                            buffCurImgDesc );
                    if ( QC_STATUS_OK == status )
                    {
                        status = frameDescriptor->SetBuffer(
                                static_cast<uint32_t>( QC_NODE_OF_FWD_MOTION_BUFF_ID ),
                                buffMVDesc );
                        if ( QC_STATUS_OK == status )
                        {
                            status = frameDescriptor->SetBuffer(
                                    static_cast<uint32_t>( QC_NODE_OF_FWD_CONF_BUFF_ID ),
                                    buffConfDesc );

                            PROFILER_BEGIN();
                            TRACE_BEGIN( frames.FrameId( 0 ) );
                            ret = static_cast<QCStatus_e>(
                                    m_of.ProcessFrameDescriptor( *frameDescriptor ) );
                            if ( QC_STATUS_OK == ret )
                            {
                                PROFILER_END();
                                TRACE_END( frames.FrameId( 0 ) );
                                DataFrames_t outFrames;
                                DataFrame_t frame;
                                frame.buffer = mv;
                                frame.frameId = frames.FrameId( 0 );
                                frame.timestamp = frames.Timestamp( 0 );
                                outFrames.Add( frame );
                                frame.buffer = mvConf;
                                outFrames.Add( frame );
                                m_pub.Publish( outFrames );
                            }
                            else
                            {
                                QC_ERROR( "OpticalFlow failed for %" PRIu64 " : %d",
                                          frames.FrameId( 0 ), ret );
                            }
                        }
                    }
                }
                if ( QC_STATUS_OK != status )
                {
                    QC_ERROR( "OpticalFlow failed with error code %d", status );
                }
            }

            m_LastFrames = frames;
        }
    }
    reinterpret_cast<QCSharedFrameDescriptorNode *>( frameDescriptor )
            ->~QCSharedFrameDescriptorNode();
}

QCStatus_e SampleOpticalFlow::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    m_LastFrames.frames.clear();

    PROFILER_SHOW();

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = static_cast<QCStatus_e>( m_of.Stop() );
    TRACE_END( SYSTRACE_TASK_STOP );

    return ret;
}

QCStatus_e SampleOpticalFlow::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = static_cast<QCStatus_e>( m_of.DeInitialize() );
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( OpticalFlow, SampleOpticalFlow );

}   // namespace sample
}   // namespace QC
