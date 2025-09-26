// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "gtest/gtest.h"
#include <condition_variable>
#include <malloc.h>
#include <stdio.h>

#include "QC/Common/Types.hpp"
#include "QC/component/VideoEncoder.hpp"

using namespace QC;
using namespace QC::component;

static std::mutex s_inMutex;
static std::condition_variable s_InCondVar;
static std::mutex s_OutMutex;
static std::condition_variable s_OutCondVar;

static VideoEncoder_InputFrame_t s_sharedInputFrame;
static VideoEncoder_OutputFrame_t s_sharedOutputFrame;

static uint64_t g_timestamp = 0;

void OnInputDoneCb( const VideoEncoder_InputFrame_t *pInputFrame, void *pPrivData )
{
    s_sharedInputFrame = *pInputFrame;
    // sent signal
    s_InCondVar.notify_one();
}

void OnOutputDoneCb( const VideoEncoder_OutputFrame_t *pOutputFrame, void *pPrivData )
{
    s_sharedOutputFrame = *pOutputFrame;
    // sent signal
    s_OutCondVar.notify_one();
}

void EventCb( const VideoEncoder_EventType_e eventId, const void *pEvent, void *pPrivData )
{
    printf( "EventCb return \n" );
    switch ( eventId )
    {
        case VIDEO_CODEC_EVT_FLUSH_INPUT_DONE:
            printf( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
            break;
        case VIDEO_CODEC_EVT_FLUSH_OUTPUT_DONE:
            printf( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
            break;
        case VIDEO_CODEC_EVT_ERROR:
            printf( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
            break;
    }
}

TEST( VideoEncoder, SANITY_VideoEncoder_Dynamic )
{
    uint32_t i = 0;
    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 176;
    config.height = 144;
    config.bitRate = 64000;
    config.gop = 20;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 30;
    config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H264;
    config.bInputDynamicMode = true;
    config.bOutputDynamicMode = true;
    config.pInputBufferList = nullptr;
    config.pOutputBufferList = nullptr;

    QCStatus_e ret;
    QCSharedBuffer_t *sharedBuffer = nullptr;

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderDynamic", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.RegisterCallback( OnInputDoneCb, OnOutputDoneCb, EventCb, (void *) &veTest );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = veTest.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, veTest.GetState() );

    VideoEncoder_OnTheFlyCmd_t onTheFlyCmd;
    onTheFlyCmd.propID = VIDEO_ENCODER_PROP_FRAME_RATE;
    onTheFlyCmd.value = 15;
    ret = veTest.Configure( &onTheFlyCmd );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t *inputList = new QCSharedBuffer_t[config.numInputBufferReq];
    ret = veTest.GetInputBuffers( inputList, config.numInputBufferReq );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );
    QCSharedBuffer_t *outputList = new QCSharedBuffer_t[config.numOutputBufferReq];
    ret = veTest.GetOutputBuffers( outputList, config.numOutputBufferReq );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );

    VideoEncoder_InputFrame_t *inputFrame =
            new VideoEncoder_InputFrame_t[config.numInputBufferReq + 1];

    VideoEncoder_OutputFrame_t *outputFrame =
            new VideoEncoder_OutputFrame_t[config.numOutputBufferReq + 1];

    for ( int i = 0; i < config.numOutputBufferReq; i++ )
    {
        sharedBuffer = &inputFrame[i].sharedBuffer;
        ret = sharedBuffer->Allocate( config.width, config.height, config.inFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );

        sharedBuffer = &outputFrame[i].sharedBuffer;
        QCImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = config.width;
        imgProps.height = config.height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = config.outFormat;
        ret = sharedBuffer->Allocate( &imgProps );
        // ret = sharedBuffer->Allocate( 118784 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = veTest.SubmitOutputFrame( &outputFrame[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    ret = veTest.SubmitOutputFrame( &outputFrame[config.numOutputBufferReq - 1] );
    ASSERT_EQ( QC_STATUS_NOMEM, ret );

    sharedBuffer = &outputFrame[config.numOutputBufferReq].sharedBuffer;
    QCImageProps_t imgProps;
    imgProps.batchSize = 1;
    imgProps.width = config.width;
    imgProps.height = config.height;
    imgProps.numPlanes = 1;
    imgProps.planeBufSize[0] = 118784;
    imgProps.format = config.outFormat;
    ret = sharedBuffer->Allocate( &imgProps );
    // ret = sharedBuffer->Allocate( 118784 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = veTest.SubmitOutputFrame( &outputFrame[config.numOutputBufferReq] );
    ASSERT_EQ( QC_STATUS_NOMEM, ret );

    VideoEncoder_OnTheFlyCmd_t *onTheFlyCmds = new VideoEncoder_OnTheFlyCmd_t[2];
    onTheFlyCmds[0].propID = VIDEO_ENCODER_PROP_BITRATE;
    onTheFlyCmds[0].value = 32000;
    onTheFlyCmds[1].propID = VIDEO_ENCODER_PROP_FRAME_RATE;
    onTheFlyCmds[1].value = 20;
    for ( int i = 0; i < config.numInputBufferReq; i++ )
    {
        inputFrame[i].timestampNs = g_timestamp;
        inputFrame[i].appMarkData = i;
        inputFrame[i].numCmd = 2;
        inputFrame[i].pOnTheFlyCmd = onTheFlyCmds;
        ret = veTest.SubmitInputFrame( &inputFrame[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        g_timestamp += 33333;
    }
    ret = veTest.SubmitInputFrame( &inputFrame[config.numInputBufferReq - 1] );
    ASSERT_EQ( QC_STATUS_NOMEM, ret );

    sharedBuffer = &inputFrame[config.numInputBufferReq].sharedBuffer;
    ret = sharedBuffer->Allocate( config.width, config.height, config.inFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = veTest.SubmitInputFrame( &inputFrame[config.numInputBufferReq] );
    ASSERT_EQ( QC_STATUS_NOMEM, ret );

    // wait inputdone siganl
    std::unique_lock<std::mutex> inLock( s_inMutex );
    s_InCondVar.wait( inLock );

    ret = veTest.SubmitInputFrame( &inputFrame[0] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // wait outputdone siganl
    std::unique_lock<std::mutex> outLock( s_OutMutex );
    s_OutCondVar.wait( outLock );

    ret = veTest.SubmitOutputFrame( &s_sharedOutputFrame );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = veTest.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    for ( i = 0; i < config.numInputBufferReq + 1; i++ )
    {
        ret = outputFrame[i].sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = inputFrame[i].sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    delete[] outputFrame;
    delete[] inputList;
    delete[] outputList;
    delete onTheFlyCmds;
}

TEST( VideoEncoder, SANITY_VideoEncoder_NonDynamic )
{
    QCStatus_e ret;
    uint32_t i = 0;

    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 1920;
    config.height = 1080;
    config.bitRate = 20000000;
    config.gop = 0;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 60;
    config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H264;
    config.bInputDynamicMode = false;
    config.bOutputDynamicMode = false;
    config.pInputBufferList = nullptr;
    config.pOutputBufferList = nullptr;

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderNormal", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.RegisterCallback( OnInputDoneCb, OnOutputDoneCb, EventCb, (void *) &veTest );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = veTest.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, veTest.GetState() );

    QCSharedBuffer_t *inputList = new QCSharedBuffer_t[config.numInputBufferReq];
    ret = veTest.GetInputBuffers( inputList, config.numInputBufferReq );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( i = 0; i < config.numInputBufferReq; i++ )
    {
        VideoEncoder_InputFrame_t inputFrame;
        inputFrame.sharedBuffer = inputList[i];
        inputFrame.timestampNs = g_timestamp;
        inputFrame.appMarkData = i;
        ret = veTest.SubmitInputFrame( &inputFrame );
        ASSERT_EQ( QC_STATUS_OK, ret );
        g_timestamp += 33333;
    }

    QCSharedBuffer_t *outputList = new QCSharedBuffer_t[config.numOutputBufferReq];
    ret = veTest.GetOutputBuffers( outputList, config.numOutputBufferReq );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( i = 0; i < config.numOutputBufferReq; i++ )
    {
        printf( "outputList[%d].data(): %p\n", i, outputList[i].data() );
    }

    // wait inputdone siganl
    std::unique_lock<std::mutex> inLock( s_inMutex );
    s_InCondVar.wait( inLock );

    // wait outputdone siganl
    std::unique_lock<std::mutex> outLock( s_OutMutex );
    s_OutCondVar.wait( outLock );

    ret = veTest.SubmitOutputFrame( &s_sharedOutputFrame );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = veTest.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    delete[] inputList;
    delete[] outputList;
}

TEST( VideoEncoder, SANITY_VideoEncoder_ConfigBuffer )
{
    QCStatus_e ret;
    uint32_t i = 0;

    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 176;
    config.height = 144;
    config.bitRate = 64000;
    config.gop = 0;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 30;
    config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H265;
    config.bInputDynamicMode = false;
    config.bOutputDynamicMode = false;

    QCSharedBuffer_t *inBufferList = new QCSharedBuffer_t[config.numInputBufferReq];
    QCSharedBuffer_t *outBufferList = new QCSharedBuffer_t[config.numOutputBufferReq];
    for ( i = 0; i < config.numInputBufferReq; i++ )
    {
        ret = inBufferList[i].Allocate( config.width, config.height, config.inFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
        QCImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = config.width;
        imgProps.height = config.height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = config.outFormat;
        ret = outBufferList[i].Allocate( &imgProps );
        // ret = sharedBuffer->Allocate( 118784 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    config.pInputBufferList = inBufferList;
    config.pOutputBufferList = outBufferList;

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderConfigBuffer", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.RegisterCallback( OnInputDoneCb, OnOutputDoneCb, EventCb, (void *) &veTest );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = veTest.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, veTest.GetState() );

    for ( i = 0; i < config.numInputBufferReq - 1; i++ )
    {
        VideoEncoder_InputFrame_t inputFrame;
        inputFrame.sharedBuffer = inBufferList[i];
        inputFrame.timestampNs = g_timestamp;
        inputFrame.appMarkData = i;
        ret = veTest.SubmitInputFrame( &inputFrame );
        ASSERT_EQ( QC_STATUS_OK, ret );
        g_timestamp += 33333;
    }
    VideoEncoder_OnTheFlyCmd_t *onTheFlyCmds = new VideoEncoder_OnTheFlyCmd_t[2];
    onTheFlyCmds[0].propID = VIDEO_ENCODER_PROP_BITRATE;
    onTheFlyCmds[0].value = 32000;
    onTheFlyCmds[1].propID = VIDEO_ENCODER_PROP_MAX;
    onTheFlyCmds[1].value = 20;
    VideoEncoder_InputFrame_t inputFrame;
    inputFrame.sharedBuffer = inBufferList[config.numInputBufferReq - 1];
    inputFrame.timestampNs = g_timestamp;
    inputFrame.appMarkData = config.numInputBufferReq - 1;
    inputFrame.numCmd = 2;
    inputFrame.pOnTheFlyCmd = onTheFlyCmds;
    ret = veTest.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    g_timestamp += 33333;

    // wait inputdone siganl
    std::unique_lock<std::mutex> inLock( s_inMutex );
    s_InCondVar.wait( inLock );

    // wait outputdone siganl
    std::unique_lock<std::mutex> outLock( s_OutMutex );
    s_OutCondVar.wait( outLock );

    ret = veTest.SubmitOutputFrame( &s_sharedOutputFrame );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = veTest.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    for ( i = 0; i < config.numInputBufferReq; i++ )
    {
        ret = inBufferList[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = outBufferList[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    delete[] inBufferList;
    delete[] outBufferList;
    delete onTheFlyCmds;
}

TEST( VideoEncoder, SANITY_VideoEncoder_Resolution )
{
    QCStatus_e ret;
    uint32_t i = 0;

    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 128;
    config.height = 128;
    config.bitRate = 512000;
    config.gop = 0;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 30;
    config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H264;
    config.bInputDynamicMode = false;
    config.bOutputDynamicMode = false;
    config.pInputBufferList = nullptr;
    config.pOutputBufferList = nullptr;

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderResolution_128x128", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );


    config.width = 176;
    config.height = 144;
    config.bitRate = 1000000;
    ret = veTest.Init( "VideoEncoderResolution_176x144", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );


    config.width = 1280;
    config.height = 720;
    config.bitRate = 2000000;
    ret = veTest.Init( "VideoEncoderResolution_1280x720", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );


    config.width = 1920;
    config.height = 1080;
    config.bitRate = 5000000;
    ret = veTest.Init( "VideoEncoderResolution_1920x1080", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );


    config.width = 1920;
    config.height = 1088;
    config.bitRate = 10000000;
    ret = veTest.Init( "VideoEncoderResolution_1920x1088", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );


    config.width = 3840;
    config.height = 2160;
    config.bitRate = 20000000;
    config.inFormat = QC_IMAGE_FORMAT_P010;
    ret = veTest.Init( "VideoEncoderResolution_3840x2160", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );
}

TEST( VideoEncoder, SANITY_VideoEncoder_InitError )
{
    QCStatus_e ret;
    uint32_t i = 0;

    VideoEncoder veTest;
    VideoEncoder_Config_t config;
    config.width = 0;
    config.height = 0;
    config.bitRate = 0;
    config.gop = 0;
    config.numInputBufferReq = 1;
    config.numOutputBufferReq = 128;
    config.frameRate = 0;
    config.profile = VIDEO_ENCODER_PROFILE_MAX;
    config.rateControlMode = VIDEO_ENCODER_RCM_UNUSED;
    config.inFormat = QC_IMAGE_FORMAT_MAX;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_MAX;
    config.bInputDynamicMode = true;
    config.bOutputDynamicMode = true;

    QCSharedBuffer_t *inBufferList = new QCSharedBuffer_t[8];
    QCSharedBuffer_t *outBufferList = new QCSharedBuffer_t[8];
    for ( i = 0; i < 8; i++ )
    {
        ret = inBufferList[i].Allocate( 176, 144, QC_IMAGE_FORMAT_NV12 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        QCImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = 176;
        imgProps.height = 144;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = QC_IMAGE_FORMAT_COMPRESSED_H264;
        ret = outBufferList[i].Allocate( &imgProps );
        // ret = sharedBuffer->Allocate( 118784 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    config.pInputBufferList = inBufferList;
    config.pOutputBufferList = outBufferList;

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderErrorTest_nullptr", nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    ret = veTest.Init( "VideoEncoderErrorTest_bad_width", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.width = 176;
    config.height = 144;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_rc", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_informat", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.inFormat = QC_IMAGE_FORMAT_NV12;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_outformat", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H265;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_inreq", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.numInputBufferReq = 8;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_outreq", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.numOutputBufferReq = 8;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_inputlist", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.pInputBufferList = nullptr;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_outputlist", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.pOutputBufferList = nullptr;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_profile", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_profile2", &config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    config.width = 1920;
    config.height = 1080;
    config.bInputDynamicMode = false;
    config.pInputBufferList = inBufferList;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_inbuffer", &config );
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    config.bInputDynamicMode = true;
    config.pInputBufferList = nullptr;
    config.bOutputDynamicMode = false;
    config.pOutputBufferList = outBufferList;
    ret = veTest.Init( "VideoEncoderErrorTest_bad_outbuffer", &config );
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest.GetState() );

    for ( i = 0; i < 8; i++ )
    {
        ret = inBufferList[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = outBufferList[i].Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    delete[] inBufferList;
    delete[] outBufferList;
}

TEST( VideoEncoder, SANITY_VideoEncoder_OtherError )
{
    QCStatus_e ret;
    uint32_t i = 0;

    VideoEncoder veTest1, veTest;
    VideoEncoder_Config_t config;
    config.width = 176;
    config.height = 144;
    config.bitRate = 512000;
    config.gop = 0;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 60;
    config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H264;
    config.bInputDynamicMode = false;
    config.bOutputDynamicMode = false;
    config.pInputBufferList = nullptr;
    config.pOutputBufferList = nullptr;

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, veTest1.GetState() );

    ret = veTest1.Init( nullptr, nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = veTest1.Start();
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = veTest1.Configure( nullptr );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = veTest1.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = veTest1.Deinit();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );


    ret = veTest.Init( "VideoEncoderErrorTest", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, veTest.GetState() );

    ret = veTest.Start();
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = veTest.RegisterCallback( nullptr, nullptr, nullptr, nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = veTest.RegisterCallback( OnInputDoneCb, nullptr, nullptr, nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    ret = veTest.Start();
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    VideoEncoder_InputFrame_t inputFrame;
    ret = veTest.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    VideoEncoder_OutputFrame_t outputFrame;
    ret = veTest.SubmitOutputFrame( &outputFrame );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = veTest.RegisterCallback( OnInputDoneCb, OnOutputDoneCb, EventCb, (void *) &veTest );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedBuffer_t *outputList = nullptr;
    ret = veTest.GetOutputBuffers( outputList, config.numOutputBufferReq );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    outputList = new QCSharedBuffer_t[2];
    ret = veTest.GetOutputBuffers( outputList, 2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    QCSharedBuffer_t *inputList = nullptr;
    ret = veTest.GetInputBuffers( inputList, config.numInputBufferReq );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    inputList = new QCSharedBuffer_t[2];
    ret = veTest.GetInputBuffers( inputList, 2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = veTest.Configure( nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    VideoEncoder_OnTheFlyCmd_t onTheFlyCmd;
    onTheFlyCmd.propID = VIDEO_ENCODER_PROP_MAX;
    ret = veTest.Configure( &onTheFlyCmd );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = veTest.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, veTest.GetState() );

    ret = veTest.SubmitInputFrame( nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = veTest.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    QCSharedBuffer_t sharedBuffer;
    ret = sharedBuffer.Allocate( 128, 128, QC_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    inputFrame.sharedBuffer = sharedBuffer;
    ret = veTest.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = sharedBuffer.Allocate( config.width, config.height, QC_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    inputFrame.sharedBuffer = sharedBuffer;
    ret = veTest.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = sharedBuffer.Allocate( config.width, config.height, QC_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    inputFrame.sharedBuffer = sharedBuffer;
    ret = veTest.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( QC_STATUS_NOMEM, ret );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = veTest.SubmitOutputFrame( nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    QCImageProps_t imgProps;
    imgProps.batchSize = 1;
    imgProps.width = config.width;
    imgProps.height = config.height;
    imgProps.numPlanes = 1;
    imgProps.planeBufSize[0] = 118784;
    imgProps.format = QC_IMAGE_FORMAT_COMPRESSED_H265;
    ret = sharedBuffer.Allocate( &imgProps );
    ASSERT_EQ( QC_STATUS_OK, ret );
    outputFrame.sharedBuffer = sharedBuffer;
    ret = veTest.SubmitOutputFrame( &outputFrame );
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    imgProps.format = QC_IMAGE_FORMAT_COMPRESSED_H264;
    ret = sharedBuffer.Allocate( &imgProps );
    ASSERT_EQ( QC_STATUS_OK, ret );
    outputFrame.sharedBuffer = sharedBuffer;
    ret = veTest.SubmitOutputFrame( &outputFrame );
    ASSERT_EQ( QC_STATUS_NOMEM, ret );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
