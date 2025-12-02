
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "md5_utils.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

#include "QC/Node/DepthFromStereo.hpp"
#include "QC/sample/BufferManager.hpp"

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

using namespace QC::Node;
using namespace QC::test::utils;
using namespace QC;
using namespace QC::Memory;
using namespace QC::sample;

#define ALIGN_S( size, align ) ( ( size + align - 1 ) / align ) * align

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


static void Eva_NodeDepthFromStereoRun( std::string name, DepthFromStereo_Config_t &config,
                                        std::string img1 = "", std::string img2 = "",
                                        std::string goldenDispMap = "",
                                        std::string goldenConfMap = "" )
{
    DepthFromStereo dfs;
    QCNodeIfs *node = dynamic_cast<QCNodeIfs *>( &dfs );
    QCStatus_e ret;

    BufferManager bufMgr( { "DFS", QC_NODE_TYPE_EVA_DFS, 0 } );

    ImageDescriptor_t priImgDesc;
    ImageBasicProps_t priImgProp;
    ImageDescriptor_t auxImgDesc;
    ImageBasicProps_t auxImgProp;
    TensorDescriptor_t dispMapDesc;
    TensorDescriptor_t confMapDesc;
    TensorDescriptor_t dispMapDescG;
    TensorDescriptor_t confMapDescG;
    NodeFrameDescriptor *frameDescriptor = nullptr;
    bool nodeInitialized = false;
    bool nodeStarted = false;

    // Setup cleanup scope guard to ensure resources are freed even if assertions fail
    SCOPE_EXIT(
            if ( nodeStarted ) { node->Stop(); } if ( nodeInitialized ) {
                node->DeInitialize();
            } bufMgr.Free( priImgDesc );
            bufMgr.Free( auxImgDesc ); bufMgr.Free( dispMapDesc ); bufMgr.Free( confMapDesc );
            bufMgr.Free( dispMapDescG ); bufMgr.Free( confMapDescG );
            if ( frameDescriptor != nullptr ) { delete frameDescriptor; } );

    priImgProp.format = config.imageFormat;
    priImgProp.batchSize = 1;
    priImgProp.width = config.width;
    priImgProp.height = config.height;

    ret = bufMgr.Allocate( priImgProp, priImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    auxImgProp.format = config.imageFormat;
    auxImgProp.batchSize = 1;
    auxImgProp.width = config.width;
    auxImgProp.height = config.height;

    ret = bufMgr.Allocate( auxImgProp, auxImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( img1.empty() == false )
    {
        LoadRaw( priImgDesc.GetDataPtr(), priImgDesc.GetDataSize(), img1 );
    }

    if ( img2.empty() == false )
    {
        LoadRaw( auxImgDesc.GetDataPtr(), auxImgDesc.GetDataSize(), img2 );
    }

    if ( ( goldenDispMap.empty() == false ) and ( goldenConfMap.empty() == false ) )
    { /* clear output buffer for accuracy test */
        memset( dispMapDesc.pBuf, 0, dispMapDesc.GetDataSize() );
        memset( confMapDesc.pBuf, 0, confMapDesc.GetDataSize() );
    }

    TensorProps_t dispMapProp = {
            QC_TENSOR_TYPE_UINT_16,
            { 1, ALIGN_S( config.height, 2 ), ALIGN_S( config.width, 128 ), 1 } };

    ret = bufMgr.Allocate( dispMapProp, dispMapDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    TensorProps_t confMapProp = {
            QC_TENSOR_TYPE_UINT_8,
            { 1, ALIGN_S( config.height, 2 ), ALIGN_S( config.width, 128 ), 1 } };
    ret = bufMgr.Allocate( confMapProp, confMapDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    printf( "-- Test for %s\n", name.c_str() );

    DataTree dt;
    DataTree top_dt;
    dt.Set<uint32_t>( "width", config.width );
    dt.Set<uint32_t>( "height", config.height );
    dt.Set<uint32_t>( "fps", config.frameRate );
    dt.Set<bool>( "confidenceOutputEn", config.confidenceOutputEn );
    dt.SetImageFormat( "format", config.imageFormat );
    dt.Set<uint8_t>( "processingMode", config.processingMode );
    dt.Set<uint8_t>( "searchDirection", config.searchDirection );
    top_dt.Set( "static", dt );
    top_dt.Set<std::string>( "static.name", "SANITY" );

    QCNodeInit_t configuration = { top_dt.Dump() };
    printf( "configuration.config: %s\n", configuration.config.c_str() );

    ret = node->Initialize( configuration );
    ASSERT_EQ( QC_STATUS_OK, ret );
    nodeInitialized = true;

    frameDescriptor = new NodeFrameDescriptor( QC_NODE_DFS_LAST_BUFF_ID );

    ret = frameDescriptor->SetBuffer( QC_NODE_DFS_PRIMARY_IMAGE_BUFF_ID, priImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = frameDescriptor->SetBuffer( QC_NODE_DFS_AUXILARY_IMAGE_BUFF_ID, auxImgDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = frameDescriptor->SetBuffer( QC_NODE_DFS_DISPARITY_MAP_BUFF_ID, dispMapDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = frameDescriptor->SetBuffer( QC_NODE_DFS_DISPARITY_CONFIDANCE_MAP_BUFF_ID, confMapDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = node->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    nodeStarted = true;

    ret = node->ProcessFrameDescriptor( *frameDescriptor );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( ( goldenDispMap.empty() == false ) and ( goldenConfMap.empty() == false ) )
    {

#if defined( __QNXNTO__ )
        // for the first run, execute the below to generate the golden output
        // SaveRaw( goldenDispMap, dispMapDesc.GetDataPtr(), dispMapDesc.GetDataSize() );
        // SaveRaw( goldenConfMap, confMapDesc.GetDataPtr(), confMapDesc.GetDataSize() );
#endif

        ret = bufMgr.Allocate( dispMapProp, dispMapDescG );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = bufMgr.Allocate( confMapProp, confMapDescG );
        ASSERT_EQ( QC_STATUS_OK, ret );

        LoadRaw( dispMapDescG.GetDataPtr(), dispMapDescG.GetDataSize(), goldenDispMap );
        LoadRaw( confMapDescG.GetDataPtr(), confMapDescG.GetDataSize(), goldenConfMap );

        std::string md5Output = MD5Sum( dispMapDesc.GetDataPtr(), dispMapDesc.GetDataSize() );
        std::string md5Golden = MD5Sum( dispMapDescG.GetDataPtr(), dispMapDescG.GetDataSize() );
        ASSERT_EQ( md5Output, md5Golden );

        md5Output = MD5Sum( confMapDesc.GetDataPtr(), confMapDesc.GetDataSize() );
        md5Golden = MD5Sum( confMapDescG.GetDataPtr(), confMapDescG.GetDataSize() );
        ASSERT_EQ( md5Output, md5Golden );
    }

    // Cleanup will be handled by scope guard
}


TEST( EVA, L0_SANITY_NV12_L2R_NodeDepthFromStereo )
{
    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 416;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12;
    config.confidenceOutputEn = true;
    config.searchDirection = SEARCH_DIRECTION_L2R;
    Eva_NodeDepthFromStereoRun( "DFS_NV12_L2R", config, "data/test/dfs/5_left.nv12",
                                "data/test/dfs/5_right.nv12", "data/test/dfs/5_disp_l2r_nv12.raw",
                                "data/test/dfs/5_conf_l2r_nv12.raw" );
}

TEST( EVA, L0_SANITY_NV12_UBWC_L2R_NodeDepthFromStereo )
{
    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 416;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12_UBWC;
    config.confidenceOutputEn = true;
    config.searchDirection = SEARCH_DIRECTION_L2R;
    Eva_NodeDepthFromStereoRun( "DFS_NV12_UBWC_L2R", config, "data/test/dfs/5_left.nv12_ubwc",
                                "data/test/dfs/5_right.nv12_ubwc",
                                "data/test/dfs/5_disp_ubwc_l2r_nv12.raw",
                                "data/test/dfs/5_conf_ubwc_l2r_nv12.raw" );
}

TEST( EVA, L0_SANITY_NV12_R2L_NodeDepthFromStereo )
{
    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 416;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12;
    config.confidenceOutputEn = true;
    config.searchDirection = SEARCH_DIRECTION_R2L;
    Eva_NodeDepthFromStereoRun( "DFS_NV12_R2L", config, "data/test/dfs/5_right.nv12",
                                "data/test/dfs/5_left.nv12", "data/test/dfs/5_disp_r2l_nv12.raw",
                                "data/test/dfs/5_conf_r2l_nv12.raw" );
}

TEST( EVA, L0_SANITY_NV12_UBWC_R2L_NodeDepthFromStereo )
{
    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 416;
    config.frameRate = 30;
    config.imageFormat = QC_IMAGE_FORMAT_NV12_UBWC;
    config.confidenceOutputEn = true;
    config.searchDirection = SEARCH_DIRECTION_R2L;
    Eva_NodeDepthFromStereoRun( "DFS_NV12_UBWC_R2L", config, "data/test/dfs/5_right.nv12_ubwc",
                                "data/test/dfs/5_left.nv12_ubwc",
                                "data/test/dfs/5_disp_ubwc_r2l_nv12.raw",
                                "data/test/dfs/5_conf_ubwc_r2l_nv12.raw" );
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
