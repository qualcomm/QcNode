// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <stdio.h>
#include <string>

#include "QC/Node/QNN.hpp"

using namespace QC::Node;
using namespace QC;

#if 1   // defined( __QNXNTO__ )
#define QNN_GTEST_ENABLE_ASYNC
#endif

QCStatus_e ConvertDtToProps( DataTree &dt, QCTensorProps_t &props )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<uint32_t> dims = dt.Get<uint32_t>( "dims", std::vector<uint32_t>( {} ) );

    if ( dims.size() <= QC_NUM_TENSOR_DIMS )
    {
        props.numDims = dims.size();
        std::copy( dims.begin(), dims.end(), props.dims );
    }
    else
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    props.type = dt.GetTensorType( "type", QC_TENSOR_TYPE_MAX );
    if ( QC_TENSOR_TYPE_MAX == props.type )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

TEST( Node, Sanity_QNNLite )
{
    QCStatus_e ret;
    std::string errors;
    QCNodeIfs *pQnn = new Qnn();
    DataTree dt;
    dt.Set<std::string>( "static.name", "QL" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );
    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    QCNodeInit_t config = { dt.Dump() };

    printf( "config: %s\n", config.config.c_str() );

    ret = pQnn->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = pQnn->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    printf( "options: %s\n", options.c_str() );
    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    std::vector<QCTensorProps_t> inputsProps;
    std::vector<QCSharedBufferDescriptor_t> inputs;
    for ( auto &inDt : inputDts )
    {
        QCTensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputsProps.push_back( props );
        QCSharedBufferDescriptor_t buffer;
        ret = buffer.buffer.Allocate( &props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( buffer );
    }
    std::vector<QCTensorProps_t> outputsProps;
    std::vector<QCSharedBufferDescriptor_t> outputs;
    for ( auto &outDt : outputDts )
    {
        QCTensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputsProps.push_back( props );
        QCSharedBufferDescriptor_t buffer;
        ret = buffer.buffer.Allocate( &props );
        outputs.push_back( buffer );
    }

    DataTree dtp;
    dtp.Set<bool>( "dynamic.enablePerf", true );
    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pQnn->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputs.size() + outputs.size() + 1 );
    uint32_t globalIdx = 0;
    for ( auto &buffer : inputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    for ( auto &buffer : outputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = pQnn->ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );


    QCNodeMonitoringIfs &monitorIfs = pQnn->GetMonitoringIfs();
    QnnRuntime_Perf_t perf;
    uint32_t size = sizeof( perf );

    ret = monitorIfs.Place( &perf, size );
    ASSERT_EQ( QC_STATUS_OK, ret );
    printf( "perf(us): QNN=%" PRIu64 " RPC=%" PRIu64 " QNN ACC=%" PRIu64 " ACC=%" PRIu64 "\n",
            perf.entireExecTime, perf.rpcExecTimeCPU, perf.rpcExecTimeHTP, perf.rpcExecTimeAcc );

    ret = pQnn->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pQnn->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto &buffer : inputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &buffer : outputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    reinterpret_cast<Qnn *>( pQnn )->~Qnn();
}

#ifdef QNN_GTEST_ENABLE_ASYNC
TEST( Node, Sanity_QNNLiteAsync )
{
    QCStatus_e ret;
    std::string errors;
    uint64_t counter = 0;
    std::mutex mtx;
    std::condition_variable condVar;
    QCFrameDescriptorNodeIfs *pFrameDesc = nullptr;
    uint32_t numIO = 0;
    QCNodeIfs *pQnn = new Qnn();
    DataTree dt;
    dt.Set<std::string>( "static.name", "QL" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );
    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    QCNodeInit_t config = {
            dt.Dump(),
            QCNodeEventCallBack_t(
                    [&condVar, &counter, &pFrameDesc, &numIO]( const QCNodeEventInfo_t &info ) {
                        if ( QC_STATUS_OK == info.status )
                        {
                            counter++;
                        }
                        else
                        { /* for example, if user know that error is in slot 5 */
                            QCBufferDescriptorBase &bufDesc = info.frameDesc.GetBuffer( 5 );
                            Qnn_NotifyStatus_t *pNotifyStatus =
                                    static_cast<Qnn_NotifyStatus_t *>( bufDesc.pBuf );
                            (void) pNotifyStatus;
                        }
                        QCFrameDescriptorNodeIfs &frameDesc = info.frameDesc;
                        for ( uint32_t i = 0; i < numIO; i++ )
                        {
                            QCBufferDescriptorBase_t &bufDescOut = frameDesc.GetBuffer( i );
                            QCBufferDescriptorBase_t &bufDescIn = pFrameDesc->GetBuffer( i );
                            ASSERT_EQ( &bufDescIn, &bufDescOut );
                        }
                        condVar.notify_one();
                    } ),
    };

    ret = pQnn->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = pQnn->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    printf( "options: %s\n", options.c_str() );
    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    std::vector<QCTensorProps_t> inputsProps;
    std::vector<QCSharedBufferDescriptor_t> inputs;
    for ( auto &inDt : inputDts )
    {
        QCTensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputsProps.push_back( props );
        QCSharedBufferDescriptor_t buffer;
        ret = buffer.buffer.Allocate( &props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( buffer );
    }
    std::vector<QCTensorProps_t> outputsProps;
    std::vector<QCSharedBufferDescriptor_t> outputs;
    for ( auto &outDt : outputDts )
    {
        QCTensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputsProps.push_back( props );
        QCSharedBufferDescriptor_t buffer;
        ret = buffer.buffer.Allocate( &props );
        outputs.push_back( buffer );
    }

    DataTree dtp;
    dtp.Set<bool>( "dynamic.enablePerf", true );
    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pQnn->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    numIO = inputs.size() + outputs.size();
    QCSharedFrameDescriptorNode frameDesc( numIO + 1 );
    pFrameDesc = &frameDesc;
    uint32_t globalIdx = 0;
    for ( auto &buffer : inputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    for ( auto &buffer : outputs )
    {
        ret = frameDesc.SetBuffer( globalIdx, buffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    // Note: the life cycle of frameDesc must last till the callback
    ret = pQnn->ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    {
        std::unique_lock<std::mutex> lock( mtx );
        auto status = condVar.wait_for( lock, std::chrono::milliseconds( 1000 ) );
        ASSERT_EQ( std::cv_status::no_timeout, status );
        ASSERT_EQ( 1, counter );
    }

    ret = pQnn->Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = pQnn->DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( auto &buffer : inputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &buffer : outputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    reinterpret_cast<Qnn *>( pQnn )->~Qnn();
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