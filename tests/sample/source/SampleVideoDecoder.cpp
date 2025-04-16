// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/SampleVideoDecoder.hpp"


namespace QC
{
namespace sample
{

SampleVideoDecoder::SampleVideoDecoder() {}
SampleVideoDecoder ::~SampleVideoDecoder() {}


void SampleVideoDecoder::InFrameCallback( const VideoDecoder_InputFrame_t *pInputFrame )
{
    uint64_t frameId = pInputFrame->appMarkData;
    uint64_t bufHandle = pInputFrame->sharedBuffer.buffer.dmaHandle;

    std::lock_guard<std::mutex> l( m_lock );
    auto it = m_inFrameMap.find( bufHandle );
    if ( it != m_inFrameMap.end() )
    {
        /* key-point: release the input buffer */
        TRACE_EVENT( SYSTRACE_EVENT_VDEC_INPUT_DONE );
        QC_DEBUG( "Dec-InFrameCallback for handle 0x%x frameId %" PRIu64, bufHandle, frameId );
        m_inFrameMap.erase( bufHandle );
    }
    else
    {
        QC_ERROR( "Dec-InFrameCallback with invalid handle 0x%x frameId %" PRIu64, bufHandle,
                  frameId );
    }
}

void SampleVideoDecoder::OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame )
{
    DataFrames_t frames;
    DataFrame_t frame;
    SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;
    uint64_t bufHandle = pOutputFrame->sharedBuffer.buffer.dmaHandle;

    pSharedBuffer->sharedBuffer = pOutputFrame->sharedBuffer;
    pSharedBuffer->pubHandle = 0;

    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        VideoDecoder_OutputFrame_t outFrame;
        outFrame.sharedBuffer = pSharedBuffer->sharedBuffer;
        outFrame.appMarkData = 0;

        QC_DEBUG( "dec-out-buf back, handle:0x%x", bufHandle );

        m_decoder.SubmitOutputFrame( &outFrame );
        delete pSharedBuffer;
    } );

    if ( false == m_frameInfoQueue.empty() )
    {
        FrameInfo info;
        {
            std::lock_guard<std::mutex> l( m_lock );
            info = m_frameInfoQueue.front();
            m_frameInfoQueue.pop();
        }
        frame.frameId = info.frameId;
        frame.buffer = buffer;
        frame.timestamp = info.timestamp;
        frames.Add( frame );
        PROFILER_END();
        TRACE_END( frame.frameId );
        m_pub.Publish( frames );
        QC_DEBUG( "OutFrameCallback for frameId %" PRIu64 " handle 0x%x size %" PRIu32,
                  info.frameId, bufHandle, pOutputFrame->sharedBuffer.size );
    }
    else
    {
        frame.frameId = pOutputFrame->appMarkData;
        frame.buffer = buffer;
        frame.timestamp = pOutputFrame->timestampNs;
        frames.frames.push_back( frame );
        TRACE_EVENT( SYSTRACE_EVENT_VDEC_OUTPUT_WITH_2ND_FRAME );
        m_pub.Publish( frames );
        QC_DEBUG( "frame info queue is empty! handle 0x%x", bufHandle );
    }
}

void SampleVideoDecoder::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload )
{
    QC_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );
}

void SampleVideoDecoder::InFrameCallback( const VideoDecoder_InputFrame_t *pInputFrame,
                                          void *pPrivData )
{
    SampleVideoDecoder *self = (SampleVideoDecoder *) pPrivData;
    self->InFrameCallback( pInputFrame );
}

void SampleVideoDecoder::OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame,
                                           void *pPrivData )
{
    SampleVideoDecoder *self = (SampleVideoDecoder *) pPrivData;
    self->OutFrameCallback( pOutputFrame );
}

void SampleVideoDecoder::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload,
                                        void *pPrivData )
{
    SampleVideoDecoder *self = (SampleVideoDecoder *) pPrivData;
    self->EventCallback( eventId, pPayload );
}

QCStatus_e SampleVideoDecoder::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_config.width = Get( config, "width", 0 );
    if ( 0 == m_config.width )
    {
        QC_ERROR( "invalid width = %u\n", m_config.width );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.height = Get( config, "height", 0 );
    if ( 0 == m_config.height )
    {
        QC_ERROR( "invalid height = %u\n", m_config.height );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.numInputBuffer = Get( config, "pool_size", 4 );
    if ( 0 == m_config.numInputBuffer )
    {
        QC_ERROR( "invalid pool_size = %u\n", m_config.numInputBuffer );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_config.numOutputBuffer = m_config.numInputBuffer;

    m_config.frameRate = Get( config, "fps", 30 );
    if ( 0 == m_config.frameRate )
    {
        QC_ERROR( "invalid fps = %u\n", m_config.frameRate );
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

    m_config.inFormat = Get( config, "input_format", QC_IMAGE_FORMAT_COMPRESSED_H265 );
    m_config.outFormat = Get( config, "output_format", QC_IMAGE_FORMAT_NV12 );

    m_config.bInputDynamicMode = true;
    m_config.bOutputDynamicMode = false;

    return ret;
}

QCStatus_e SampleVideoDecoder::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( VPU );
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_decoder.Init( (char *) name.c_str(), &m_config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_decoder.RegisterCallback( SampleVideoDecoder::InFrameCallback,
                                          SampleVideoDecoder::OutFrameCallback,
                                          SampleVideoDecoder::EventCallback, (void *) this );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        /* queueDepth should be bigger than input-buf num */
        ret = m_sub.Init( name, m_inputTopicName, 128, false );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

QCStatus_e SampleVideoDecoder::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_decoder.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleVideoDecoder::ThreadMain, this );
    }

    return ret;
}


void SampleVideoDecoder::ThreadMain()
{
    QCStatus_e ret;
    uint64_t bufHandle;

    while ( false == m_stop )
    {
        DataFrames_t frames;
        DataFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( 0 == ret )
        {
            frame = frames.frames[0];

            VideoDecoder_InputFrame_t inputFrame;
            inputFrame.sharedBuffer = frame.buffer->sharedBuffer;
            inputFrame.timestampNs = frame.timestamp;
            inputFrame.appMarkData = frame.frameId;
            bufHandle = inputFrame.sharedBuffer.buffer.dmaHandle;

            QC_DEBUG( "receive frameId %" PRIu64 ", handle 0x%x ts %" PRIu64 "\n ", frame.frameId,
                      bufHandle, frame.timestamp );
            {
                std::lock_guard<std::mutex> l( m_lock );
                m_inFrameMap[bufHandle] = frame;
            }
            PROFILER_BEGIN();
            TRACE_BEGIN( frame.frameId );
            ret = m_decoder.SubmitInputFrame( &inputFrame );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "failed to submit input handle 0x%x frameId %" PRIu64, bufHandle,
                          frame.frameId );
                std::lock_guard<std::mutex> l( m_lock );
                m_inFrameMap.erase( bufHandle );
            }
            else
            {
                FrameInfo info = { frame.frameId, frame.timestamp };
                std::lock_guard<std::mutex> l( m_lock );
                m_frameInfoQueue.push( info );
            }
        }
    }
}

QCStatus_e SampleVideoDecoder::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_decoder.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );

    return ret;
}

QCStatus_e SampleVideoDecoder::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_decoder.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( VideoDecoder, SampleVideoDecoder );

}   // namespace sample
}   // namespace QC
