// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/Node/Camera.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using namespace QC::Node;
using namespace QC;

QC::Node::Camera g_camera;
uint32_t g_frameIdx = 0;

void readJsonFile( const std::string &filePath, nlohmann::json &jsonData )
{
    std::ifstream file( filePath );
    if ( file.is_open() )
    {
        file >> jsonData;
        file.close();
    }
    else
    {
        std::cout << "Failed to open file" << filePath << std::endl;
    }
}

void ProcessDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    QCFrameDescriptorNodeIfs &frameDescIfs = eventInfo.frameDesc;
    QCBufferDescriptorBase_t &bufDesc = frameDescIfs.GetBuffer( 0 );
    QCSharedFrameDescriptorNode frameDesc( 1 );

    const QCSharedCameraFrameDescriptor_t *pCamFrameDesc =
            dynamic_cast<const QCSharedCameraFrameDescriptor_t *>( &bufDesc );

    if ( QC_STATUS_OK == ret )
    {
        QCSharedCameraFrameDescriptor_t camFrameDesc = *pCamFrameDesc;
        ret = frameDesc.SetBuffer( 0, camFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = g_camera.ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        std::cout << "Process Frame index: " << g_frameIdx << std::endl;
        g_frameIdx++;
    }
}

TEST( Camera, SANITY_Camera )
{
    std::string filePath = "./data/test/camera/camera_config.json";
    nlohmann::json jsonData;
    readJsonFile( filePath, jsonData );
    std::string jsonStr = jsonData.dump();

    QCStatus_e ret;
    DataTree dt;
    QCNodeInit_t config;
    std::string errors;

    ret = dt.Load( jsonStr, errors );
    if ( QC_STATUS_OK == ret )
    {
        config = { dt.Dump() };
        std::cout << "config: " << config.config << std::endl;
    }

    config.callback = ProcessDoneCb;

    ret = g_camera.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = g_camera.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    sleep( 1 );

    ret = g_camera.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = g_camera.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif

