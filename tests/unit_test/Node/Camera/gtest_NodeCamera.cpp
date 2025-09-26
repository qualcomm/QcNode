// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "QC/Node/Camera.hpp"
#include "QC/sample/SharedBufferPool.hpp"
#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Node;
using namespace QC::sample;

QC::Node::Camera g_camera;
uint32_t g_frameIdx = 0;
std::vector<SharedBufferPool> g_bufferPools;

void ReadJsonFile( const std::string &filePath, nlohmann::json &jsonData )
{
    std::ifstream file( filePath );
    if ( file.is_open() )
    {
        file >> jsonData;
        file.close();
    }
    else
    {
        QC_LOG_ERROR( "Failed to open file %s", filePath );
    }
}

void ProcessDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    QCFrameDescriptorNodeIfs &frameDescIfs = eventInfo.frameDesc;
    QCBufferDescriptorBase_t &bufDesc = frameDescIfs.GetBuffer( 0 );
    QCSharedFrameDescriptorNode frameDesc( 1 );

    const CameraFrameDescriptor_t *pCamFrameDesc =
            dynamic_cast<const CameraFrameDescriptor_t *>( &bufDesc );

    if ( QC_STATUS_OK == ret )
    {
        CameraFrameDescriptor_t camFrameDesc = *pCamFrameDesc;
        ret = frameDesc.SetBuffer( 0, camFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = g_camera.ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        std::cout << "Process Frame index: " << g_frameIdx << std::endl;
        g_frameIdx++;
    }
}

static void SANITY_Camera( DataTree &dt )
{
    QCStatus_e ret;
    DataTree staticCfg;
    std::vector<DataTree> streamConfigs;
    QCNodeInit_t config;

    config = { dt.Dump() };
    std::cout << "config: " << config.config << std::endl;

    config.callback = ProcessDoneCb;

    ret = dt.Get( "static", staticCfg );
    ASSERT_EQ( QC_STATUS_OK, ret );

    std::string name = staticCfg.Get<std::string>( "name", "" );
    ASSERT_EQ( name, "camera" );

    QCNodeID_t nodeId;
    nodeId.name = name;
    nodeId.type = QC_NODE_TYPE_QCX;
    nodeId.id = staticCfg.Get<uint32_t>( "id", UINT32_MAX );
    ASSERT_EQ( nodeId.id, 0 );

    ret = staticCfg.Get( "streamConfigs", streamConfigs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // Allocate buffers
    DataTree streamConfig;
    QCImageProps_t imgProp;
    uint32_t streamId = 0;
    uint32_t bufCnt = 0;
    uint32_t bufferId = 0;
    uint32_t numStream = streamConfigs.size();
    g_bufferPools.resize( numStream );

    for ( uint32_t i = 0; i < numStream; i++ )
    {
        std::string bufPoolName = name + std::to_string( i );
        streamConfig = streamConfigs[i];
        streamId = streamConfig.Get<uint32_t>( "streamId", UINT32_MAX );
        bufCnt = streamConfig.Get<uint32_t>( "bufCnt", UINT32_MAX );
        imgProp.format = streamConfig.GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
        imgProp.width = streamConfig.Get<uint32_t>( "width", UINT32_MAX );
        imgProp.height = streamConfig.Get<uint32_t>( "height", UINT32_MAX );

        if ( ( QC_IMAGE_FORMAT_RGB888 == imgProp.format ) ||
             ( QC_IMAGE_FORMAT_BGR888 == imgProp.format ) )
        {
            imgProp.batchSize = 1;
            imgProp.stride[0] = QC_ALIGN_SIZE( imgProp.width * 3, 16 );
            imgProp.actualHeight[0] = imgProp.height;
            imgProp.numPlanes = 1;
            imgProp.planeBufSize[0] = 0;

            ret = g_bufferPools[i].Init( bufPoolName, nodeId, LOGGER_LEVEL_ERROR, bufCnt, imgProp,
                                         QC_MEMORY_ALLOCATOR_DMA_CAMERA, QC_CACHEABLE );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        else
        {
            ret = g_bufferPools[i].Init( bufPoolName, nodeId, LOGGER_LEVEL_ERROR, bufCnt,
                                         imgProp.width, imgProp.height, imgProp.format,
                                         QC_MEMORY_ALLOCATOR_DMA_CAMERA, QC_CACHEABLE );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }

        ret = g_bufferPools[i].GetBuffers( config.buffers );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = g_camera.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    sleep( 1 );

    ret = g_camera.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = g_camera.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( uint32_t i = 0; i < numStream; i++ )
    {
        ret = g_bufferPools[i].Deinit();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( Camera, SANITY_Camera_IMX728_RequestMode )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    SANITY_Camera( dt );
}

TEST( Camera, SANITY_Camera_OV3F_RequestMode )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    std::vector<DataTree> streamConfigs;
    nlohmann::json jsonData;
    std::string errors;
    std::string filePath = "./data/test/camera/camera_config.json";

    ReadJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    ret = dt.Load( jsonStr, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = dt.Get( "static", staticCfg );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = staticCfg.Get( "streamConfigs", streamConfigs );
    ASSERT_EQ( QC_STATUS_OK, ret );

    staticCfg.Set<uint32_t>( "inputId", 4 );

    for ( uint32_t i = 0; i < streamConfigs.size(); i++ )
    {
        streamConfigs[i].Set<uint32_t>( "width", 1824 );
        streamConfigs[i].Set<uint32_t>( "height", 1536 );
        DataTree streamConfig = streamConfigs[i];
    }
    staticCfg.Set( "streamConfigs", streamConfigs );
    dt.Set( "static", staticCfg );

    SANITY_Camera( dt );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif

