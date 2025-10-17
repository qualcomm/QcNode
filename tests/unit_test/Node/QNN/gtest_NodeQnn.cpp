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

#include "QnnLog.h"
#include "QnnTypeMacros.hpp"

#include "MockCLib.hpp"

namespace QC
{
namespace Node
{
extern void QnnLog_Callback( const char *fmt, QnnLog_Level_t logLevel, uint64_t timestamp,
                             va_list args );
size_t memscpy( void *dst, size_t dstSize, const void *src, size_t copySize );
}   // namespace Node
}   // namespace QC


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

    void SetupConfig( std::string name, std::string loadType, std::string modelPath,
                      std::string processorType, std::string type = "QNN", uint32_t id = 0,
                      std::string perfProfile = "default", std::string priority = "normal" )
    {
        dt = DataTree();
        dt.Set<std::string>( "static.name", name );
        dt.Set<uint32_t>( "static.id", id );
        dt.Set<std::string>( "static.loadType", loadType );
        dt.Set<std::string>( "static.modelPath", modelPath );
        dt.Set<std::string>( "static.processorType", processorType );
        dt.Set<std::string>( "static.type", type );
        dt.Set<std::string>( "static.perfProfile", perfProfile );
        dt.Set<std::string>( "static.priority", priority );

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

        if ( globalBufferNames.size() > 0 )
        {
            std::vector<DataTree> globalBufferMaps;
            for ( size_t i = 0; i < globalBufferNames.size(); i++ )
            {
                DataTree gbm;
                gbm.Set<std::string>( "name", globalBufferNames[i] );
                gbm.Set<uint32_t>( "id", globalBufferIds[i] );
                globalBufferMaps.push_back( gbm );

                if ( globalBufferIds[i] > bufferDescNum )
                {
                    bufferDescNum = globalBufferIds[i] + 1;
                }
            }
            dt.Set( "static.globalBufferIdMap", globalBufferMaps );
        }

        config.config = dt.Dump();
    }

    void Initialize( std::string name, std::string loadType, std::string modelPath,
                     std::string processorType, std::string type = "QNN", uint32_t id = 0,
                     std::string perfProfile = "default", std::string priority = "normal" )
    {
        SetupConfig( name, loadType, modelPath, processorType, type, id, perfProfile, priority );
        ret = qnn.Initialize( config );
    }

    void Init( std::string name, std::string loadType, std::string modelPath,
               std::string processorType, std::string type = "QNN", uint32_t id = 0,
               std::string perfProfile = "default", std::string priority = "normal" )
    {
        Initialize( name, loadType, modelPath, processorType, type, id, perfProfile, priority );
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

        if ( 0 == bufferDescNum )
        {
            bufferDescNum = inputDts.size() + outputDts.size() + 1;
        }

        pFrameDesc = new NodeFrameDescriptor( bufferDescNum );
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
            if ( globalBufferIds.size() > 0 )
            {
                uint32_t globalIdxCfg = globalBufferIds[globalIdx];
                ret = pFrameDesc->SetBuffer( globalIdxCfg, inputs.back() );
            }
            else
            {
                ret = pFrameDesc->SetBuffer( globalIdx, inputs.back() );
            }
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
            if ( globalBufferIds.size() > 0 )
            {
                uint32_t globalIdxCfg = globalBufferIds[globalIdx];
                ret = pFrameDesc->SetBuffer( globalIdxCfg, outputs.back() );
            }
            else
            {
                ret = pFrameDesc->SetBuffer( globalIdx, outputs.back() );
            }
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

        bufferDescNum = 0;
        globalBufferNames.clear();
        globalBufferIds.clear();
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
    NodeFrameDescriptor *pFrameDesc = nullptr;

    /* use to load from context buffer */
    std::shared_ptr<uint8_t> buffer = nullptr;
    uint64_t bufferSize{ 0 };
    QCBufferDescriptorBase_t ctxBufDesc;

    std::vector<uint32_t> globalBufferIds;
    std::vector<std::string> globalBufferNames;
    uint32_t bufferDescNum = 0;
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

    ret = qnn.Initialize( config );
    ASSERT_EQ( QC_STATUS_BAD_STATE, ret );

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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

    QnnImplMonitorConfig_t &config = qnn.GetMonitorConifg();
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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
    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
    NodeFrameDescriptor frameDesc1( inputDts.size() + outputDts.size() + 1 );
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

TEST_F( QnnTest, TestAccuracy )
{
    Init( "SANITY", "binary", "data/centernet/program.bin", "htp0" );
    AllocateBuffers();
    Start();

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
    Execute();

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

    Stop();
    Deinit();
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

    NodeFrameDescriptor frameDesc2( inputDts.size() + outputDts.size() + 1 );
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

    NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );

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

        NodeFrameDescriptor frameDesc( inputDts.size() + outputDts.size() + 1 );
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

TEST_F( QnnTest, QnnConfig )
{
    QCNodeConfigIfs &cfgIfs = qnn.GetConfigurationIfs();
    SetupConfig( "", "binary", "data/centernet/program.bin", "htp0" );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "no name specified, " );

    SetupConfig( "QCFG", "binary", "data/centernet/program.bin", "htp0", "QNNx" );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the type is not QNN, " );

    SetupConfig( "QCFG", "binary", "data/centernet/program.bin", "htp0", "QNNx", UINT32_MAX );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the type is not QNN, the id is empty, " );

    SetupConfig( "QCFG", "binary", "data/centernet/program.bin", "htpx" );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the processorType htpx is invalid, " );

    std::vector<std::string> validProcessors = { "htp0", "htp1", "htp2", "htp3", "cpu", "gpu" };
    for ( auto &processor : validProcessors )
    {
        SetupConfig( "QCFG", "binary", "data/centernet/program.bin", processor );
        ret = cfgIfs.VerifyAndSet( config.config, errors );
        ASSERT_EQ( ret, QC_STATUS_OK );
    }

    SetupConfig( "QCFG", "binary", "data/centernet/program.bin", "htp0", "QNN", 1, "invalid perf" );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the perfProfile invalid perf is invalid, " );

    std::vector<std::string> validPerfs = { "default",
                                            "low_balanced",
                                            "balanced",
                                            "high_performance",
                                            "sustained_high_performance",
                                            "burst",
                                            "low_power_saver",
                                            "power_saver",
                                            "high_power_saver",
                                            "extreme_power_saver" };
    for ( auto &perfProfile : validPerfs )
    {
        SetupConfig( "QCFG", "binary", "data/centernet/program.bin", "htp0", "QNN", 1,
                     perfProfile );
        ret = cfgIfs.VerifyAndSet( config.config, errors );
        ASSERT_EQ( ret, QC_STATUS_OK );

        Initialize( "QCFG", "binary", "data/centernet/program.bin", "htp0", "QNN", 1, perfProfile );
        if ( ret != QC_STATUS_OK )
        {
            printf( "setting perf %s failed\n", perfProfile.c_str() );
        }
        else
        {
            printf( "setting perf %s OK\n", perfProfile.c_str() );
            Deinit();
        }
    }

    SetupConfig( "QCFG", "binary", "invalid.bin", "htp0" );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the modelPath <invalid.bin> is invalid, " );

    SetupConfig( "QCFG", "invalid", "data/centernet/program.bin", "htp0" );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the loadType <invalid> is invalid, " );

    SetupConfig( "QCFG", "buffer", "data/centernet/program.bin", "htp0" );
    dt.Set<uint32_t>( "static.contextBufferId", UINT32_MAX );   // override with invalid ID
    config.config = dt.Dump();
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the contextBufferId is empty, " );

    SetupConfig( "QCFG", "buffer", "data/centernet/program.bin", "htp0" );
    std::vector<uint32_t> bufferIds;
    dt.Set<uint32_t>( "static.bufferIds", bufferIds );
    config.config = dt.Dump();
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the bufferIds is invalid, " );


    SetupConfig( "QCFG", "binary", "data/centernet/program.bin", "htp0", "QNN", 0, "default",
                 "invprio" );
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the priority is invalid, " );

    std::vector<std::string> validPrios = { "low", "normal", "normal_high", "high" };
    for ( auto &prio : validPrios )
    {
        SetupConfig( "QCFG", "binary", "data/centernet/program.bin", "htp0", "QNN", 0, "default",
                     prio );
        ret = cfgIfs.VerifyAndSet( config.config, errors );
        ASSERT_EQ( ret, QC_STATUS_OK );
    }

    SetupConfig( "QCFG", "buffer", "data/centernet/program.bin", "htp0" );
    dt.Set<std::string>( "static.udoPackages", "invalid" );
    config.config = dt.Dump();
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the udoPackages is invalid, " );

    SetupConfig( "QCFG", "buffer", "data/centernet/program.bin", "htp0" );
    std::vector<DataTree> udoPkgs;
    DataTree udt;
    udt.Set<std::string>( "udoLibPath", "" );
    udt.Set<std::string>( "interfaceProvider", "" );
    udoPkgs.push_back( udt );
    dt.Set( "static.udoPackages", udoPkgs );
    config.config = dt.Dump();
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the udo 0 library path is empty, the udo 0 interface is empty, " );

    SetupConfig( "QCFG", "buffer", "data/centernet/program.bin", "htp0" );
    dt.Set<std::string>( "static.globalBufferIdMap", "invalid" );
    config.config = dt.Dump();
    ret = cfgIfs.VerifyAndSet( config.config, errors );
    ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    ASSERT_EQ( errors, "the globalBufferIdMap is invalid, " );

    {
        SetupConfig( "QCFG", "buffer", "data/centernet/program.bin", "htp0" );
        std::vector<DataTree> globalBufferMaps;
        DataTree gbm;
        gbm.Set<std::string>( "name", "" );
        gbm.Set<uint32_t>( "id", UINT32_MAX );
        globalBufferMaps.push_back( gbm );
        dt.Set( "static.globalBufferIdMap", globalBufferMaps );
        config.config = dt.Dump();
        ret = cfgIfs.VerifyAndSet( config.config, errors );
        ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
        ASSERT_EQ( errors, "the globalIdMap 0 name is empty, the globalIdMap 0 id is empty, " );
    }

    {
        SetupConfig( "QCFG", "buffer", "data/centernet/program.bin", "htp0" );
        std::vector<DataTree> globalBufferMaps;
        DataTree gbm;
        gbm.Set<std::string>( "name", "input0" );
        gbm.Set<uint32_t>( "id", 0 );
        globalBufferMaps.push_back( gbm );
        gbm.Set<std::string>( "name", "output0" );
        gbm.Set<uint32_t>( "id", 1 );
        globalBufferMaps.push_back( gbm );
        dt.Set( "static.globalBufferIdMap", globalBufferMaps );
        config.config = dt.Dump();
        ret = cfgIfs.VerifyAndSet( config.config, errors );
        ASSERT_EQ( ret, QC_STATUS_OK );

        DataTree dtp;
        dtp.Set<bool>( "dynamic.invalid", true );
        ret = cfgIfs.VerifyAndSet( dtp.Dump(), errors );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
    }

    {
        Init( "SANITY", "binary", "data/centernet/program.bin", "htp0" );
        std::string options = cfgIfs.GetOptions();
        std::string options2 = cfgIfs.GetOptions();
        ASSERT_EQ( options, options2 );
        Deinit();
    }
}

TEST_F( QnnTest, QnnMonitor )
{
    QCNodeMonitoringIfs &monitorIfs = qnn.GetMonitoringIfs();

    ret = monitorIfs.VerifyAndSet( "{}", errors );
    ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );

    {
        std::string options = monitorIfs.GetOptions();
        ASSERT_EQ( options, "{}" );
    }

    {
        uint32_t maxSize = monitorIfs.GetMaximalSize();
        ASSERT_EQ( maxSize, sizeof( Qnn_Perf_t ) );
    }

    {
        uint32_t curSize = monitorIfs.GetCurrentSize();
        ASSERT_EQ( curSize, sizeof( Qnn_Perf_t ) );
    }

    {
        uint32_t size = sizeof( Qnn_Perf_t );
        ret = monitorIfs.Place( nullptr, size );
        ASSERT_EQ( ret, QC_STATUS_BAD_ARGUMENTS );
    }
}

TEST_F( QnnTest, GlobalBufferIdMap )
{
    globalBufferNames = std::vector<std::string>( { "input", "_256", "_259", "_262", "ERROR" } );
    globalBufferIds = std::vector<uint32_t>( { 13, 21, 11, 9, 17 } );
    Init( "QNN", "buffer", "data/centernet/program.bin", "htp0" );
    AllocateBuffers();
    Start();

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
    Execute();

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

    Stop();
    Deinit();

    // missing one
    globalBufferNames = std::vector<std::string>( { "input", "_259", "_262", "ERROR" } );
    globalBufferIds = std::vector<uint32_t>( { 13, 11, 9, 17 } );
    Initialize( "QNN", "buffer", "data/centernet/program.bin", "htp0" );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // input with wrong name
    globalBufferNames = std::vector<std::string>( { "inputxxx", "_256", "_259", "_262", "ERROR" } );
    globalBufferIds = std::vector<uint32_t>( { 13, 21, 11, 9, 17 } );
    Initialize( "QNN", "buffer", "data/centernet/program.bin", "htp0" );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // output with wrong name
    globalBufferNames = std::vector<std::string>( { "input", "_256", "_259xx", "_262", "ERROR" } );
    globalBufferIds = std::vector<uint32_t>( { 13, 21, 11, 9, 17 } );
    Initialize( "QNN", "buffer", "data/centernet/program.bin", "htp0" );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
}

void QnnLog_CallbackTest( QnnLog_Level_t logLevel, uint64_t timestamp, const char *fmt, ... )
{
    va_list args;
    va_start( args, fmt );
    QC::Node::QnnLog_Callback( fmt, logLevel, timestamp, args );
    va_end( args );
}

TEST( QNN, LogCallback )
{
    QnnLog_CallbackTest( QNN_LOG_LEVEL_ERROR, 0, "error message: %d", 1234 );
    QnnLog_CallbackTest( QNN_LOG_LEVEL_WARN, 1, "warn message: %d", 2234 );
    QnnLog_CallbackTest( QNN_LOG_LEVEL_INFO, 2, "info message: %d", 3234 );
    QnnLog_CallbackTest( QNN_LOG_LEVEL_VERBOSE, 3, "verbose message: %d", 4234 );
    QnnLog_CallbackTest( QNN_LOG_LEVEL_DEBUG, 4, "debug message: %d", 5234 );
    QnnLog_CallbackTest( QNN_LOG_LEVEL_MAX, 5, "max message: %d", 6234 );
}

TEST( QNN, memscpy )
{
    size_t copied;
    uint8_t dst[32];
    uint8_t src[64];

    for ( size_t i = 0; i < sizeof( src ); i++ )
    {
        src[i] = i;
    }

    copied = QC::Node::memscpy( dst, 32, src, 64 );
    ASSERT_EQ( copied, 32 );
    ASSERT_EQ( 0, memcmp( src, dst, 32 ) );

    copied = QC::Node::memscpy( nullptr, 32, src, 64 );
    ASSERT_EQ( copied, 0 );

    copied = QC::Node::memscpy( dst, 0, src, 64 );
    ASSERT_EQ( copied, 0 );

    copied = QC::Node::memscpy( dst, 32, nullptr, 64 );
    ASSERT_EQ( copied, 0 );

    copied = QC::Node::memscpy( dst, 32, src, 0 );
    ASSERT_EQ( copied, 0 );
}

TEST( QNN, QnnImplUnitTest )
{
    QCStatus_e status;
    QCNodeID_t nodeId;
    Logger logger;
    logger.Init( "UNIT_TEST" );

    {
        QnnImpl qnn( nodeId, logger );
        status = qnn.GetQnnFunctionPointers( "libQnnNotExist.so", "libQnnModel.so", false );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        QnnImpl qnn( nodeId, logger );
        status = qnn.GetQnnFunctionPointers( "libQCNode.so", "libQnnModel.so", false );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        QnnImpl qnn( nodeId, logger );
        status = qnn.GetQnnFunctionPointers( "libQnnHtp.so", "libQnnModelNotExist.so", true );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        QnnImpl qnn( nodeId, logger );
        status = qnn.GetQnnFunctionPointers( "libQnnHtp.so", "libQCNode.so", true );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        QnnImpl qnn( nodeId, logger );
        status = qnn.GetQnnSystemFunctionPointers( "libQnnNotExist.so" );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        QnnImpl qnn( nodeId, logger );
        status = qnn.GetQnnSystemFunctionPointers( "libQCNode.so" );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        Qnn_Tensor_t src;
        QnnImpl qnn( nodeId, logger );
        status = qnn.DeepCopyQnnTensorInfo( nullptr, &src );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        Qnn_Tensor_t dst;
        QnnImpl qnn( nodeId, logger );
        status = qnn.DeepCopyQnnTensorInfo( &dst, nullptr );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        uint32_t dims[4] = { 1, 2, 3, 4 };
        Qnn_Tensor_t src = QNN_TENSOR_INIT, dst = QNN_TENSOR_INIT;
        src.version = QNN_TENSOR_VERSION_1;
        QNN_TENSOR_SET_NAME( &src, nullptr );
        QNN_TENSOR_SET_DIMENSIONS( &src, dims );
        QNN_TENSOR_SET_RANK( &src, 4 );
        Qnn_QuantizeParams_t quantizeParams = QNN_QUANTIZE_PARAMS_INIT;
        Qnn_ScaleOffset_t scaleOffsets[3] = { { 0.1, 100 }, { 0.2, 200 }, { 0.3, 300 } };
        quantizeParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET;
        quantizeParams.axisScaleOffsetEncoding.axis = 2;
        quantizeParams.axisScaleOffsetEncoding.numScaleOffsets = 3;
        quantizeParams.axisScaleOffsetEncoding.scaleOffset = scaleOffsets;
        QNN_TENSOR_SET_QUANT_PARAMS( &src, quantizeParams );
        QnnImpl qnn( nodeId, logger );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( nullptr, QNN_TENSOR_GET_NAME( &dst ) );
        ASSERT_EQ( 4, QNN_TENSOR_GET_RANK( &dst ) );
        ASSERT_EQ( QNN_TENSOR_GET_DIMENSIONS( &src )[0], QNN_TENSOR_GET_DIMENSIONS( &dst )[0] );
        ASSERT_EQ( QNN_TENSOR_GET_DIMENSIONS( &src )[1], QNN_TENSOR_GET_DIMENSIONS( &dst )[1] );
        ASSERT_EQ( QNN_TENSOR_GET_DIMENSIONS( &src )[2], QNN_TENSOR_GET_DIMENSIONS( &dst )[2] );
        ASSERT_EQ( QNN_TENSOR_GET_DIMENSIONS( &src )[3], QNN_TENSOR_GET_DIMENSIONS( &dst )[3] );

        Qnn_QuantizeParams_t qParams = QNN_TENSOR_GET_QUANT_PARAMS( &dst );
        ASSERT_EQ( quantizeParams.quantizationEncoding, qParams.quantizationEncoding );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.axis,
                   qParams.axisScaleOffsetEncoding.axis );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.numScaleOffsets,
                   qParams.axisScaleOffsetEncoding.numScaleOffsets );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.scaleOffset[0].scale,
                   qParams.axisScaleOffsetEncoding.scaleOffset[0].scale );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.scaleOffset[1].scale,
                   qParams.axisScaleOffsetEncoding.scaleOffset[1].scale );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.scaleOffset[2].scale,
                   qParams.axisScaleOffsetEncoding.scaleOffset[2].scale );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.scaleOffset[0].offset,
                   qParams.axisScaleOffsetEncoding.scaleOffset[0].offset );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.scaleOffset[1].offset,
                   qParams.axisScaleOffsetEncoding.scaleOffset[1].offset );
        ASSERT_EQ( quantizeParams.axisScaleOffsetEncoding.scaleOffset[2].offset,
                   qParams.axisScaleOffsetEncoding.scaleOffset[2].offset );

        qnn.FreeQnnTensor( dst );

        MockC_MallocCtrl( 1 );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_NOMEM, status );

        quantizeParams.axisScaleOffsetEncoding.numScaleOffsets = 0;
        QNN_TENSOR_SET_QUANT_PARAMS( &src, quantizeParams );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        quantizeParams.axisScaleOffsetEncoding.numScaleOffsets = 3;
        quantizeParams.axisScaleOffsetEncoding.scaleOffset = nullptr;
        QNN_TENSOR_SET_QUANT_PARAMS( &src, quantizeParams );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        quantizeParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_BLOCK;
        QNN_TENSOR_SET_QUANT_PARAMS( &src, quantizeParams );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );

        quantizeParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
        QNN_TENSOR_SET_QUANT_PARAMS( &src, quantizeParams );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_OK, status );

        qnn.FreeQnnTensor( dst );

        QNN_TENSOR_SET_RANK( &src, 0 );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        quantizeParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET;
        quantizeParams.axisScaleOffsetEncoding.scaleOffset = nullptr;
        QNN_TENSOR_SET_QUANT_PARAMS( &dst, quantizeParams );
        qnn.FreeQnnTensor( dst );

        QNN_TENSOR_SET_RANK( &src, 4 );
        QNN_TENSOR_SET_DIMENSIONS( &src, nullptr );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        uint32_t dims[4] = { 1, 2, 3, 4 };
        uint8_t isDynamic[4] = { 0, 1, 0, 0 };
        Qnn_Tensor_t src = QNN_TENSOR_INIT, dst = QNN_TENSOR_INIT;
        src.version = QNN_TENSOR_VERSION_2;
        QNN_TENSOR_SET_NAME( &src, nullptr );
        QNN_TENSOR_SET_DIMENSIONS( &src, dims );
        QNN_TENSOR_SET_RANK( &src, 4 );
        Qnn_QuantizeParams_t quantizeParams = QNN_QUANTIZE_PARAMS_INIT;
        quantizeParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
        QNN_TENSOR_SET_QUANT_PARAMS( &src, quantizeParams );
        QNN_TENSOR_SET_IS_DYNAMIC_DIMENSIONS( &src, isDynamic );
        QnnImpl qnn( nodeId, logger );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_OK, status );

        qnn.FreeQnnTensor( dst );

        MockC_MallocCtrl( 1 );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_NOMEM, status );

        MockC_MallocCtrl( 2 );
        status = qnn.DeepCopyQnnTensorInfo( &dst, &src );
        ASSERT_EQ( QC_STATUS_NOMEM, status );

        Qnn_Tensor_t *tensorWrappers;

        MockC_CallocCtrl( 1 );
        status = qnn.CopyTensorsInfo( &src, tensorWrappers, 1 );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_MallocCtrl( 1 );
        status = qnn.CopyTensorsInfo( &src, tensorWrappers, 1 );
        ASSERT_EQ( QC_STATUS_NOMEM, status );
    }

    {
        uint32_t dims[4] = { 1, 2, 3, 4 };
        Qnn_Tensor_t input = QNN_TENSOR_INIT, output = QNN_TENSOR_INIT;
        input.version = QNN_TENSOR_VERSION_1;
        QNN_TENSOR_SET_NAME( &input, "input" );
        QNN_TENSOR_SET_DIMENSIONS( &input, dims );
        QNN_TENSOR_SET_RANK( &input, 4 );
        output.version = QNN_TENSOR_VERSION_1;
        QNN_TENSOR_SET_NAME( &output, "output" );
        QNN_TENSOR_SET_DIMENSIONS( &output, dims );
        QNN_TENSOR_SET_RANK( &output, 4 );

        QnnSystemContext_GraphInfoV1_t graphInfoSrc;
        qnn_wrapper_api::GraphInfo_t graphInfoDst;

        graphInfoSrc.graphName = "TestGraphV1";
        graphInfoSrc.numGraphInputs = 1;
        graphInfoSrc.numGraphOutputs = 1;
        graphInfoSrc.graphInputs = &input;
        graphInfoSrc.graphOutputs = &output;

        QnnImpl qnn( nodeId, logger );

        status = qnn.CopyGraphsInfoV1( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );

        MockC_MallocCtrl( 1 );
        status = qnn.CopyGraphsInfoV1( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 1 );
        status = qnn.CopyGraphsInfoV1( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 2 );
        status = qnn.CopyGraphsInfoV1( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        graphInfoSrc.graphName = nullptr;
        status = qnn.CopyGraphsInfoV1( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );
        graphInfoSrc.graphName = "TestGraphV1";

        graphInfoSrc.graphInputs = nullptr;
        status = qnn.CopyGraphsInfoV1( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );

        graphInfoSrc.graphInputs = &input;
        graphInfoSrc.graphOutputs = nullptr;
        status = qnn.CopyGraphsInfoV1( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );
        graphInfoSrc.graphOutputs = &output;

        QnnSystemContext_GraphInfo_t graphsInput;
        qnn_wrapper_api::GraphInfo_t **graphsInfo = nullptr;
        graphsInput.version = QNN_SYSTEM_CONTEXT_GRAPH_INFO_VERSION_1;
        graphsInput.graphInfoV1 = graphInfoSrc;

        status = qnn.CopyGraphsInfo( nullptr, 1, graphsInfo );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        status = qnn.CopyGraphsInfo( &graphsInput, 1, graphsInfo );
        ASSERT_EQ( QC_STATUS_OK, status );

        MockC_CallocCtrl( 1 );
        status = qnn.CopyGraphsInfo( &graphsInput, 1, graphsInfo );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 2 );
        status = qnn.CopyGraphsInfo( &graphsInput, 1, graphsInfo );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        QnnSystemContext_GraphInfo_t graphsInputArr[2];
        graphsInputArr[0].version = QNN_SYSTEM_CONTEXT_GRAPH_INFO_VERSION_1;
        graphsInputArr[0].graphInfoV1 = graphInfoSrc;
        graphsInputArr[1].version = QNN_SYSTEM_CONTEXT_GRAPH_INFO_UNDEFINED;
        graphsInputArr[1].graphInfoV1 = graphInfoSrc;

        status = qnn.CopyGraphsInfo( graphsInputArr, 2, graphsInfo );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );

        graphInfoSrc.graphName = nullptr;
        graphsInputArr[0].graphInfoV1 = graphInfoSrc;
        status = qnn.CopyGraphsInfo( graphsInputArr, 2, graphsInfo );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );

        QnnSystemContext_BinaryInfo_t binaryInfo;
        uint32_t graphsCount = 0;

        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_1;
        binaryInfo.contextBinaryInfoV1.numGraphs = 1;
        binaryInfo.contextBinaryInfoV1.graphs = &graphsInput;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_OK, status );

        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_2;
        binaryInfo.contextBinaryInfoV2.numGraphs = 1;
        binaryInfo.contextBinaryInfoV2.graphs = &graphsInput;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_OK, status );

        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_3;
        binaryInfo.contextBinaryInfoV3.numGraphs = 1;
        binaryInfo.contextBinaryInfoV3.graphs = &graphsInput;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_OK, status );

        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_UNDEFINED;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );

        status = qnn.CopyMetadataToGraphsInfo( nullptr, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_1;
        binaryInfo.contextBinaryInfoV1.graphs = nullptr;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_2;
        binaryInfo.contextBinaryInfoV2.graphs = nullptr;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_3;
        binaryInfo.contextBinaryInfoV3.graphs = nullptr;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 1 );
        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_1;
        binaryInfo.contextBinaryInfoV1.numGraphs = 1;
        binaryInfo.contextBinaryInfoV1.graphs = &graphsInput;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 1 );
        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_2;
        binaryInfo.contextBinaryInfoV2.numGraphs = 1;
        binaryInfo.contextBinaryInfoV2.graphs = &graphsInput;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 1 );
        binaryInfo.version = QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_3;
        binaryInfo.contextBinaryInfoV3.numGraphs = 1;
        binaryInfo.contextBinaryInfoV3.graphs = &graphsInput;
        status = qnn.CopyMetadataToGraphsInfo( &binaryInfo, graphsInfo, graphsCount );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        uint32_t dims[4] = { 1, 2, 3, 4 };
        Qnn_Tensor_t input = QNN_TENSOR_INIT, output = QNN_TENSOR_INIT;
        input.version = QNN_TENSOR_VERSION_1;
        QNN_TENSOR_SET_NAME( &input, "input" );
        QNN_TENSOR_SET_DIMENSIONS( &input, dims );
        QNN_TENSOR_SET_RANK( &input, 4 );
        output.version = QNN_TENSOR_VERSION_1;
        QNN_TENSOR_SET_NAME( &output, "output" );
        QNN_TENSOR_SET_DIMENSIONS( &output, dims );
        QNN_TENSOR_SET_RANK( &output, 4 );

        QnnSystemContext_GraphInfoV3_t graphInfoSrc;
        qnn_wrapper_api::GraphInfo_t graphInfoDst;

        graphInfoSrc.graphName = "TestGraphV3";
        graphInfoSrc.numGraphInputs = 1;
        graphInfoSrc.numGraphOutputs = 1;
        graphInfoSrc.graphInputs = &input;
        graphInfoSrc.graphOutputs = &output;

        QnnImpl qnn( nodeId, logger );

        status = qnn.CopyGraphsInfoV3( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );

        MockC_MallocCtrl( 1 );
        status = qnn.CopyGraphsInfoV3( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 1 );
        status = qnn.CopyGraphsInfoV3( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        MockC_CallocCtrl( 2 );
        status = qnn.CopyGraphsInfoV3( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        graphInfoSrc.graphName = nullptr;
        status = qnn.CopyGraphsInfoV3( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );

        graphInfoSrc.graphInputs = nullptr;
        status = qnn.CopyGraphsInfoV3( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );

        graphInfoSrc.graphInputs = &input;
        graphInfoSrc.graphOutputs = nullptr;
        status = qnn.CopyGraphsInfoV3( &graphInfoSrc, &graphInfoDst );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        QnnImpl qnn( nodeId, logger );
        qnn_wrapper_api::GraphInfoPtr_t *graphsInfo[1] = { nullptr };
        status = qnn.FreeGraphsInfo( nullptr, 1 );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        status = qnn.FreeGraphsInfo( graphsInfo, 1 );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }

    {
        QnnImpl qnn( nodeId, logger );
        QnnLog_Level_t level;

        level = qnn.GetQnnLogLevel( LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( level, QNN_LOG_LEVEL_VERBOSE );

        level = qnn.GetQnnLogLevel( LOGGER_LEVEL_DEBUG );
        ASSERT_EQ( level, QNN_LOG_LEVEL_DEBUG );

        level = qnn.GetQnnLogLevel( LOGGER_LEVEL_INFO );
        ASSERT_EQ( level, QNN_LOG_LEVEL_INFO );

        level = qnn.GetQnnLogLevel( LOGGER_LEVEL_ERROR );
        ASSERT_EQ( level, QNN_LOG_LEVEL_ERROR );

        level = qnn.GetQnnLogLevel( LOGGER_LEVEL_WARN );
        ASSERT_EQ( level, QNN_LOG_LEVEL_WARN );

        level = qnn.GetQnnLogLevel( LOGGER_LEVEL_MAX );
        ASSERT_EQ( level, QNN_LOG_LEVEL_WARN );
    }

    {
        QnnImpl qnn( nodeId, logger );
        uint32_t deviceId;

        deviceId = qnn.GetQnnDeviceId( QNN_PROCESSOR_HTP0 );
        ASSERT_EQ( deviceId, 0 );

        deviceId = qnn.GetQnnDeviceId( QNN_PROCESSOR_HTP1 );
        ASSERT_EQ( deviceId, 1 );

        deviceId = qnn.GetQnnDeviceId( QNN_PROCESSOR_HTP2 );
        ASSERT_EQ( deviceId, 2 );

        deviceId = qnn.GetQnnDeviceId( QNN_PROCESSOR_HTP3 );
        ASSERT_EQ( deviceId, 3 );

        deviceId = qnn.GetQnnDeviceId( QNN_PROCESSOR_CPU );
        ASSERT_EQ( deviceId, 0 );

        deviceId = qnn.GetQnnDeviceId( QNN_PROCESSOR_GPU );
        ASSERT_EQ( deviceId, 0 );

        deviceId = qnn.GetQnnDeviceId( QNN_PROCESSOR_MAX );
        ASSERT_EQ( deviceId, UINT32_MAX );
    }

    {
        QnnImpl qnn( nodeId, logger );
        QnnImplConfig_t &cfg = qnn.GetConifg();
        cfg.loadType = QNN_LOAD_CONTEXT_BIN_FROM_FILE;
        cfg.processorType = QNN_PROCESSOR_HTP0;
        cfg.coreIds = { 0 };
        cfg.priority = QNN_PRIORITY_NORMAL;
        cfg.perfProfile = QNN_PERF_PROFILE_DEFAULT;
        std::vector<std::reference_wrapper<QCBufferDescriptorBase>> buffers;

        cfg.modelPath = "";
        status = qnn.Initialize( nullptr, buffers );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        cfg.loadType = QNN_LOAD_CONTEXT_BIN_FROM_BUFFER;
        cfg.contextBufferId = 1;
        status = qnn.Initialize( nullptr, buffers );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        cfg.modelPath = "data/centernet/program.bin";
        cfg.loadType = QNN_LOAD_CONTEXT_BIN_FROM_FILE;
        cfg.processorType = QNN_PROCESSOR_MAX;
        cfg.coreIds = { 0 };
        status = qnn.Initialize( nullptr, buffers );
        ASSERT_EQ( QC_STATUS_FAIL, status );

        cfg.modelPath = "data/centernet/program.bin";
        cfg.loadType = QNN_LOAD_CONTEXT_BIN_FROM_FILE;
        cfg.processorType = QNN_PROCESSOR_HTP0;
        cfg.coreIds = { UINT32_MAX };
        status = qnn.Initialize( nullptr, buffers );
        ASSERT_EQ( QC_STATUS_FAIL, status );
    }
}

TEST_F( QnnTest, GetQnnFunctionPointersDllOpenFailed )
{
    // NOTE: do move libQnnHtp.so as libQnnHtp.so.bak and then recover it
    Initialize( "DLOPEN", "binary", "data/centernet/program.bin", "htp0" );
    ASSERT_EQ( ret, QC_STATUS_FAIL );
}

TEST_F( QnnTest, GetQnnSystemFunctionPointersDllOpenFailed )
{
    // NOTE: do move libQnnSystem.so as libQnnSystem.so.bak and then recover it
    Initialize( "DLOPEN", "binary", "data/centernet/program.bin", "htp0" );
    ASSERT_EQ( ret, QC_STATUS_FAIL );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif