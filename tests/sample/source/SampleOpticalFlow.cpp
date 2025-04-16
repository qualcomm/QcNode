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

    std::string evaModeStr = Get( config, "eva_mode", "dsp" );
    if ( "dsp" == evaModeStr )
    {
        m_config.filterOperationMode = EVA_OF_MODE_DSP;
    }
    else if ( "cpu" == evaModeStr )
    {
        m_config.filterOperationMode = EVA_OF_MODE_CPU;
    }
    else if ( "disable" == evaModeStr )
    {
        m_config.filterOperationMode = EVA_OF_MODE_DISABLE;
    }
    else
    {
        QC_ERROR( "invalid eva_mode %s\n", evaModeStr.c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string dirStr = Get( config, "direction", "forward" );
    if ( "forward" == dirStr )
    {
        m_config.direction = EVA_OF_FORWARD_DIRECTION;
    }
    else if ( "backward" == dirStr )
    {
        m_config.direction = EVA_OF_BACKWARD_DIRECTION;
    }
    else
    {
        QC_ERROR( "invalid direction %s\n", dirStr.c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.width = Get( config, "width", 1920u );
    m_config.height = Get( config, "height", 1024u );
    m_config.format = Get( config, "format", QC_IMAGE_FORMAT_NV12 );
    m_config.frameRate = Get( config, "fps", 30u );
    m_config.amFilter.nStepSize = Get( config, "step_size", 1 );

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

QCStatus_e SampleOpticalFlow::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    uint32_t width = ( m_config.width >> m_config.amFilter.nStepSize )
                     << m_config.amFilter.nUpScale;
    uint32_t height = ( m_config.height >> m_config.amFilter.nStepSize )
                      << m_config.amFilter.nUpScale;

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t mvMapTsProp = { QC_TENSOR_TYPE_UINT_16,
                                        { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 },
                                        4 };

        ret = m_mvPool.Init( name + ".mv", LOGGER_LEVEL_INFO, m_poolSize, mvMapTsProp,
                             QC_BUFFER_USAGE_EVA );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t mvConfTsProp = { QC_TENSOR_TYPE_UINT_8,
                                         { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 },
                                         4 };

        ret = m_mvConfPool.Init( name + ".mvConf", LOGGER_LEVEL_INFO, m_poolSize, mvConfTsProp,
                                 QC_BUFFER_USAGE_EVA );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_ofl.Init( name.c_str(), &m_config );
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

    m_LastFrames.frames.clear();

    return ret;
}

QCStatus_e SampleOpticalFlow::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_ofl.Start();
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
                QCSharedBuffer_t &refImg = m_LastFrames.SharedBuffer( 0 );
                QCSharedBuffer_t &curImg = frames.SharedBuffer( 0 );

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                ret = m_ofl.Execute( &refImg, &curImg, &mv->sharedBuffer, &mvConf->sharedBuffer );
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
                    QC_ERROR( "OpticalFlow failed for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
                }
            }

            m_LastFrames = frames;
        }
    }
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
    ret = m_ofl.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );


    return ret;
}

QCStatus_e SampleOpticalFlow::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_ofl.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( OpticalFlow, SampleOpticalFlow );

}   // namespace sample
}   // namespace QC
