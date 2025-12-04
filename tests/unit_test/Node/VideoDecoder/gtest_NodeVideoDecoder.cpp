// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

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
    const VideoFrameDescriptor *pInSharedBuffer = dynamic_cast<VideoFrameDescriptor *>( &inBufDesc );

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
    const VideoFrameDescriptor *pOutSharedBuffer = dynamic_cast<VideoFrameDescriptor *>( &outBufDesc );

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

void VdTestDynamic( uint32_t bufferNum, QCImageFormat_e outFormat, const char *videoFile )
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
    uint32_t frameNum = 10;

    vidcDemuxConfig.pVideoFileName = videoFile;
    vidcDemuxConfig.startFrameIdx = 0;

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, vidcDecoder.GetState() );

    ret = vidcDemuxer.Init( &vidcDemuxConfig );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = vidcDemuxer.GetVideoInfo( videoInfo );
    ASSERT_EQ( QC_STATUS_OK, ret );

    dt.Set<std::string>( "name", "SANITY_VideoDecoder_Dynamic" );
    dt.Set<uint32_t>( "id", 1 );
    dt.Set<std::string>( "logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", videoInfo.frameWidth );
    dt.Set<uint32_t>( "height", videoInfo.frameHeight );
    dt.Set<bool>( "bInputDynamicMode", true );
    dt.Set<bool>( "bOutputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", bufferNum );
    dt.Set<uint32_t>( "numOutputBufferReq", bufferNum );
    dt.Set<uint32_t>( "frameRate", 30 );

    const char* inFmt = nullptr;
    const char* outFmt = nullptr;

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
        inFmt = "h265";
        break;
    default:
        printf("error: unrecognized input format %s\n", inFmt);
        return;
    }

    switch( outFormat ) {
    case QC_IMAGE_FORMAT_RGB888:
        outFmt = "rgb888";
        break;
    case QC_IMAGE_FORMAT_BGR888:
        outFmt = "bgr888";
        break;
    case QC_IMAGE_FORMAT_UYVY:
        outFmt = "uyvy";
        break;
    case QC_IMAGE_FORMAT_NV12:
        outFmt = "nv12";
        break;
    case QC_IMAGE_FORMAT_P010:
        outFmt = "p010";
        break;
    case QC_IMAGE_FORMAT_NV12_UBWC:
        outFmt = "nv12_ubwc";
        break;
    case QC_IMAGE_FORMAT_TP10_UBWC:
        outFmt = "tp10_ubwc";
        break;
    case QC_IMAGE_FORMAT_COMPRESSED_H264:
        outFmt = "h264";
        break;
    case QC_IMAGE_FORMAT_COMPRESSED_H265:
        outFmt = "h265";
        break;
    default:
        printf("error: unrecognized input format %s\n", inFmt);
        return;
    }

    dt.Set<std::string>( "inputImageFormat", inFmt );
    dt.Set<std::string>( "outputImageFormat", outFmt );

    DataTree dataTree;
    dataTree.Set( "static", dt );

    QCNodeInit_t config = { .config = dataTree.Dump(), .callback = OnDoneCb };

    printf( "config: %s\n", config.config.c_str() );

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    QCNodeConfigIfs &cfgIfs = pNodeVide->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t numInputBufferReq = dt.Get( "numInputBufferReq", bufferNum );
    uint32_t numOutputBufferReq = dt.Get( "numOutputBufferReq", bufferNum );

    printf( "options: %s\n", options.c_str() );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    std::vector<VideoFrameDescriptor_t> inputs;
    std::vector<VideoFrameDescriptor_t> outputs;
    ImageDescriptor_t imgDesc;

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        ImageProps_t inputImgProps;
        inputImgProps.batchSize = 1;
        inputImgProps.width = videoInfo.frameWidth;
        inputImgProps.height = videoInfo.frameHeight;
        inputImgProps.numPlanes = 1;
        inputImgProps.planeBufSize[0] = videoInfo.maxFrameSize;
        inputImgProps.format = videoInfo.format;
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( inputImgProps, bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( bufDesc );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( ImageBasicProps( videoInfo.frameWidth, videoInfo.frameHeight,
                                                outFormat ),
                               bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( bufDesc );
    }

    NodeFrameDescriptor inFrameDesc( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID + 1 );
    inFrameDesc.Clear();

    NodeFrameDescriptor outFrameDesc( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID + 1 );
    outFrameDesc.Clear();

    uint32_t nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        ret = vidcDemuxer.GetFrame( inputs[i], frameInfo );
        ASSERT_EQ( QC_STATUS_OK, ret );

        inputs[i].timestampNs = frameInfo.startTime;
        inputs[i].appMarkData = i;
        //  Set up an input buffer for the frame
        ret = inFrameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( inFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );

        //  Set up an output buffer for the frame
        ret = outFrameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( outFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = bufferNum; i < frameNum; i++ )
    {
        uint32_t bufferIdx = i % bufferNum;
        ret = vidcDemuxer.GetFrame( inputs[bufferIdx], frameInfo );
        ASSERT_EQ( QC_STATUS_OK, ret );

        inputs[bufferIdx].timestampNs = frameInfo.startTime;
        inputs[bufferIdx].appMarkData = i;

        ret = inFrameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, inputs[bufferIdx] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( inFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        std::unique_lock<std::mutex> outLock( g_OutMutex );
        g_OutCondVar.wait( outLock );
        std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );

        //  Set up an output buffer for the frame
        ret = outFrameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, outputs[bufferIdx] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( outFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

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

TEST( Decoder, SANITY_Decoder_Dynamic_HEVC_NV12 )
{
    uint32_t bufferNum = 4;
    char videoFile[] = "./data/test/VideoDecoder/test.mp4";
    VdTestDynamic( bufferNum, QC_IMAGE_FORMAT_NV12, videoFile );
}

TEST( Decoder, SANITY_Decoder_Dynamic_HEVC_P010 )
{
    uint32_t bufferNum = 4;
    char videoFile[] = "./data/test/VideoDecoder/test.mp4";
    VdTestDynamic( bufferNum, QC_IMAGE_FORMAT_P010, videoFile );
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
