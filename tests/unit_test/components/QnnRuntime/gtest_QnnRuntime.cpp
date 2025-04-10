// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#define QNNRUNTIME_UNIT_TEST
#include "accuracy.hpp"
#include "md5_utils.hpp"
#include "ridehal/component/QnnRuntime.hpp"
#include "gtest/gtest.h"
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdio.h>
#include <thread>

using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::test::utils;


#if defined( __QNXNTO__ )
#define QNN_GTEST_ENABLE_BUFFER_FREE
#endif

#if defined( __QNXNTO__ )
#define QNN_GTEST_ENABLE_ASYNC
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

static void Qnn_OutputCallback( void *pAppPriv, void *pOutputPriv )
{
    std::condition_variable *pCondVar = (std::condition_variable *) pAppPriv;
    uint64_t *pCounter = (uint64_t *) pOutputPriv;
    *pCounter += 1;
    pCondVar->notify_one();
}

static void Qnn_ErrorCallback( void *pAppPriv, void *pOutputPriv, Qnn_NotifyStatus_t notifyStatus )
{
    std::condition_variable *pCondVar = (std::condition_variable *) pAppPriv;
    uint64_t *pCounter = (uint64_t *) pOutputPriv;
    *pCounter = 0xdeadbeef;
    pCondVar->notify_one();
}

TEST( QnnRuntime, SANITY_General )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "SANITY";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnnRuntime.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, CreateModelFromBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "MODEL_FROM_BUF";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP1;
    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER;
    std::string modelPath = std::string( qnnConfig.modelPath );
    uint64_t bufferSize{ 0 };
    qnn::tools::datautil::StatusCode status{ qnn::tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = qnn::tools::datautil::getFileSize( modelPath );
    std::shared_ptr<uint8_t> buffer = std::shared_ptr<uint8_t>( new uint8_t[bufferSize] );
    qnn::tools::datautil::readBinaryFromFile(
            modelPath, reinterpret_cast<uint8_t *>( buffer.get() ), bufferSize );

    // buffer is null
    qnnConfig.contextBuffer = nullptr;
    qnnConfig.contextSize = bufferSize;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    // buffer size is 0
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = 0;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    // buffer size is incorrect
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = 8;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    // correct loading
    qnnConfig.contextBuffer = buffer.get();
    qnnConfig.contextSize = bufferSize;
    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_DEBUG );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, Perf )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "QPERF";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_INFO );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_ALREADY, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_Perf_t perf;
    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetPerf( (QnnRuntime_Perf_t *) nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    printf( "perf(us): QNN=%" PRIu64 " RPC=%" PRIu64 " QNN ACC=%" PRIu64 " ACC=%" PRIu64 "\n",
            perf.entireExecTime, perf.rpcExecTimeCPU, perf.rpcExecTimeHTP, perf.rpcExecTimeAcc );

    ret = qnnRuntime.DisablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DisablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, RegisterBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "REGBUF";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_WARN );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnnRuntime.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetInputInfo( (QnnRuntime_TensorInfoList_t *) nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.GetOutputInfo( (QnnRuntime_TensorInfoList_t *) nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    uint64_t dmaHandle = inputs[0].buffer.dmaHandle;
    void *pData = inputs[0].buffer.pData;
    inputs[0].buffer.dmaHandle = (uint64_t) -1;
    inputs[0].buffer.pData = (void *) 123;
    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    inputs[0].buffer.dmaHandle = dmaHandle;
    inputs[0].buffer.pData = pData;

    ret = qnnRuntime.RegisterBuffers( inputs, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.RegisterBuffers( (RideHal_SharedBuffer_t *) nullptr, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    // Register same buffer again
    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.RegisterBuffers( outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DeRegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    // DeRegister same buffer again
    ret = qnnRuntime.DeRegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

    ret = qnnRuntime.DeRegisterBuffers( inputs, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.DeRegisterBuffers( (RideHal_SharedBuffer_t *) nullptr, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, LoadModel )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "LOAD_MODEL";

    ret = qnnRuntime.Init( (const char *) nullptr, &qnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    qnnConfig.modelPath = "data/noexistingfile.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP1;

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.modelPath = nullptr;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    qnnConfig.modelPath = "data/centernet/zero_buffer_size.bin";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    // pConfig is null
    ret = qnnRuntime.Init( pName, nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
}

TEST( QnnRuntime, StateMachine )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QnnRuntime qnnRuntime;

    QnnRuntime_TensorInfoList_t infoList;
    ret = qnnRuntime.GetInputInfo( &infoList );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.GetOutputInfo( &infoList );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    RideHal_SharedBuffer_t sharedBuffer[1];
    ret = qnnRuntime.RegisterBuffers( sharedBuffer, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.DeRegisterBuffers( sharedBuffer, 1 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.DisablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    QnnRuntime_Perf_t perf;
    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
}

TEST( QnnRuntime, LoadOpPackage )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "OP_PKG";

    qnnConfig.modelPath = "data/bevdet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;
    const size_t numOfUdoPackages = 1;
    QnnRuntime_UdoPackage_t udoPackages[numOfUdoPackages];

    // numOfUdoPackages is - 1;
    qnnConfig.numOfUdoPackages = -1;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    //  pUdoPackages is null
    qnnConfig.pUdoPackages = nullptr;
    qnnConfig.numOfUdoPackages = numOfUdoPackages;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    // invalid so
    udoPackages[0].udoLibPath = "invalid.so";
    udoPackages[0].interfaceProvider = "AutoAiswOpPackageInterfaceProvider";
    qnnConfig.pUdoPackages = udoPackages;
    qnnConfig.numOfUdoPackages = numOfUdoPackages;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    udoPackages[0].udoLibPath = "libQnnAutoAiswOpPackage.so";
    udoPackages[0].interfaceProvider = "AutoAiswOpPackageInterfaceProvider";
    qnnConfig.pUdoPackages = udoPackages;
    qnnConfig.numOfUdoPackages = numOfUdoPackages;
    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, DataType )
{
    QnnRuntime qnnRuntime;
    EXPECT_EQ( QNN_DATATYPE_INT_8, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_16, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_32, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_INT_64, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_INT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_8, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_UINT_64,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UINT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_FLOAT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_FLOAT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_FLOAT_64,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_FLOAT_64 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_8,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_SFIXED_POINT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_8,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_16,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16 ) );
    EXPECT_EQ( QNN_DATATYPE_UFIXED_POINT_32,
               qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32 ) );

    EXPECT_EQ( QNN_DATATYPE_UNDEFINED, qnnRuntime.SwitchToQnnDataType( RIDEHAL_TENSOR_TYPE_MAX ) );

    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_8, qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_INT_64,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_INT_64 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_8,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UINT_64,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UINT_64 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_FLOAT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_FLOAT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_FLOAT_64,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_FLOAT_64 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_SFIXED_POINT_32 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_8 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_16 ) );
    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UFIXED_POINT_32 ) );

    EXPECT_EQ( RIDEHAL_TENSOR_TYPE_MAX,
               qnnRuntime.SwitchFromQnnDataType( QNN_DATATYPE_UNDEFINED ) );
}

TEST( QnnRuntime, CreateModelFromSo )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "FROM_SO";

    qnnConfig.processorType = RIDEHAL_PROCESSOR_CPU;
    qnnConfig.loadType = QnnRuntime_LoadType_e::QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;

    // invalid path
    qnnConfig.modelPath = "invalid.so";
    ret = qnnRuntime.Init( pName, pQnnConfig );
    EXPECT_EQ( RIDEHAL_ERROR_FAIL, ret );

#if defined( __QNXNTO__ )
    qnnConfig.modelPath = "data/centernet/aarch64-qnx/libqride_centernet.so";
#else
    qnnConfig.modelPath = "data/centernet/aarch64-oe-linux-gcc9.3/libqride_centernet.so";
    return;
    /* Note: the build lib complains version `GLIBCXX_3.4.29' not found */
#endif
    qnnConfig.loadType = QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
    qnnConfig.processorType = RIDEHAL_PROCESSOR_CPU;
    ret = qnnRuntime.Init( "CNT0", &qnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    /* not supported for CPU */
    ret = qnnRuntime.RegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_UNSUPPORTED, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /* NOTE: Now QnnRuntime has issue to get CPU performance data, it will failed but should not
     * block the execute  */
    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, DynamicBatchSize )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "DYN_BATCH";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /****** Dynamic dimension test ******/
    uint32_t batchSize = 10;
    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorInputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorOutputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    batchSize = 3;
    for ( int i = 0; i < inputNum; ++i )
    {
        inputs[i].tensorProps.dims[0] = batchSize;
        inputs[i].size = inputs[i].buffer.size / 10 * 3;
    }

    for ( int i = 0; i < outputNum; ++i )
    {
        outputs[i].tensorProps.dims[0] = batchSize;
        outputs[i].size = outputs[i].buffer.size / 10 * 3;
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, BufferFree )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "BUF_FREE";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /****** Dynamic dimension test ******/
    uint32_t batchSize = 3;
    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorInputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        RideHal_TensorProps_t properties = tensorOutputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DeRegisterBuffers( inputs, inputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.DeRegisterBuffers( outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    batchSize = 10;
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        RideHal_TensorProps_t properties = tensorInputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = inputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        RideHal_TensorProps_t properties = tensorOutputList.pInfo[i].properties;
        properties.dims[0] = batchSize;
        ret = outputs[i].Allocate( &properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, OneBufferMutipleTensors )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "ONE_BUF_MUL_TS";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];

    size_t outputTotalSize = 0;
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        outputTotalSize += outputs[i].size;
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /**************  one buffer  **************/
    RideHal_SharedBuffer_t outputs1[outputNum];
    RideHal_SharedBuffer_t sharedBuffer;
    size_t offset = 0u;
    sharedBuffer.Allocate( outputTotalSize, RIDEHAL_BUFFER_USAGE_HTP,
                           RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );
    for ( int i = 0; i < outputNum; ++i )
    {
        outputs1[i] = outputs[i];
        outputs1[i].buffer.size = outputTotalSize;
        outputs1[i].buffer.pData = (void *) ( (uint8_t *) sharedBuffer.buffer.pData + offset );
        outputs1[i].buffer.dmaHandle = sharedBuffer.buffer.dmaHandle;
        outputs1[i].offset = offset;
        offset += outputs1[i].size;
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs1, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( int i = 0; i < outputNum; ++i )
    {
        std::string md5OneBuffer = MD5Sum( outputs1[i].buffer.pData, outputs1[i].size );
        std::string md5Output = MD5Sum( outputs[i].buffer.pData, outputs[i].size );
        EXPECT_EQ( md5OneBuffer, md5Output );
    }

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = sharedBuffer.Free();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#endif
}

TEST( QnnRuntime, TestAccuracy )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "ACCURACY";

    std::mutex mtx;
    std::condition_variable condVar;
    uint64_t counter = 0;

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_ASYNC
    ret = qnnRuntime.RegisterCallback( Qnn_OutputCallback, Qnn_ErrorCallback, &condVar );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#endif
    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetInputInfo( &tensorInputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    std::vector<std::string> inputDataPaths;
    inputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_input.raw" );
    ASSERT_EQ( inputDataPaths.size(), inputNum );
    for ( int i = 0; i < inputNum; ++i )
    {
        size_t inputSize = 0;
        void *pInputData = LoadRaw( inputDataPaths[i], inputSize );
        ASSERT_EQ( inputs[i].size, inputSize );
        memcpy( inputs[i].data(), pInputData, inputSize );
        free( pInputData );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    }
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
#ifdef QNN_GTEST_ENABLE_ASYNC
    RideHal_SharedBuffer_t outputsAsync[outputNum];
#endif
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_ASYNC
        ret = outputsAsync[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#endif
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

#ifdef QNN_GTEST_ENABLE_ASYNC
    ret = qnnRuntime.Execute( inputs, inputNum, outputsAsync, outputNum, &counter );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    {
        std::unique_lock<std::mutex> lock( mtx );
        auto status = condVar.wait_for( lock, std::chrono::milliseconds( 1000 ) );
        ASSERT_EQ( std::cv_status::no_timeout, status );
        ASSERT_EQ( 1, counter );
    }
#endif
    std::vector<std::string> outputDataPaths;
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_0.raw" );
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_1.raw" );
    outputDataPaths.push_back( "data/test/qnn/centernet/gd_uint8_output_2.raw" );
    ASSERT_EQ( outputDataPaths.size(), outputNum );

    for ( int i = 0; i < outputNum; ++i )
    {
        size_t outputSize = 0;
        void *pOutputData = LoadRaw( outputDataPaths[i], outputSize );
        ASSERT_EQ( outputs[i].size, outputSize );
        double cosSim = CosineSimilarity( (uint8_t *) pOutputData,
                                          (uint8_t *) outputs[i].buffer.pData, outputSize );
        printf( "output: %d: cosine similarity = %f\n", i, (float) cosSim );
        ASSERT_GT( cosSim, 0.99d );
#ifdef QNN_GTEST_ENABLE_ASYNC
        cosSim = CosineSimilarity( (uint8_t *) pOutputData,
                                   (uint8_t *) outputsAsync[i].buffer.pData, outputSize );
        printf( "output async: %d: cosine similarity = %f\n", i, (float) cosSim );
        ASSERT_GT( cosSim, 0.99d );
#endif
        free( pOutputData );
    }
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_ASYNC
        ret = outputsAsync[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#endif
    }
#endif
}

TEST( QnnRuntime, TwoModelWithSameBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnn0, qnn1;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Init( "QNN0", pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn1.Init( "QNN1", pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn1.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnn0.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnn0.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnn0.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn1.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn1.Execute( inputs, inputNum, outputs, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn1.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn1.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

#ifdef QNN_GTEST_ENABLE_ASYNC
TEST( QnnRuntime, AsyncExecute )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    std::mutex mtx;
    std::condition_variable condVar;
    uint64_t counter = 0;
    QnnRuntime_Perf_t perf;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "ASYNC";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.RegisterCallback( Qnn_OutputCallback, Qnn_ErrorCallback, &condVar );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.RegisterCallback( nullptr, Qnn_ErrorCallback, &condVar );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.RegisterCallback( Qnn_OutputCallback, nullptr, &condVar );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.RegisterCallback( Qnn_OutputCallback, Qnn_ErrorCallback, nullptr );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.RegisterCallback( Qnn_OutputCallback, Qnn_ErrorCallback, &condVar );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.EnablePerf();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnnRuntime.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum, &counter );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    {
        std::unique_lock<std::mutex> lock( mtx );
        auto status = condVar.wait_for( lock, std::chrono::milliseconds( 1000 ) );
        ASSERT_EQ( std::cv_status::no_timeout, status );
        ASSERT_EQ( 1, counter );
    }

    ret = qnnRuntime.GetPerf( &perf );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    printf( "perf(us): QNN=%" PRIu64 " RPC=%" PRIu64 " QNN ACC=%" PRIu64 " ACC=%" PRIu64 "\n",
            perf.entireExecTime, perf.rpcExecTimeCPU, perf.rpcExecTimeHTP, perf.rpcExecTimeAcc );

    auto begin = std::chrono::high_resolution_clock::now();
    for ( int i = 0; i < QNNRUNTIME_NOTIFY_PARAM_NUM; i++ )
    {
        ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum, &counter );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    auto end = std::chrono::high_resolution_clock::now();

    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum, &counter );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    double elpasedMs =
            (double) std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count() /
            1000.d;

    printf( "counter = %" PRIu64 ", async perf=%.3f ms\n", counter,
            (float) ( elpasedMs / QNNRUNTIME_NOTIFY_PARAM_NUM ) );

    std::this_thread::sleep_for( std::chrono::microseconds(
            perf.entireExecTime * QNNRUNTIME_NOTIFY_PARAM_NUM + 1000000 ) );
    {
        std::unique_lock<std::mutex> lock( mtx );
        condVar.wait_for( lock, std::chrono::milliseconds( 1 ) );
        ASSERT_EQ( 1 + QNNRUNTIME_NOTIFY_PARAM_NUM, counter );
    }

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    ret = qnnRuntime.RegisterCallback( Qnn_OutputCallback, Qnn_ErrorCallback, &condVar );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}
#endif

TEST( QnnRuntime, InputOutputCheck )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnnRuntime;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;
    char pName[20] = "IOVAL";

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Init( pName, pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );


    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnnRuntime.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    RideHal_SharedBuffer_t inputs2[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        inputs2[i] = inputs[i];
    }

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnnRuntime.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    RideHal_SharedBuffer_t outputs2[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        outputs2[i] = outputs[i];
    }

    ret = qnnRuntime.Execute( inputs2, inputNum + 1, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum + 1 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    // input tensor props
    inputs2[0].buffer.pData = nullptr;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    inputs2[0] = inputs[0];

    inputs2[0].tensorProps.type = RIDEHAL_TENSOR_TYPE_INT_8;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    inputs2[0] = inputs[0];

    inputs2[0].tensorProps.numDims += 1;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    inputs2[0] = inputs[0];

    inputs2[0].tensorProps.dims[1] += 1;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    inputs2[0] = inputs[0];

    // output tensor props
    outputs2[0].buffer.pData = nullptr;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    outputs2[0] = outputs[0];

    outputs2[0].tensorProps.type = RIDEHAL_TENSOR_TYPE_INT_8;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    outputs2[0] = outputs[0];

    outputs2[0].tensorProps.numDims += 1;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    outputs2[0] = outputs[0];

    outputs2[0].tensorProps.dims[1] += 1;
    ret = qnnRuntime.Execute( inputs2, inputNum, outputs2, outputNum );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
    outputs2[0] = outputs[0];

    ret = qnnRuntime.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnnRuntime.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, ExecuteWithRegDeRegEachTime )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnn0;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ret = qnn0.Init( "QNNBR", pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnn0.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnn0.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    const uint32_t inputNum = tensorInputList.num;
    RideHal_SharedBuffer_t inputs[inputNum];
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    const uint32_t outputNum = tensorOutputList.num;
    RideHal_SharedBuffer_t outputs[outputNum];
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
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

        ret = qnn0.RegisterBuffers( inputs, inputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.RegisterBuffers( outputs, outputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.Execute( inputs, inputNum, outputs, outputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.DeRegisterBuffers( inputs, inputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.DeRegisterBuffers( outputs, outputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = qnn0.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

#ifdef QNN_GTEST_ENABLE_BUFFER_FREE
    for ( int i = 0; i < inputNum; ++i )
    {
        ret = inputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    for ( int i = 0; i < outputNum; ++i )
    {
        ret = outputs[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
#endif
}

TEST( QnnRuntime, ExecuteWithAllocBufferEachTime )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnn0;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    ret = qnn0.Init( "QNNBF", pQnnConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorInputList;
    ret = qnn0.GetInputInfo( &tensorInputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    QnnRuntime_TensorInfoList_t tensorOutputList;
    ret = qnn0.GetOutputInfo( &tensorOutputList );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    uint32_t loopNumber = 100;
    const char *envValue = getenv( "QNN_TEST_LOOP_NUMBER" );
    if ( nullptr != envValue )
    {
        loopNumber = (uint32_t) atoi( envValue );
    }
    for ( uint32_t l = 0; l < loopNumber; l++ )
    {
        printf( "ExecuteWithAllocBufferEachTime: %u\n", l );
        const uint32_t inputNum = tensorInputList.num;
        RideHal_SharedBuffer_t inputs[inputNum];
        for ( int i = 0; i < inputNum; ++i )
        {
            ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        const uint32_t outputNum = tensorOutputList.num;
        RideHal_SharedBuffer_t outputs[outputNum];
        for ( int i = 0; i < outputNum; ++i )
        {
            ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        ret = qnn0.RegisterBuffers( inputs, inputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.RegisterBuffers( outputs, outputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.Execute( inputs, inputNum, outputs, outputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.DeRegisterBuffers( inputs, inputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.DeRegisterBuffers( outputs, outputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        for ( int i = 0; i < inputNum; ++i )
        {
            ret = inputs[i].Free();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
        for ( int i = 0; i < outputNum; ++i )
        {
            ret = outputs[i].Free();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
    }

    ret = qnn0.Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = qnn0.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
}


TEST( QnnRuntime, InitDeInitStress )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    QnnRuntime qnn0;
    QnnRuntime_Config_t qnnConfig;
    QnnRuntime_Config_t *pQnnConfig = &qnnConfig;

    qnnConfig.modelPath = "data/centernet/program.bin";
    qnnConfig.processorType = RIDEHAL_PROCESSOR_HTP0;

    uint32_t loopNumber = 100;
    const char *envValue = getenv( "QNN_TEST_LOOP_NUMBER" );
    if ( nullptr != envValue )
    {
        loopNumber = (uint32_t) atoi( envValue );
    }
    for ( uint32_t l = 0; l < loopNumber; l++ )
    {
        printf( "InitDeInitStress: %u\n", l );

        ret = qnn0.Init( "QNNBF", pQnnConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        QnnRuntime_TensorInfoList_t tensorInputList;
        ret = qnn0.GetInputInfo( &tensorInputList );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        QnnRuntime_TensorInfoList_t tensorOutputList;
        ret = qnn0.GetOutputInfo( &tensorOutputList );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        const uint32_t inputNum = tensorInputList.num;
        RideHal_SharedBuffer_t inputs[inputNum];
        for ( int i = 0; i < inputNum; ++i )
        {
            ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        const uint32_t outputNum = tensorOutputList.num;
        RideHal_SharedBuffer_t outputs[outputNum];
        for ( int i = 0; i < outputNum; ++i )
        {
            ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        ret = qnn0.Execute( inputs, inputNum, outputs, outputNum );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = qnn0.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        for ( int i = 0; i < inputNum; ++i )
        {
            ret = inputs[i].Free();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
        for ( int i = 0; i < outputNum; ++i )
        {
            ret = outputs[i].Free();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
    }
}


#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
