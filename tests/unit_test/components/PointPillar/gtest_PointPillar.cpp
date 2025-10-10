// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "QC/component/Voxelization.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <cstdlib>
#include <stdio.h>
#include <unistd.h>

using namespace QC;
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

TEST( FadasPlr, E2E_CFG0_PreProcCPU )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_CPU, plrPreConfig0 );
}

TEST( FadasPlr, E2E_CFG0_PreProcDSP )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_HTP0, plrPreConfig0 );
}

TEST( FadasPlr, E2E_CFG1_PreProcCPU )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_CPU, plrPreConfig1 );
}

TEST( FadasPlr, E2E_CFG1_PreProcDSP )
{
    FadasPlr_E2E_PreProc( QC_PROCESSOR_HTP0, plrPreConfig1 );
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


#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif