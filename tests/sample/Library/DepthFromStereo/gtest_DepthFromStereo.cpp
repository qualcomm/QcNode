// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "DepthFromStereo.hpp"
#include "QC/sample/BufferManager.hpp"
#include "md5_utils.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace QC;
using namespace QC::sample;
using namespace QC::Memory;
using namespace QC::test::utils;

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

static void Eva_DepthFromStereoRun( std::string name, DepthFromStereo_Config_t &config,
                                    std::string img1 = "", std::string img2 = "",
                                    std::string goldenDispMap = "", std::string goldenConfMap = "" )
{
    BufferManager bufMgr( { name, QC_NODE_TYPE_EVA_DFS, 0 } );
    Logger logger;
    logger.Init( name.c_str(), LOGGER_LEVEL_ERROR );
    DepthFromStereo dfs( logger );
    QCStatus_e ret;
    ImageDescriptor_t priImg;
    ImageDescriptor_t auxImg;
    TensorDescriptor_t dispMap;
    TensorDescriptor_t confMap;
    TensorDescriptor_t dispMapG;
    TensorDescriptor_t confMapG;

    uint32_t width = config.width;
    uint32_t height = config.height;

    TensorProps_t dispMapTsProp( QC_TENSOR_TYPE_UINT_16,
                                 { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 } );
    TensorProps_t confMapTsProp( QC_TENSOR_TYPE_UINT_8,
                                 { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 } );

    printf( "-- Test for %s\n", name.c_str() );

    ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                           priImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                           auxImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Allocate( dispMapTsProp, dispMap );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Allocate( confMapTsProp, confMap );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( false == img1.empty() )
    {
        LoadRaw( priImg.GetDataPtr(), priImg.size, img1 );
    }

    if ( false == img2.empty() )
    {
        LoadRaw( auxImg.GetDataPtr(), auxImg.size, img2 );
    }

    if ( ( false == goldenDispMap.empty() ) && ( false == goldenConfMap.empty() ) )
    { /* clear output buffer for accuracy test */
        memset( dispMap.GetDataPtr(), 0, dispMap.size );
        memset( confMap.GetDataPtr(), 0, confMap.size );
    }

    ret = dfs.Init( name.c_str(), &config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = dfs.RegisterBuffers( &priImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dfs.RegisterBuffers( &auxImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dfs.RegisterBuffers( &dispMap, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dfs.RegisterBuffers( &confMap, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = dfs.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = dfs.Execute( &priImg, &auxImg, &dispMap, &confMap );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( ( false == goldenDispMap.empty() ) && ( false == goldenConfMap.empty() ) )
    {
#if 0
        // for the first run, with below to generate the golden
        SaveRaw( goldenDispMap, dispMap.GetDataPtr(), dispMap.size );
        SaveRaw( goldenConfMap, confMap.GetDataPtr(), confMap.size );
#endif
        ret = bufMgr.Allocate( dispMapTsProp, dispMapG );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( confMapTsProp, confMapG );
        ASSERT_EQ( QC_STATUS_OK, ret );

        LoadRaw( dispMapG.GetDataPtr(), dispMapG.size, goldenDispMap );
        LoadRaw( confMapG.GetDataPtr(), confMapG.size, goldenConfMap );

        std::string md5Output = MD5Sum( dispMap.GetDataPtr(), dispMap.size );
        std::string md5Golden = MD5Sum( dispMapG.GetDataPtr(), dispMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        md5Output = MD5Sum( confMap.GetDataPtr(), confMap.size );
        md5Golden = MD5Sum( confMapG.GetDataPtr(), confMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        bufMgr.Free( dispMapG );
        bufMgr.Free( confMapG );
    }

    ret = dfs.Execute( &auxImg, &priImg, &dispMap, &confMap );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = dfs.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = dfs.DeRegisterBuffers( &priImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dfs.DeRegisterBuffers( &auxImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dfs.DeRegisterBuffers( &dispMap, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dfs.DeRegisterBuffers( &confMap, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = dfs.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = bufMgr.Free( priImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Free( auxImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Free( dispMap );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Free( confMap );
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( EVA, SANITY_DepthFromStereo )
{
    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 416;
    Eva_DepthFromStereoRun( "DFS0", config, "data/test/dfs/5_left.nv12",
                            "data/test/dfs/5_right.nv12", "data/test/dfs/5_disp.raw",
                            "data/test/dfs/5_conf.raw" );
}

TEST( EVA, L2_DepthFromStereoFormat )
{
    {
        DepthFromStereo_Config_t config;
        config.width = 1280;
        config.height = 720;
        config.format = QC_IMAGE_FORMAT_P010;
        Eva_DepthFromStereoRun( "DFS0_P010", config );
    }

    {
        DepthFromStereo_Config_t config;
        config.width = 1280;
        config.height = 720;
        config.format = QC_IMAGE_FORMAT_NV12_UBWC;
        Eva_DepthFromStereoRun( "DFS0_NV12_UBWC", config );
    }


    {
        DepthFromStereo_Config_t config;
        config.width = 1280;
        config.height = 720;
        config.format = QC_IMAGE_FORMAT_TP10_UBWC;
        Eva_DepthFromStereoRun( "DFS0_TP10_UBWC", config );
    }
}

TEST( EVA, L2_DepthFromStereoR2L )
{
    {
        DepthFromStereo_Config_t config;
        config.width = 1280;
        config.height = 720;
        config.dfsSearchDir = EVA_DFS_SEARCH_R2L;
        Eva_DepthFromStereoRun( "DFS0-R2L", config );
    }
}

TEST( EVA, L2_DepthFromStereoError )
{
    BufferManager bufMgr( { "DFS", QC_NODE_TYPE_EVA_DFS, 0 } );
    Logger logger;
    logger.Init( "DFS", LOGGER_LEVEL_ERROR );
    QCStatus_e ret;
    {
        DepthFromStereo dfs( logger );
        DepthFromStereo_Config_t configDft;
        configDft.width = 1280;
        configDft.height = 720;
        DepthFromStereo_Config_t config = configDft;

        ret = dfs.Init( "DFS0", nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = configDft;
        config.format = QC_IMAGE_FORMAT_UYVY;
        ret = dfs.Init( "DFS0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = configDft;
        config.dfsSearchDir = EVA_DFS_SEARCH_MAX;
        ret = dfs.Init( "DFS0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = configDft; /* super large image */
        config.width = 409600;
        config.height = 409600;
        ret = dfs.Init( "DFS0", &config );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = dfs.Deinit();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        config = configDft;
        ImageDescriptor_t priImg;
        ImageDescriptor_t auxImg;
        TensorDescriptor_t dispMap;
        TensorDescriptor_t confMap;

        TensorProps_t dispMapTsProp( QC_TENSOR_TYPE_UINT_16, { 1, ALIGN_S( config.height, 2 ),
                                                               ALIGN_S( config.width, 128 ), 1 } );
        TensorProps_t confMapTsProp( QC_TENSOR_TYPE_UINT_8, { 1, ALIGN_S( config.height, 2 ),
                                                              ALIGN_S( config.width, 128 ), 1 } );

        ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                               priImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                               auxImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( dispMapTsProp, dispMap );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( confMapTsProp, confMap );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = dfs.RegisterBuffers( &priImg, 1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = dfs.DeRegisterBuffers( &priImg, 1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = dfs.Start();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = dfs.Stop();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = dfs.Init( "DFS0", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = dfs.RegisterBuffers( nullptr, 1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = dfs.RegisterBuffers( &priImg, 0 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ImageDescriptor_t invalidImg = priImg;
        invalidImg.dmaHandle = 0xdeadbeef;
        ret = dfs.RegisterBuffers( &invalidImg, 1 );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = dfs.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = dfs.RegisterBuffers( &auxImg, 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = dfs.Execute( &priImg, &auxImg, &dispMap, nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = dfs.Execute( &priImg, &auxImg, nullptr, &confMap );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = dfs.Execute( &priImg, nullptr, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = dfs.Execute( nullptr, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        TensorDescriptor_t invalidDispMap = dispMap;
        invalidDispMap.type = QC_BUFFER_TYPE_IMAGE;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.pBuf = nullptr;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.numDims = 8;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.tensorType = QC_TENSOR_TYPE_INT_16;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.dims[0] = 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.dims[1] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.dims[2] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.dims[3] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t invalidConfMap = confMap;
        invalidConfMap.type = QC_BUFFER_TYPE_IMAGE;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.pBuf = nullptr;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.numDims = 8;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.tensorType = QC_TENSOR_TYPE_INT_16;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.dims[0] = 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.dims[1] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.dims[2] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.dims[3] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.type = QC_BUFFER_TYPE_TENSOR;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.pBuf = nullptr;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.format = QC_IMAGE_FORMAT_RGB888;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.width += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.height += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.numPlanes += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.stride[0] += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.planeBufSize[0] += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = dfs.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = dfs.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = bufMgr.Free( priImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( auxImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( dispMap );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( confMap );
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
