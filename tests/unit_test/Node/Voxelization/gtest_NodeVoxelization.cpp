// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Node/Voxelization.hpp"
#include "QC/sample/BufferManager.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using namespace QC;
using namespace QC::Node;
using namespace QC::sample;

#define EXPAND_JSON( ... ) #__VA_ARGS__

extern const size_t VOXELIZATION_PILLAR_COORDS_DIM;

std::string g_Config_XYZR = EXPAND_JSON( {
    "static": {
        "name": "voxelization",
        "id": 0,
        "processorType": "cpu",
        "Xsize": 0.16,
        "Ysize": 0.16,
        "Zsize": 4.0,
        "Xmin": 0.0,
        "Ymin": -39.68,
        "Zmin": -3.0,
        "Xmax": 69.12,
        "Ymax": 39.68,
        "Zmax": 1.0,
        "maxPointNum": 300000,
        "maxPlrNum": 12000,
        "maxPointNumPerPlr": 32,
        "inputMode": "xyzr",
        "outputFeatureDimNum": 10,
        "outputPlrBufferIds": [0, 1, 2, 3],
        "outputFeatureBufferIds": [4, 5, 6, 7],
        "plrPointsBufferId": 8,
        "coordToPlrIdxBufferId": 9
    }
} );

std::string g_Config_XYZRT = EXPAND_JSON( {
    "static": {
        "name": "voxelization",
        "id": 0,
        "processorType": "gpu",
        "Xsize": 0.2,
        "Ysize": 0.2,
        "Zsize": 8.0,
        "Xmin": -51.2,
        "Ymin": -51.2,
        "Zmin": -5.0,
        "Xmax": 51.2,
        "Ymax": 51.2,
        "Zmax": 3.0,
        "maxPointNum": 300000,
        "maxPlrNum": 25000,
        "maxPointNumPerPlr": 32,
        "inputMode": "xyzrt",
        "outputFeatureDimNum": 10,
        "outputPlrBufferIds": [0, 1, 2, 3],
        "outputFeatureBufferIds": [4, 5, 6, 7],
        "plrPointsBufferId": 8,
        "coordToPlrIdxBufferId": 9
    }
} );

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

static void SANITY_Voxelization( std::string &jsonStr, std::string &processorType,
                                 std::string &inputMode, const char *pcdFile = nullptr )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    QCNodeInit_t config;
    std::string errors;
    uint32_t globalIdx = 0;

    float Xsize = 0;
    float Ysize = 0;
    float Zsize = 0;
    float Xmin = 0;
    float Ymin = 0;
    float Zmin = 0;
    float Xmax = 0;
    float Ymax = 0;
    float Zmax = 0;
    uint32_t maxPointNum = 0;
    uint32_t maxPlrNum = 0;
    uint32_t maxPointNumPerPlr = 0;
    uint32_t inputFeatureDimNum = 0;
    uint32_t outputFeatureDimNum = 0;
    uint32_t plrPointsBufferId = 0;
    uint32_t coordToPlrIdxBufferId = 0;
    uint32_t gridXSize = 0;
    uint32_t gridYSize = 0;
    QCProcessorType_e processor;

    std::vector<uint32_t> outputPlrBufferIds;
    std::vector<uint32_t> outputFeatureBufferIds;

    TensorDescriptor_t inputTensor;
    TensorDescriptor_t outputPlrTensor;
    TensorDescriptor_t outputFeatureTensor;
    TensorDescriptor_t plrPointsTensor;
    TensorDescriptor_t coordToPlrIdxTensor;

    TensorProps_t inputTensorProp;
    TensorProps_t outputPlrTensorProp;
    TensorProps_t outputFeatureTensorProp;
    TensorProps_t plrPointsTensorProp;
    TensorProps_t coordToPlrIdxTensorProp;

    BufferManager bufMgr = BufferManager( { "VOXEL", QC_NODE_TYPE_VOXEL, 0 } );

    QC::Node::Voxelization voxel;

    ret = dt.Load( jsonStr, errors );
    if ( QC_STATUS_OK == ret )
    {
        dt.Set<std::string>( "static.processorType", processorType );
        dt.Set<std::string>( "static.inputMode", inputMode );
        ret = dt.Get( "static", staticCfg );
    }
    else
    {
        std::cout << "Get config error: " << errors << std::endl;
    }

    if ( QC_STATUS_OK == ret )
    {
        Xsize = staticCfg.Get<float>( "Xsize", 0 );
        Ysize = staticCfg.Get<float>( "Ysize", 0 );
        Zsize = staticCfg.Get<float>( "Zsize", 0 );
        Xmin = staticCfg.Get<float>( "Xmin", 0 );
        Ymin = staticCfg.Get<float>( "Ymin", 0 );
        Zmin = staticCfg.Get<float>( "Zmin", 0 );
        Xmax = staticCfg.Get<float>( "Xmax", 0 );
        Ymax = staticCfg.Get<float>( "Ymax", 0 );
        Zmax = staticCfg.Get<float>( "Zmax", 0 );
        maxPointNum = staticCfg.Get<uint32_t>( "maxPointNum", 0 );
        maxPlrNum = staticCfg.Get<uint32_t>( "maxPlrNum", 0 );
        maxPointNumPerPlr = staticCfg.Get<uint32_t>( "maxPointNumPerPlr", 0 );
        outputFeatureDimNum = staticCfg.Get<uint32_t>( "outputFeatureDimNum", 0 );
        outputPlrBufferIds =
                staticCfg.Get<uint32_t>( "outputPlrBufferIds", std::vector<uint32_t>{} );
        outputFeatureBufferIds =
                staticCfg.Get<uint32_t>( "outputFeatureBufferIds", std::vector<uint32_t>{} );
        plrPointsBufferId = staticCfg.Get<uint32_t>( "plrPointsBufferId", 0 );
        coordToPlrIdxBufferId = staticCfg.Get<uint32_t>( "coordToPlrIdxBufferId", 0 );
        processor = staticCfg.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
    }

    if ( QC_STATUS_OK == ret )
    {
        config = { dt.Dump() };
        std::cout << "config: " << config.config << std::endl;
    }

    if ( inputMode == "xyzr" )
    {
        inputFeatureDimNum = 4;
    }
    else if ( inputMode == "xyzrt" )
    {
        inputFeatureDimNum = 5;
    }

    if ( ( maxPointNum > 0 ) && ( inputFeatureDimNum > 0 ) )
    {
        inputTensorProp.tensorType = QC_TENSOR_TYPE_FLOAT_32;
        inputTensorProp.dims[0] = maxPointNum;
        inputTensorProp.dims[1] = inputFeatureDimNum;
        inputTensorProp.dims[2] = 0;
        inputTensorProp.numDims = 2;
    }

    if ( ( maxPlrNum > 0 ) && ( maxPointNumPerPlr > 0 ) && ( outputFeatureDimNum > 0 ) )
    {
        if ( inputMode == "xyzrt" )
        {
            outputPlrTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
            outputPlrTensorProp.dims[0] = maxPlrNum;
            outputPlrTensorProp.dims[1] = 2;
            outputPlrTensorProp.dims[2] = 0;
            outputPlrTensorProp.numDims = 2;
        }
        else
        {
            outputPlrTensorProp.tensorType = QC_TENSOR_TYPE_FLOAT_32;
            outputPlrTensorProp.dims[0] = maxPlrNum;
            outputPlrTensorProp.dims[1] = VOXELIZATION_PILLAR_COORDS_DIM;
            outputPlrTensorProp.dims[2] = 0;
            outputPlrTensorProp.numDims = 2;
        }

        outputFeatureTensorProp.tensorType = QC_TENSOR_TYPE_FLOAT_32;
        outputFeatureTensorProp.dims[0] = maxPlrNum;
        outputFeatureTensorProp.dims[1] = maxPointNumPerPlr;
        outputFeatureTensorProp.dims[2] = outputFeatureDimNum;
        outputFeatureTensorProp.dims[3] = 0;
        outputFeatureTensorProp.numDims = 3;

        size_t gridXSize = ceil( ( Xmax - Xmin ) / Xsize );
        size_t gridYSize = ceil( ( Ymax - Ymin ) / Ysize );

        plrPointsTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
        plrPointsTensorProp.dims[0] = maxPlrNum + 1;
        plrPointsTensorProp.dims[1] = 0;
        plrPointsTensorProp.numDims = 1;

        coordToPlrIdxTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
        coordToPlrIdxTensorProp.dims[0] = (uint32_t) ( gridXSize * gridYSize * 2 );
        coordToPlrIdxTensorProp.dims[1] = 0;
        coordToPlrIdxTensorProp.numDims = 1;
    }

    const uint32_t inputBufferNum = 4;
    const uint32_t outputPlrBufferNum = outputPlrBufferIds.size();
    const uint32_t outputFeatureBufferNum = outputFeatureBufferIds.size();

    TensorDescriptor_t inputTensors[inputBufferNum];
    TensorDescriptor_t outputPlrTensors[outputPlrBufferNum];
    TensorDescriptor_t outputFeatureTensors[outputFeatureBufferNum];

    NodeFrameDescriptor frameDesc( 3 );

    for ( uint32_t i = 0; i < inputBufferNum; i++ )
    {
        ret = bufMgr.Allocate( inputTensorProp, inputTensors[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );

        uint32_t numPts = 0;
        if ( nullptr == pcdFile )
        {
            if ( nullptr == pcdFile )
            {
                std::cout << "point cloud file is not provided " << std::endl;
            }
        }
        else
        {
            LoadPoints( inputTensors[i].pBuf, inputTensors[i].size, numPts, pcdFile );
            std::cout << "using point cloud file: " << pcdFile << std::endl;
        }
        inputTensors[i].dims[0] = numPts;
    }

    for ( uint32_t i = 0; i < outputPlrBufferNum; i++ )
    {
        ret = bufMgr.Allocate( outputPlrTensorProp, outputPlrTensors[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );

        config.buffers.push_back( outputPlrTensors[i] );
    }

    for ( uint32_t i = 0; i < outputFeatureBufferNum; i++ )
    {
        ret = bufMgr.Allocate( outputFeatureTensorProp, outputFeatureTensors[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );

        config.buffers.push_back( outputFeatureTensors[i] );
    }

    if ( QC_PROCESSOR_GPU == processor )
    {
        ret = bufMgr.Allocate( plrPointsTensorProp, plrPointsTensor );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = bufMgr.Allocate( coordToPlrIdxTensorProp, coordToPlrIdxTensor );
        ASSERT_EQ( QC_STATUS_OK, ret );

        config.buffers.push_back( plrPointsTensor );
        config.buffers.push_back( coordToPlrIdxTensor );
    }

    ret = frameDesc.SetBuffer( 0, inputTensors[0] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = frameDesc.SetBuffer( 1, outputPlrTensors[0] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = frameDesc.SetBuffer( 2, outputFeatureTensors[0] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = voxel.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = voxel.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = voxel.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = voxel.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = voxel.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t i = 0; i < inputBufferNum; i++ )
    {
        ret = bufMgr.Free( inputTensors[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < outputPlrBufferNum; i++ )
    {
        ret = bufMgr.Free( outputPlrTensors[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < outputFeatureBufferNum; i++ )
    {
        ret = bufMgr.Free( outputFeatureTensors[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    if ( QC_PROCESSOR_GPU == processor )
    {
        ret = bufMgr.Free( coordToPlrIdxTensor );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = bufMgr.Free( plrPointsTensor );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( FadasPlr, SANITY_VoxelizationCPU_XYZR )
{
    std::string processorType = "cpu";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/voxelization/pointcloud.bin";
    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationGPU_XYZR )
{
    std::string processorType = "gpu";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/voxelization/pointcloud.bin";
    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationGPU_XYZRT )
{
    std::string processorType = "gpu";
    std::string inputMode = "xyzrt";
    const char *pcdFile = "./data/test/voxelization/pointcloud_XYZRT.bin";
    SANITY_Voxelization( g_Config_XYZRT, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationHTP0_XYZR )
{
    std::string processorType = "htp0";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/voxelization/pointcloud.bin";
    // SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationHTP1_XYZR )
{
    std::string processorType = "htp1";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/voxelization/pointcloud.bin";
    // SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
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

