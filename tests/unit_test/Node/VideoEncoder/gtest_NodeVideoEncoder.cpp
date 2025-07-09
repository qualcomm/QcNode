// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <stdio.h>
#include <string>

#include "QC/Node/VideoEncoder.hpp"

using namespace QC;
using namespace QC::Node;

static std::mutex g_InMutex;
static std::condition_variable g_InCondVar;
static std::mutex g_OutMutex;
static std::condition_variable g_OutCondVar;

static VideoEncoder_InputFrame_t g_sharedInputFrame;
static VideoEncoder_OutputFrame_t g_sharedOutputFrame;

static uint64_t g_timestamp = 0;

void OnDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    QCFrameDescriptorNodeIfs &frameDesc = eventInfo.frameDesc;

    // INPUT frame:
    QCBufferDescriptorBase_t &inBufDesc =
            frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID );
    const QCSharedVideoFrameDescriptor_t *pInSharedBuffer =
            static_cast<QCSharedVideoFrameDescriptor_t *>( &inBufDesc );

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
            frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID );
    const QCSharedVideoFrameDescriptor_t *pOutSharedBuffer =
            static_cast<QCSharedVideoFrameDescriptor_t *>( &outBufDesc );

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

TEST( NodeVideoEncoder, SANITY_VideoEncoder_Dynamic )
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "static.name", "SANITY_VideoEncoder_Dynamic" );
    dt.Set<uint32_t>( "id", 1 );
    dt.Set<std::string>( "static.logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", 176 );
    dt.Set<uint32_t>( "height", 144 );
    dt.Set<uint32_t>( "bitRate", 64000 );
    dt.Set<uint32_t>( "gop", 20 );
    dt.Set<bool>( "inputDynamicMode", true );
    dt.Set<bool>( "outputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 30 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

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
    uint32_t bitRate;
    uint32_t frameRate;
    uint32_t gop;
    VideoEncoder_Profile_e profile;
    VideoEncoder_RateControlMode_e rateControlMode;
    bool bInputDynamicMode;
    bool bOutputDynamicMode;

    width = dt.Get( "width", 0 );
    height = dt.Get( "height", 0 );
    bitRate = dt.Get( "bitRate", 0 );
    frameRate = dt.Get( "frameRate", 0 );
    gop = dt.Get( "gop", 0 );

    bInputDynamicMode = dt.Get( "inputDynamicMode", 1 );
    bOutputDynamicMode = dt.Get( "outputDynamicMode", 1 );
    rateControlMode = static_cast<VideoEncoder_RateControlMode_e>( dt.Get( "rateControlMode", 1 ) );
    numInputBufferReq = dt.Get( "numInputBufferReq", 1 );
    numOutputBufferReq = dt.Get( "numOutputBufferReq", 1 );

    profile = static_cast<VideoEncoder_Profile_e>( dt.Get( "profile", 0 ) );

    QCImageFormat_e inFormat = dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );
    QCImageFormat_e outFormat =
            dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H264 );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID + 1 );

    std::vector<QCSharedBufferDescriptor_t> inputs;
    std::vector<QCSharedBufferDescriptor_t> outputs;

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        QCSharedBufferDescriptor_t bufDesc;
        ret = bufDesc.buffer.Allocate( width, height, inFormat );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( bufDesc );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        QCImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = width;
        imgProps.height = height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = outFormat;
        QCSharedBufferDescriptor_t bufDesc;
        ret = static_cast<QCStatus_e>( bufDesc.buffer.Allocate( &imgProps ) );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( bufDesc );
    }

    frameDesc.Clear();

    uint32_t nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    for ( uint32_t i = 0; i < nr; i++ )
    {
        //  Set up an input buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Set up an output buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, outputs[i] );
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
        ret = static_cast<QCStatus_e>( output.buffer.Free() );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &input : inputs )
    {
        ret = static_cast<QCStatus_e>( input.buffer.Free() );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( NodeVideoEncoder, SANITY_VideoEncoder_NonDynamic )
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "static.name", "SANITY_VideoEncoder_NonDynamic" );
    dt.Set<uint32_t>( "id", 2 );
    dt.Set<std::string>( "static.logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", 1920 );
    dt.Set<uint32_t>( "height", 1080 );
    dt.Set<uint32_t>( "bitRate", 20000000 );
    dt.Set<uint32_t>( "gop", 0 );
    dt.Set<bool>( "inputDynamicMode", false );
    dt.Set<bool>( "outputDynamicMode", false );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 60 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

    QCNodeInit_t config = { .config = dt.Dump(), .callback = OnDoneCb };

    printf( "config: %s\n", config.config.c_str() );

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    QCNodeConfigIfs &cfgIfs = pNodeVide->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    printf( "options: %s\n", options.c_str() );

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    // wait inputdone siganl
    std::unique_lock<std::mutex> inLock( g_InMutex );
    g_InCondVar.wait( inLock );

    // wait outputdone siganl
    std::unique_lock<std::mutex> outLock( g_OutMutex );
    g_OutCondVar.wait( outLock );

    ret = pNodeVide->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );
}

TEST( NodeVideoEncoder, SANITY_VideoEncoder_Resolution )
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "static.name", "VideoEncoderResolution_128x128" );
    dt.Set<std::string>( "static.logLevel", "ERROR" );
    dt.Set<uint32_t>( "id", 2 );
    dt.Set<uint32_t>( "width", 128 );
    dt.Set<uint32_t>( "height", 128 );
    dt.Set<uint32_t>( "bitRate", 512000 );
    dt.Set<uint32_t>( "gop", 0 );
    dt.Set<bool>( "inputDynamicMode", true );
    dt.Set<bool>( "outputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 30 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

    QCNodeInit_t config = { .config = dt.Dump(), .callback = OnDoneCb };

    printf( "config: %s\n", config.config.c_str() );

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    dt.Set<std::string>( "static.name", "VideoEncoderResolution_176x144" );
    dt.Set<uint32_t>( "width", 176 );
    dt.Set<uint32_t>( "height", 144 );
    dt.Set<uint32_t>( "bitRate", 1000000 );

    config.config = dt.Dump();
    config.callback = OnDoneCb;

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    dt.Set<std::string>( "static.name", "VideoEncoderResolution_1280x720" );
    dt.Set<uint32_t>( "width", 1280 );
    dt.Set<uint32_t>( "height", 720 );
    dt.Set<uint32_t>( "bitRate", 2000000 );

    config.config = dt.Dump();
    config.callback = OnDoneCb;

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    dt.Set<std::string>( "static.name", "VideoEncoderResolution_1920x1080" );
    dt.Set<uint32_t>( "width", 1920 );
    dt.Set<uint32_t>( "height", 1080 );
    dt.Set<uint32_t>( "bitRate", 5000000 );

    config.config = dt.Dump();
    config.callback = OnDoneCb;

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    dt.Set<std::string>( "static.name", "VideoEncoderResolution_1920x1088" );
    dt.Set<uint32_t>( "width", 1920 );
    dt.Set<uint32_t>( "height", 1088 );
    dt.Set<uint32_t>( "bitRate", 10000000 );

    config.config = dt.Dump();
    config.callback = OnDoneCb;

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    dt.Set<std::string>( "static.name", "VideoEncoderResolution_3840x2160" );
    dt.Set<uint32_t>( "width", 3840 );
    dt.Set<uint32_t>( "height", 2160 );
    dt.Set<uint32_t>( "bitRate", 20000000 );
    dt.Set<std::string>( "inputImageFormat", "p010" );

    config.config = dt.Dump();
    config.callback = OnDoneCb;

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );
}

TEST( NodeVideoEncoder, SANITY_VideoEncoder_InitError )
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_width" );
    dt.Set<std::string>( "static.logLevel", "ERROR" );
    dt.Set<uint32_t>( "id", 3 );
    dt.Set<uint32_t>( "width", 0 );
    dt.Set<uint32_t>( "height", 0 );
    dt.Set<uint32_t>( "bitRate", 0 );
    dt.Set<uint32_t>( "gop", 0 );
    dt.Set<bool>( "inputDynamicMode", true );
    dt.Set<bool>( "outputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 1 );
    dt.Set<uint32_t>( "numOutputBufferReq", 128 );
    dt.Set<uint32_t>( "frameRate", 0 );
    dt.Set<std::string>( "profile", "max" );
    dt.Set<std::string>( "rateControlMode", "UNUSED" );
    dt.Set<std::string>( "inputImageFormat", "MAX" );
    dt.Set<std::string>( "outputImageFormat", "MAX" );

    QCNodeInit_t config = { .config = dt.Dump(), .callback = OnDoneCb };

    uint32_t numInputBufferReq = 1;
    uint32_t numOutputBufferReq = 128;

    printf( "config: %s\n", config.config.c_str() );

    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID + 1 );

    std::vector<QCSharedBufferDescriptor_t> inputs;
    std::vector<QCSharedBufferDescriptor_t> outputs;

    for ( unsigned int i = 0; i < numInputBufferReq; ++i )
    {
        QCSharedBufferDescriptor_t bufDesc;
        ret = (QCStatus_e) bufDesc.buffer.Allocate( 176, 144, QC_IMAGE_FORMAT_NV12 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( bufDesc );
    }

    for ( unsigned int i = 0; i < numOutputBufferReq; ++i )
    {
        QCImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = 176;
        imgProps.height = 144;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = QC_IMAGE_FORMAT_COMPRESSED_H264;
        QCSharedBufferDescriptor_t bufDesc;
        ret = (QCStatus_e) bufDesc.buffer.Allocate( &imgProps );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( bufDesc );
    }

    frameDesc.Clear();

    unsigned nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, pNodeVide->GetState() );

    for ( unsigned int i = 0; i < nr; i++ )
    {
        //  Set up the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_rc" );
    dt.Set<uint32_t>( "width", 176 );
    dt.Set<uint32_t>( "height", 144 );
    dt.Set<uint32_t>( "bitRate", 20000000 );
    dt.Set<std::string>( "inputImageFormat", "p010" );
    config.config = dt.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_rc" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    config.config = dt.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_informat" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    config.config = dt.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_outformat" );
    dt.Set<std::string>( "outputImageFormat", "h265" );
    config.config = dt.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_inreq" );
    dt.Set<uint32_t>( "numInputBufferReq", 8 );
    config.config = dt.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_outreq" );
    dt.Set<uint32_t>( "numOutputBufferReq", 8 );
    config.config = dt.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.name", "VideoEncoderErrorTest_bad_profile" );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    config.config = dt.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    for ( auto &output : outputs )
    {
        ret = output.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &input : inputs )
    {
        ret = input.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
