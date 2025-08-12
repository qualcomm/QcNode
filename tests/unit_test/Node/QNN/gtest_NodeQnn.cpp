// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <string>
#include <thread>

#include "QC/Node/QNN.hpp"
#include "QC/sample/BufferManager.hpp"
#include "accuracy.hpp"
#include "md5_utils.hpp"
#include "gtest/gtest.h"

#include "QnnImpl.hpp"


using namespace QC;
using namespace QC::Node;
using namespace QC::test::utils;
using namespace QC::Memory;
using namespace QC::sample;

#if defined( __QNXNTO__ )
#define QNN_GTEST_ENABLE_ASYNC
#endif


#ifdef QNN_GTEST_ENABLE_ASYNC
static QCFrameDescriptorNodeIfs *s_frameDesc;
static std::condition_variable s_condVar;
static uint64_t s_counter = 0;
#endif

static void *LoadRaw( std::string path, size_t &size )
{
    void *pOut = nullptr;
    FILE *pFile = fopen( path.c_str(), "rb" );

    if ( nullptr != pFile )
    {
        fseek( pFile, 0, SEEK_END );
        size = ftell( pFile );
        fseek( pFile, 0, SEEK_SET );
        pOut = malloc( size );
        if ( nullptr != pOut )
        {
            auto readSize = fread( pOut, 1, size, pFile );
            (void) readSize;
        }
        fclose( pFile );
        printf( "load raw %s %d\n", path.c_str(), (int) size );
    }
    else
    {
        printf( "no raw file %s\n", path.c_str() );
    }

    return pOut;
}

#ifdef QNN_GTEST_ENABLE_ASYNC
static void Qnn_EventCallback( const QCNodeEventInfo_t &info )
{
    if ( QC_STATUS_OK == info.status )
    {
        s_counter++;
        s_frameDesc = &info.frameDesc;
        s_condVar.notify_one();
    }
    else
    {
        s_counter = 0xdeadbeef;
        s_condVar.notify_one();
    }
}
#endif

QCStatus_e ConvertDtToProps( DataTree &dt, TensorProps_t &props )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::vector<uint32_t> dims = dt.Get<uint32_t>( "dims", std::vector<uint32_t>( {} ) );

    if ( dims.size() <= QC_NUM_TENSOR_DIMS )
    {
        props.numDims = dims.size();
        std::copy( dims.begin(), dims.end(), props.dims );
    }
    else
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    props.tensorType = dt.GetTensorType( "type", QC_TENSOR_TYPE_MAX );
    if ( QC_TENSOR_TYPE_MAX == props.tensorType )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

class QnnTest : public ::testing::Test
{
protected:
    void SetUp() override {}

    void SetLogLevel( std::string level ) { dt.Set<std::string>( "static.logLevel", level ); }

    void Init( std::string name, std::string loadType, std::string modelPath,
               std::string processorType )
    {
        dt.Set<std::string>( "static.name", name );
        dt.Set<uint32_t>( "static.id", 0 );
        dt.Set<std::string>( "static.loadType", loadType );
        dt.Set<std::string>( "static.modelPath", modelPath );
        dt.Set<std::string>( "static.processorType", processorType );

        config.buffers.clear();
        config.callback = nullptr;

        if ( "buffer" == loadType )
        {
            dt.Set<uint32_t>( "static.contextBufferId", 0 );
            FILE *pFile = fopen( modelPath.c_str(), "rb" );
            ASSERT_NE( pFile, nullptr );
            fseek( pFile, 0, SEEK_END );
            bufferSize = ftell( pFile );
            fseek( pFile, 0, SEEK_SET );
            buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize] );
            auto readSize = fread( buffer.get(), 1, bufferSize, pFile );
            ASSERT_EQ( readSize, bufferSize );
            fclose( pFile );
            ctxBufDesc.name = modelPath;
            ctxBufDesc.pBuf = buffer.get();
            ctxBufDesc.size = bufferSize;
            config.buffers.push_back( ctxBufDesc );
        }

        config.config = dt.Dump();
        ret = qnn.Initialize( config );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    void AllocateBuffers()
    {
        QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
        const std::string &options = cfgIfs.GetOptions();
        // printf( "options: %s\n", options.c_str() );
        DataTree optionsDt;

        std::vector<DataTree> inputDts;
        std::vector<DataTree> outputDts;
        ret = optionsDt.Load( options, errors );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = optionsDt.Get( "model.inputs", inputDts );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = optionsDt.Get( "model.outputs", outputDts );
        ASSERT_EQ( QC_STATUS_OK, ret );

        pFrameDesc = new QCSharedFrameDescriptorNode( inputDts.size() + outputDts.size() + 1 );
        uint32_t globalIdx = 0;
        inputs.reserve( inputDts.size() );
        for ( auto &inDt : inputDts )
        {
            TensorProps_t props;
            ret = ConvertDtToProps( inDt, props );
            ASSERT_EQ( QC_STATUS_OK, ret );
            TensorDescriptor_t tensorDesc;
            ret = bufMgr.Allocate( props, tensorDesc );
            ASSERT_EQ( QC_STATUS_OK, ret );
            inputs.push_back( tensorDesc );
            ret = pFrameDesc->SetBuffer( globalIdx, inputs.back() );
            ASSERT_EQ( QC_STATUS_OK, ret );
            globalIdx++;
        }
        outputs.reserve( outputDts.size() );
        for ( auto &outDt : outputDts )
        {
            TensorProps_t props;
            ret = ConvertDtToProps( outDt, props );
            ASSERT_EQ( QC_STATUS_OK, ret );
            TensorDescriptor_t tensorDesc;
            ret = bufMgr.Allocate( props, tensorDesc );
            ASSERT_EQ( QC_STATUS_OK, ret );
            outputs.push_back( tensorDesc );
            ret = pFrameDesc->SetBuffer( globalIdx, outputs.back() );
            ASSERT_EQ( QC_STATUS_OK, ret );
            globalIdx++;
        }
    }

    void Start()
    {
        ret = qnn.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    void Execute()
    {
        ret = qnn.ProcessFrameDescriptor( *pFrameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    void Stop()
    {
        ret = qnn.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    void Deinit()
    {
        ret = qnn.DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    void TearDown() override
    {
        (void) qnn.Stop();
        (void) qnn.DeInitialize();
        if ( nullptr != pFrameDesc )
        {
            delete pFrameDesc;
            pFrameDesc = nullptr;
        }

        for ( size_t i = 0; i < inputs.size(); ++i )
        {
            ret = bufMgr.Free( inputs[i] );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        inputs.clear();
        for ( size_t i = 0; i < outputs.size(); ++i )
        {
            ret = bufMgr.Free( outputs[i] );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        outputs.clear();
    }

    Qnn qnn;
    QCNodeInit_t config;
    BufferManager bufMgr = BufferManager( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    std::string errors;
    QCStatus_e ret = QC_STATUS_OK;
    DataTree dt;

    std::vector<TensorDescriptor_t> inputs;
    std::vector<TensorDescriptor_t> outputs;
    QCSharedFrameDescriptorNode *pFrameDesc = nullptr;

    /* use to load from context buffer */
    std::shared_ptr<uint8_t> buffer = nullptr;
    uint64_t bufferSize{ 0 };
    QCBufferDescriptorBase ctxBufDesc;
};

TEST_F( QnnTest, SANITY_General )
{
    Init( "SANITY", "binary", "data/centernet/program.bin", "htp0" );
    AllocateBuffers();
    Start();
    Execute();
    Stop();
    Deinit();
}

TEST_F( QnnTest, CreateModelFromBuffer )
{
    Init( "MODEL_FROM_BUF", "buffer", "data/centernet/program.bin", "htp0" );
    AllocateBuffers();
    Start();
    Execute();
    Stop();
    Deinit();

    // buffer is null
    ctxBufDesc.pBuf = nullptr;
    ctxBufDesc.size = bufferSize;
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // buffer size is 0
    ctxBufDesc.pBuf = buffer.get();
    ctxBufDesc.size = 0;
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // buffer size is incorrect
    ctxBufDesc.pBuf = buffer.get();
    ctxBufDesc.size = 8;
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_FAIL, ret );
}

TEST_F( QnnTest, Perf )
{
    Init( "PERF", "binary", "data/centernet/program.bin", "htp0" );
    AllocateBuffers();

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    DataTree dtp;
    dtp.Set<bool>( "dynamic.enablePerf", true );
    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_ALREADY, ret );

    Start();

    Qnn_Perf_t perf = { 0, 0, 0, 0 };
    QCNodeMonitoringIfs &monitorIfs = qnn.GetMonitoringIfs();
    uint32_t size = sizeof( perf );
    ret = monitorIfs.Place( &perf, size );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );

    Execute();

    uint32_t size0 = 0u;
    ret = monitorIfs.Place( &perf, size0 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = monitorIfs.Place( &perf, size );
    ASSERT_EQ( QC_STATUS_OK, ret );

    printf( "perf(us): QNN=%" PRIu64 " RPC=%" PRIu64 " QNN ACC=%" PRIu64 " ACC=%" PRIu64 "\n",
            perf.entireExecTime, perf.rpcExecTimeCPU, perf.rpcExecTimeHTP, perf.rpcExecTimeAcc );

    dtp.Set<bool>( "dynamic.enablePerf", false );
    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    Execute();

    ret = monitorIfs.Place( &perf, size );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );

    Stop();
    Deinit();
}

TEST_F( QnnTest, RegisterBuffer )
{
    Init( "MODEL_FROM_BUF", "binary", "data/centernet/program.bin", "htp0" );
    AllocateBuffers();
    Start();
    Execute();
    Stop();
    Deinit();

    uint32_t globalIdx = 0;
    std::vector<uint32_t> bufferIds;
    for ( auto &bufDesc : inputs )
    {
        config.buffers.push_back( bufDesc );
        bufferIds.push_back( globalIdx );
        globalIdx++;
    }
    for ( auto &bufDesc : outputs )
    {
        config.buffers.push_back( bufDesc );
        bufferIds.push_back( globalIdx );
        globalIdx++;
    }

    dt.Set<uint32_t>( "static.bufferIds", bufferIds );
    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    Start();
    Execute();
    Stop();
    Deinit();

    // give an invalid buffer id
    bufferIds.push_back( bufferIds.size() + 5 );
    dt.Set<uint32_t>( "static.bufferIds", bufferIds );
    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
}

TEST( QNN, LoadModel )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    DataTree dt;
    dt.Set<std::string>( "static.name", "LOAD_MODEL" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    QCNodeInit_t config;
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.modelPath", "data/noexistingfile.bin" );
    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    dt.Set<std::string>( "static.modelPath", "data/centernet/zero_buffer_size.bin" );
    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.loadType", "library" );
    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_FAIL, ret );
}

TEST_F( QnnTest, StateMachine )
{
    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();
    ASSERT_EQ( "{}", options );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    DataTree dtp;
    dtp.Set<bool>( "dynamic.enablePerf", true );
    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    Qnn_Perf_t perf = { 0, 0, 0, 0 };
    QCNodeMonitoringIfs &monitorIfs = qnn.GetMonitoringIfs();
    uint32_t size = sizeof( perf );
    ret = monitorIfs.Place( &perf, size );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    QCObjectState_e state = qnn.GetState();
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, state );

    Init( "StateMachine", "binary", "data/centernet/program.bin", "htp0" );
    state = qnn.GetState();
    ASSERT_EQ( QC_OBJECT_STATE_READY, state );

    AllocateBuffers();

    ret = qnn.ProcessFrameDescriptor( *pFrameDesc );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

    Start();
    state = qnn.GetState();
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, state );

    Stop();
    state = qnn.GetState();
    ASSERT_EQ( QC_OBJECT_STATE_READY, state );

    Deinit();
    state = qnn.GetState();
    ASSERT_EQ( QC_OBJECT_STATE_INITIAL, state );
}

TEST( QNN, LoadOpPackage )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "OP_PKG" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/bevdet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );
    std::vector<DataTree> udoPkgs;

    // empty udoPackages;
    dt.Set( "static.udoPackages", udoPkgs );
    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    // invalid so
    DataTree udt;
    udt.Set<std::string>( "udoLibPath", "invalid.so" );
    udt.Set<std::string>( "interfaceProvider", "AutoAiswOpPackageInterfaceProvider" );
    udoPkgs.push_back( udt );
    dt.Set( "static.udoPackages", udoPkgs );
    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    udt.Set<std::string>( "udoLibPath", "libQnnAutoAiswOpPackage.so" );
    udt.Set<std::string>( "interfaceProvider", "AutoAiswOpPackageInterfaceProvider" );
    udoPkgs.clear();
    udoPkgs.push_back( udt );
    dt.Set( "static.udoPackages", udoPkgs );
    config.config = dt.Dump();
    printf( "config: %s\n", config.config.c_str() );
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
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

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( QNN, DataType )
{
    QCNodeID_t nodeId;
    Logger logger;
    logger.Init( "QNN_DATA_TYPE" );
    QnnImpl qnn( nodeId, logger );
    EXPECT_EQ( QNN_DATATYPE_INT_8, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_INT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_16, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_INT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_32, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_INT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_64, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_INT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_8, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_UINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_16, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_UINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_32, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_UINT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_64, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_UINT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_16, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_FLOAT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_32, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_FLOAT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_64, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_FLOAT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_8,
               qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_SFIXED_POINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_16,
               qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_SFIXED_POINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_32,
               qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_SFIXED_POINT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_8,
               qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_UFIXED_POINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_16,
               qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_UFIXED_POINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_32,
               qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_UFIXED_POINT_32 ) );

    EXPECT_EQ( QNN_DATATYPE_UNDEFINED, qnn.SwitchToQnnDataType( QC_TENSOR_TYPE_MAX ) );

    EXPECT_EQ( QC_TENSOR_TYPE_INT_8, qnn.SwitchFromQnnDataType( QNN_DATATYPE_INT_8 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_INT_16, qnn.SwitchFromQnnDataType( QNN_DATATYPE_INT_16 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_INT_32, qnn.SwitchFromQnnDataType( QNN_DATATYPE_INT_32 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_INT_64, qnn.SwitchFromQnnDataType( QNN_DATATYPE_INT_64 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_UINT_8, qnn.SwitchFromQnnDataType( QNN_DATATYPE_UINT_8 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_UINT_16, qnn.SwitchFromQnnDataType( QNN_DATATYPE_UINT_16 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_UINT_32, qnn.SwitchFromQnnDataType( QNN_DATATYPE_UINT_32 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_UINT_64, qnn.SwitchFromQnnDataType( QNN_DATATYPE_UINT_64 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_FLOAT_16, qnn.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_16 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_FLOAT_32, qnn.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_32 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_FLOAT_64, qnn.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_64 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_SFIXED_POINT_8,
               qnn.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_8 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_SFIXED_POINT_16,
               qnn.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_16 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_SFIXED_POINT_32,
               qnn.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_32 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_UFIXED_POINT_8,
               qnn.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_8 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_UFIXED_POINT_16,
               qnn.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_16 ) );
    EXPECT_EQ( QC_TENSOR_TYPE_UFIXED_POINT_32,
               qnn.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_32 ) );

    EXPECT_EQ( QC_TENSOR_TYPE_MAX, qnn.SwitchFromQnnDataType( QNN_DATATYPE_UNDEFINED ) );
}

TEST( QNN, CreateModelFromSo )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "FROM_SO" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "library" );
    dt.Set<std::string>( "static.processorType", "cpu" );

    // invalid path
    dt.Set<std::string>( "static.modelPath", "invalid.so" );
    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

#if defined( __QNXNTO__ )
    std::string modelPath = "data/centernet/aarch64-qnx/libqride_centernet.so";
#else
    std::string modelPath = "data/centernet/aarch64-oe-linux-gcc9.3/libqride_centernet.so";
    return;
    /* Note: the build lib complains version `GLIBCXX_3.4.29' not found */
#endif
    dt.Set<std::string>( "static.modelPath", modelPath );
    config.config = dt.Dump();
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( QNN, DynamicBatchSize )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "DYN_BATCH" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );


    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();
    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t batchSize = 10;
    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        props.dims[0] = batchSize;
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        props.dims[0] = batchSize;
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    batchSize = 3;
    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        inputs[i].dims[0] = batchSize;
        inputs[i].validSize = inputs[i].size / 10 * 3;
    }

    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        outputs[i].dims[0] = batchSize;
        outputs[i].validSize = outputs[i].size / 10 * 3;
    }

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( QNN, BufferFree )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "BUF_FREE" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );
    dt.Set<bool>( "static.deRegisterAllBuffersWhenStop", true );

    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    globalIdx = 0;
    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        auto &inDt = inputDts[i];
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( props, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = frameDesc.SetBuffer( globalIdx, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        auto &outDt = outputDts[i];
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = bufMgr.Allocate( props, outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = frameDesc.SetBuffer( globalIdx, outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( QNN, OneBufferMutipleTensors )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "ONE_BUF_MUL_TS" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    QCSharedFrameDescriptorNode frameDesc1( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    uint32_t globalIdx1 = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = frameDesc1.SetBuffer( globalIdx1, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
        globalIdx1++;
    }
    std::vector<TensorDescriptor_t> outputs;
    uint32_t outputTotalSize = 0;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
        outputTotalSize += outputs.back().size;
    }
    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    /**************  one buffer  **************/
    std::vector<TensorDescriptor_t> outputs1;
    outputs1.resize( outputDts.size() );
    size_t offset = 0u;
    BufferDescriptor_t bufDesc;
    ret = bufMgr.Allocate( BufferProps_t( outputTotalSize ), bufDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        outputs1[i] = outputs[i];
        outputs1[i].pBuf = bufDesc.pBuf;
        outputs1[i].size = bufDesc.size;
        outputs1[i].dmaHandle = bufDesc.dmaHandle;
        outputs1[i].offset = offset;
        offset += outputs1[i].validSize;
        ret = frameDesc1.SetBuffer( globalIdx1, outputs1[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx1++;
    }

    ret = qnn.ProcessFrameDescriptor( frameDesc1 );
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        std::string md5OneBuffer = MD5Sum( outputs1[i].GetDataPtr(), outputs1[i].GetDataSize() );
        std::string md5Output = MD5Sum( outputs[i].GetDataPtr(), outputs[i].GetDataSize() );
        EXPECT_EQ( md5OneBuffer, md5Output );
    }

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = bufMgr.Free( bufDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( QNN, TestAccuracy )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "ACCURACY" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    std::vector<std::string> inputDataPaths;
    inputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_input.raw" );
    ASSERT_EQ( inputDataPaths.size(), inputs.size() );
    for ( int i = 0; i < inputs.size(); ++i )
    {
        size_t inputSize = 0;
        void *pInputData = LoadRaw( inputDataPaths[i], inputSize );
        ASSERT_EQ( inputs[i].size, inputSize );
        memcpy( inputs[i].pBuf, pInputData, inputSize );
        free( pInputData );
    }

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    std::vector<std::string> outputDataPaths;
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_0.raw" );
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_1.raw" );
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_2.raw" );
    ASSERT_EQ( outputDataPaths.size(), outputs.size() );

    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        size_t outputSize = 0;
        void *pOutputData = LoadRaw( outputDataPaths[i], outputSize );
        ASSERT_EQ( outputs[i].size, outputSize );
        double cosSim = CosineSimilarity( (uint8_t *) pOutputData, (uint8_t *) outputs[i].pBuf,
                                          outputSize );
        printf( "output: %" PRIu64 ": cosine similarity = %f\n", i, (float) cosSim );
        ASSERT_GT( cosSim, 0.99d );
        free( pOutputData );
    }

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( QNN, TwoModelWithSameBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn0, qnn1;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "QNN0" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    QCNodeInit_t config = { dt.Dump() };
    ret = qnn0.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    dt.Set<std::string>( "static.name", "QNN1" );
    config.config = dt.Dump();
    ret = qnn1.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn0.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn1.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn0.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    ret = qnn0.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn1.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn0.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn0.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn1.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn1.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn1.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

#ifdef QNN_GTEST_ENABLE_ASYNC
TEST( QNN, AsyncExecute )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::mutex mtx;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "ASYNC" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    QCNodeInit_t config = { dt.Dump(), Qnn_EventCallback };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    DataTree dtp;
    dtp.Set<bool>( "dynamic.enablePerf", true );
    ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );

    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    s_counter = 0;
    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    {
        std::unique_lock<std::mutex> lock( mtx );
        auto status = s_condVar.wait_for( lock, std::chrono::milliseconds( 1000 ) );
        ASSERT_EQ( std::cv_status::no_timeout, status );
        ASSERT_EQ( 1, s_counter );
        ASSERT_EQ( s_frameDesc, &frameDesc );
    }

    Qnn_Perf_t perf = { 0, 0, 0, 0 };
    QCNodeMonitoringIfs &monitorIfs = qnn.GetMonitoringIfs();
    uint32_t size = sizeof( perf );
    ret = monitorIfs.Place( &perf, size );
    ASSERT_EQ( QC_STATUS_OK, ret );

    printf( "perf(us): QNN=%" PRIu64 " RPC=%" PRIu64 " QNN ACC=%" PRIu64 " ACC=%" PRIu64 "\n",
            perf.entireExecTime, perf.rpcExecTimeCPU, perf.rpcExecTimeHTP, perf.rpcExecTimeAcc );

    auto begin = std::chrono::high_resolution_clock::now();
    for ( size_t i = 0; i < QNN_NOTIFY_PARAM_NUM; i++ )
    {
        ret = qnn.ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    auto end = std::chrono::high_resolution_clock::now();

    ret = qnn.ProcessFrameDescriptor( frameDesc );
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    double elpasedMs =
            (double) std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count() /
            1000.d;

    printf( "counter = %" PRIu64 ", async perf=%.3f ms\n", s_counter,
            (float) ( elpasedMs / QNN_NOTIFY_PARAM_NUM ) );

    std::this_thread::sleep_for(
            std::chrono::microseconds( perf.entireExecTime * QNN_NOTIFY_PARAM_NUM + 1000000 ) );
    {
        std::unique_lock<std::mutex> lock( mtx );
        s_condVar.wait_for( lock, std::chrono::milliseconds( 1 ) );
        ASSERT_EQ( 1 + QNN_NOTIFY_PARAM_NUM, s_counter );
    }

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}
#endif

TEST( QNN, InputOutputCheck )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "IOCHK", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "ACCURACY" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.Start();
    ASSERT_EQ( QC_STATUS_OK, ret );


    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    QCSharedFrameDescriptorNode frameDesc2( inputDts.size() + outputDts.size() + 1 );
    QCBufferDescriptorBase_t dummy = frameDesc.GetBuffer( inputDts.size() + outputDts.size() + 1 );

    // force input 0 as invalid dummy buffer
    frameDesc2 = frameDesc;
    ret = frameDesc2.SetBuffer( 0, dummy );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

    frameDesc2 = frameDesc;
    ret = frameDesc2.SetBuffer( inputDts.size(), dummy );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

    // input tensor info
    TensorDescriptor_t tsDesc = inputs[0];
    tsDesc.pBuf = nullptr;
    frameDesc2 = frameDesc;
    ret = frameDesc2.SetBuffer( 0, tsDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = inputs[0];

    tsDesc.tensorType = QC_TENSOR_TYPE_INT_8;
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = inputs[0];

    tsDesc.numDims += 1;
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = inputs[0];

    tsDesc.dims[1] += 1;
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = inputs[0];

    // output tensor info
    tsDesc = outputs[0];
    tsDesc.pBuf = nullptr;
    frameDesc2 = frameDesc;
    ret = frameDesc2.SetBuffer( inputDts.size(), tsDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = outputs[0];

    tsDesc.tensorType = QC_TENSOR_TYPE_INT_8;
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = outputs[0];

    tsDesc.numDims += 1;
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = outputs[0];

    tsDesc.dims[1] += 1;
    ret = qnn.ProcessFrameDescriptor( frameDesc2 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    tsDesc = outputs[0];

    ret = qnn.Stop();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( QNN, ExecuteWithRegDeRegEachTime )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "SANITY" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );
    dt.Set<bool>( "static.deRegisterAllBuffersWhenStop", true );

    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
    uint32_t globalIdx = 0;
    std::vector<TensorDescriptor_t> inputs;
    inputs.reserve( inputDts.size() );
    for ( auto &inDt : inputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( inDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }
    std::vector<TensorDescriptor_t> outputs;
    outputs.reserve( outputDts.size() );
    for ( auto &outDt : outputDts )
    {
        TensorProps_t props;
        ret = ConvertDtToProps( outDt, props );
        ASSERT_EQ( QC_STATUS_OK, ret );
        TensorDescriptor_t tensorDesc;
        ret = bufMgr.Allocate( props, tensorDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( tensorDesc );
        ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
        ASSERT_EQ( QC_STATUS_OK, ret );
        globalIdx++;
    }

    uint32_t loopNumber = 100;
    const char *envValue = getenv( "QNN_TEST_LOOP_NUMBER" );
    if ( nullptr != envValue )
    {
        loopNumber = (uint32_t) atoi( envValue );
    }
    for ( uint32_t l = 0; l < loopNumber; l++ )
    {
        printf( "ExecuteWithRegDeRegEachTime: %u\n", l );

        ret = qnn.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = qnn.ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = qnn.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );

    for ( size_t i = 0; i < inputs.size(); ++i )
    {
        ret = bufMgr.Free( inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    for ( size_t i = 0; i < outputs.size(); ++i )
    {
        ret = bufMgr.Free( outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( QNN, ExecuteWithAllocBufferEachTime )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "SANITY" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );
    dt.Set<bool>( "static.deRegisterAllBuffersWhenStop", true );
    QCNodeInit_t config = { dt.Dump() };
    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    DataTree optionsDt;
    std::vector<DataTree> inputDts;
    std::vector<DataTree> outputDts;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = optionsDt.Get( "model.inputs", inputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ret = optionsDt.Get( "model.outputs", outputDts );
    ASSERT_EQ( QC_STATUS_OK, ret );

    QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );

    uint32_t loopNumber = 100;
    const char *envValue = getenv( "QNN_TEST_LOOP_NUMBER" );
    if ( nullptr != envValue )
    {
        loopNumber = (uint32_t) atoi( envValue );
    }
    for ( uint32_t l = 0; l < loopNumber; l++ )
    {
        printf( "ExecuteWithAllocBufferEachTime: %u\n", l );
        uint32_t globalIdx = 0;
        std::vector<TensorDescriptor_t> inputs;
        inputs.reserve( inputDts.size() );
        for ( auto &inDt : inputDts )
        {
            TensorProps_t props;
            ret = ConvertDtToProps( inDt, props );
            ASSERT_EQ( QC_STATUS_OK, ret );
            TensorDescriptor_t tensorDesc;
            ret = bufMgr.Allocate( props, tensorDesc );
            ASSERT_EQ( QC_STATUS_OK, ret );
            inputs.push_back( tensorDesc );
            ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
            ASSERT_EQ( QC_STATUS_OK, ret );
            globalIdx++;
        }
        std::vector<TensorDescriptor_t> outputs;
        outputs.reserve( outputDts.size() );
        for ( auto &outDt : outputDts )
        {
            TensorProps_t props;
            ret = ConvertDtToProps( outDt, props );
            ASSERT_EQ( QC_STATUS_OK, ret );
            TensorDescriptor_t tensorDesc;
            ret = bufMgr.Allocate( props, tensorDesc );
            ASSERT_EQ( QC_STATUS_OK, ret );
            outputs.push_back( tensorDesc );
            ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
            ASSERT_EQ( QC_STATUS_OK, ret );
            globalIdx++;
        }

        ret = qnn.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = qnn.ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = qnn.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );

        for ( size_t i = 0; i < inputs.size(); ++i )
        {
            ret = bufMgr.Free( inputs[i] );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        for ( size_t i = 0; i < outputs.size(); ++i )
        {
            ret = bufMgr.Free( outputs[i] );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
    }

    ret = qnn.DeInitialize();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( QNN, InitDeInitializeStress )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::string errors;
    Qnn qnn;
    BufferManager bufMgr( { "TENSOR", QC_NODE_TYPE_QNN, 0 } );
    DataTree dt;
    dt.Set<std::string>( "static.name", "SANITY" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<std::string>( "static.loadType", "binary" );

    dt.Set<std::string>( "static.modelPath", "data/centernet/program.bin" );
    dt.Set<std::string>( "static.processorType", "htp0" );

    uint32_t loopNumber = 100;
    const char *envValue = getenv( "QNN_TEST_LOOP_NUMBER" );
    if ( nullptr != envValue )
    {
        loopNumber = (uint32_t) atoi( envValue );
    }
    for ( uint32_t l = 0; l < loopNumber; l++ )
    {
        printf( "InitDeInitializeStress: %u\n", l );

        QCNodeInit_t config = { dt.Dump() };
        ret = qnn.Initialize( config );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = qnn.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );

        QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
        const std::string &options = cfgIfs.GetOptions();

        DataTree optionsDt;
        std::vector<DataTree> inputDts;
        std::vector<DataTree> outputDts;
        ret = optionsDt.Load( options, errors );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = optionsDt.Get( "model.inputs", inputDts );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = optionsDt.Get( "model.outputs", outputDts );
        ASSERT_EQ( QC_STATUS_OK, ret );

        QCSharedFrameDescriptorNode frameDesc( inputDts.size() + outputDts.size() + 1 );
        uint32_t globalIdx = 0;
        std::vector<TensorDescriptor_t> inputs;
        inputs.reserve( inputDts.size() );
        for ( auto &inDt : inputDts )
        {
            TensorProps_t props;
            ret = ConvertDtToProps( inDt, props );
            ASSERT_EQ( QC_STATUS_OK, ret );
            TensorDescriptor_t tensorDesc;
            ret = bufMgr.Allocate( props, tensorDesc );
            ASSERT_EQ( QC_STATUS_OK, ret );
            inputs.push_back( tensorDesc );
            ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
            ASSERT_EQ( QC_STATUS_OK, ret );
            globalIdx++;
        }
        std::vector<TensorDescriptor_t> outputs;
        outputs.reserve( outputDts.size() );
        for ( auto &outDt : outputDts )
        {
            TensorProps_t props;
            ret = ConvertDtToProps( outDt, props );
            ASSERT_EQ( QC_STATUS_OK, ret );
            TensorDescriptor_t tensorDesc;
            ret = bufMgr.Allocate( props, tensorDesc );
            ASSERT_EQ( QC_STATUS_OK, ret );
            outputs.push_back( tensorDesc );
            ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
            ASSERT_EQ( QC_STATUS_OK, ret );
            globalIdx++;
        }

        ret = qnn.ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = qnn.Stop();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = qnn.DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, ret );

        for ( size_t i = 0; i < inputs.size(); ++i )
        {
            ret = bufMgr.Free( inputs[i] );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
        for ( size_t i = 0; i < outputs.size(); ++i )
        {
            ret = bufMgr.Free( outputs[i] );
            ASSERT_EQ( QC_STATUS_OK, ret );
        }
    }
}

TEST_F( QnnTest, CovLogLevel )
{
    SetLogLevel( "VERBOSE" );
    Init( "LOGLEVEL", "binary", "data/centernet/program.bin", "htp0" );
    AllocateBuffers();
    Start();
    Execute();
    Stop();
    Deinit();

    SetLogLevel( "DEBUG" );
    Init( "LOGLEVEL", "binary", "data/centernet/program.bin", "htp0" );
    Start();
    Execute();
    Stop();
    Deinit();

    SetLogLevel( "INFO" );
    Init( "LOGLEVEL", "binary", "data/centernet/program.bin", "htp0" );
    Start();
    Execute();
    Stop();
    Deinit();

    SetLogLevel( "WARN" );
    Init( "LOGLEVEL", "binary", "data/centernet/program.bin", "htp0" );
    Start();
    Execute();
    Stop();
    Deinit();
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif