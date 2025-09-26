// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "QC/sample/SampleC2D.hpp"


namespace QC
{
namespace sample
{

SampleC2D::SampleC2D() : m_c2d( m_logger ) {}
SampleC2D::~SampleC2D() {}

QCStatus_e SampleC2D::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_outputWidth = Get( config, "output_width", 1928 );
    if ( 0 == m_outputWidth )
    {
        QC_ERROR( "invalid output_width\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputHeight = Get( config, "output_height", 1208 );
    if ( 0 == m_outputHeight )
    {
        QC_ERROR( "invalid output_height\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputFormat = Get( config, "output_format", QC_IMAGE_FORMAT_UYVY );
    if ( QC_IMAGE_FORMAT_MAX == m_outputFormat )
    {
        QC_ERROR( "invalid output_format\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.numOfInputs = Get( config, "batch_size", 1 );
    if ( 0 == m_config.numOfInputs )
    {
        QC_ERROR( "invalid batch_size\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    for ( uint32_t i = 0; i < m_config.numOfInputs; i++ )
    {
        m_config.inputConfigs[i].inputResolution.width =
                Get( config, "input_width" + std::to_string( i ), 1928 );
        if ( 0 == m_config.inputConfigs[i].inputResolution.width )
        {
            QC_ERROR( "invalid input_width%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputResolution.height =
                Get( config, "input_height" + std::to_string( i ), 1208 );
        if ( 0 == m_config.inputConfigs[i].inputResolution.height )
        {
            QC_ERROR( "invalid input_height%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputFormat =
                Get( config, "input_format" + std::to_string( i ), QC_IMAGE_FORMAT_NV12 );
        if ( QC_IMAGE_FORMAT_MAX == m_config.inputConfigs[i].inputFormat )
        {
            QC_ERROR( "invalid input_format%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.topX = Get( config, "roi_x" + std::to_string( i ), 0 );
        if ( m_config.inputConfigs[i].ROI.topX >= m_config.inputConfigs[i].inputResolution.width )
        {
            QC_ERROR( "invalid roi_x%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.topY = Get( config, "roi_y" + std::to_string( i ), 0 );
        if ( m_config.inputConfigs[i].ROI.topY >= m_config.inputConfigs[i].inputResolution.height )
        {
            QC_ERROR( "invalid roi_y%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.width = Get( config, "roi_width" + std::to_string( i ),
                                                  m_config.inputConfigs[i].inputResolution.width );
        if ( 0 == m_config.inputConfigs[i].ROI.width )
        {
            QC_ERROR( "invalid roi_width%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.height =
                Get( config, "roi_height" + std::to_string( i ),
                     m_config.inputConfigs[i].inputResolution.height );
        if ( 0 == m_config.inputConfigs[i].ROI.height )
        {
            QC_ERROR( "invalid roi_height%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferCache = QC_CACHEABLE_NON;
    }
    else
    {
        m_bufferCache = QC_CACHEABLE;
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

QCStatus_e SampleC2D::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( GPU );
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_imagePool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, m_config.numOfInputs,
                                m_outputWidth, m_outputHeight, m_outputFormat,
                                QC_MEMORY_ALLOCATOR_DMA_GPU, m_bufferCache );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_c2d.Init( name.c_str(), &m_config );
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

QCStatus_e SampleC2D::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_c2d.Start();
    TRACE_END( SYSTRACE_TASK_START );

    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleC2D::ThreadMain, this );
    }


    return ret;
}

void SampleC2D::ThreadMain()
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
            std::shared_ptr<SharedBuffer_t> buffer = m_imagePool.Get();
            if ( nullptr != buffer )
            {
                std::vector<ImageDescriptor_t> inputs;
                for ( auto &frame : frames.frames )
                {
                    QCBufferDescriptorBase_t &bufDesc = frame.GetBuffer();
                    ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
                    inputs.push_back( *pImage );
                }

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                {
                    QCBufferDescriptorBase_t &bufDesc = buffer->GetBuffer();
                    ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
                    ret = m_c2d.Execute( inputs.data(), inputs.size(), pImage );
                }
                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_END();
                    TRACE_END( frames.FrameId( 0 ) );
                    DataFrames_t outFrames;
                    DataFrame_t frame;
                    frame.buffer = buffer;
                    frame.frameId = frames.FrameId( 0 );
                    frame.timestamp = frames.Timestamp( 0 );
                    outFrames.Add( frame );
                    m_pub.Publish( outFrames );
                }
                else
                {
                    QC_ERROR( "c2d execute failed for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
                }
            }
        }
    }
}

QCStatus_e SampleC2D::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_c2d.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleC2D::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_c2d.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( C2D, SampleC2D );

}   // namespace sample
}   // namespace QC
