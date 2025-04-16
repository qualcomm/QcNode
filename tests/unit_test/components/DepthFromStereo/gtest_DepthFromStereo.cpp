// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/component/DepthFromStereo.hpp"
#include "md5_utils.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace QC;
using namespace QC::component;
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
    DepthFromStereo dfs;
    QCStatus_e ret;
    QCSharedBuffer_t priImg;
    QCSharedBuffer_t auxImg;
    QCSharedBuffer_t dispMap;
    QCSharedBuffer_t confMap;
    QCSharedBuffer_t dispMapG;
    QCSharedBuffer_t confMapG;

    uint32_t width = config.width;
    uint32_t height = config.height;

    QCTensorProps_t dispMapTsProp = { QC_TENSOR_TYPE_UINT_16,
                                      { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                      4 };
    QCTensorProps_t confMapTsProp = { QC_TENSOR_TYPE_UINT_8,
                                      { 1, ALIGN_S( height, 2 ), ALIGN_S( width, 128 ), 1 },
                                      4 };


    printf( "-- Test for %s\n", name.c_str() );

    ret = priImg.Allocate( config.width, config.height, config.format );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = auxImg.Allocate( config.width, config.height, config.format );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dispMap.Allocate( &dispMapTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = confMap.Allocate( &confMapTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( false == img1.empty() )
    {
        LoadRaw( priImg.data(), priImg.size, img1 );
    }

    if ( false == img2.empty() )
    {
        LoadRaw( auxImg.data(), auxImg.size, img2 );
    }

    if ( ( false == goldenDispMap.empty() ) && ( false == goldenConfMap.empty() ) )
    { /* clear output buffer for accuracy test */
        memset( dispMap.data(), 0, dispMap.size );
        memset( confMap.data(), 0, confMap.size );
    }

    ret = dfs.Init( name.c_str(), &config, LOGGER_LEVEL_VERBOSE );
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
        SaveRaw( goldenDispMap, dispMap.data(), dispMap.size );
        SaveRaw( goldenConfMap, confMap.data(), confMap.size );
#endif
        ret = dispMapG.Allocate( &dispMapTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = confMapG.Allocate( &confMapTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        LoadRaw( dispMapG.data(), dispMapG.size, goldenDispMap );
        LoadRaw( confMapG.data(), confMapG.size, goldenConfMap );

        std::string md5Output = MD5Sum( dispMap.data(), dispMap.size );
        std::string md5Golden = MD5Sum( dispMapG.data(), dispMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        md5Output = MD5Sum( confMap.data(), confMap.size );
        md5Golden = MD5Sum( confMapG.data(), confMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        dispMapG.Free();
        confMapG.Free();
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

    ret = priImg.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = auxImg.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = dispMap.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = confMap.Free();
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
    QCStatus_e ret;
    {
        DepthFromStereo dfs;
        DepthFromStereo_Config_t configDft;
        configDft.width = 1280;
        configDft.height = 720;
        DepthFromStereo_Config_t config = configDft;

        ret = dfs.Init( nullptr, &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

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
        QCSharedBuffer_t priImg;
        QCSharedBuffer_t auxImg;
        QCSharedBuffer_t dispMap;
        QCSharedBuffer_t confMap;

        QCTensorProps_t dispMapTsProp = {
                QC_TENSOR_TYPE_UINT_16,
                { 1, ALIGN_S( config.height, 2 ), ALIGN_S( config.width, 128 ), 1 },
                4 };
        QCTensorProps_t confMapTsProp = {
                QC_TENSOR_TYPE_UINT_8,
                { 1, ALIGN_S( config.height, 2 ), ALIGN_S( config.width, 128 ), 1 },
                4 };
        ret = priImg.Allocate( config.width, config.height, config.format );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = auxImg.Allocate( config.width, config.height, config.format );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = dispMap.Allocate( &dispMapTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = confMap.Allocate( &confMapTsProp );
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

        QCSharedBuffer_t invalidImg = priImg;
        invalidImg.buffer.dmaHandle = 0xdeadbeef;
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

        QCSharedBuffer_t invalidDispMap = dispMap;
        invalidDispMap.type = QC_BUFFER_TYPE_IMAGE;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.buffer.pData = nullptr;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.tensorProps.numDims = 8;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.tensorProps.type = QC_TENSOR_TYPE_INT_16;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.tensorProps.dims[0] = 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.tensorProps.dims[1] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.tensorProps.dims[2] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidDispMap = dispMap;
        invalidDispMap.tensorProps.dims[3] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &invalidDispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t invalidConfMap = confMap;
        invalidConfMap.type = QC_BUFFER_TYPE_IMAGE;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.buffer.pData = nullptr;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.tensorProps.numDims = 8;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.tensorProps.type = QC_TENSOR_TYPE_INT_16;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.tensorProps.dims[0] = 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.tensorProps.dims[1] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.tensorProps.dims[2] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidConfMap = confMap;
        invalidConfMap.tensorProps.dims[3] += 2;
        ret = dfs.Execute( &priImg, &auxImg, &dispMap, &invalidConfMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.type = QC_BUFFER_TYPE_TENSOR;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.buffer.pData = nullptr;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.imgProps.format = QC_IMAGE_FORMAT_RGB888;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.imgProps.width += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.imgProps.height += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.imgProps.numPlanes += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.imgProps.stride[0] += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = priImg;
        invalidImg.imgProps.planeBufSize[0] += 1;
        ret = dfs.Execute( &invalidImg, &auxImg, &dispMap, &confMap );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = dfs.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = dfs.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = priImg.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = auxImg.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = dispMap.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = confMap.Free();
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
