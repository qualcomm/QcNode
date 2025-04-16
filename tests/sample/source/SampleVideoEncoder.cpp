// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/SampleVideoEncoder.hpp"


namespace QC
{
namespace sample
{

SampleVideoEncoder::SampleVideoEncoder() {}
SampleVideoEncoder ::~SampleVideoEncoder() {}


void SampleVideoEncoder::InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame )
{
    uint64_t frameId = pInputFrame->appMarkData;

    QC_DEBUG( "InFrameCallback for frameId %" PRIu64, frameId );
    TRACE_EVENT( SYSTRACE_EVENT_VENC_INPUT_DONE );

    std::unique_lock<std::mutex> l( m_lock );
    m_frameReleaseQueue.push( frameId );
    m_condVar.notify_one();
}

void SampleVideoEncoder::OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame )
{
    DataFrames_t frames;
    DataFrame_t frame;
    SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;

    pSharedBuffer->sharedBuffer = pOutputFrame->sharedBuffer;
    pSharedBuffer->pubHandle = 0;

    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        VideoEncoder_OutputFrame_t outFrame;
        outFrame.sharedBuffer = pSharedBuffer->sharedBuffer;
        outFrame.appMarkData = 0;

        QC_DEBUG( "enc-out-buf back, handle:0x%x", pSharedBuffer->sharedBuffer.buffer.dmaHandle );

        m_encoder.SubmitOutputFrame( &outFrame );
        delete pSharedBuffer;
    } );

    if ( false == m_frameInfoQueue.empty() )
    {
        FrameInfo info;
        {
            std::unique_lock<std::mutex> l( m_lock );
            info = m_frameInfoQueue.front();
            m_frameInfoQueue.pop();
        }
        frame.frameId = info.frameId;
        frame.buffer = buffer;
        frame.timestamp = info.timestamp;
        frames.Add( frame );
        PROFILER_END();
        TRACE_END( frame.frameId );
        QC_DEBUG( "enc-outFrameCallback, frameId %" PRIu64 " tsNs:%" PRIu64
                  " type %d size %" PRIu32,
                  info.frameId, info.timestamp, pOutputFrame->frameType,
                  pOutputFrame->sharedBuffer.size );
        m_pub.Publish( frames );
    }
    else
    {
        /* for the first input-buffer, will produce two output buffer */
        frame.frameId = pOutputFrame->appMarkData;
        frame.buffer = buffer;
        frame.timestamp = pOutputFrame->timestampNs;
        frames.frames.push_back( frame );
        TRACE_EVENT( SYSTRACE_EVENT_VENC_OUTPUT_WITH_2ND_FRAME );
        QC_DEBUG( "enc-outFrameCallback, frame info queue is empty, frameId:%" PRIu64
                  " tsNs:%" PRIu64,
                  frame.frameId, frame.timestamp );
        m_pub.Publish( frames );
    }
}

void SampleVideoEncoder::EventCallback( const VideoEncoder_EventType_e eventId,
                                        const void *pPayload )
{
    QC_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );
}

void SampleVideoEncoder::InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame,
                                          void *pPrivData )
{
    SampleVideoEncoder *self = (SampleVideoEncoder *) pPrivData;
    self->InFrameCallback( pInputFrame );
}

void SampleVideoEncoder::OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame,
                                           void *pPrivData )
{
    SampleVideoEncoder *self = (SampleVideoEncoder *) pPrivData;
    self->OutFrameCallback( pOutputFrame );
}

void SampleVideoEncoder::EventCallback( const VideoEncoder_EventType_e eventId,
                                        const void *pPayload, void *pPrivData )
{
    SampleVideoEncoder *self = (SampleVideoEncoder *) pPrivData;
    self->EventCallback( eventId, pPayload );
}

QCStatus_e SampleVideoEncoder::ParseConfig( SampleConfig_t &config )
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

    m_config.numInputBufferReq = Get( config, "pool_size", 4 );
    if ( 0 == m_config.numInputBufferReq )
    {
        QC_ERROR( "invalid pool_size = %u\n", m_config.numInputBufferReq );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_config.numOutputBufferReq = m_config.numInputBufferReq;

    m_config.bitRate = Get( config, "bitrate", 8000000 );
    if ( 0 == m_config.bitRate )
    {
        QC_ERROR( "invalid bitrate = %u\n", m_config.bitRate );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.frameRate = Get( config, "fps", 30 );
    if ( 0 == m_config.frameRate )
    {
        QC_ERROR( "invalid fps = %u\n", m_config.frameRate );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.bSyncFrameSeqHdr = Get( config, "sync_frame", false );

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

    m_config.gop = 20;
    m_config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    m_config.inFormat = Get( config, "format", QC_IMAGE_FORMAT_NV12 );
    m_config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H265;
    m_config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    m_config.bInputDynamicMode = true;
    m_config.bOutputDynamicMode = false;

    return ret;
}

QCStatus_e SampleVideoEncoder::Init( std::string name, SampleConfig_t &config )
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
        ret = m_encoder.Init( (char *) name.c_str(), &m_config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_encoder.RegisterCallback( SampleVideoEncoder::InFrameCallback,
                                          SampleVideoEncoder::OutFrameCallback,
                                          SampleVideoEncoder::EventCallback, (void *) this );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_sub.Init( name, m_inputTopicName );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

QCStatus_e SampleVideoEncoder::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_encoder.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleVideoEncoder::ThreadMain, this );
        m_threadRelease = std::thread( &SampleVideoEncoder::ThreadReleaseMain, this );
    }

    return ret;
}


void SampleVideoEncoder::ThreadMain()
{
    QCStatus_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        DataFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( 0 == ret )
        {
            frame = frames.frames[0];
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n ", frame.frameId,
                      frame.timestamp );

            VideoEncoder_InputFrame_t inputFrame;
            inputFrame.sharedBuffer = frame.buffer->sharedBuffer;
            inputFrame.timestampNs = frame.timestamp;
            inputFrame.appMarkData = frame.frameId;
            inputFrame.pOnTheFlyCmd = nullptr;

            {
                std::unique_lock<std::mutex> l( m_lock );
                m_camFrameMap[frame.frameId] = frame;
            }
            PROFILER_BEGIN();
            TRACE_BEGIN( frame.frameId );
            ret = m_encoder.SubmitInputFrame( &inputFrame );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "failed to submit input frameId %" PRIu64, frame.frameId );
                std::unique_lock<std::mutex> l( m_lock );
                m_camFrameMap.erase( frame.frameId );
            }
            else
            {
                FrameInfo info = { frame.frameId, frame.timestamp };
                std::unique_lock<std::mutex> l( m_lock );
                m_frameInfoQueue.push( info );
            }
        }
    }
}

void SampleVideoEncoder::ThreadReleaseMain()
{
    while ( false == m_stop )
    {
        std::unique_lock<std::mutex> l( m_lock );
        (void) m_condVar.wait_for( l, std::chrono::milliseconds( 10 ) );
        if ( false == m_frameReleaseQueue.empty() )
        {
            uint64_t frameId;
            frameId = m_frameReleaseQueue.front();
            m_frameReleaseQueue.pop();
            auto it = m_camFrameMap.find( frameId );
            if ( it != m_camFrameMap.end() )
            { /* release the input camera frame */
                QC_DEBUG( "release frameId %" PRIu64, frameId );
                m_camFrameMap.erase( frameId );
            }
            else
            {
                QC_ERROR( "InFrameCallback with invalid frameId %" PRIu64, frameId );
            }
        }
    }
}

QCStatus_e SampleVideoEncoder::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    if ( m_threadRelease.joinable() )
    {
        m_threadRelease.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_encoder.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );

    m_camFrameMap.clear();

    return ret;
}

QCStatus_e SampleVideoEncoder::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_encoder.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( VideoEncoder, SampleVideoEncoder );

}   // namespace sample
}   // namespace QC
