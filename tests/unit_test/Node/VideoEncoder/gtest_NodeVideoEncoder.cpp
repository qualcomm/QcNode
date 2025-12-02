// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <stdio.h>
#include <string>

#include "QC/sample/BufferManager.hpp"
#include "QC/Node/VideoEncoder.hpp"

using namespace QC;
using namespace QC::Node;
using namespace QC::sample;

static std::mutex g_InMutex;
static std::condition_variable g_InCondVar;
static std::mutex g_OutMutex;
static std::condition_variable g_OutCondVar;

static uint32_t g_nodeId = 0;
static uint64_t g_timestamp = 0;

void OnDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    QCFrameDescriptorNodeIfs &frameDesc = eventInfo.frameDesc;

    // INPUT frame:
    QCBufferDescriptorBase_t &inBufDesc =
            frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID );
    const VideoFrameDescriptor_t *pInSharedBuffer = dynamic_cast<VideoFrameDescriptor_t *>( &inBufDesc );

    if ( nullptr != pInSharedBuffer )
    {
        printf( "%s::%s(): Received input video frame from node %d (%s), status %d, state %d.\n",
                __FILE__, __func__, eventInfo.node.id, eventInfo.node.name.c_str(),
                eventInfo.status, eventInfo.state );
        ASSERT_EQ( QC_STATUS_OK, eventInfo.status );
        ASSERT_EQ( g_nodeId, eventInfo.node.id );
    }

    // Send signal
    g_InCondVar.notify_one();

    // OUTPUT frame:
    QCBufferDescriptorBase_t &outBufDesc =
            frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID );
    const VideoFrameDescriptor_t *pOutSharedBuffer = dynamic_cast<VideoFrameDescriptor_t *>( &outBufDesc );

    if ( nullptr != pOutSharedBuffer )
    {
        printf( "%s::%s(): Received output video frame from node %d (%s), status %d, state %d.\n",
                __FILE__, __func__, eventInfo.node.id, eventInfo.node.name.c_str(),
                eventInfo.status, eventInfo.state );
        ASSERT_EQ( QC_STATUS_OK, eventInfo.status );
        ASSERT_EQ( g_nodeId, eventInfo.node.id );
    }

    // Send signal
    g_OutCondVar.notify_one();
}

TEST( NodeVideoEncoder, SANITY_VideoEncoder_Dynamic )
{
    QCStatus_e ret;
    std::string errors;
    BufferManager bufMgr = BufferManager( { "VENC", QC_NODE_TYPE_VENC, 0 } );
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "name", "SANITY_VideoEncoder_Dynamic" );
    dt.Set<uint32_t>( "id", g_nodeId = 1 );
    dt.Set<std::string>( "logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", 176 );
    dt.Set<uint32_t>( "height", 144 );
    dt.Set<uint32_t>( "bitrate", 64000 );
    dt.Set<uint32_t>( "gop", 20 );
    dt.Set<bool>( "bInputDynamicMode", true );
    dt.Set<bool>( "bOutputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 30 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

    DataTree dataTree;
    dataTree.Set( "static", dt );

    QCNodeInit_t config = { .config = dataTree.Dump(), .callback = OnDoneCb };

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

    uint32_t width = dt.Get( "width", 0 );
    uint32_t height = dt.Get( "height", 0 );
    uint32_t numInputBufferReq = dt.Get( "numInputBufferReq", 1 );
    uint32_t numOutputBufferReq = dt.Get( "numOutputBufferReq", 1 );
    QCImageFormat_e inFormat = dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );
    QCImageFormat_e outFormat =
            dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H264 );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID + 1 );

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
        ret = bufMgr.Free(output);
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &input : inputs )
    {
        ret = bufMgr.Free(input);
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( NodeVideoEncoder, SANITY_VideoEncoder_NonDynamic )
{
    QCStatus_e ret;
    std::string errors;
    BufferManager bufMgr = BufferManager( { "VENC", QC_NODE_TYPE_VENC, 0 } );
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "name", "SANITY_VideoEncoder_NonDynamic" );
    dt.Set<uint32_t>( "id", g_nodeId = 2 );
    dt.Set<std::string>( "logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", 1920 );
    dt.Set<uint32_t>( "height", 1080 );
    dt.Set<uint32_t>( "bitrate", 20000000 );
    dt.Set<uint32_t>( "gop", 0 );
    dt.Set<bool>( "bInputDynamicMode", false );
    dt.Set<bool>( "bOutputDynamicMode", false );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 60 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

    DataTree dataTree;
    dataTree.Set( "static", dt );

    uint32_t width = dt.Get( "width", 0 );
    uint32_t height = dt.Get( "height", 0 );
    uint32_t numInputBufferReq = dt.Get( "numInputBufferReq", 1 );
    uint32_t numOutputBufferReq = dt.Get( "numOutputBufferReq", 1 );
    QCImageFormat_e inFormat = dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );
    QCImageFormat_e outFormat =
            dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H264 );

    std::vector<VideoFrameDescriptor_t> buffers;

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( ImageBasicProps( width, height, inFormat,
                                                QC_MEMORY_ALLOCATOR_DMA_VPU, QC_CACHEABLE ),
                               bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        buffers.push_back( bufDesc );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = width;
        imgProps.height = height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 2 * 1024 * 1024;
        imgProps.format = outFormat;
        imgProps.allocatorType = QC_MEMORY_ALLOCATOR_DMA_VPU;
        imgProps.cache = QC_CACHEABLE;
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( imgProps, bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        buffers.push_back( bufDesc );
    }

    std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> bufferRefs;

    for ( VideoFrameDescriptor_t &frameDesc : buffers )
    {
        bufferRefs.push_back(frameDesc);
    }

    QCNodeInit_t config = { .config = dataTree.Dump(), .callback = OnDoneCb, .buffers = bufferRefs };

    printf( "config: %s\n", config.config.c_str() );

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    QCNodeConfigIfs &cfgIfs = pNodeVide->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    printf( "options: %s\n", options.c_str() );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    uint32_t nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID + 1 );
    frameDesc.Clear();

    for ( uint32_t i = 0; i < nr; i++ )
    {
        //  Set up an input buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID, buffers[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the input frame
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

    frameDesc.Clear();
    ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, buffers[numInputBufferReq] );
    ASSERT_EQ( QC_STATUS_OK, ret );
    //  Process the output frame
    ret = pNodeVide->ProcessFrameDescriptor( frameDesc );

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
    dt.Set<std::string>( "name", "VideoEncoderResolution_128x128" );
    dt.Set<std::string>( "logLevel", "ERROR" );
    dt.Set<uint32_t>( "id", g_nodeId = 3 );
    dt.Set<uint32_t>( "width", 128 );
    dt.Set<uint32_t>( "height", 128 );
    dt.Set<uint32_t>( "bitrate", 512000 );
    dt.Set<uint32_t>( "gop", 0 );
    dt.Set<bool>( "bInputDynamicMode", true );
    dt.Set<bool>( "bOutputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 30 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

    DataTree dataTree;
    dataTree.Set( "static", dt );

    QCNodeInit_t config = { .config = dataTree.Dump(), .callback = OnDoneCb };

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
    dt.Set<uint32_t>( "bitrate", 1000000 );
    dataTree.Set( "static", dt );

    config.config = dataTree.Dump();
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
    dt.Set<uint32_t>( "bitrate", 2000000 );
    dataTree.Set( "static", dt );

    config.config = dataTree.Dump();
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
    dt.Set<uint32_t>( "bitrate", 5000000 );
    dataTree.Set( "static", dt );

    config.config = dataTree.Dump();
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
    dt.Set<uint32_t>( "bitrate", 10000000 );
    dataTree.Set( "static", dt );

    config.config = dataTree.Dump();
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
    dt.Set<uint32_t>( "bitrate", 20000000 );
    dt.Set<std::string>( "inputImageFormat", "p010" );
    dataTree.Set( "static", dt );

    config.config = dataTree.Dump();
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
    BufferManager bufMgr = BufferManager( { "VENC", QC_NODE_TYPE_VENC, 0 } );
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_width" );
    dt.Set<std::string>( "logLevel", "ERROR" );
    dt.Set<uint32_t>( "id", g_nodeId = 4 );
    dt.Set<uint32_t>( "width", 0 );
    dt.Set<uint32_t>( "height", 0 );
    dt.Set<uint32_t>( "bitrate", 0 );
    dt.Set<uint32_t>( "gop", 0 );
    dt.Set<bool>( "bInputDynamicMode", true );
    dt.Set<bool>( "bOutputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 1 );
    dt.Set<uint32_t>( "numOutputBufferReq", 128 );
    dt.Set<uint32_t>( "frameRate", 0 );
    dt.Set<std::string>( "profile", "max" );
    dt.Set<std::string>( "rateControlMode", "UNUSED" );
    dt.Set<std::string>( "inputImageFormat", "MAX" );
    dt.Set<std::string>( "outputImageFormat", "MAX" );

    DataTree dataTree;
    dataTree.Set( "static", dt );

    QCNodeInit_t config = { .config = dataTree.Dump(), .callback = OnDoneCb };

    uint32_t numInputBufferReq = 1;
    uint32_t numOutputBufferReq = 128;

    printf( "config: %s\n", config.config.c_str() );

    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID + 1 );

    std::vector<VideoFrameDescriptor_t> inputs;
    std::vector<VideoFrameDescriptor_t> outputs;

    for ( unsigned int i = 0; i < numInputBufferReq; ++i )
    {
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( ImageBasicProps( 176, 144, QC_IMAGE_FORMAT_NV12 ), bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( bufDesc );
    }

    for ( unsigned int i = 0; i < numOutputBufferReq; ++i )
    {
        ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = 176;
        imgProps.height = 144;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = QC_IMAGE_FORMAT_COMPRESSED_H264;
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( imgProps, bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( bufDesc );
    }

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_rc" );
    dt.Set<uint32_t>( "width", 176 );
    dt.Set<uint32_t>( "height", 144 );
    dt.Set<uint32_t>( "bitrate", 20000000 );
    dt.Set<std::string>( "inputImageFormat", "p010" );

    dataTree.Set( "static", dt );
    config.config = dataTree.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_rc" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dataTree.Set( "static", dt );
    config.config = dataTree.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_informat" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dataTree.Set( "static", dt );
    config.config = dataTree.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_outformat" );
    dt.Set<std::string>( "outputImageFormat", "h265" );
    dataTree.Set( "static", dt );
    config.config = dataTree.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_inreq" );
    dt.Set<uint32_t>( "numInputBufferReq", 8 );
    dataTree.Set( "static", dt );
    config.config = dataTree.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_outreq" );
    dt.Set<uint32_t>( "numOutputBufferReq", 8 );
    dataTree.Set( "static", dt );
    config.config = dataTree.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "name", "VideoEncoderErrorTest_bad_profile" );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dataTree.Set( "static", dt );
    config.config = dataTree.Dump();
    config.callback = OnDoneCb;
    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    for ( auto &output : outputs )
    {
        ret = bufMgr.Free(output);
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &input : inputs )
    {
        ret = bufMgr.Free(input);
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
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
