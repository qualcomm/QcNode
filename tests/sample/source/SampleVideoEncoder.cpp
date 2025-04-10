// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleVideoEncoder.hpp"


namespace ridehal
{
namespace sample
{

SampleVideoEncoder::SampleVideoEncoder() {}
SampleVideoEncoder ::~SampleVideoEncoder() {}


void SampleVideoEncoder::InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame )
{
    uint64_t frameId = pInputFrame->appMarkData;

    RIDEHAL_DEBUG( "InFrameCallback for frameId %" PRIu64, frameId );
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

        RIDEHAL_DEBUG( "enc-out-buf back, handle:0x%x",
                       pSharedBuffer->sharedBuffer.buffer.dmaHandle );

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
        RIDEHAL_DEBUG( "enc-outFrameCallback, frameId %" PRIu64 " tsNs:%" PRIu64
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
        RIDEHAL_DEBUG( "enc-outFrameCallback, frame info queue is empty, frameId:%" PRIu64
                       " tsNs:%" PRIu64,
                       frame.frameId, frame.timestamp );
        m_pub.Publish( frames );
    }
}

void SampleVideoEncoder::EventCallback( const VideoEncoder_EventType_e eventId,
                                        const void *pPayload )
{
    RIDEHAL_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );
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

RideHalError_e SampleVideoEncoder::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_config.width = Get( config, "width", 0 );
    if ( 0 == m_config.width )
    {
        RIDEHAL_ERROR( "invalid width = %u\n", m_config.width );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.height = Get( config, "height", 0 );
    if ( 0 == m_config.height )
    {
        RIDEHAL_ERROR( "invalid height = %u\n", m_config.height );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.numInputBufferReq = Get( config, "pool_size", 4 );
    if ( 0 == m_config.numInputBufferReq )
    {
        RIDEHAL_ERROR( "invalid pool_size = %u\n", m_config.numInputBufferReq );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    m_config.numOutputBufferReq = m_config.numInputBufferReq;

    m_config.bitRate = Get( config, "bitrate", 8000000 );
    if ( 0 == m_config.bitRate )
    {
        RIDEHAL_ERROR( "invalid bitrate = %u\n", m_config.bitRate );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.frameRate = Get( config, "fps", 30 );
    if ( 0 == m_config.frameRate )
    {
        RIDEHAL_ERROR( "invalid fps = %u\n", m_config.frameRate );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.bSyncFrameSeqHdr = Get( config, "sync_frame", false );

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

    m_config.gop = 20;
    m_config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    m_config.inFormat = Get( config, "format", RIDEHAL_IMAGE_FORMAT_NV12 );
    m_config.outFormat = RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265;
    m_config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    m_config.bInputDynamicMode = true;
    m_config.bOutputDynamicMode = false;

    return ret;
}

RideHalError_e SampleVideoEncoder::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_ON( VPU );
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_encoder.Init( (char *) name.c_str(), &m_config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_encoder.RegisterCallback( SampleVideoEncoder::InFrameCallback,
                                          SampleVideoEncoder::OutFrameCallback,
                                          SampleVideoEncoder::EventCallback, (void *) this );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_sub.Init( name, m_inputTopicName );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleVideoEncoder::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_encoder.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleVideoEncoder::ThreadMain, this );
        m_threadRelease = std::thread( &SampleVideoEncoder::ThreadReleaseMain, this );
    }

    return ret;
}


void SampleVideoEncoder::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        DataFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( 0 == ret )
        {
            frame = frames.frames[0];
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n ", frame.frameId,
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
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "failed to submit input frameId %" PRIu64, frame.frameId );
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
                RIDEHAL_DEBUG( "release frameId %" PRIu64, frameId );
                m_camFrameMap.erase( frameId );
            }
            else
            {
                RIDEHAL_ERROR( "InFrameCallback with invalid frameId %" PRIu64, frameId );
            }
        }
    }
}

RideHalError_e SampleVideoEncoder::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

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

RideHalError_e SampleVideoEncoder::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_encoder.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( VideoEncoder, SampleVideoEncoder );

}   // namespace sample
}   // namespace ridehal

