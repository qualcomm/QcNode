// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/component/PostCenterPoint.hpp"
#include "QC/component/Voxelization.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace QC::common;
using namespace QC::component;

static Voxelization_Config_t plrPreConfig0 = {
        QC_PROCESSOR_HTP0,
        0.16,
        0.16,
        4.0, /* pillar size: x, y, z */
        0.0,
        -39.68,
        -3.0, /* min Range, x, y, z */
        69.12,
        39.68,
        1.0,    /* max Range, x, y, z */
        300000, /* maxNumInPts */
        4,      /* numInFeatureDim*/
        12000,  /* maxNumPlrs */
        32,     /* maxNumPtsPerPlr */
        10,     /* numOutFeatureDim */
};

static Voxelization_Config_t plrPreConfig1 = {
        QC_PROCESSOR_HTP0,
        0.2,
        0.2,
        5.0, /* pillar size: x, y, z */
        -2.0,
        -40.0,
        -2.0, /* min Range, x, y, z */
        150.0,
        40.0,
        3.0,    /* max Range, x, y, z */
        300000, /* maxNumInPts */
        4,      /* numInFeatureDim*/
        40000,  /* maxNumPlrs */
        32,     /* maxNumPtsPerPlr */
        10,     /* numOutFeatureDim */
};

static Voxelization_Config_t plrPreConfig2 = {
        QC_PROCESSOR_GPU,
        0.2,
        0.2,
        8.0, /* pillar size: x, y, z */
        -51.2,
        -51.2,
        -5.0, /* min Range, x, y, z */
        51.2,
        51.2,
        3.0,                      /* max Range, x, y, z */
        300000,                   /* maxNumInPts */
        5,                        /* numInFeatureDim*/
        25000,                    /* maxNumPlrs */
        32,                       /* maxNumPtsPerPlr */
        10,                       /* numOutFeatureDim */
        VOXELIZATION_INPUT_XYZRT, /* voxelization input pointclouds type */
};

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

static void RandomGenPoints( float *pVoxels, uint32_t numPts )
{
    for ( uint32_t i = 0; i < numPts; i++ )
    {
        pVoxels[0] = ( rand() % 10000 ) / 100;
        pVoxels[1] = ( rand() % 10000 ) / 100;
        pVoxels[2] = ( rand() % 300 ) / 100;
        pVoxels[3] = 0.9;
        pVoxels += 4;
    }
}

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

static void SaveRaw( std::string path, void *pData, size_t size )
{
    FILE *pFile = fopen( path.c_str(), "wb" );
    if ( nullptr != pFile )
    {
        fwrite( pData, 1, size, pFile );
        fclose( pFile );
        printf( "save raw %s\n", path.c_str() );
    }
}


static void SANITY_Voxelization( QCProcessorType_e processor, Voxelization_Config_t &cfg,
                                 const char *pcdFile = nullptr, bool bDumpOutput = false,
                                 bool bPerformanceTest = false, int times = 100 )
{
    Voxelization_Config_t config = cfg;
    config.processor = processor;
    Voxelization plrPre;
    QCStatus_e ret;

    QCTensorProps_t inPtsTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { config.maxNumInPts, config.numInFeatureDim, 0 },
            2,
    };
    QCTensorProps_t outPlrsTsProp;
    if ( ( VOXELIZATION_INPUT_XYZRT == config.inputMode ) )
    {
        outPlrsTsProp = {
                QC_TENSOR_TYPE_INT_32,
                { config.maxNumPlrs, 2, 0 },
                2,
        };
    }
    else
    {
        outPlrsTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumPlrs, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
                2,
        };
    }
    QCTensorProps_t outFeatureTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { config.maxNumPlrs, config.maxNumPtsPerPlr, config.numOutFeatureDim, 0 },
            3,
    };

    QCSharedBuffer_t inPts;
    QCSharedBuffer_t outPlrs;
    QCSharedBuffer_t outFeature;

    ret = inPts.Allocate( &inPtsTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t numPts = 0;
    if ( nullptr == pcdFile )
    {
        numPts = ( config.maxNumInPts / 3 ) + rand() % ( 2 * config.maxNumInPts / 3 );
        RandomGenPoints( (float *) inPts.data(), numPts );
    }
    else
    {
        LoadPoints( inPts.data(), inPts.size, numPts, pcdFile );
    }
    inPts.tensorProps.dims[0] = numPts;

    ret = outPlrs.Allocate( &outPlrsTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = outFeature.Allocate( &outFeatureTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPre.Init( "PLRPRE0", &config, LOGGER_LEVEL_INFO );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPre.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( bPerformanceTest )
    {
        auto start = std::chrono::high_resolution_clock::now();
        for ( int i = 0; i < times; i++ )
        {
            ret = plrPre.Execute( &inPts, &outPlrs, &outFeature );
        }
        auto end = std::chrono::high_resolution_clock::now();
        double duration_ms = std::chrono::duration<double, std::milli>( end - start ).count();
        printf( "\nexecute time = %f ms\n", (float) duration_ms / (float) times );
    }
    else
    {
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeature );
    }
    ASSERT_EQ( QC_STATUS_OK, ret );

    if ( bDumpOutput )
    {
        SaveRaw( "/tmp/coords.raw", outPlrs.data(), outPlrs.size );
        SaveRaw( "/tmp/features.raw", outFeature.data(), outFeature.size );
    }

    ret = plrPre.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPre.Deinit();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = outPlrs.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = outFeature.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( FadasPlr, SANITY_VoxelizationGPU )
{
    SANITY_Voxelization( QC_PROCESSOR_GPU, plrPreConfig0 );
    SANITY_Voxelization( QC_PROCESSOR_GPU, plrPreConfig1 );

    SANITY_Voxelization( QC_PROCESSOR_GPU, plrPreConfig0, "data/test/plr/pointcloud.bin", false,
                         true, 100 );
    SANITY_Voxelization( QC_PROCESSOR_GPU, plrPreConfig1, "data/test/plr/pointcloud.bin", false,
                         true, 100 );
    SANITY_Voxelization( QC_PROCESSOR_GPU, plrPreConfig2, "data/test/plr/pointcloud_XYZRT.bin",
                         false, true, 100 );
}

TEST( FadasPlr, SANITY_VoxelizationCPU )
{
    SANITY_Voxelization( QC_PROCESSOR_CPU, plrPreConfig0 );
    SANITY_Voxelization( QC_PROCESSOR_CPU, plrPreConfig1 );

    SANITY_Voxelization( QC_PROCESSOR_CPU, plrPreConfig0, "data/test/plr/pointcloud.bin" );
    SANITY_Voxelization( QC_PROCESSOR_CPU, plrPreConfig1, "data/test/plr/pointcloud.bin" );
}

TEST( FadasPlr, SANITY_VoxelizationDSP )
{
    SANITY_Voxelization( QC_PROCESSOR_HTP0, plrPreConfig0 );
    SANITY_Voxelization( QC_PROCESSOR_HTP0, plrPreConfig1 );


    SANITY_Voxelization( QC_PROCESSOR_HTP0, plrPreConfig0, "data/test/plr/pointcloud.bin", false,
                         true, 100 );
    SANITY_Voxelization( QC_PROCESSOR_HTP0, plrPreConfig1, "data/test/plr/pointcloud.bin", false,
                         true, 100 );
}


void SANITY_PostCenterPoint( QCProcessorType_e processor, PostCenterPoint_Config_t &cfg,
                             const char *pcdFile, const char *hmFile, const char *xyFile,
                             const char *zFile, const char *sizeFile, const char *thetaFile,
                             bool bDumpOutput = false )
{
    PostCenterPoint_Config_t config = cfg;
    PostCenterPoint plrPost;
    config.processor = processor;
    QCStatus_e ret;

    uint32_t numCellsX =
            ( uint32_t )( ( config.maxXRange - config.minXRange ) / config.pillarXSize );
    uint32_t numCellsY =
            ( uint32_t )( ( config.maxYRange - config.minYRange ) / config.pillarYSize );

    uint32_t width = numCellsX / config.stride;
    uint32_t height = numCellsY / config.stride;
    QCTensorProps_t inPtsTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { config.maxNumInPts, config.numInFeatureDim, 0 },
            2,
    };
    QCTensorProps_t hmTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, config.numClass },
            4,
    };
    QCTensorProps_t xyTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 2 },
            4,
    };
    QCTensorProps_t zTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 1 },
            4,
    };
    QCTensorProps_t sizeTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 3 },
            4,
    };
    QCTensorProps_t thetaTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { 1, height, width, 2 },
            4,
    };
    QCTensorProps_t detTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM },
            2,
    };

    QCSharedBuffer_t inPts;
    QCSharedBuffer_t hm;
    QCSharedBuffer_t xy;
    QCSharedBuffer_t z;
    QCSharedBuffer_t size;
    QCSharedBuffer_t theta;
    QCSharedBuffer_t det;

    ret = inPts.Allocate( &inPtsTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t numPts = 0;
    LoadPoints( inPts.data(), inPts.size, numPts, pcdFile );
    inPts.tensorProps.dims[0] = numPts;

    ret = hm.Allocate( &hmTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( hm.data(), hm.size, hmFile );

    ret = xy.Allocate( &xyTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( xy.data(), xy.size, xyFile );

    ret = z.Allocate( &zTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( z.data(), z.size, zFile );

    ret = size.Allocate( &sizeTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( size.data(), size.size, sizeFile );

    ret = theta.Allocate( &thetaTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    LoadRaw( theta.data(), theta.size, thetaFile );

    ret = det.Allocate( &detTsProp );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_INFO );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = plrPost.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    det.tensorProps.dims[0] = config.maxNumDetOut;
    ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &det );
    ASSERT_EQ( QC_STATUS_OK, ret );

    PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.data();
    for ( uint32_t i = 0; i < det.tensorProps.dims[0]; i++ )
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
        PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.data();
        for ( uint32_t i = 0; i < det.tensorProps.dims[0]; i++ )
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

    ret = inPts.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = hm.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = xy.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = z.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = size.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = theta.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = det.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
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


static void FadasPlr_E2E_PreProc( QCProcessorType_e processor, Voxelization_Config_t &cfg )
{
    int exist = access( "/tmp/pointcloud.bin", F_OK );
    if ( 0 == exist )
    {
        SANITY_Voxelization( processor, cfg, "/tmp/pointcloud.bin", true );
    }
    else
    {
        printf( "skip E2E_PreProc\n" );
    }
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

TEST( FadasPlr, E2E_CFG0_PreProcCPU )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_CPU, plrPreConfig0 );
}

TEST( FadasPlr, E2E_CFG0_PostProcCPU )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_CPU, plrPostConfig0 );
}

TEST( FadasPlr, E2E_CFG0_PreProcDSP )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_HTP0, plrPreConfig0 );
}

TEST( FadasPlr, E2E_CFG0_PostProcDSP )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_HTP0, plrPostConfig0 );
}

TEST( FadasPlr, E2E_CFG1_PreProcCPU )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_CPU, plrPreConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PostProcCPU )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_CPU, plrPostConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PreProcDSP )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_HTP0, plrPreConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PostProcDSP )
{
    FadasPlr_E2E_PostProc( QC_PROCESSOR_HTP0, plrPostConfig1 );
}

TEST( FadasPlr, L2_Voxelization )
{
    {
        Voxelization plrPre;
        QCStatus_e ret;

        ret = plrPre.Init( "PLR0PRE", nullptr, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        Voxelization_Config_t config = plrPreConfig0;
        config.processor = QC_PROCESSOR_MAX;
        ret = plrPre.Init( "PLR0PRE", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config.processor = QC_PROCESSOR_CPU;
        config.numOutFeatureDim = 0;
        ret = plrPre.Init( "PLR0PRE", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        config.processor = QC_PROCESSOR_HTP0;
        config.numOutFeatureDim = 0;
        ret = plrPre.Init( "PLR0PRE", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = plrPre.Start();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPre.Stop();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPre.Deinit();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
    }

    QCProcessorType_e processors[2] = { QC_PROCESSOR_HTP0, QC_PROCESSOR_CPU };
    for ( int i = 0; i < 2; i++ )
    {
        Voxelization plrPre;
        QCStatus_e ret;
        Voxelization_Config_t config = plrPreConfig0;
        config.processor = processors[i];

        QCTensorProps_t inPtsTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumInPts, config.numInFeatureDim, 0 },
                2,
        };
        QCTensorProps_t outPlrsTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumPlrs, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
                2,
        };
        QCTensorProps_t outFeatureTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumPlrs, config.maxNumPtsPerPlr, config.numOutFeatureDim, 0 },
                3,
        };

        QCSharedBuffer_t inPts;
        QCSharedBuffer_t outPlrs;
        QCSharedBuffer_t outFeature;

        ret = inPts.Allocate( &inPtsTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = outPlrs.Allocate( &outPlrsTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = outFeature.Allocate( &outFeatureTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.RegisterBuffers( &inPts, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
        ret = plrPre.RegisterBuffers( &outPlrs, 1, FADAS_BUF_TYPE_OUT );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );
        ret = plrPre.DeRegisterBuffers( &inPts, 1 );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPre.Execute( &inPts, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPre.Init( "PLR0PRE", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.Init( "PLR0PRE", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPre.RegisterBuffers( &inPts, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = plrPre.RegisterBuffers( &outPlrs, 1, FADAS_BUF_TYPE_OUT );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = plrPre.RegisterBuffers( &outFeature, 1, FADAS_BUF_TYPE_OUT );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.RegisterBuffers( nullptr, 1, FADAS_BUF_TYPE_OUT );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        std::vector<uint8_t> inPtsNonDmaMem;
        inPtsNonDmaMem.resize( inPts.size );
        QCSharedBuffer_t inPtsNonDma = inPts;
        inPtsNonDma.buffer.pData = inPtsNonDmaMem.data();
        ret = plrPre.RegisterBuffers( &inPtsNonDma, 1, FADAS_BUF_TYPE_IN );
        if ( QC_PROCESSOR_CPU == config.processor )
        {
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        else
        {
            ASSERT_EQ( QC_STATUS_FAIL, ret );
        }

        QCSharedBuffer_t inPtsNonTensor = inPts;
        inPtsNonTensor.type = QC_BUFFER_TYPE_RAW;
        ret = plrPre.RegisterBuffers( &inPtsNonTensor, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = plrPre.DeRegisterBuffers( &inPtsNonTensor, 1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = plrPre.DeRegisterBuffers( &inPts, 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = plrPre.DeRegisterBuffers( &outPlrs, 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = plrPre.DeRegisterBuffers( &outFeature, 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.DeRegisterBuffers( nullptr, 1 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = plrPre.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.RegisterBuffers( &inPts, 1, FADAS_BUF_TYPE_IN );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = plrPre.DeRegisterBuffers( &inPts, 1 );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.Execute( nullptr, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPre.Execute( &inPts, nullptr, &outFeature );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
        ret = plrPre.Execute( &inPts, &outPlrs, nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        QCSharedBuffer_t inPtsWrong = inPts;
        inPtsWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPre.Execute( &inPtsWrong, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        inPtsWrong = inPts;
        inPtsWrong.buffer.pData = nullptr;
        ret = plrPre.Execute( &inPtsWrong, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        inPtsWrong = inPts;
        inPtsWrong.tensorProps.numDims = 3;
        ret = plrPre.Execute( &inPtsWrong, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        inPtsWrong = inPts;
        inPtsWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPre.Execute( &inPtsWrong, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        inPtsWrong = inPts;
        inPtsWrong.tensorProps.dims[1] = 35;
        ret = plrPre.Execute( &inPtsWrong, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t outPlrsWrong = outPlrs;
        outPlrsWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPre.Execute( &inPts, &outPlrsWrong, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outPlrsWrong = outPlrs;
        outPlrsWrong.buffer.pData = nullptr;
        ret = plrPre.Execute( &inPts, &outPlrsWrong, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outPlrsWrong = outPlrs;
        outPlrsWrong.tensorProps.numDims = 3;
        ret = plrPre.Execute( &inPts, &outPlrsWrong, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outPlrsWrong = outPlrs;
        outPlrsWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPre.Execute( &inPts, &outPlrsWrong, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outPlrsWrong = outPlrs;
        outPlrsWrong.tensorProps.dims[0] = 16;
        ret = plrPre.Execute( &inPts, &outPlrsWrong, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outPlrsWrong = outPlrs;
        outPlrsWrong.tensorProps.dims[1] = 35;
        ret = plrPre.Execute( &inPts, &outPlrsWrong, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t outFeatureWrong = outFeature;
        outFeatureWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeatureWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outFeatureWrong = outFeature;
        outFeatureWrong.buffer.pData = nullptr;
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeatureWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outFeatureWrong = outFeature;
        outFeatureWrong.tensorProps.numDims = 2;
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeatureWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outFeatureWrong = outFeature;
        outFeatureWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeatureWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outFeatureWrong = outFeature;
        outFeatureWrong.tensorProps.dims[0] = 16;
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeatureWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outFeatureWrong = outFeature;
        outFeatureWrong.tensorProps.dims[1] = 35;
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeatureWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        outFeatureWrong = outFeature;
        outFeatureWrong.tensorProps.dims[2] = 18;
        ret = plrPre.Execute( &inPts, &outPlrs, &outFeatureWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = plrPre.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( FadasPlr, L2_FadasPlrPreProc )
{
    QCProcessorType_e processors[2] = { QC_PROCESSOR_HTP0, QC_PROCESSOR_CPU };
    for ( int i = 0; i < 2; i++ )
    {
        FadasPlrPreProc plrPre;
        QCStatus_e ret;
        Voxelization_Config_t config = plrPreConfig0;

        QCTensorProps_t inPtsTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumInPts, config.numInFeatureDim, 0 },
                2,
        };
        QCTensorProps_t outPlrsTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumPlrs, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
                2,
        };
        QCTensorProps_t outFeatureTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumPlrs, config.maxNumPtsPerPlr, config.numOutFeatureDim, 0 },
                3,
        };

        QCSharedBuffer_t inPts;
        QCSharedBuffer_t outPlrs;
        QCSharedBuffer_t outFeature;

        ret = inPts.Allocate( &inPtsTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = outPlrs.Allocate( &outPlrsTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = outFeature.Allocate( &outFeatureTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.Init( processors[i], "PLRPRE0", LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.PointPillarRun( &inPts, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = plrPre.CreatePreProc();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        config.numOutFeatureDim = 0;
        ret = plrPre.SetParams( config.pillarXSize, config.pillarYSize, config.pillarZSize,
                                config.minXRange, config.minYRange, config.minZRange,
                                config.maxXRange, config.maxYRange, config.maxZRange,
                                config.maxNumInPts, config.numInFeatureDim, config.maxNumPlrs,
                                config.maxNumPtsPerPlr, config.numOutFeatureDim );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.CreatePreProc();
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        ret = plrPre.DestroyPreProc();
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        config = plrPreConfig0;
        ret = plrPre.SetParams( config.pillarXSize, config.pillarYSize, config.pillarZSize,
                                config.minXRange, config.minYRange, config.minZRange,
                                config.maxXRange, config.maxYRange, config.maxZRange,
                                config.maxNumInPts, config.numInFeatureDim, config.maxNumPlrs,
                                config.maxNumPtsPerPlr, config.numOutFeatureDim );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.CreatePreProc();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.PointPillarRun( &inPts, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.PointPillarRun( nullptr, &outPlrs, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = plrPre.PointPillarRun( &inPts, nullptr, &outFeature );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = plrPre.PointPillarRun( &inPts, &outPlrs, nullptr );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = plrPre.DestroyPreProc();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPre.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( FadasPlr, L2_PostCenterPoint )
{
    {
        PostCenterPoint_Config_t config = plrPostConfig0;
        PostCenterPoint plrPost;
        QCStatus_e ret;

        ret = plrPost.Init( "PLRPOST0", nullptr, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config.stride = 0;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config.stride = 3;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config.stride = 4;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

        ret = plrPost.Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );

        config = plrPostConfig0;
        config.processor = QC_PROCESSOR_MAX;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = plrPostConfig0;
        config.bMapPtsToBBox = false;
        config.bBBoxFilter = true;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        config = plrPostConfig1;
        config.pillarXSize = 0;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        config.processor = QC_PROCESSOR_CPU;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_FAIL, ret );
    }

    QCProcessorType_e processors[2] = { QC_PROCESSOR_HTP0, QC_PROCESSOR_CPU };
    for ( int i = 0; i < 2; i++ )
    {
        PostCenterPoint_Config_t config = plrPostConfig0;
        PostCenterPoint plrPost;
        QCStatus_e ret;
        bool labelSelect[config.numClass] = { true };
        for ( int i = 1; i < config.numClass; i++ )
        {
            labelSelect[i] = false;
        }
        PostCenterPoint_3DBBoxFilterParams_t filterParams = { 10,
                                                              config.minXRange,
                                                              config.minYRange,
                                                              plrPreConfig0.minZRange,
                                                              config.maxXRange,
                                                              config.maxYRange,
                                                              plrPreConfig0.maxZRange,
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
        PostCenterPoint plrPost;
        QCStatus_e ret;

        uint32_t numCellsX =
                ( uint32_t )( ( config.maxXRange - config.minXRange ) / config.pillarXSize );
        uint32_t numCellsY =
                ( uint32_t )( ( config.maxYRange - config.minYRange ) / config.pillarYSize );

        uint32_t width = numCellsX / config.stride;
        uint32_t height = numCellsY / config.stride;
        QCTensorProps_t inPtsTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumInPts, config.numInFeatureDim, 0 },
                2,
        };
        QCTensorProps_t hmTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, config.numClass },
                4,
        };
        QCTensorProps_t xyTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 2 },
                4,
        };
        QCTensorProps_t zTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 1 },
                4,
        };
        QCTensorProps_t sizeTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 3 },
                4,
        };
        QCTensorProps_t thetaTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 2 },
                4,
        };
        QCTensorProps_t detTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM },
                2,
        };

        QCSharedBuffer_t inPts;
        QCSharedBuffer_t hm;
        QCSharedBuffer_t xy;
        QCSharedBuffer_t z;
        QCSharedBuffer_t size;
        QCSharedBuffer_t theta;
        QCSharedBuffer_t det;

        ret = inPts.Allocate( &inPtsTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = hm.Allocate( &hmTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = xy.Allocate( &xyTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = z.Allocate( &zTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = size.Allocate( &sizeTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = theta.Allocate( &thetaTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = det.Allocate( &detTsProp );
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
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_FAIL, ret );

        config = plrPostConfig0;
        ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_ERROR );
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
        QCSharedBuffer_t inPtsNonDma = inPts;
        inPtsNonDma.buffer.pData = inPtsNonDmaMem.data();
        ret = plrPost.RegisterBuffers( &inPtsNonDma, 1, FADAS_BUF_TYPE_IN );
        if ( QC_PROCESSOR_CPU == config.processor )
        {
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        else
        {
            ASSERT_EQ( QC_STATUS_FAIL, ret );
        }

        QCSharedBuffer_t inPtsNonTensor = inPts;
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

        QCSharedBuffer_t hmWrong = hm;
        hmWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.buffer.pData = nullptr;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.tensorProps.numDims = 5;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.tensorProps.dims[0] = 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.tensorProps.dims[1] /= 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.tensorProps.dims[2] /= 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        hmWrong = hm;
        hmWrong.tensorProps.dims[3] /= 2;
        ret = plrPost.Execute( &hmWrong, &xy, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t xyWrong = xy;
        xyWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.buffer.pData = nullptr;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.tensorProps.numDims = 5;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.tensorProps.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.tensorProps.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.tensorProps.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        xyWrong = xy;
        xyWrong.tensorProps.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xyWrong, &z, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t zWrong = z;
        zWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.buffer.pData = nullptr;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.tensorProps.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.tensorProps.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.tensorProps.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.tensorProps.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        zWrong = z;
        zWrong.tensorProps.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xy, &zWrong, &size, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t sizeWrong = size;
        sizeWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.buffer.pData = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.tensorProps.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.tensorProps.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.tensorProps.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.tensorProps.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        sizeWrong = size;
        sizeWrong.tensorProps.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &sizeWrong, &theta, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t thetaWrong = theta;
        thetaWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.buffer.pData = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.tensorProps.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.tensorProps.dims[0] = 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.tensorProps.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.tensorProps.dims[2] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        thetaWrong = theta;
        thetaWrong.tensorProps.dims[3] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &thetaWrong, &inPts, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t inPtsWrong = inPts;
        inPtsWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.buffer.pData = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.tensorProps.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        inPtsWrong = inPts;
        inPtsWrong.tensorProps.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPtsWrong, &det );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        QCSharedBuffer_t detWrong = det;
        detWrong.type = QC_BUFFER_TYPE_RAW;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.buffer.pData = nullptr;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.tensorProps.numDims = 5;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.tensorProps.type = QC_TENSOR_TYPE_FLOAT_16;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
        detWrong = det;
        detWrong.tensorProps.dims[1] /= 2;
        ret = plrPost.Execute( &hm, &xy, &z, &size, &theta, &inPts, &detWrong );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    }
}

TEST( FadasPlr, L2_FadasPlrPostProc )
{
    QCProcessorType_e processors[2] = { QC_PROCESSOR_HTP0, QC_PROCESSOR_CPU };
    for ( int i = 0; i < 2; i++ )
    {
        FadasPlrPostProc plrPost;
        QCStatus_e ret;
        PostCenterPoint_Config_t config = plrPostConfig1;
        config.processor = processors[i];

        uint32_t numCellsX =
                ( uint32_t )( ( config.maxXRange - config.minXRange ) / config.pillarXSize );
        uint32_t numCellsY =
                ( uint32_t )( ( config.maxYRange - config.minYRange ) / config.pillarYSize );

        uint32_t width = numCellsX / config.stride;
        uint32_t height = numCellsY / config.stride;
        QCTensorProps_t inPtsTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumInPts, config.numInFeatureDim, 0 },
                2,
        };
        QCTensorProps_t hmTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, config.numClass },
                4,
        };
        QCTensorProps_t xyTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 2 },
                4,
        };
        QCTensorProps_t zTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 1 },
                4,
        };
        QCTensorProps_t sizeTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 3 },
                4,
        };
        QCTensorProps_t thetaTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { 1, height, width, 2 },
                4,
        };
        QCTensorProps_t detTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM },
                2,
        };

        QCSharedBuffer_t inPts;
        QCSharedBuffer_t hm;
        QCSharedBuffer_t xy;
        QCSharedBuffer_t z;
        QCSharedBuffer_t size;
        QCSharedBuffer_t theta;
        QCSharedBuffer_t BBoxList;
        QCSharedBuffer_t labels;
        QCSharedBuffer_t scores;
        QCSharedBuffer_t metadata;

        ret = inPts.Allocate( &inPtsTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = hm.Allocate( &hmTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = xy.Allocate( &xyTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = z.Allocate( &zTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = size.Allocate( &sizeTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = theta.Allocate( &thetaTsProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        {
            QCTensorProps_t tensorProps;
            tensorProps.type = QC_TENSOR_TYPE_FLOAT_32;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.dims[1] = sizeof( FadasCuboidf32_t ) / sizeof( float );
            tensorProps.numDims = 2;
            ret = BBoxList.Allocate( &tensorProps );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        {
            QCTensorProps_t tensorProps;
            tensorProps.type = QC_TENSOR_TYPE_UINT_32;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.numDims = 1;
            ret = labels.Allocate( &tensorProps );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        {
            QCTensorProps_t tensorProps;
            tensorProps.type = QC_TENSOR_TYPE_FLOAT_32;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.numDims = 1;
            ret = scores.Allocate( &tensorProps );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        {
            QCTensorProps_t tensorProps;
            tensorProps.type = QC_TENSOR_TYPE_UINT_8;
            tensorProps.dims[0] = config.maxNumDetOut;
            tensorProps.dims[1] = sizeof( FadasPlr3DBBoxMetadata_t );
            tensorProps.numDims = 2;
            ret = metadata.Allocate( &tensorProps );
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
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif