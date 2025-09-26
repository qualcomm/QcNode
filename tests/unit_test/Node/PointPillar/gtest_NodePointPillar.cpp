// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/Node/Voxelization.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using namespace QC::Node;
using namespace QC;

#define EXPAND_JSON( ... ) #__VA_ARGS__

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
        "inputBufferIds": [0, 1, 2, 3],
        "outputPlrBufferIds": [4, 5, 6, 7],
        "outputFeatureBufferIds": [8, 9, 10, 11]
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
        "inputBufferIds": [0, 1, 2, 3],
        "outputPlrBufferIds": [4, 5, 6, 7],
        "outputFeatureBufferIds": [8, 9, 10, 11]
    }
} );

QCSharedBufferDescriptor_t g_inputBuffers[QC_MAX_INPUTS];
QCSharedBufferDescriptor_t g_outputPlrBuffers[QC_MAX_INPUTS];
QCSharedBufferDescriptor_t g_outputFeatureBuffers[QC_MAX_INPUTS];

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

    uint32_t maxPointNum = 0;
    uint32_t maxPlrNum = 0;
    uint32_t maxPointNumPerPlr = 0;
    uint32_t inputFeatureDimNum = 0;
    uint32_t outputFeatureDimNum = 0;

    std::vector<uint32_t> inputBufferIds;
    std::vector<uint32_t> outputPlrBufferIds;
    std::vector<uint32_t> outputFeatureBufferIds;

    QCTensorProps_t inputTensorProp;
    QCTensorProps_t outputPlrTensorProp;
    QCTensorProps_t outputFeatureTensorProp;

    uint32_t inputBufferId = 0;
    uint32_t outputPlrBufferId = 6;
    uint32_t outputFeatureBufferId = 9;

    std::vector<QCSharedBufferDescriptor_t> inputBuffers;
    std::vector<QCSharedBufferDescriptor_t> outputPlrBuffers;
    std::vector<QCSharedBufferDescriptor_t> outputFeatureBuffers;

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
        maxPointNum = staticCfg.Get<uint32_t>( "maxPointNum", 0 );
        maxPlrNum = staticCfg.Get<uint32_t>( "maxPlrNum", 0 );
        maxPointNumPerPlr = staticCfg.Get<uint32_t>( "maxPointNumPerPlr", 0 );
        outputFeatureDimNum = staticCfg.Get<uint32_t>( "outputFeatureDimNum", 0 );
        inputBufferIds = staticCfg.Get<uint32_t>( "inputBufferIds", std::vector<uint32_t>{} );
        outputPlrBufferIds =
                staticCfg.Get<uint32_t>( "outputPlrBufferIds", std::vector<uint32_t>{} );
        outputFeatureBufferIds =
                staticCfg.Get<uint32_t>( "outputFeatureBufferIds", std::vector<uint32_t>{} );
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
        inputTensorProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { maxPointNum, inputFeatureDimNum, 0 },
                2,
        };
    }

    if ( ( maxPlrNum > 0 ) && ( maxPointNumPerPlr > 0 ) && ( outputFeatureDimNum > 0 ) )
    {
        if ( inputMode == "xyzrt" )
        {
            outputPlrTensorProp = {
                    QC_TENSOR_TYPE_INT_32,
                    { maxPlrNum, 2, 0 },
                    2,
            };
        }
        else
        {
            outputPlrTensorProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { maxPlrNum, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
                    2,
            };
        }

        outputFeatureTensorProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { maxPlrNum, maxPointNumPerPlr, outputFeatureDimNum, 0 },
                3,
        };
    }

    const uint32_t inputBufferNum = inputBufferIds.size();
    const uint32_t outputPlrBufferNum = outputPlrBufferIds.size();
    const uint32_t outputFeatureBufferNum = outputFeatureBufferIds.size();
    const uint32_t bufferNum = inputBufferNum + outputPlrBufferNum + outputFeatureBufferNum;
    QCSharedBufferDescriptor_t sharedBuffers[bufferNum];
    QCSharedFrameDescriptorNode frameDesc( 3 );

    for ( uint32_t i = 0; i < inputBufferNum; i++ )
    {
        uint32_t bufferIdx = inputBufferIds[i];
        ret = sharedBuffers[bufferIdx].buffer.Allocate( &inputTensorProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        if ( bufferIdx == inputBufferId )
        {
            uint32_t numPts = 0;
            if ( nullptr == pcdFile )
            {
                numPts = ( maxPointNum / 3 ) + rand() % ( 2 * maxPointNum / 3 );
                RandomGenPoints( (float *) sharedBuffers[bufferIdx].buffer.data(), numPts );
            }
            else
            {
                LoadPoints( sharedBuffers[bufferIdx].buffer.data(),
                            sharedBuffers[bufferIdx].buffer.size, numPts, pcdFile );
            }
            sharedBuffers[bufferIdx].buffer.tensorProps.dims[0] = numPts;
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        config.buffers.push_back( sharedBuffers[bufferIdx] );
    }

    for ( uint32_t i = 0; i < outputPlrBufferNum; i++ )
    {
        uint32_t bufferIdx = outputPlrBufferIds[i];
        ret = sharedBuffers[bufferIdx].buffer.Allocate( &outputPlrTensorProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        config.buffers.push_back( sharedBuffers[bufferIdx] );
    }

    for ( uint32_t i = 0; i < outputFeatureBufferNum; i++ )
    {
        uint32_t bufferIdx = outputFeatureBufferIds[i];
        ret = sharedBuffers[bufferIdx].buffer.Allocate( &outputFeatureTensorProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        config.buffers.push_back( sharedBuffers[bufferIdx] );
    }

    ret = frameDesc.SetBuffer( 0, sharedBuffers[inputBufferId] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = frameDesc.SetBuffer( 1, sharedBuffers[outputPlrBufferId] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = frameDesc.SetBuffer( 2, sharedBuffers[outputFeatureBufferId] );
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
        uint32_t bufferIdx = inputBufferIds[i];
        ret = sharedBuffers[bufferIdx].buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < outputPlrBufferNum; i++ )
    {
        uint32_t bufferIdx = outputPlrBufferIds[i];
        ret = sharedBuffers[bufferIdx].buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( uint32_t i = 0; i < outputPlrBufferNum; i++ )
    {
        uint32_t bufferIdx = outputFeatureBufferIds[i];
        ret = sharedBuffers[bufferIdx].buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( FadasPlr, SANITY_VoxelizationCPU_XYZR )
{
    std::string processorType = "cpu";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/pointpillar/pointcloud.bin";

    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode );
    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationGPU_XYZR )
{
    std::string processorType = "gpu";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/pointpillar/pointcloud.bin";

    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode );
    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationHTP0_XYZR )
{
    std::string processorType = "htp0";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/pointpillar/pointcloud.bin";

    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode );
    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationHTP1_XYZR )
{
    std::string processorType = "htp1";
    std::string inputMode = "xyzr";
    const char *pcdFile = "./data/test/pointpillar/pointcloud.bin";

    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode );
    SANITY_Voxelization( g_Config_XYZR, processorType, inputMode, pcdFile );
}

TEST( FadasPlr, SANITY_VoxelizationGPU_XYZRT )
{
    std::string processorType = "gpu";
    std::string inputMode = "xyzrt";
    const char *pcdFile = "./data/test/pointpillar/pointcloud_XYZRT.bin";

    SANITY_Voxelization( g_Config_XYZRT, processorType, inputMode );
    SANITY_Voxelization( g_Config_XYZRT, processorType, inputMode, pcdFile );
}


#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif

