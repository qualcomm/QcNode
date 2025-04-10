// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "md5_utils.hpp"
#include "ridehal/component/OpticalFlow.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::test::utils;

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

static void Eva_OpticalFlowRun( std::string name, OpticalFlow_Config_t &config,
                                std::string img1 = "", std::string img2 = "",
                                std::string goldenMvMap = "", std::string goldenMvConf = "" )
{
    OpticalFlow ofl;
    RideHalError_e ret;
    RideHal_SharedBuffer_t refImg;
    RideHal_SharedBuffer_t curImg;
    RideHal_SharedBuffer_t mvFwdMap;
    RideHal_SharedBuffer_t mvConf;
    RideHal_SharedBuffer_t mvFwdMapG;
    RideHal_SharedBuffer_t mvConfG;

    uint32_t width = ( config.width >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;
    uint32_t height = ( config.height >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;

    RideHal_TensorProps_t mvFwdMapTsProp = {
            RIDEHAL_TENSOR_TYPE_UINT_16,
            { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 },
            4 };
    RideHal_TensorProps_t mvConfTsProp = { RIDEHAL_TENSOR_TYPE_UINT_8,
                                           { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 },
                                           4 };


    printf( "-- Test for %s\n", name.c_str() );

    ret = refImg.Allocate( config.width, config.height, config.format );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = curImg.Allocate( config.width, config.height, config.format );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvFwdMap.Allocate( &mvFwdMapTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvConf.Allocate( &mvConfTsProp );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( false == img1.empty() )
    {
        LoadRaw( refImg.data(), refImg.size, img1 );
    }

    if ( false == img2.empty() )
    {
        LoadRaw( curImg.data(), curImg.size, img2 );
    }

    if ( ( false == goldenMvMap.empty() ) && ( false == goldenMvConf.empty() ) )
    { /* clear output buffer for accuracy test */
        memset( mvFwdMap.data(), 0, mvFwdMap.size );
        memset( mvConf.data(), 0, mvConf.size );
    }

    ret = ofl.Init( name.c_str(), &config, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.RegisterBuffers( &refImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.RegisterBuffers( &curImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.RegisterBuffers( &mvFwdMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.RegisterBuffers( &mvConf, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &mvConf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( ( false == goldenMvMap.empty() ) && ( false == goldenMvConf.empty() ) )
    {
        // for the first run, with below to generate the golden
        // SaveRaw( goldenMvMap, mvFwdMap.data(), mvFwdMap.size );
        // SaveRaw( goldenMvConf, mvConf.data(), mvConf.size );

        ret = mvFwdMapG.Allocate( &mvFwdMapTsProp );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = mvConfG.Allocate( &mvConfTsProp );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        LoadRaw( mvFwdMapG.data(), mvFwdMapG.size, goldenMvMap );
        LoadRaw( mvConfG.data(), mvConfG.size, goldenMvConf );

        std::string md5Output = MD5Sum( mvFwdMap.data(), mvFwdMap.size );
        std::string md5Golden = MD5Sum( mvFwdMapG.data(), mvFwdMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        md5Output = MD5Sum( mvConf.data(), mvConf.size );
        md5Golden = MD5Sum( mvConfG.data(), mvConfG.size );
        ASSERT_EQ( md5Output, md5Golden );

        mvFwdMapG.Free();
        mvConfG.Free();
    }

    ret = ofl.Execute( &curImg, &refImg, &mvFwdMap, &mvConf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.DeRegisterBuffers( &refImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.DeRegisterBuffers( &curImg, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.DeRegisterBuffers( &mvFwdMap, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = ofl.DeRegisterBuffers( &mvConf, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = ofl.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = refImg.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = curImg.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvFwdMap.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    ret = mvConf.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}

TEST( EVA, SANITY_OpticalFlowCPU )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.filterOperationMode = EVA_OF_MODE_CPU;
    Eva_OpticalFlowRun( "OFL0_CPU", config, "data/test/ofl/0.nv12", "data/test/ofl/1.nv12",
                        "data/test/ofl/golden/semi-cpu-mv-map.raw",
                        "data/test/ofl/golden/semi-cpu-mv-conf.raw" );
}

TEST( EVA, SANITY_OpticalFlowDSP )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.filterOperationMode = EVA_OF_MODE_DSP;
    /* output the same as cpu */
    Eva_OpticalFlowRun( "OFL0_DSP", config, "data/test/ofl/0.nv12", "data/test/ofl/1.nv12",
                        "data/test/ofl/golden/semi-cpu-mv-map.raw",
                        "data/test/ofl/golden/semi-cpu-mv-conf.raw" );
}

TEST( EVA, SANITY_OpticalFlowNONE )
{
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.filterOperationMode = EVA_OF_MODE_DISABLE;
    Eva_OpticalFlowRun( "OFL0_NONE", config, "data/test/ofl/0.nv12", "data/test/ofl/1.nv12",
                        "data/test/ofl/golden/semi-none-mv-map.raw",
                        "data/test/ofl/golden/semi-none-mv-conf.raw" );
}

#if defined( __QNXNTO__ )
/* below case run separately on linux, no issues, but in the same gtest without filter, it
 * will fail, maybe some platform issue related, under debug. */
TEST( EVA, L2_OpticalFlowFormat )
{
    /* UYVY Only supported for EvaOFFilterOperationMode_e != EVA_OF_MODE_DISABLE */
    {
        OpticalFlow_Config_t config;
        config.width = 1920;
        config.height = 1024;
        config.format = RIDEHAL_IMAGE_FORMAT_UYVY;
        config.filterOperationMode = EVA_OF_MODE_CPU;
        Eva_OpticalFlowRun( "OFL0_CPU_UYVY", config );
    }
    {
        OpticalFlow_Config_t config;
        config.width = 1920;
        config.height = 1024;
        config.format = RIDEHAL_IMAGE_FORMAT_UYVY;
        config.filterOperationMode = EVA_OF_MODE_DSP;
        Eva_OpticalFlowRun( "OFL0_DSP_UYVY", config );
    }

    /* P010 Only supported for EVA_OF_MODE_DISABLE in EvaOFFilterOperationMode_e */
    {
        OpticalFlow_Config_t config;
        config.width = 1920;
        config.height = 1024;
        config.format = RIDEHAL_IMAGE_FORMAT_P010;
        config.filterOperationMode = EVA_OF_MODE_DISABLE;
        Eva_OpticalFlowRun( "OFL0_NONE_P010", config );
    }
}

TEST( EVA, L2_OpticalFlowBackward )
{
    {
        OpticalFlow_Config_t config;
        config.width = 1920;
        config.height = 1024;
        config.filterOperationMode = EVA_OF_MODE_CPU;
        config.direction = EVA_OF_BACKWARD_DIRECTION;
        Eva_OpticalFlowRun( "OFL0_CPU_BWK", config );
    }
    {
        OpticalFlow_Config_t config;
        config.width = 1920;
        config.height = 1024;
        config.filterOperationMode = EVA_OF_MODE_DSP;
        config.direction = EVA_OF_BACKWARD_DIRECTION;
        Eva_OpticalFlowRun( "OFL0_DSP_BWK", config );
    }
}

TEST( EVA, L2_OpticalFlowError )
{
    RideHalError_e ret;
    {
        OpticalFlow ofl;
        OpticalFlow_Config_t configDft;
        configDft.width = 1920;
        configDft.height = 1024;
        OpticalFlow_Config_t config = configDft;

        ret = ofl.Init( nullptr, &config );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = ofl.Init( "OFL0", nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        config = configDft;
        config.format = RIDEHAL_IMAGE_FORMAT_NV12_UBWC;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        config = configDft;
        config.filterOperationMode = EVA_OF_MODE_MAX;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        config = configDft;
        config.direction = EVA_OF_BIDIRECTIONAL;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        config = configDft; /* super large image */
        config.width = 409600;
        config.height = 409600;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        ret = ofl.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        config = configDft;
        RideHal_SharedBuffer_t refImg;
        RideHal_SharedBuffer_t curImg;
        RideHal_SharedBuffer_t mvFwdMap;
        RideHal_SharedBuffer_t mvConf;

        uint32_t width = ( config.width >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;
        uint32_t height = ( config.height >> config.amFilter.nStepSize )
                          << config.amFilter.nUpScale;

        RideHal_TensorProps_t mvFwdMapTsProp = {
                RIDEHAL_TENSOR_TYPE_UINT_16,
                { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 },
                4 };
        RideHal_TensorProps_t mvConfTsProp = {
                RIDEHAL_TENSOR_TYPE_UINT_8,
                { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 },
                4 };

        ret = refImg.Allocate( config.width, config.height, config.format );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = curImg.Allocate( config.width, config.height, config.format );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = mvFwdMap.Allocate( &mvFwdMapTsProp );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = mvConf.Allocate( &mvConfTsProp );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = ofl.RegisterBuffers( &refImg, 1 );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = ofl.DeRegisterBuffers( &refImg, 1 );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = ofl.Start();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = ofl.Stop();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = ofl.RegisterBuffers( nullptr, 1 );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = ofl.RegisterBuffers( &refImg, 0 );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        RideHal_SharedBuffer_t invalidImg = refImg;
        invalidImg.buffer.dmaHandle = 0xdeadbeef;
        ret = ofl.RegisterBuffers( &invalidImg, 1 );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        ret = ofl.Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = ofl.RegisterBuffers( &curImg, 1 );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = ofl.Execute( &refImg, &curImg, nullptr, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = ofl.Execute( &refImg, nullptr, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = ofl.Execute( nullptr, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        RideHal_SharedBuffer_t invalidFwdMap = mvFwdMap;
        invalidFwdMap.type = RIDEHAL_BUFFER_TYPE_IMAGE;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.buffer.pData = nullptr;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.tensorProps.numDims = 8;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.tensorProps.type = RIDEHAL_TENSOR_TYPE_INT_16;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.tensorProps.dims[0] = 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.tensorProps.dims[1] += 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.tensorProps.dims[2] += 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.tensorProps.dims[3] += 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        RideHal_SharedBuffer_t invalidMvConf = mvConf;
        invalidMvConf.type = RIDEHAL_BUFFER_TYPE_IMAGE;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.buffer.pData = nullptr;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.tensorProps.numDims = 8;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.tensorProps.type = RIDEHAL_TENSOR_TYPE_INT_16;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.tensorProps.dims[0] = 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.tensorProps.dims[1] += 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.tensorProps.dims[2] += 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.tensorProps.dims[3] += 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.type = RIDEHAL_BUFFER_TYPE_TENSOR;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.buffer.pData = nullptr;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.imgProps.format = RIDEHAL_IMAGE_FORMAT_RGB888;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.imgProps.width += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.imgProps.height += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.imgProps.numPlanes += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.imgProps.stride[0] += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.imgProps.planeBufSize[0] += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( RIDEHAL_ERROR_INVALID_BUF, ret );

        ret = ofl.Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = ofl.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = refImg.Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = curImg.Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = mvFwdMap.Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        ret = mvConf.Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
}
#endif

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif