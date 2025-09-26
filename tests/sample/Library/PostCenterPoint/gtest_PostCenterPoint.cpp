// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "PostCenterPoint.hpp"
#include "QC/sample/BufferManager.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace QC;
using namespace QC::sample;
using namespace QC::Memory;

static float plrPreConfig0_minZRange = -3.0;
static float plrPreConfig0_maxZRange = 1.0;

static PostCenterPoint_Config_t plrPostConfig0 = {
        QC_PROCESSOR_HTP0,
        0.16,
        0.16, /* pillar size: x, y */
        0.0,
        -39.68, /* min Range, x, y */
        69.12,
        39.68,                                        /* max Range, x, y */
        3,                                            /* numClass */
        300000,                                       /* maxNumInPts */
        4,                                            /* numInFeatureDim*/
        500,                                          /* maxNumDetOut */
        2,                                            /* stride */
        0.1,                                          /* threshScore */
        0.1,                                          /* threshIOU */
        { 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, nullptr }, /* filterParams */
        true,                                         /* bMapPtsToBBox */
        false,                                        /* bBBoxFilter */
};

static PostCenterPoint_Config_t plrPostConfig1 = {
        QC_PROCESSOR_HTP0,
        0.2,
        0.2, /* pillar size: x, y */
        -2.0,
        -40.0, /* min Range, x, y */
        150,
        40,                                           /* max Range, x, y */
        8,                                            /* numClass */
        300000,                                       /* maxNumInPts */
        4,                                            /* numInFeatureDim*/
        500,                                          /* maxNumDetOut */
        2,                                            /* stride */
        0.1,                                          /* threshScore */
        0.1,                                          /* threshIOU */
        { 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, nullptr }, /* filterParams */
        true,                                         /* bMapPtsToBBox */
        false,                                        /* bBBoxFilter */
};

static void LoadPoints( void *pData, uint32_t size, uint32_t &numPts, const char *pcdFile )
{
    FILE *pFile = fopen( pcdFile, "rb" );
    ASSERT_NE( nullptr, pFile );

    fseek( pFile, 0, SEEK_END );
    int length = ftell( pFile );
    ASSERT_LT( length, size );
    numPts = length / 16;
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, numPts * 16, pFile );
    ASSERT_EQ( r, length );
    printf( "load %u points from %s\n", numPts, pcdFile );
    fclose( pFile );

    ASSERT_NE( 0, numPts );
}

static void LoadRaw( void *pData, uint32_t length, const char *rawFile )
{
    printf( "load raw from %s\n", rawFile );
    FILE *pFile = fopen( rawFile, "rb" );
    ASSERT_NE( nullptr, pFile );
    fseek( pFile, 0, SEEK_END );
    int size = ftell( pFile );
    ASSERT_EQ( size, length );
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, length, pFile );
    ASSERT_EQ( r, length );
    fclose( pFile );
}

void SANITY_PostCenterPoint( QCProcessorType_e processor, PostCenterPoint_Config_t &cfg,
                             const char *pcdFile, const char *hmFile, const char *xyFile,
                             const char *zFile, const char *sizeFile, const char *thetaFile,
                             bool bDumpOutput = false )
{
    PostCenterPoint_Config_t config = cfg;
    BufferManager *pBufMgr = BufferManager::Get( { "PLRPOST", QC_NODE_TYPE_CUSTOM_0, 0 } );
    Logger logger;
    logger.Init( "PLRPOST", LOGGER_LEVEL_ERROR );
    PostCenterPoint plrPost( logger );
    config.processor = processor;
    QCStatus_e ret;

    uint32_t numCellsX =
            (uint32_t) ( ( config.maxXRange - config.minXRange ) / config.pillarXSize );
    uint32_t numCellsY =
            (uint32_t) ( ( config.maxYRange - config.minYRange ) / config.pillarYSize );

    uint32_t width = numCellsX / config.stride;
    uint32_t height = numCellsY / config.stride;
    TensorProps_t inPtsTsProp( QC_TENSOR_TYPE_FLOAT_32,
                               { config.maxNumInPts, config.numInFeatureDim } );
    TensorProps_t hmTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, config.numClass } );
    TensorProps_t xyTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 2 } );
    TensorProps_t zTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 1 } );
    TensorProps_t sizeTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 3 } );
    TensorProps_t thetaTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 2 } );
    TensorProps_t detTsProp( QC_TENSOR_TYPE_FLOAT_32,
                             { config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM } );

    TensorDescriptor_t inPts;
    TensorDescriptor_t hm;
    TensorDescriptor_t xy;
    TensorDescriptor_t z;
    TensorDescriptor_t size;
    TensorDescriptor_t theta;
    TensorDescriptor_t det;

    ret = pBufMgr->Allocate( inPtsTsProp, inPts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t numPts = 0;
    LoadPoints( inPts.GetDataPtr(), inPts.size, numPts, pcdFile );
    inPts.dims[0] = numPts;

    ret = pBufMgr->Allocate( hmTsProp, hm );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( hm.GetDataPtr(), hm.size, hmFile );

    ret = pBufMgr->Allocate( xyTsProp, xy );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( xy.GetDataPtr(), xy.size, xyFile );

    ret = pBufMgr->Allocate( zTsProp, z );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( z.GetDataPtr(), z.size, zFile );

    ret = pBufMgr->Allocate( sizeTsProp, size );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( size.GetDataPtr(), size.size, sizeFile );

    ret = pBufMgr->Allocate( thetaTsProp, theta );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( theta.GetDataPtr(), theta.size, thetaFile );

    ret = pBufMgr->Allocate( detTsProp, det );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPost.Init( "PLRPOST0", &config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPost.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    det.dims[0] = config.maxNumDetOut;
    ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &det );
    ASSERT_EQ( QC_STATUS_OK, ret );

    PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.GetDataPtr();
    for ( uint32_t i = 0; i < det.dims[0]; i++ )
    {
        printf( "[%d] class=%d score=%.3f bbox=[%.3f %.3f %.3f %.3f %.3f %.3f] "
                "theta=%.3f, mean=[%.3f %.3f %.3f %.3f]x%u\n",
                i, pObj->label, pObj->score, pObj->x, pObj->y, pObj->z, pObj->length, pObj->width,
                pObj->height, pObj->theta, pObj->meanPtX, pObj->meanPtY, pObj->meanPtZ,
                pObj->meanIntensity, pObj->numPts );
        pObj++;
    }

    if ( bDumpOutput )
    {
        /* dump in a python list format for visualization with vis3d.py */
        PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.GetDataPtr();
        for ( uint32_t i = 0; i < det.dims[0]; i++ )
        {
            printf( "[%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %d],\n", pObj->x, pObj->y,
                    pObj->z, pObj->length, pObj->width, pObj->height, pObj->theta, pObj->score,
                    pObj->label );
            pObj++;
        }
    }

    ret = plrPost.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPost.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pBufMgr->Free( inPts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = pBufMgr->Free( hm );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = pBufMgr->Free( xy );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = pBufMgr->Free( z );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = pBufMgr->Free( size );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = pBufMgr->Free( theta );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = pBufMgr->Free( det );
    ASSERT_EQ( QC_STATUS_OK, ret );

    logger.Deinit();

    BufferManager::Put( pBufMgr );
}

TEST( FadasPlr, SANITY_PostCenterPointCPU )
{
    SANITY_PostCenterPoint( QC_PROCESSOR_CPU, plrPostConfig0, "data/test/plr/CFG0/000008.bin",
                            "data/test/plr/CFG0/hm-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center_z-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/dim_exp-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/rot-activation-0-inf-1.bin" );
    SANITY_PostCenterPoint( QC_PROCESSOR_CPU, plrPostConfig1, "data/test/plr/pointcloud.bin",
                            "data/test/plr/hm-activation-0-inf-1.bin",
                            "data/test/plr/reg-activation-0-inf-1.bin",
                            "data/test/plr/height-activation-0-inf-1.bin",
                            "data/test/plr/dim-activation-0-inf-1.bin",
                            "data/test/plr/rot-activation-0-inf-1.bin" );
}

TEST( FadasPlr, SANITY_PostCenterPointDSP )
{
    SANITY_PostCenterPoint( QC_PROCESSOR_HTP0, plrPostConfig0, "data/test/plr/CFG0/000008.bin",
                            "data/test/plr/CFG0/hm-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/center_z-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/dim_exp-activation-0-inf-1.bin",
                            "data/test/plr/CFG0/rot-activation-0-inf-1.bin" );
    SANITY_PostCenterPoint( QC_PROCESSOR_HTP0, plrPostConfig1, "data/test/plr/pointcloud.bin",
                            "data/test/plr/hm-activation-0-inf-1.bin",
                            "data/test/plr/reg-activation-0-inf-1.bin",
                            "data/test/plr/height-activation-0-inf-1.bin",
                            "data/test/plr/dim-activation-0-inf-1.bin",
                            "data/test/plr/rot-activation-0-inf-1.bin", true );
}

static void FadasPlr_E2E_PostProc( QCProcessorType_e processor, PostCenterPoint_Config_t &cfg )
{
    int exist = access( "/tmp/pointcloud.bin", F_OK );
    exist |= access( "/tmp/hm.raw", F_OK );
    if ( 0 == exist )
    {
        SANITY_PostCenterPoint( processor, cfg, "/tmp/pointcloud.bin", "/tmp/hm.raw",
                                "/tmp/center.raw", "/tmp/center_z.raw", "/tmp/dim_exp.raw",
                                "/tmp/rot.raw", true );
    }
    else
    {
        printf( "skip E2E_PostProc\n" );
    }
}


TEST( FadasPlr, E2E_CFG0_PostProcCPU )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_CPU, plrPostConfig0 );
}


TEST( FadasPlr, E2E_CFG0_PostProcDSP )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_HTP0, plrPostConfig0 );
}

TEST( FadasPlr, E2E_CFG1_PostProcCPU )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_CPU, plrPostConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PostProcDSP )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_HTP0, plrPostConfig1 );
}

TEST( FadasPlr, L2_PostCenterPoint )
{
    BufferManager *pBufMgr = BufferManager::Get( { "PLRPOST", QC_NODE_TYPE_CUSTOM_0, 0 } );
    Logger logger;
    logger.Init( "PLRPOST", LOGGER_LEVEL_ERROR );
    {
        PostCenterPoint_Config_t config = plrPostConfig0;
        PostCenterPoint plrPost( logger );
        QCStatus_e ret;

        ret = plrPost.Init( "PLRPOST0", nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config.stride = 0;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config.stride = 3;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config.stride = 4;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );

        config = plrPostConfig0;
        config.processor = QC_PROCESSOR_MAX;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = plrPostConfig0;
        config.bMapPtsToBBox = false;
        config.bBBoxFilter = true;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = plrPostConfig1;
        config.pillarXSize = 0;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        config.processor = QC_PROCESSOR_CPU;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_FAIL, ret );
    }

    QCProcessorType_e processors[2] = { QC_PROCESSOR_HTP0, QC_PROCESSOR_CPU };
    for ( int i = 0; i < 2; i++ )
    {
        PostCenterPoint_Config_t config = plrPostConfig0;
        PostCenterPoint plrPost( logger );
        QCStatus_e ret;
        bool labelSelect[config.numClass] = { true };
        for ( int i = 1; i < config.numClass; i++ )
        {
            labelSelect[i] = false;
        }
        PostCenterPoint_3DBBoxFilterParams_t filterParams = { 10,
                                                              config.minXRange,
                                                              config.minYRange,
                                                              plrPreConfig0_minZRange,
                                                              config.maxXRange,
                                                              config.maxYRange,
                                                              plrPreConfig0_maxZRange,
                                                              labelSelect };
        config.filterParams = filterParams;
        config.bBBoxFilter = true;
        SANITY_PostCenterPoint( processors[i], config, "data/test/plr/CFG0/000008.bin",
                                "data/test/plr/CFG0/hm-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/center-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/center_z-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/dim_exp-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/rot-activation-0-inf-1.bin" );
        config.bMapPtsToBBox = false;
        config.bBBoxFilter = false;
        SANITY_PostCenterPoint( processors[i], config, "data/test/plr/CFG0/000008.bin",
                                "data/test/plr/CFG0/hm-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/center-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/center_z-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/dim_exp-activation-0-inf-1.bin",
                                "data/test/plr/CFG0/rot-activation-0-inf-1.bin" );
    }

    for ( int i = 0; i < 2; i++ )
    {
        PostCenterPoint_Config_t config = plrPostConfig0;
        config.processor = processors[i];
        PostCenterPoint plrPost( logger );
        QCStatus_e ret;

        uint32_t numCellsX =
                (uint32_t) ( ( config.maxXRange - config.minXRange ) / config.pillarXSize );
        uint32_t numCellsY =
                (uint32_t) ( ( config.maxYRange - config.minYRange ) / config.pillarYSize );

        uint32_t width = numCellsX / config.stride;
        uint32_t height = numCellsY / config.stride;
        TensorProps_t inPtsTsProp( QC_TENSOR_TYPE_FLOAT_32,
                                   { config.maxNumInPts, config.numInFeatureDim } );
        TensorProps_t hmTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, config.numClass } );
        TensorProps_t xyTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 2 } );
        TensorProps_t zTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 1 } );
        TensorProps_t sizeTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 3 } );
        TensorProps_t thetaTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 2 } );
        TensorProps_t detTsProp( QC_TENSOR_TYPE_FLOAT_32,
                                 { config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM } );

        TensorDescriptor_t inPts;
        TensorDescriptor_t hm;
        TensorDescriptor_t xy;
        TensorDescriptor_t z;
        TensorDescriptor_t size;
        TensorDescriptor_t theta;
        TensorDescriptor_t det;

        ret = pBufMgr->Allocate( inPtsTsProp, inPts );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( hmTsProp, hm );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( xyTsProp, xy );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( zTsProp, z );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( sizeTsProp, size );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( thetaTsProp, theta );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( detTsProp, det );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.RegisterBuffers( &inPts, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.DeRegisterBuffers( &inPts, 1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.Start();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.Stop();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.Deinit();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        config = plrPostConfig0;
        config.pillarXSize = 0;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        config = plrPostConfig0;
        ret = plrPost.Init( "PLRPOST0", &config );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.RegisterBuffers( nullptr, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = plrPost.DeRegisterBuffers( nullptr, 1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = plrPost.RegisterBuffers( &inPts, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.DeRegisterBuffers( &inPts, 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );

        std::vector<uint8_t> inPtsNonDmaMem;
        inPtsNonDmaMem.resize( inPts.size );
        TensorDescriptor_t inPtsNonDma = inPts;
        inPtsNonDma.pBuf = inPtsNonDmaMem.data();
        ret = plrPost.RegisterBuffers( &inPtsNonDma, 1, FADAS_BUF_TYPE_IN );
        if ( QC_PROCESSOR_CPU == config.processor )
        {
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        else
        {
            ASSERT_EQ( QC_STATUS_FAIL, ret );
        }

        TensorDescriptor_t inPtsNonTensor = inPts;
        inPtsNonTensor.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.RegisterBuffers( &inPtsNonTensor, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = plrPost.DeRegisterBuffers( &inPtsNonTensor, 1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = plrPost.Execute( nullptr, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPost.Execute( &hm, nullptr, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPost.Execute( &hm, &xy, nullptr, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPost.Execute( &hm, &xy, &z, nullptr, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPost.Execute( &hm, &xy, &z, &size, nullptr, &inPts, &det );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, nullptr, &det );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        TensorDescriptor_t hmWrong = hm;
        hmWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.pBuf = nullptr;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.numDims = 5;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.tensorType = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.dims[0] = 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.dims[1] /= 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.dims[2] /= 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.dims[3] /= 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t xyWrong = xy;
        xyWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.pBuf = nullptr;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.numDims = 5;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.tensorType = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t zWrong = z;
        zWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.pBuf = nullptr;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.tensorType = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t sizeWrong = size;
        sizeWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.pBuf = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.tensorType = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t thetaWrong = theta;
        thetaWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.pBuf = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.tensorType = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t inPtsWrong = inPts;
        inPtsWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.pBuf = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.tensorType = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        TensorDescriptor_t detWrong = det;
        detWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.pBuf = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.tensorType = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    }

    logger.Deinit();
    BufferManager::Put( pBufMgr );
}

TEST( FadasPlr, L2_FadasPlrPostProc )
{
    BufferManager *pBufMgr = BufferManager::Get( { "PLRPOST", QC_NODE_TYPE_CUSTOM_0, 0 } );
    QCProcessorType_e processors[2] = { QC_PROCESSOR_HTP0, QC_PROCESSOR_CPU };
    for ( int i = 0; i < 2; i++ )
    {
        FadasPlrPostProc plrPost;
        QCStatus_e ret;
        PostCenterPoint_Config_t config = plrPostConfig1;
        config.processor = processors[i];

        uint32_t numCellsX =
                (uint32_t) ( ( config.maxXRange - config.minXRange ) / config.pillarXSize );
        uint32_t numCellsY =
                (uint32_t) ( ( config.maxYRange - config.minYRange ) / config.pillarYSize );

        uint32_t width = numCellsX / config.stride;
        uint32_t height = numCellsY / config.stride;
        TensorProps_t inPtsTsProp( QC_TENSOR_TYPE_FLOAT_32,
                                   { config.maxNumInPts, config.numInFeatureDim } );
        TensorProps_t hmTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, config.numClass } );
        TensorProps_t xyTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 2 } );
        TensorProps_t zTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 1 } );
        TensorProps_t sizeTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 3 } );
        TensorProps_t thetaTsProp( QC_TENSOR_TYPE_FLOAT_32, { 1, height, width, 2 } );
        TensorProps_t detTsProp( QC_TENSOR_TYPE_FLOAT_32,
                                 { config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM } );

        TensorDescriptor_t inPts;
        TensorDescriptor_t hm;
        TensorDescriptor_t xy;
        TensorDescriptor_t z;
        TensorDescriptor_t size;
        TensorDescriptor_t theta;
        TensorDescriptor_t BBoxList;
        TensorDescriptor_t labels;
        TensorDescriptor_t scores;
        TensorDescriptor_t metadata;


        ret = pBufMgr->Allocate( inPtsTsProp, inPts );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( hmTsProp, hm );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( xyTsProp, xy );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( zTsProp, z );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( sizeTsProp, size );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = pBufMgr->Allocate( thetaTsProp, theta );
        ASSERT_EQ( QC_STATUS_OK, ret );

        {
            TensorProps_t tensorProps;
            tensorProps.tensorType = QC_TENSOR_TYPE_FLOAT_32;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.dims[1] = sizeof( FadasCuboidf32_t ) / sizeof( float );
            tensorProps.numDims = 2;
            ret = pBufMgr->Allocate( tensorProps, BBoxList );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        {
            TensorProps_t tensorProps;
            tensorProps.tensorType = QC_TENSOR_TYPE_UINT_32;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.numDims = 1;
            ret = pBufMgr->Allocate( tensorProps, labels );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        {
            TensorProps_t tensorProps;
            tensorProps.tensorType = QC_TENSOR_TYPE_FLOAT_32;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.numDims = 1;
            ret = pBufMgr->Allocate( tensorProps, scores );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        {
            TensorProps_t tensorProps;
            tensorProps.tensorType = QC_TENSOR_TYPE_UINT_8;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.dims[1] = sizeof( FadasPlr3DBBoxMetadata_t );
            tensorProps.numDims = 2;
            ret = pBufMgr->Allocate( tensorProps, metadata );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        ret = plrPost.SetFilterParams(
                config.filterParams.minCentreX, config.filterParams.minCentreY,
                config.filterParams.minCentreZ, config.filterParams.maxCentreX,
                config.filterParams.maxCentreY, config.filterParams.maxCentreZ,
                config.filterParams.labelSelect, config.filterParams.maxNumFilter );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.CreatePostProc();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.Init( config.processor, "PLRPOST0", LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.SetParams( config.pillarXSize, config.pillarYSize, config.minXRange,
                                 config.minYRange, config.maxXRange, config.maxYRange,
                                 config.numClass, config.maxNumInPts, config.numInFeatureDim,
                                 config.maxNumDetOut, config.threshScore, config.threshIOU,
                                 config.bMapPtsToBBox );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.DestroyPostProc();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.CreatePostProc();
        ASSERT_EQ( QC_STATUS_OK, ret );

        uint32_t numDetOut;
        ret = plrPost.ExtractBBoxRun( nullptr, &xy, &z, &size, &theta, &inPts, &BBoxList, &labels,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, nullptr, &z, &size, &theta, &inPts, &BBoxList, &labels,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, nullptr, &size, &theta, &inPts, &BBoxList, &labels,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, nullptr, &theta, &inPts, &BBoxList, &labels,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, &size, nullptr, &inPts, &BBoxList, &labels,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, &size, &theta, nullptr, &BBoxList, &labels,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, &size, &theta, &inPts, nullptr, &labels,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, &size, &theta, &inPts, &BBoxList, nullptr,
                                      &scores, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, &size, &theta, &inPts, &BBoxList, &labels,
                                      nullptr, &metadata, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, &size, &theta, &inPts, &BBoxList, &labels,
                                      &scores, nullptr, &numDetOut );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        ret = plrPost.ExtractBBoxRun( &hm, &xy, &z, &size, &theta, &inPts, &BBoxList, &labels,
                                      &scores, &metadata, nullptr );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = plrPost.DestroyPostProc();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    BufferManager::Put( pBufMgr );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif