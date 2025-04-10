// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "gtest/gtest.h"
#include <chrono>
#include <condition_variable>
#include <malloc.h>
#include <stdio.h>
#include <thread>

#include "VidcDemuxer.hpp"
#include "md5_utils.hpp"
#include "ridehal/common/Types.hpp"
#include "ridehal/component/VideoDecoder.hpp"

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::sample;
using namespace ridehal::test::utils;

static std::mutex s_inMutex;
static std::condition_variable s_InCondVar;
static std::mutex s_OutMutex;
static std::condition_variable s_OutCondVar;
static uint64_t g_timestamp = 0;

void VdInputDoneCb( const VideoDecoder_InputFrame_t *pInputFrame, void *pPrivData )
{
    RIDEHAL_LOG_INFO( "Decoder Input Done\n" );
}

void VdOutputDoneCb( const VideoDecoder_OutputFrame_t *pOutputFrame, void *pPrivData )
{
    s_OutCondVar.notify_one();
    RIDEHAL_LOG_INFO( "Decoder Output Done\n" );
}

void VdEventCb( const VideoDecoder_EventType_e eventId, const void *pEvent, void *pPrivData )
{
    RIDEHAL_LOG_INFO( "EventCb return" );
    switch ( eventId )
    {
        case VIDEO_CODEC_EVT_FLUSH_INPUT_DONE:
            RIDEHAL_LOG_INFO( "Received event: %d, pPrivData:%p", eventId, pPrivData );
            break;
        case VIDEO_CODEC_EVT_FLUSH_OUTPUT_DONE:
            RIDEHAL_LOG_INFO( "Received event: %d, pPrivData:%p", eventId, pPrivData );
            break;
        case VIDEO_CODEC_EVT_ERROR:
            RIDEHAL_LOG_INFO( "Received event: %d, pPrivData:%p", eventId, pPrivData );
            break;
    }
}

void VdTestDynamic( uint32_t bufferNum, RideHal_ImageFormat_e outFormat, char *videoFile )
{
    RideHalError_e ret;
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
    vidcDecoderConfig.numInputBuffer = bufferNum;
    vidcDecoderConfig.numOutputBuffer = bufferNum;

    RideHal_SharedBuffer_t inputBuffers[bufferNum];
    RideHal_SharedBuffer_t outputBuffers[bufferNum];

    ASSERT_EQ( RIDEHAL_COMPONENT_STATE_INITIAL, vidcDecoder.GetState() );

    ret = vidcDemuxer.Init( &vidcDemuxConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = vidcDemuxer.GetVideoInfo( videoInfo );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    RideHal_ImageProps_t inputImgProps;
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
    vidcDecoderConfig.pInputBufferList = nullptr;
    vidcDecoderConfig.pOutputBufferList = nullptr;

    ret = vidcDecoder.Init( pName, &vidcDecoderConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDEHAL_COMPONENT_STATE_READY, vidcDecoder.GetState() );

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = inputBuffers[i].Allocate( &inputImgProps );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = outputBuffers[i].Allocate( videoInfo.frameWidth, videoInfo.frameHeight, outFormat );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = vidcDecoder.RegisterCallback( VdInputDoneCb, VdOutputDoneCb, VdEventCb,
                                        (void *) &vidcDecoder );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = vidcDecoder.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDEHAL_COMPONENT_STATE_RUNNING, vidcDecoder.GetState() );

    VideoDecoder_InputFrame_t inputFrame;
    VideoDecoder_OutputFrame_t outputFrame;

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        inputFrame.sharedBuffer = inputBuffers[i];
        ret = vidcDemuxer.GetFrame( &inputBuffers[i], frameInfo );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        inputFrame.timestampNs = frameInfo.startTime;
        inputFrame.appMarkData = i;

        ret = vidcDecoder.SubmitInputFrame( &inputFrame );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        outputFrame.sharedBuffer = outputBuffers[i];

        std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
        ret = vidcDecoder.SubmitOutputFrame( &outputFrame );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( uint32_t i = bufferNum; i < frameNum; i++ )
    {
        uint32_t bufferIdx = i % bufferNum;
        inputFrame.sharedBuffer = inputBuffers[bufferIdx];
        outputFrame.sharedBuffer = outputBuffers[bufferIdx];

        ret = vidcDemuxer.GetFrame( &inputBuffers[bufferIdx], frameInfo );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        inputFrame.timestampNs = frameInfo.startTime;
        inputFrame.appMarkData = i;

        ret = vidcDecoder.SubmitInputFrame( &inputFrame );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        std::unique_lock<std::mutex> outLock( s_OutMutex );
        s_OutCondVar.wait( outLock );
        ret = vidcDecoder.SubmitOutputFrame( &outputFrame );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = vidcDecoder.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDEHAL_COMPONENT_STATE_READY, vidcDecoder.GetState() );

    ret = vidcDecoder.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ASSERT_EQ( RIDEHAL_COMPONENT_STATE_INITIAL, vidcDecoder.GetState() );

    ret = vidcDemuxer.DeInit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = inputBuffers[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( uint32_t i = 0; i < bufferNum; i++ )
    {
        ret = outputBuffers[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
}

TEST( Demuxer, SANITY_Demuxer )
{
    RideHalError_e ret;

    VidcDemuxer vidcDemuxer;
    VidcDemuxer_Config_t vidcDemuxConfig;
    VidcDemuxer_VideoInfo_t videoInfo;

    vidcDemuxConfig.pVideoFileName = "./data/test/VideoDecoder/test.mp4";
    vidcDemuxConfig.startFrameIdx = 0;

    ret = vidcDemuxer.Init( &vidcDemuxConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = vidcDemuxer.GetVideoInfo( videoInfo );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ASSERT_EQ( 1920, videoInfo.frameWidth );
    ASSERT_EQ( 1024, videoInfo.frameHeight );
    ASSERT_EQ( 101, videoInfo.format );

    ret = vidcDemuxer.DeInit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( Decoder, SANITY_Decoder_Dynamic_HEVC_NV12 )
{
    uint32_t bufferNum = 4;
    RideHal_ImageFormat_e outFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    char videoFile[] = "./data/test/VideoDecoder/test.mp4";
    VdTestDynamic( bufferNum, outFormat, videoFile );
}

TEST( Decoder, SANITY_Decoder_Dynamic_HEVC_P010 )
{
    uint32_t bufferNum = 4;
    RideHal_ImageFormat_e outFormat = RIDEHAL_IMAGE_FORMAT_P010;
    char videoFile[] = "./data/test/VideoDecoder/test.mp4";
    VdTestDynamic( bufferNum, outFormat, videoFile );
}


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
