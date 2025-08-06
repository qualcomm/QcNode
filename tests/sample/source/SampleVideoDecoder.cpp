// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SampleVideoDecoder.hpp"

namespace QC
{
namespace sample
{

void SampleVideoDecoder::OnDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    auto &fd = eventInfo.frameDesc;
    auto &bufIn = fd.GetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID );
    QCSharedVideoFrameDescriptor_t *pBufIn =
            dynamic_cast<QCSharedVideoFrameDescriptor_t *>( &bufIn );
    if ( nullptr != pBufIn )
    {
        InFrameCallback( *pBufIn, eventInfo );
    }

    auto &bufOut = fd.GetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID );
    QCSharedVideoFrameDescriptor_t *pBufOut =
            dynamic_cast<QCSharedVideoFrameDescriptor_t *>( &bufOut );
    if ( nullptr != pBufOut )
    {
        OutFrameCallback( *pBufOut, eventInfo );
    }
}

void SampleVideoDecoder::InFrameCallback( QCSharedVideoFrameDescriptor_t &inFrame,
                                          const QCNodeEventInfo_t &eventInfo )
{
    uint64_t frameId = inFrame.appMarkData;

    QC_DEBUG( "Received input video frame Id %" PRIu64 " from node %d, status %d\n", frameId,
              eventInfo.node, eventInfo.status, eventInfo.state );
    TRACE_EVENT( SYSTRACE_EVENT_VENC_INPUT_DONE );

    std::unique_lock<std::mutex> l( m_lock );
    m_frameReleaseQueue.push( frameId );
    m_condVar.notify_one();
    // unlock @m_lock
}

void SampleVideoDecoder::OutFrameCallback( QCSharedVideoFrameDescriptor_t &outFrame,
                                           const QCNodeEventInfo_t &eventInfo )
{
    uint64_t frameId = outFrame.appMarkData;

    QC_DEBUG( "Received output video frame Id %" PRIu64 " from node %d, status %d\n", frameId,
              eventInfo.node, eventInfo.status, eventInfo.state );

    DataFrames_t frames;
    DataFrame_t frame;

    SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;
    pSharedBuffer->sharedBuffer = outFrame.buffer;
    pSharedBuffer->pubHandle = 0;

    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        QCSharedVideoFrameDescriptor buffDesc;
        buffDesc.buffer = pSharedBuffer->sharedBuffer;

        QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID + 1 );
        frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, buffDesc );

        QC_DEBUG( "enc-out-buf back, handle:0x%x", pSharedBuffer->sharedBuffer.buffer.dmaHandle );

        m_encoder.ProcessFrameDescriptor( frameDesc );
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
                  info.frameId, info.timestamp, outFrame.frameType, outFrame.buffer.size );
        m_pub.Publish( frames );
    }
    else
    {
        /* for the first input-buffer, will produce two output buffer */
        frame.frameId = outFrame.appMarkData;
        frame.buffer = buffer;
        frame.timestamp = outFrame.timestampNs;
        frames.frames.push_back( frame );
        TRACE_EVENT( SYSTRACE_EVENT_VENC_OUTPUT_WITH_2ND_FRAME );
        QC_DEBUG( "enc-outFrameCallback, frame info queue is empty, frameId:%" PRIu64
                  " tsNs:%" PRIu64,
                  frame.frameId, frame.timestamp );
        m_pub.Publish( frames );
    }
}

QCStatus_e SampleVideoDecoder::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = SampleIF::Init( name );

    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        using std::placeholders::_1;

        QCNodeInit_t config = { .config = m_dataTree.Dump(),
                                .callback = std::bind( &SampleVideoDecoder::OnDoneCb, this, _1 ) };

        ret = m_encoder.Initialize( config );
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

QCStatus_e SampleVideoDecoder::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = (QCStatus_e) m_encoder.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleVideoDecoder::ThreadMain, this );
        m_threadRelease = std::thread( &SampleVideoDecoder::ThreadReleaseMain, this );
    }

    return ret;
}

void SampleVideoDecoder::ThreadMain()
{
    QCStatus_e ret;

    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID +
                                           1 );   // for input only

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            for ( auto &frame : frames.frames )
            {
                QC_DEBUG( "Received frameId %" PRIu64 ", type %d, size %lu, timestamp %" PRIu64,
                          frame.frameId, frame.GetBufferType(), frame.GetDataSize(),
                          frame.timestamp );

                QCSharedVideoFrameDescriptor_t frameSharedBuffer;
                frameSharedBuffer.buffer = frame.SharedBuffer();
                frameSharedBuffer.timestampNs = frame.timestamp;
                frameSharedBuffer.appMarkData = frame.frameId;

                ret = frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, frameSharedBuffer );
                if ( QC_STATUS_OK != ret )
                {
                    break;
                }

                {
                    std::unique_lock<std::mutex> l( m_lock );
                    m_camFrameMap[frame.frameId] = frame;
                    // unlock @m_lock
                }

                PROFILER_BEGIN();
                TRACE_BEGIN( frame.frameId );

                ret = m_encoder.ProcessFrameDescriptor( frameDesc );
                if ( QC_STATUS_OK == ret )
                {
                    FrameInfo info = { frame.frameId, frame.timestamp };
                    std::unique_lock<std::mutex> l( m_lock );
                    m_frameInfoQueue.push( info );
                    // unlock @m_lock
                }
                else
                {
                    QC_ERROR( "failed to process input frameId %" PRIu64, frame.frameId );
                    std::unique_lock<std::mutex> l( m_lock );
                    m_camFrameMap.erase( frame.frameId );
                    // unlock @m_lock
                }
            }
        }
    }
}

void SampleVideoDecoder::ThreadReleaseMain()
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
            {   // release the input camera frame
                QC_DEBUG( "release frameId %" PRIu64, frameId );
                m_camFrameMap.erase( frameId );
            }
            else
            {
                QC_ERROR( "ThreadReleaseMain with invalid frameId %" PRIu64, frameId );
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

    if ( m_threadRelease.joinable() )
    {
        m_threadRelease.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = (QCStatus_e) m_encoder.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );

    m_camFrameMap.clear();

    return ret;
}

QCStatus_e SampleVideoDecoder::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_encoder.DeInitialize();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

QCStatus_e SampleVideoDecoder::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_config.Set<std::string>( "name", m_name );

    uint32 width = Get( config, "width", 0 );
    if ( 0 == width )
    {
        QC_ERROR( "invalid width = %u\n", width );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        m_config.Set<uint32_t>( "width", width );
    }

    uint32_t height = Get( config, "height", 0 );
    if ( 0 == height )
    {
        QC_ERROR( "invalid height = %u\n", height );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        m_config.Set<uint32_t>( "height", height );
    }

    uint32_t numInputBuffer = Get( config, "pool_size", 4 );
    if ( 0 == numInputBuffer )
    {
        QC_ERROR( "invalid pool_size = %u\n", numInputBuffer );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        m_config.Set<uint32_t>( "numInputBuffer", numInputBuffer );
    }

    uint32_t numOutputBuffer = Get( config, "numOutputBuffer", numInputBuffer );
    m_config.Set<uint32_t>( "numOutputBuffer", numOutputBuffer );

    uint32_t frameRate = Get( config, "fps", 30 );
    if ( 0 == frameRate )
    {
        QC_ERROR( "invalid fps = %u\n", frameRate );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        m_config.Set<uint32_t>( "frameRate", frameRate );
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
    }

    m_config.Set( "inputImageFormat", "h265" );
    m_config.Set( "outputlImageFormat", "nv12" );
    m_config.Set<bool>( "bInputDynamicMode", true );
    m_config.Set<bool>( "bOutputDynamicMode", false );

    m_dataTree.Set( "static", m_config );

    return ret;
}

REGISTER_SAMPLE( VideoDecoder, SampleVideoDecoder );

}   // namespace sample
}   // namespace QC
