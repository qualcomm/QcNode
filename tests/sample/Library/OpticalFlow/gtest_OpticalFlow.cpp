// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "OpticalFlow.hpp"
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

static void Eva_OpticalFlowRun( std::string name, OpticalFlow_Config_t &config,
                                std::string img1 = "", std::string img2 = "",
                                std::string goldenMvMap = "", std::string goldenMvConf = "" )
{
    BufferManager bufMgr( { name, QC_NODE_TYPE_EVA_OPTICAL_FLOW, 0 } );
    Logger logger;
    logger.Init( name.c_str(), LOGGER_LEVEL_ERROR );
    OpticalFlow ofl( logger );
    QCStatus_e ret;
    ImageDescriptor_t refImg;
    ImageDescriptor_t curImg;
    TensorDescriptor_t mvFwdMap;
    TensorDescriptor_t mvConf;
    TensorDescriptor_t mvFwdMapG;
    TensorDescriptor_t mvConfG;

    uint32_t width = ( config.width >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;
    uint32_t height = ( config.height >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;

    TensorProps_t mvFwdMapTsProp( QC_TENSOR_TYPE_UINT_16,
                                  { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 } );
    TensorProps_t mvConfTsProp( QC_TENSOR_TYPE_UINT_8,
                                { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 } );

    printf( "-- Test for %s\n", name.c_str() );

    ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                           refImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                           curImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Allocate( mvFwdMapTsProp, mvFwdMap );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Allocate( mvConfTsProp, mvConf );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( false == img1.empty() )
    {
        LoadRaw( refImg.GetDataPtr(), refImg.size, img1 );
    }

    if ( false == img2.empty() )
    {
        LoadRaw( curImg.GetDataPtr(), curImg.size, img2 );
    }

    if ( ( false == goldenMvMap.empty() ) && ( false == goldenMvConf.empty() ) )
    { /* clear output buffer for accuracy test */
        memset( mvFwdMap.GetDataPtr(), 0, mvFwdMap.size );
        memset( mvConf.GetDataPtr(), 0, mvConf.size );
    }

    ret = ofl.Init( name.c_str(), &config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = ofl.RegisterBuffers( &refImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = ofl.RegisterBuffers( &curImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = ofl.RegisterBuffers( &mvFwdMap, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = ofl.RegisterBuffers( &mvConf, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = ofl.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &mvConf );
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( ( false == goldenMvMap.empty() ) && ( false == goldenMvConf.empty() ) )
    {
        // for the first run, with below to generate the golden
        // SaveRaw( goldenMvMap, mvFwdMap.GetDataPtr(), mvFwdMap.size );
        // SaveRaw( goldenMvConf, mvConf.GetDataPtr(), mvConf.size );

        ret = bufMgr.Allocate( mvFwdMapTsProp, mvFwdMapG );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( mvConfTsProp, mvConfG );
        ASSERT_EQ( QC_STATUS_OK, ret );

        LoadRaw( mvFwdMapG.GetDataPtr(), mvFwdMapG.size, goldenMvMap );
        LoadRaw( mvConfG.GetDataPtr(), mvConfG.size, goldenMvConf );

        std::string md5Output = MD5Sum( mvFwdMap.GetDataPtr(), mvFwdMap.size );
        std::string md5Golden = MD5Sum( mvFwdMapG.GetDataPtr(), mvFwdMapG.size );
        ASSERT_EQ( md5Output, md5Golden );

        md5Output = MD5Sum( mvConf.GetDataPtr(), mvConf.size );
        md5Golden = MD5Sum( mvConfG.GetDataPtr(), mvConfG.size );
        ASSERT_EQ( md5Output, md5Golden );

        bufMgr.Free( mvFwdMapG );
        bufMgr.Free( mvConfG );
    }

    ret = ofl.Execute( &curImg, &refImg, &mvFwdMap, &mvConf );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = ofl.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = ofl.DeRegisterBuffers( &refImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = ofl.DeRegisterBuffers( &curImg, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = ofl.DeRegisterBuffers( &mvFwdMap, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = ofl.DeRegisterBuffers( &mvConf, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = ofl.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = bufMgr.Free( refImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Free( curImg );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Free( mvFwdMap );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = bufMgr.Free( mvConf );
    ASSERT_EQ( QC_STATUS_OK, ret );
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
        config.format = QC_IMAGE_FORMAT_UYVY;
        config.filterOperationMode = EVA_OF_MODE_CPU;
        Eva_OpticalFlowRun( "OFL0_CPU_UYVY", config );
    }
    {
        OpticalFlow_Config_t config;
        config.width = 1920;
        config.height = 1024;
        config.format = QC_IMAGE_FORMAT_UYVY;
        config.filterOperationMode = EVA_OF_MODE_DSP;
        Eva_OpticalFlowRun( "OFL0_DSP_UYVY", config );
    }

    /* P010 Only supported for EVA_OF_MODE_DISABLE in EvaOFFilterOperationMode_e */
    {
        OpticalFlow_Config_t config;
        config.width = 1920;
        config.height = 1024;
        config.format = QC_IMAGE_FORMAT_P010;
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
    BufferManager bufMgr( { "OFL", QC_NODE_TYPE_EVA_OPTICAL_FLOW, 0 } );
    Logger logger;
    logger.Init( "OFL", LOGGER_LEVEL_ERROR );
    QCStatus_e ret;
    {
        OpticalFlow ofl( logger );
        OpticalFlow_Config_t configDft;
        configDft.width = 1920;
        configDft.height = 1024;
        OpticalFlow_Config_t config = configDft;

        ret = ofl.Init( "OFL0", nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = configDft;
        config.format = QC_IMAGE_FORMAT_NV12_UBWC;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = configDft;
        config.filterOperationMode = EVA_OF_MODE_MAX;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        config = configDft;
        config.direction = EVA_OF_BIDIRECTIONAL;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = configDft; /* super large image */
        config.width = 409600;
        config.height = 409600;
        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = ofl.Deinit();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        config = configDft;
        ImageDescriptor_t refImg;
        ImageDescriptor_t curImg;
        TensorDescriptor_t mvFwdMap;
        TensorDescriptor_t mvConf;

        uint32_t width = ( config.width >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;
        uint32_t height = ( config.height >> config.amFilter.nStepSize )
                          << config.amFilter.nUpScale;

        TensorProps_t mvFwdMapTsProp( QC_TENSOR_TYPE_UINT_16,
                                      { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 } );
        TensorProps_t mvConfTsProp( QC_TENSOR_TYPE_UINT_8,
                                    { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 } );

        ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                               refImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( ImageBasicProps_t( config.width, config.height, config.format ),
                               curImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( mvFwdMapTsProp, mvFwdMap );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( mvConfTsProp, mvConf );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = ofl.RegisterBuffers( &refImg, 1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = ofl.DeRegisterBuffers( &refImg, 1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = ofl.Start();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = ofl.Stop();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = ofl.Init( "OFL0", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = ofl.RegisterBuffers( nullptr, 1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = ofl.RegisterBuffers( &refImg, 0 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ImageDescriptor_t invalidImg = refImg;
        invalidImg.dmaHandle = 0xdeadbeef;
        ret = ofl.RegisterBuffers( &invalidImg, 1 );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = ofl.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = ofl.RegisterBuffers( &curImg, 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = ofl.Execute( &refImg, &curImg, nullptr, &mvConf );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = ofl.Execute( &refImg, nullptr, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = ofl.Execute( nullptr, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        TensorDescriptor_t invalidFwdMap = mvFwdMap;
        invalidFwdMap.type = QC_BUFFER_TYPE_IMAGE;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.pBuf = nullptr;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.numDims = 8;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.tensorType = QC_TENSOR_TYPE_INT_16;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.dims[0] = 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.dims[1] += 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.dims[2] += 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidFwdMap = mvFwdMap;
        invalidFwdMap.dims[3] += 2;
        ret = ofl.Execute( &refImg, &curImg, &invalidFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t invalidMvConf = mvConf;
        invalidMvConf.type = QC_BUFFER_TYPE_IMAGE;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.pBuf = nullptr;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.numDims = 8;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.tensorType = QC_TENSOR_TYPE_INT_16;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.dims[0] = 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.dims[1] += 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.dims[2] += 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidMvConf = mvConf;
        invalidMvConf.dims[3] += 2;
        ret = ofl.Execute( &refImg, &curImg, &mvFwdMap, &invalidMvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.type = QC_BUFFER_TYPE_TENSOR;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.pBuf = nullptr;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.format = QC_IMAGE_FORMAT_RGB888;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.width += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.height += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.numPlanes += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.stride[0] += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        invalidImg = refImg;
        invalidImg.planeBufSize[0] += 1;
        ret = ofl.Execute( &invalidImg, &curImg, &mvFwdMap, &mvConf );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = ofl.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = ofl.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = bufMgr.Free( refImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( curImg );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( mvFwdMap );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( mvConf );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}
#endif

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif