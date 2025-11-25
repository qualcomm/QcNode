// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "md5_utils.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "QC/Node/OpticalFlow.hpp"
#include "QC/sample/BufferManager.hpp"

using namespace QC::Node;
using namespace QC::test::utils;
using namespace QC;
using namespace QC::Memory;
using namespace QC::sample;

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

// Scope guard implementation for automatic cleanup
template<typename F>
class ScopeGuard
{
public:
    explicit ScopeGuard( F &&f ) : func_( std::forward<F>( f ) ), active_( true ) {}

    ~ScopeGuard()
    {
        if ( active_ )
        {
            func_();
        }
    }

    void dismiss() { active_ = false; }

    ScopeGuard( const ScopeGuard & ) = delete;
    ScopeGuard &operator=( const ScopeGuard & ) = delete;

    ScopeGuard( ScopeGuard &&other ) : func_( std::move( other.func_ ) ), active_( other.active_ )
    {
        other.active_ = false;
    }

private:
    F func_;
    bool active_;
};

template<typename F>
ScopeGuard<F> MakeScopeGuard( F &&f )
{
    return ScopeGuard<F>( std::forward<F>( f ) );
}

#define SCOPE_GUARD_CONCAT_IMPL( x, y ) x##y
#define SCOPE_GUARD_CONCAT( x, y ) SCOPE_GUARD_CONCAT_IMPL( x, y )
#define SCOPE_EXIT( code )                                                                         \
    auto SCOPE_GUARD_CONCAT( scope_guard_, __LINE__ ) = MakeScopeGuard( [&]() { code; } )

static void LoadRaw( void *pData, uint32_t length, std::string path )
{
    printf( "  load raw from %s\n", path.c_str() );
    FILE *pFile = fopen( path.c_str(), "rb" );
    ASSERT_NE( nullptr, pFile );
    fseek( pFile, 0, SEEK_END );
    int size = ftell( pFile );
    ASSERT_LE( size, length );
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, size, pFile );
    ASSERT_EQ( r, size );
    fclose( pFile );
}

static void SaveRaw( std::string path, void *pData, size_t size )
{
    FILE *pFile = fopen( path.c_str(), "wb" );
    if ( nullptr != pFile )
    {
        fwrite( pData, 1, size, pFile );
        fclose( pFile );
        printf( "  save raw %s\n", path.c_str() );
    }
}


static void Eva_NodeOpticalFlowRun( std::string name, OpticalFlow_Config_t &config,
                                    std::string img1 = "", std::string img2 = "",
                                    std::string goldenFwdMvMap = "",
                                    std::string goldenFwdMvConf = "",
                                    std::string goldenBwdMvMap = "",
                                    std::string goldenBwdMvConf = "" )
{
    QC::Node::OpticalFlow opticalFlow;
    QCNodeIfs *node = dynamic_cast<QCNodeIfs *>( &opticalFlow );
    QCStatus_e ret;
    QCStatus_e status;

    BufferManager bufMgr( { "LME", QC_NODE_TYPE_EVA_OPTICAL_FLOW, 0 } );

    ImageDescriptor_t refImgDesc;
    ImageBasicProps_t refImgProp;
    ImageDescriptor_t curImgDesc;
    ImageBasicProps_t curImgProp;
    TensorDescriptor_t mvFwdMapDesc;
    TensorDescriptor_t mvFwdconfMapDesc;
    TensorDescriptor_t mvFwdMapDescG;
    TensorDescriptor_t mvFwdconfMapDescG;
    TensorDescriptor_t mvBwdMapDesc;
    TensorDescriptor_t mvBwdconfMapDesc;
    TensorDescriptor_t mvBwdMapDescG;
    TensorDescriptor_t mvBwdconfMapDescG;
    TensorProps_t mvFwdMapProp;
    TensorProps_t mvFwdConfProp;
    TensorProps_t mvBwdMapProp;
    TensorProps_t mvBwdConfProp;

    NodeFrameDescriptor *frameDescriptor = nullptr;
    bool nodeInitialized = false;
    bool nodeStarted = false;

    // Scope guard to ensure cleanup happens even if ASSERT fails
    SCOPE_EXIT(
            // Stop node if started
            if ( nodeStarted && node ) {
                QCStatus_e stopStatus = node->Stop();
                EXPECT_EQ( QC_STATUS_OK, stopStatus );
            }

            // DeInitialize node if initialized
            if ( nodeInitialized && node ) {
                QCStatus_e deinitStatus = node->DeInitialize();
                EXPECT_EQ( QC_STATUS_OK, deinitStatus );
            }

            // Free all buffers
            bufMgr.Free( refImgDesc );
            bufMgr.Free( curImgDesc ); bufMgr.Free( mvFwdMapDesc ); bufMgr.Free( mvFwdconfMapDesc );
            bufMgr.Free( mvFwdMapDescG ); bufMgr.Free( mvFwdconfMapDescG );
            bufMgr.Free( mvBwdMapDesc ); bufMgr.Free( mvBwdconfMapDesc );
            bufMgr.Free( mvBwdMapDescG ); bufMgr.Free( mvBwdconfMapDescG );

            // Delete frame descriptor
            if ( frameDescriptor ) { delete frameDescriptor; } );

    refImgProp.format = config.imageFormat;
    refImgProp.batchSize = 1;
    refImgProp.width = config.width;
    refImgProp.height = config.height;

    ret = bufMgr.Allocate( refImgProp, refImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    curImgProp.format = config.imageFormat;
    curImgProp.batchSize = 1;
    curImgProp.width = config.width;
    curImgProp.height = config.height;

    ret = bufMgr.Allocate( curImgProp, curImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( img1.empty() == false )
    {
        LoadRaw( refImgDesc.GetDataPtr(), refImgDesc.GetDataSize(), img1 );
    }

    if ( img2.empty() == false )
    {
        LoadRaw( curImgDesc.GetDataPtr(), curImgDesc.GetDataSize(), img2 );
    }


    if ( ( config.motionDirection == MOTION_DIRECTION_FORWARD ) or
         ( config.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
    {

        mvFwdMapProp = { QC_TENSOR_TYPE_UINT_16,
                         { 1, ALIGN_S( config.height, 8 ), ALIGN_S( config.width * 2, 128 ), 1 } };

        ret = bufMgr.Allocate( mvFwdMapProp, mvFwdMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        mvFwdConfProp = { QC_TENSOR_TYPE_UINT_8,
                          { 1, ALIGN_S( config.height, 8 ), ALIGN_S( config.width, 128 ), 1 } };


        ret = bufMgr.Allocate( mvFwdConfProp, mvFwdconfMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        memset( mvFwdMapDesc.GetDataPtr(), 0, mvFwdMapDesc.GetDataSize() );
        memset( mvFwdconfMapDesc.GetDataPtr(), 0, mvFwdconfMapDesc.GetDataSize() );
    }

    if ( ( config.motionDirection == MOTION_DIRECTION_BACKWARD ) or
         ( config.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
    {

        mvBwdMapProp = { QC_TENSOR_TYPE_UINT_16,
                         { 1, ALIGN_S( config.height, 8 ), ALIGN_S( config.width * 2, 128 ), 1 } };

        ret = bufMgr.Allocate( mvBwdMapProp, mvBwdMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        mvBwdConfProp = { QC_TENSOR_TYPE_UINT_8,
                          { 1, ALIGN_S( config.height, 8 ), ALIGN_S( config.width, 128 ), 1 } };


        ret = bufMgr.Allocate( mvBwdConfProp, mvBwdconfMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        memset( mvBwdMapDesc.GetDataPtr(), 0, mvBwdMapDesc.GetDataSize() );
        memset( mvBwdconfMapDesc.GetDataPtr(), 0, mvBwdconfMapDesc.GetDataSize() );
    }

    printf( "-- Test for %s\n", name.c_str() );

    DataTree dt;
    DataTree top_dt;
    dt.Set<uint32_t>( "width", config.width );
    dt.Set<uint32_t>( "height", config.height );
    dt.Set<uint32_t>( "fps", config.frameRate );
    dt.Set<bool>( "confidenceOutputEn", config.confidenceOutputEn );
    dt.Set<uint8_t>( "computationAccuracy", config.computationAccuracy );
    dt.Set<uint32_t>( "edgeAlignMetric", config.edgeAlignMetric );
    dt.Set<float32_t>( "imageSharpnessThreshold", config.imageSharpnessThreshold );
    dt.Set<float32_t>( "textureThreshold", config.textureThreshold );
    dt.Set<bool>( "isFirstRequest", config.isFirstRequest );
    dt.SetImageFormat( "format", config.imageFormat );
    dt.Set<uint8_t>( "motionDirection", config.motionDirection );

    top_dt.Set( "static", dt );
    top_dt.Set<std::string>( "static.name", "SANITY" );

    QCNodeInit_t configuration = { top_dt.Dump() };
    printf( "configuration.config: %s\n", configuration.config.c_str() );
    status = node->Initialize( configuration );
    ASSERT_EQ( QC_STATUS_OK, status );
    nodeInitialized = true;


    frameDescriptor = new NodeFrameDescriptor( QC_NODE_OF_LAST_BUFF_ID );

    ret = frameDescriptor->SetBuffer( QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID, refImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = frameDescriptor->SetBuffer( QC_NODE_OF_CURRENT_IMAGE_BUFF_ID, curImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    if ( ( config.motionDirection == MOTION_DIRECTION_FORWARD ) or
         ( config.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
    {
        ret = frameDescriptor->SetBuffer( QC_NODE_OF_FWD_MOTION_BUFF_ID, mvFwdMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = frameDescriptor->SetBuffer( QC_NODE_OF_FWD_CONF_BUFF_ID, mvFwdconfMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    if ( ( config.motionDirection == MOTION_DIRECTION_BACKWARD ) or
         ( config.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
    {
        ret = frameDescriptor->SetBuffer( QC_NODE_OF_BWD_MOTION_BUFF_ID, mvBwdMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = frameDescriptor->SetBuffer( QC_NODE_OF_BWD_CONF_BUFF_ID, mvBwdconfMapDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }


    status = node->Start();
    ASSERT_EQ( QC_STATUS_OK, status );
    nodeStarted = true;

    status = node->ProcessFrameDescriptor( *frameDescriptor );
    ASSERT_EQ( QC_STATUS_OK, status );

    if ( ( config.motionDirection == MOTION_DIRECTION_FORWARD ) or
         ( config.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
    {
        if ( ( goldenFwdMvMap.empty() == false ) and ( goldenFwdMvConf.empty() == false ) )
        {
// for the first run, execute the below to generate the golden output
#if defined( __QNXNTO__ )
            // SaveRaw( goldenFwdMvMap, mvFwdMapDesc.GetDataPtr(), mvFwdMapDesc.GetDataSize() );
            // SaveRaw( goldenFwdMvConf, mvFwdconfMapDesc.GetDataPtr(),
            //          mvFwdconfMapDesc.GetDataSize() );
#endif

            ret = bufMgr.Allocate( mvFwdMapProp, mvFwdMapDescG );
            ASSERT_EQ( QC_STATUS_OK, ret );

            ret = bufMgr.Allocate( mvFwdConfProp, mvFwdconfMapDescG );
            ASSERT_EQ( QC_STATUS_OK, ret );

            memset( mvFwdMapDescG.GetDataPtr(), 0, mvFwdMapDescG.GetDataSize() );
            memset( mvFwdconfMapDescG.GetDataPtr(), 0, mvFwdconfMapDescG.GetDataSize() );

            auto file_size = [&]( const std::string &path ) -> long long {
                struct stat st;
                if ( stat( path.c_str(), &st ) != 0 )
                {
                    printf( "stat failed for %s: errno=%d\n", path.c_str(), errno );
                    return -1;
                }
                return static_cast<long long>( st.st_size );
            };

            LoadRaw( mvFwdMapDescG.GetDataPtr(), mvFwdMapDescG.GetDataSize(), goldenFwdMvMap );
            LoadRaw( mvFwdconfMapDescG.GetDataPtr(), mvFwdconfMapDescG.GetDataSize(),
                     goldenFwdMvConf );

            std::string md5Output = MD5Sum( mvFwdMapDesc.GetDataPtr(), mvFwdMapDesc.GetDataSize() );
            std::string md5Golden =
                    MD5Sum( mvFwdMapDescG.GetDataPtr(), mvFwdMapDescG.GetDataSize() );
            ASSERT_EQ( md5Output, md5Golden );

            md5Output = MD5Sum( mvFwdconfMapDesc.GetDataPtr(), mvFwdconfMapDesc.GetDataSize() );
            md5Golden = MD5Sum( mvFwdconfMapDescG.GetDataPtr(), mvFwdconfMapDescG.GetDataSize() );
            ASSERT_EQ( md5Output, md5Golden );
        }
    }

    if ( ( config.motionDirection == MOTION_DIRECTION_BACKWARD ) or
         ( config.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
    {
        if ( ( goldenBwdMvMap.empty() == false ) and ( goldenBwdMvConf.empty() == false ) )
        {
// for the first run, execute the below to generate the golden output
#if defined( __QNXNTO__ )
            // SaveRaw( goldenBwdMvMap, mvBwdMapDesc.GetDataPtr(), mvBwdMapDesc.GetDataSize() );
            // SaveRaw( goldenBwdMvConf, mvBwdconfMapDesc.GetDataPtr(),
            //          mvBwdconfMapDesc.GetDataSize() );
#endif

            ret = bufMgr.Allocate( mvBwdMapProp, mvBwdMapDescG );
            ASSERT_EQ( QC_STATUS_OK, ret );

            ret = bufMgr.Allocate( mvBwdConfProp, mvBwdconfMapDescG );
            ASSERT_EQ( QC_STATUS_OK, ret );

            memset( mvBwdMapDescG.GetDataPtr(), 0, mvBwdMapDescG.GetDataSize() );
            memset( mvBwdconfMapDescG.GetDataPtr(), 0, mvBwdconfMapDescG.GetDataSize() );

            LoadRaw( mvBwdMapDescG.GetDataPtr(), mvBwdMapDescG.GetDataSize(), goldenBwdMvMap );
            LoadRaw( mvBwdconfMapDescG.GetDataPtr(), mvBwdconfMapDescG.GetDataSize(),
                     goldenBwdMvConf );

            std::string md5Output = MD5Sum( mvBwdMapDesc.GetDataPtr(), mvBwdMapDesc.GetDataSize() );
            std::string md5Golden =
                    MD5Sum( mvBwdMapDescG.GetDataPtr(), mvBwdMapDescG.GetDataSize() );
            ASSERT_EQ( md5Output, md5Golden );

            md5Output = MD5Sum( mvBwdconfMapDesc.GetDataPtr(), mvBwdconfMapDesc.GetDataSize() );
            md5Golden = MD5Sum( mvBwdconfMapDescG.GetDataPtr(), mvBwdconfMapDescG.GetDataSize() );
            ASSERT_EQ( md5Output, md5Golden );
        }
    }


    // Cleanup will be handled by scope guard, but we still verify operations succeed
    status = node->Stop();
    EXPECT_EQ( QC_STATUS_OK, status );
    nodeStarted = false;   // Mark as stopped so scope guard doesn't try again

    status = node->DeInitialize();
    EXPECT_EQ( QC_STATUS_OK, status );
    nodeInitialized = false;   // Mark as deinitialized so scope guard doesn't try again

    // Note: Buffer cleanup and frameDescriptor deletion will be handled by scope guard
}


TEST( EVA, L0_NV12_FWD_NodeOpticalFlow )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12;
    config.confidenceOutputEn = true;
    config.computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    config.edgeAlignMetric = 0;
    config.imageSharpnessThreshold = 0.0f;
    config.textureThreshold = 0.5f;
    config.isFirstRequest = true;
    config.motionDirection = MOTION_DIRECTION_FORWARD;
    Eva_NodeOpticalFlowRun( "OFL0_FWD_NV12", config, "data/test/ofl/0.nv12", "data/test/ofl/1.nv12",
                            "data/test/ofl/fwd_mv-map_nv12.raw",
                            "data/test/ofl/fwd_mv-conf_nv12.raw" );
}

TEST( EVA, L0_NV12_UBWC_FWD_NodeOpticalFlow )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12_UBWC;
    config.confidenceOutputEn = true;
    config.computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    config.edgeAlignMetric = 0;
    config.imageSharpnessThreshold = 0.0f;
    config.textureThreshold = 0.5f;
    config.isFirstRequest = true;
    config.motionDirection = MOTION_DIRECTION_FORWARD;
    Eva_NodeOpticalFlowRun( "OFL0_FWD_NV12_UBWC", config, "data/test/ofl/0.nv12_ubwc",
                            "data/test/ofl/1.nv12_ubwc", "data/test/ofl/fwd_mv-map_nv12_ubwc.raw",
                            "data/test/ofl/fwd_mv-conf_nv12_ubwc.raw" );
}

TEST( EVA, L0_NV12_BWD_NodeOpticalFlow )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12;
    config.confidenceOutputEn = true;
    config.computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    config.edgeAlignMetric = 0;
    config.imageSharpnessThreshold = 0.0f;
    config.textureThreshold = 0.5f;
    config.isFirstRequest = true;
    config.motionDirection = MOTION_DIRECTION_BACKWARD;
    Eva_NodeOpticalFlowRun( "OFL0_BWD_NV12", config, "data/test/ofl/0.nv12", "data/test/ofl/1.nv12",
                            "", "", "data/test/ofl/bwd_mv-map_nv12.raw",
                            "data/test/ofl/bwd_mv-conf_nv12.raw" );
}

TEST( EVA, L0_NV12_UBWC_BWD_NodeOpticalFlow )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12_UBWC;
    config.confidenceOutputEn = true;
    config.computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    config.edgeAlignMetric = 0;
    config.imageSharpnessThreshold = 0.0f;
    config.textureThreshold = 0.5f;
    config.isFirstRequest = true;
    config.motionDirection = MOTION_DIRECTION_BACKWARD;
    Eva_NodeOpticalFlowRun( "OFL0_BWD_NV12_UBWC", config, "data/test/ofl/0.nv12_ubwc",
                            "data/test/ofl/1.nv12_ubwc", "", "",
                            "data/test/ofl/bwd_mv-map_nv12_ubwc.raw",
                            "data/test/ofl/bwd_mv-conf_nv12_ubwc.raw" );
}

TEST( EVA, L0_NV12_BID_NodeOpticalFlow )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12;
    config.confidenceOutputEn = true;
    config.computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    config.edgeAlignMetric = 0;
    config.imageSharpnessThreshold = 0.0f;
    config.textureThreshold = 0.5f;
    config.isFirstRequest = true;
    config.motionDirection = MOTION_DIRECTION_BIDIRECTIONAL;
    Eva_NodeOpticalFlowRun(
            "OFL0_BID_NV12", config, "data/test/ofl/0.nv12", "data/test/ofl/1.nv12",
            "data/test/ofl/bid_fwd_mv-map_nv12.raw", "data/test/ofl/bid_fwd_mv-conf_nv12.raw",
            "data/test/ofl/bid_bwd_mv-map_nv12.raw", "data/test/ofl/bid_bwd_mv-conf_nv12.raw" );
}

TEST( EVA, L0_NV12_UBWC_BID_NodeOpticalFlow )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12_UBWC;
    config.confidenceOutputEn = true;
    config.computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    config.edgeAlignMetric = 0;
    config.imageSharpnessThreshold = 0.0f;
    config.textureThreshold = 0.5f;
    config.isFirstRequest = true;
    config.motionDirection = MOTION_DIRECTION_BIDIRECTIONAL;
    Eva_NodeOpticalFlowRun( "OFL0_BID_NV12_UBWC", config, "data/test/ofl/0.nv12_ubwc",
                            "data/test/ofl/1.nv12_ubwc",
                            "data/test/ofl/bid_fwd_mv-map_nv12_ubwc.raw",
                            "data/test/ofl/bid_fwd_mv-conf_nv12_ubwc.raw",
                            "data/test/ofl/bid_bwd_mv-map_nv12_ubwc.raw",
                            "data/test/ofl/bid_bwd_mv-conf_nv12_ubwc.raw" );
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
