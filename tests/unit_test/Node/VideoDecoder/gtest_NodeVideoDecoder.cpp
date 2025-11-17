// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "gtest/gtest.h"
#include <chrono>
#include <condition_variable>
#include <malloc.h>
#include <stdio.h>
#include <thread>

#include "QC/sample/BufferManager.hpp"
#include "QC/Node/VideoDecoder.hpp"

#include "QC/Common/Types.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "VidcDemuxer.hpp"
#include "md5_utils.hpp"

using namespace QC;
using namespace QC::Memory;
using namespace QC::Node;
using namespace QC::sample;
using namespace QC::test::utils;

static std::mutex g_InMutex;
static std::condition_variable g_InCondVar;
static std::mutex g_OutMutex;
static std::condition_variable g_OutCondVar;
static uint64_t g_timestamp = 0;

void OnDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    QCFrameDescriptorNodeIfs &frameDesc = eventInfo.frameDesc;

    // INPUT frame:
    QCBufferDescriptorBase_t &inBufDesc =
            frameDesc.GetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID );
    const VideoFrameDescriptor *pInSharedBuffer = static_cast<VideoFrameDescriptor *>( &inBufDesc );

    if ( nullptr != pInSharedBuffer )
    {
        printf( "%s::%s(): Received input video frame from node %d (%s), status %d, state %d.\n",
                __FILE__, __func__, eventInfo.node.id, eventInfo.node.name.c_str(),
                eventInfo.status, eventInfo.state );
        ASSERT_EQ( QC_STATUS_OK, eventInfo.status );
        ASSERT_EQ( 1, eventInfo.node.id );
    }

    // Send signal
    g_InCondVar.notify_one();

    // OUTPUT frame:
    QCBufferDescriptorBase_t &outBufDesc =
            frameDesc.GetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID );
    const VideoFrameDescriptor *pOutSharedBuffer = static_cast<VideoFrameDescriptor *>( &outBufDesc );

    if ( nullptr != pOutSharedBuffer )
    {
        printf( "%s::%s(): Received output video frame from node %d (%s), status %d, state %d.\n",
                __FILE__, __func__, eventInfo.node.id, eventInfo.node.name.c_str(),
                eventInfo.status, eventInfo.state );
        ASSERT_EQ( QC_STATUS_OK, eventInfo.status );
        ASSERT_EQ( 1, eventInfo.node.id );
    }

    // Send signal
    g_OutCondVar.notify_one();
}

TEST( NodeVideoDecoder, SANITY_VideoDecoder_Dynamic )
{
    QCStatus_e ret;
    std::string errors;
    BufferManager bufMgr = BufferManager( { "VDEC", QC_NODE_TYPE_VDEC, 0 } );
    QCNodeIfs *pNodeVide = new QC::Node::VideoDecoder();
    DataTree dt;
    dt.Set<std::string>( "static.name", "SANITY_VideoDecoder_Dynamic" );
    dt.Set<uint32_t>( "id", 1 );
    dt.Set<std::string>( "static.logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", 176 );
    dt.Set<uint32_t>( "height", 144 );
    dt.Set<bool>( "inputDynamicMode", true );
    dt.Set<bool>( "outputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 30 );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h265" );

    QCNodeInit_t config = { .config = dt.Dump(), .callback = OnDoneCb };

    printf( "config: %s\n", config.config.c_str() );

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    QCNodeConfigIfs &cfgIfs = pNodeVide->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    printf( "options: %s\n", options.c_str() );

    DataTree optionsDt;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    uint32_t width;
    uint32_t height;
    uint32_t frameRate;
    bool bInputDynamicMode;
    bool bOutputDynamicMode;

    width = dt.Get( "width", 0 );
    height = dt.Get( "height", 0 );
    frameRate = dt.Get( "frameRate", 0 );

    bInputDynamicMode = dt.Get( "inputDynamicMode", 1 );
    bOutputDynamicMode = dt.Get( "outputDynamicMode", 1 );
    numInputBufferReq = dt.Get( "numInputBufferReq", 1 );
    numOutputBufferReq = dt.Get( "numOutputBufferReq", 1 );

    QCImageFormat_e inFormat = dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );
    QCImageFormat_e outFormat =
            dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H264 );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_DECODER_EVENT_BUFF_ID + 1 );

    std::vector<VideoFrameDescriptor_t> inputs;
    std::vector<VideoFrameDescriptor_t> outputs;

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( ImageBasicProps( width, height, inFormat ), bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( bufDesc );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = width;
        imgProps.height = height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = outFormat;
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( imgProps, bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( bufDesc );
    }

    frameDesc.Clear();

    uint32_t nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    for ( uint32_t i = 0; i < nr; i++ )
    {
        //  Set up an input buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Set up an output buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    // wait input done signal
    std::unique_lock<std::mutex> inLock( g_InMutex );
    g_InCondVar.wait( inLock );

    // wait output done signal
    std::unique_lock<std::mutex> outLock( g_OutMutex );
    g_OutCondVar.wait( outLock );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    ret = pNodeVide->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto &output : outputs )
    {
        ret = bufMgr.Free( output );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &input : inputs )
    {
        ret = bufMgr.Free( input );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( Demuxer, SANITY_Demuxer )
{
    QCStatus_e ret;

    VidcDemuxer vidcDemuxer;
    VidcDemuxer_Config_t vidcDemuxConfig;
    VidcDemuxer_VideoInfo_t videoInfo;

    vidcDemuxConfig.pVideoFileName = "./data/test/VideoDecoder/test.mp4";
    vidcDemuxConfig.startFrameIdx = 0;

    ret = vidcDemuxer.Init( &vidcDemuxConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = vidcDemuxer.GetVideoInfo( videoInfo );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( 1920, videoInfo.frameWidth );
    ASSERT_EQ( 1024, videoInfo.frameHeight );
    ASSERT_EQ( 101, videoInfo.format );

    ret = vidcDemuxer.DeInit();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

void VdTestDynamic( uint32_t bufferNum, const char *outFmt, const char *videoFile )
{
    QCStatus_e ret;
    std::string errors;
    BufferManager bufMgr = BufferManager( { "VDEC", QC_NODE_TYPE_VDEC, 0 } );
    QCNodeIfs *pNodeVide = new QC::Node::VideoDecoder();
    DataTree dt;
    VidcDemuxer vidcDemuxer;
    VideoDecoder vidcDecoder;
    VidcDemuxer_Config_t vidcDemuxConfig;
    VideoDecoder_Config_t vidcDecoderConfig;
    VidcDemuxer_VideoInfo_t videoInfo;
    VidcDemuxer_FrameInfo_t frameInfo;

    dt.Set<std::string>( "static.name", "SANITY_VideoDecoder_Dynamic" );
    dt.Set<uint32_t>( "id", 1 );
    dt.Set<std::string>( "static.logLevel", "ERROR" );
    dt.Set<bool>( "inputDynamicMode", true );
    dt.Set<bool>( "outputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", bufferNum );
    dt.Set<uint32_t>( "numOutputBufferReq", bufferNum );
    dt.Set<uint32_t>( "frameRate", 30 );

    const char* inFmt = nullptr;

    switch( videoInfo.format ) {
    case QC_IMAGE_FORMAT_RGB888:
        inFmt = "rgb888";
        break;
    case QC_IMAGE_FORMAT_BGR888:
        inFmt = "bgr888";
        break;
    case QC_IMAGE_FORMAT_UYVY:
        inFmt = "uyvy";
        break;
    case QC_IMAGE_FORMAT_NV12:
        inFmt = "nv12";
        break;
    case QC_IMAGE_FORMAT_P010:
        inFmt = "p010";
        break;
    case QC_IMAGE_FORMAT_NV12_UBWC:
        inFmt = "nv12_ubwc";
        break;
    case QC_IMAGE_FORMAT_TP10_UBWC:
        inFmt = "tp10_ubwc";
        break;
    case QC_IMAGE_FORMAT_COMPRESSED_H264:
        inFmt = "h264";
        break;
    case QC_IMAGE_FORMAT_COMPRESSED_H265:
        inFmt = "265";
        break;
    default:
        printf("error: unrecognized input format %s\n", inFmt);
    }

    dt.Set<std::string>( "inputImageFormat", inFmt );
    dt.Set<std::string>( "outputImageFormat", outFmt );

    ret = vidcDemuxer.Init( &vidcDemuxConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = vidcDemuxer.GetVideoInfo( videoInfo );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeInit_t config = { .config = dt.Dump(), .callback = OnDoneCb };

    printf( "config: %s\n", config.config.c_str() );

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    QCNodeConfigIfs &cfgIfs = pNodeVide->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    uint32_t width;
    uint32_t height;
    QCImageFormat_e inFormat;
    QCImageFormat_e outFormat;
    uint32_t frameRate;
    bool bInputDynamicMode;
    bool bOutputDynamicMode;

    inFormat = dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );
    outFormat = dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H265 );
    width = dt.Get( "width", videoInfo.frameWidth );
    height = dt.Get( "height", videoInfo.frameHeight );
    frameRate = dt.Get( "frameRate", videoInfo.frameRate);

    printf( "options: %s\n", options.c_str() );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_DECODER_EVENT_BUFF_ID + 1 );

    std::vector<VideoFrameDescriptor_t> inputs;
    std::vector<VideoFrameDescriptor_t> outputs;

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( ImageBasicProps( width, height, inFormat ), bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( bufDesc );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = videoInfo.frameWidth;
        imgProps.height = videoInfo.frameHeight;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = videoInfo.maxFrameSize;
        imgProps.format = videoInfo.format;
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( imgProps, bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( bufDesc );
    }

    frameDesc.Clear();

    uint32_t nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    for ( uint32_t i = 0; i < nr; i++ )
    {
        //  Set up an input buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Set up an output buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    // wait input done signal
    std::unique_lock<std::mutex> inLock( g_InMutex );
    g_InCondVar.wait( inLock );

    // wait output done signal
    std::unique_lock<std::mutex> outLock( g_OutMutex );
    g_OutCondVar.wait( outLock );


    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    ret = pNodeVide->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto &output : outputs )
    {
        ret = bufMgr.Free( output );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &input : inputs )
    {
        ret = bufMgr.Free( input );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

}

#if 0
void Vd2TestDynamic( uint32_t bufferNum, QCImageFormat_e outFormat, char *videoFile )
{
    QCStatus_e ret;
    char pName[20] = "VidcDecoder";
    uint32_t frameNum = 10;

    VidcDemuxer vidcDemuxer;
    VideoDecoder vidcDecoder;
    VidcDemuxer_Config_t vidcDemuxConfig;
    VideoDecoder_Config_t vidcDecoderConfig;
    VidcDemuxer_VideoInfo_t videoInfo;
    VidcDemuxer_FrameInfo_t frameInfo;
    vidcDemuxConfig.pVideoFileName = videoFile;
    vidcDemuxConfig.startFrameIdx = 0;
    vidcDecoderConfig.bInputDynamicMode = true;
    vidcDecoderConfig.bOutputDynamicMode = true;
    vidcDecoderConfig.numInputBufferReq = bufferNum;
    vidcDecoderConfig.numOutputBufferReq = bufferNum;

    QCSharedBuffer_t inputBuffers[bufferNum];
    QCSharedBuffer_t outputBuffers[bufferNum];

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, vidcDecoder.GetState() );

    ret = vidcDemuxer.Init( &vidcDemuxConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = vidcDemuxer.GetVideoInfo( videoInfo );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ImageProps_t inputImgProps;
    inputImgProps.batchSize = 1;
    inputImgProps.width = videoInfo.frameWidth;
    inputImgProps.height = videoInfo.frameHeight;
    inputImgProps.numPlanes = 1;
    inputImgProps.planeBufSize[0] = videoInfo.maxFrameSize;
    inputImgProps.format = videoInfo.format;

    vidcDecoderConfig.width = videoInfo.frameWidth;
    vidcDecoderConfig.height = videoInfo.frameHeight;
    vidcDecoderConfig.inFormat = videoInfo.format;
    vidcDecoderConfig.outFormat = outFormat;
    vidcDecoderConfig.frameRate = 30;

    ret = vidcDecoder.Init( vidcDecoderConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, vidcDecoder.GetState() );

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = inputBuffers[i].Allocate( &inputImgProps );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = outputBuffers[i].Allocate( videoInfo.frameWidth, videoInfo.frameHeight, outFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = vidcDecoder.RegisterCallback( VdInputDoneCb, VdOutputDoneCb, VdEventCb,
                                        (void *) &vidcDecoder );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = vidcDecoder.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, vidcDecoder.GetState() );

    VideoDecoder_InputFrame_t inputFrame;
    VideoDecoder_OutputFrame_t outputFrame;
    ImageDescriptor_t imgDesc;

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        inputFrame.sharedBuffer = inputBuffers[i];
        imgDesc = inputFrame.sharedBuffer;
        ret = vidcDemuxer.GetFrame( imgDesc, frameInfo );
        ASSERT_EQ( QC_STATUS_OK, ret );

        inputFrame.timestampNs = frameInfo.startTime;
        inputFrame.appMarkData = i;

        ret = vidcDecoder.SubmitInputFrame( &inputFrame );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        outputFrame.sharedBuffer = outputBuffers[i];

        std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
        ret = vidcDecoder.SubmitOutputFrame( &outputFrame );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = bufferNum; i < frameNum; i++ )
    {
        uint32_t bufferIdx = i % bufferNum;
        inputFrame.sharedBuffer = inputBuffers[bufferIdx];
        outputFrame.sharedBuffer = outputBuffers[bufferIdx];
        imgDesc = inputFrame.sharedBuffer;
        ret = vidcDemuxer.GetFrame( imgDesc, frameInfo );
        ASSERT_EQ( QC_STATUS_OK, ret );

        inputFrame.timestampNs = frameInfo.startTime;
        inputFrame.appMarkData = i;

        ret = vidcDecoder.SubmitInputFrame( &inputFrame );
        ASSERT_EQ( QC_STATUS_OK, ret );

        std::unique_lock<std::mutex> outLock( s_OutMutex );
        s_OutCondVar.wait( outLock );
        ret = vidcDecoder.SubmitOutputFrame( &outputFrame );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = vidcDecoder.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, vidcDecoder.GetState() );

    ret = vidcDecoder.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, vidcDecoder.GetState() );

    ret = vidcDemuxer.DeInit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = inputBuffers[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = outputBuffers[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}
#endif

TEST( Decoder, SANITY_Decoder_Dynamic_HEVC_NV12 )
{
    uint32_t bufferNum = 4;
    char videoFile[] = "./data/test/VideoDecoder/test.mp4";
    VdTestDynamic( bufferNum, "nv12", videoFile );
}

TEST( Decoder, SANITY_Decoder_Dynamic_HEVC_P010 )
{
    uint32_t bufferNum = 4;
    char videoFile[] = "./data/test/VideoDecoder/test.mp4";
    VdTestDynamic( bufferNum, "p010", videoFile );
}


#ifndef GTEST_QCNODE
#if __CTC__
extern "C" void ctc_append_all( void );
#endif
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
#if __CTC__
    ctc_append_all();
#endif
    return nVal;
}
#endif
