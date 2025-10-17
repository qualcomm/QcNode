// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <string>

#include "QC/Common/DataTree.hpp"
#include "QC/Node/NodeBase.hpp"
#include "QC/sample/BufferManager.hpp"

using namespace QC::Node;
using namespace QC::sample;
using namespace QC;

TEST( NodeBase, Sanity_NodeFrameDescriptor )
{
    BufferManager bufMgr = BufferManager( { "TEST", QC_NODE_TYPE_CUSTOM_0, 0 } );
    QCStatus_e ret;
    ImageDescriptor_t buffer;
    NodeFrameDescriptor *pFrameDesc;

    ret = bufMgr.Allocate( ImageBasicProps_t( 1920, 1080, QC_IMAGE_FORMAT_NV12 ), buffer );
    ASSERT_EQ( QC_STATUS_OK, ret );

    pFrameDesc = new NodeFrameDescriptor( 1 );

    QCBufferDescriptorBase_t &bufDescNull = pFrameDesc->GetBuffer( 0 );
    BufferDescriptor_t *pDescCast = dynamic_cast<BufferDescriptor_t *>( &bufDescNull );
    ASSERT_EQ( nullptr, pDescCast );

    ret = (QCStatus_e) pFrameDesc->SetBuffer( 0, buffer );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = (QCStatus_e) pFrameDesc->SetBuffer( 1, buffer );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );

    QCBufferDescriptorBase_t &bufDesc = pFrameDesc->GetBuffer( 0 );
    pDescCast = dynamic_cast<BufferDescriptor_t *>( &bufDesc );
    ASSERT_EQ( &buffer, pDescCast );

    QCBufferDescriptorBase_t &bufDescOV = pFrameDesc->GetBuffer( 1 );
    pDescCast = dynamic_cast<BufferDescriptor_t *>( &bufDescOV );
    ASSERT_EQ( nullptr, pDescCast );

    ImageDescriptor_t *pImgDescCast = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
    ASSERT_EQ( &buffer, pImgDescCast );

    NodeFrameDescriptor Fd2( 1 );
    pImgDescCast = dynamic_cast<ImageDescriptor_t *>( &Fd2.GetBuffer( 0 ) );
    ASSERT_EQ( nullptr, pImgDescCast );

    Fd2 = *pFrameDesc;

    pImgDescCast = dynamic_cast<ImageDescriptor_t *>( &Fd2.GetBuffer( 0 ) );
    ASSERT_EQ( &buffer, pImgDescCast );

    Fd2.Clear();
    pImgDescCast = dynamic_cast<ImageDescriptor_t *>( &Fd2.GetBuffer( 0 ) );
    ASSERT_EQ( nullptr, pImgDescCast );

    Fd2 = dynamic_cast<QCFrameDescriptorNodeIfs &>( *pFrameDesc );

    pImgDescCast = dynamic_cast<ImageDescriptor_t *>( &Fd2.GetBuffer( 0 ) );
    ASSERT_EQ( &buffer, pImgDescCast );


    delete pFrameDesc;

    pFrameDesc = &Fd2;

    Fd2 = *pFrameDesc;

    Fd2 = dynamic_cast<QCFrameDescriptorNodeIfs &>( *pFrameDesc );

    ret = bufMgr.Free( buffer );
    ASSERT_EQ( QC_STATUS_OK, ret );
}

class NodeConfigTest : public NodeConfigBase
{
public:
    NodeConfigTest( Logger &logger ) : NodeConfigBase( logger ) {};
    ~NodeConfigTest() {}
    const std::string &GetOptions() { return m_options; }
    const QCNodeConfigBase_t &Get() { return m_configBase; };

private:
    std::string m_options;
    QCNodeConfigBase_t m_configBase;
};

TEST( NodeBase, Sanity_NodeConfigBase )
{
    QCStatus_e status;
    std::string jsStr;
    std::string errors;

    {
        Logger logger;
        NodeConfigTest configTest( logger );

        status = configTest.VerifyAndSet( R"(xxxx)", errors );   // invalid json string
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = configTest.VerifyAndSet( R"({"static":{"name": ""}})", errors );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        ASSERT_EQ( errors, "no name specified, " );

        jsStr = R"({"static":{"name": "TEST", "logLevel":"Xxxx"}})";
        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
        ASSERT_EQ( errors, "invalid logLevel:<Xxxx>, " );

        jsStr = R"({"static":{"name": "TEST", "logLevel":"ERROR"}})";
        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        Logger logger;
        NodeConfigTest configTest( logger );
        jsStr = R"({"static":{"name": "TEST", "logLevel":"VERBOSE"}})";
        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        Logger logger;
        NodeConfigTest configTest( logger );
        jsStr = R"({"static":{"name": "TEST", "logLevel":"DEBUG"}})";
        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        Logger logger;
        NodeConfigTest configTest( logger );
        jsStr = R"({"static":{"name": "TEST", "logLevel":"INFO"}})";
        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        Logger logger;
        NodeConfigTest configTest( logger );
        jsStr = R"({"static":{"name": "TEST", "logLevel":"WARN"}})";
        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        Logger logger;
        logger.Init( "TEST" );
        NodeConfigTest configTest( logger );
        jsStr = R"({"static":{"name": "TEST", "logLevel":"WARN"}})";
        status = configTest.VerifyAndSet( jsStr, errors );
        ASSERT_EQ( QC_STATUS_BAD_STATE, status );
    }
}

class MonitorTest : public QCNodeMonitoringIfs
{
public:
    MonitorTest( Logger &logger ) : m_logger( logger ) {}
    ~MonitorTest() {}
    MonitorTest( const MonitorTest &other ) = delete;
    QCStatus_e VerifyAndSet( const std::string config, std::string &errors )
    {
        return QC_STATUS_OK;
    }
    const std::string &GetOptions() { return m_options; }
    const QCNodeMonitoringBase_t &Get() { return m_config; }
    uint32_t GetMaximalSize() { return 0; }
    uint32_t GetCurrentSize() { return 0; }
    QCStatus_e Place( void *pData, uint32_t &size ) { return QC_STATUS_OK; }

private:
    Logger &m_logger;
    std::string m_options;
    QCNodeMonitoringBase_t m_config;
};


class NodeBaseTest : public NodeBase
{
public:
    NodeBaseTest() : m_config( m_logger ), m_monitor( m_logger ) {};
    ~NodeBaseTest() {};

    QCStatus_e Initialize( QCNodeInit_t &config ) { return QC_STATUS_OK; }

    QCNodeConfigIfs &GetConfigurationIfs() { return m_config; }

    QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitor; }
    QCStatus_e Start() { return QC_STATUS_OK; }
    QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
    {
        return QC_STATUS_OK;
    }
    QCStatus_e Stop() { return QC_STATUS_OK; }
    QCStatus_e DeInitialize() { return NodeBase::DeInitialize(); }

    QCObjectState_e GetState() { return QC_OBJECT_STATE_INITIAL; }


    QCStatus_e Init( QCNodeID_t nodeId, Logger_Level_e level = LOGGER_LEVEL_ERROR )
    {
        return NodeBase::Init( nodeId, level );
    }

private:
    NodeConfigTest m_config;
    MonitorTest m_monitor;
};


TEST( NodeBase, Sanity_NodeBase )
{
    QCStatus_e status;
    {
        NodeBaseTest node;
        status = node.Init( { "TEST", QC_NODE_TYPE_CUSTOM_0, 0 } );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = node.Init( { "TEST", QC_NODE_TYPE_CUSTOM_0, 0 } );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = node.DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, status );

        status = node.Init( { "TEST", QC_NODE_TYPE_CUSTOM_0, 0 } );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = node.DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, status );
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