// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include <chrono>
#include <cinttypes>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include "QC/component/QnnRuntime.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"
#include "QnnSampleAppUtils.hpp"

using namespace QC::component;
using namespace QC;

static bool s_bDisableDumpingOutputs = false;

const char *s_processorName[] = {
        "HTP0",
        "HTP1",
        "CPU",
        "GPU",
};

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

typedef struct
{
    void *pData;
    size_t size;
} QnnTest_Buffer_t;

typedef struct
{
    std::string name;
    std::string modelPath;
    int nLoops = 100;
    QCProcessorType_e processor = QC_PROCESSOR_HTP0;
    std::vector<QnnTest_Buffer_t> inputs;
    std::vector<QnnRuntime_UdoPackage_t> opPackagePaths;
    int tid; /* deploy this on which thread */
    int delayMs = 0;
    int periodMs = 0;
    int batchMultiplier = 1;
} QnnTest_Parameters_t;

static std::string GetTensorInfoStr( const QnnRuntime_TensorInfo_t &info )
{
    std::string str = "";
    std::stringstream ss;

    ss << "type=" << info.properties.type << " dims=[";
    for ( uint32_t i = 0; i < info.properties.numDims; i++ )
    {
        ss << info.properties.dims[i] << ", ";
    }
    ss << "] scale=" << info.quantScale << " offset=" << info.quantOffset;
    str = ss.str();

    return str;
}

class QnnTestRunner
{
public:
    QnnTestRunner() {}
    ~QnnTestRunner() {}

    QCStatus_e Init( QnnTest_Parameters_t &params )
    {
        m_params = params;

        QCStatus_e ret = QC_STATUS_OK;
        auto &name = m_params.name;
        auto &modelPath = m_params.modelPath;
        auto processor = m_params.processor;
        auto &inputs = m_params.inputs;
        const int batchMultiplier = m_params.batchMultiplier;
        printf( "[%s] Test models %s run %d nLoops on processor %d with %" PRIu64 " inputs\n",
                name.c_str(), modelPath.c_str(), m_params.nLoops, processor, inputs.size() );

        auto begin = std::chrono::high_resolution_clock::now();
        m_config = { QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE,
                     modelPath.c_str(),
                     nullptr,
                     0,
                     processor,
                     QNN_PRIORITY_DEFAULT,
                     nullptr,
                     0 };
        if ( ( QC_PROCESSOR_CPU == processor ) || ( QC_PROCESSOR_GPU == processor ) )
        {
            m_config.loadType = QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
        }

        if ( m_params.opPackagePaths.size() > 0 )
        {
            m_config.numOfUdoPackages = m_params.opPackagePaths.size();
            m_config.pUdoPackages = &m_params.opPackagePaths[0];
        }

        ret = m_qnn.Init( name.c_str(), &m_config, LOGGER_LEVEL_INFO );
        if ( QC_STATUS_OK != ret )
        {
            printf( "[%s] Failed to create QNN Runtime, error is %d\n", name.c_str(), ret );
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        printf( "[%s] Init cost %.2f ms\n", name.c_str(), (float) cost / 1000.0 );

        if ( QC_STATUS_OK == ret )
        {
            ret = m_qnn.GetInputInfo( &m_inputInfoList );
        }

        if ( QC_STATUS_OK == ret )
        {
            m_inputBuffers.resize( m_inputInfoList.num );
        }


        uint32_t outputNum = 0;
        if ( QC_STATUS_OK == ret )
        {
            ret = m_qnn.GetOutputInfo( &m_outputInfoList );
        }

        if ( QC_STATUS_OK == ret )
        {
            m_outputBuffers.resize( m_outputInfoList.num );
        }

        for ( size_t i = 0; ( i < m_inputInfoList.num ) && ( QC_STATUS_OK == ret ); i++ )
        {
            auto &info = m_inputInfoList.pInfo[i];
            printf( "[%s] input %" PRIu64 " name=%s %s\n", name.c_str(), i, info.pName,
                    GetTensorInfoStr( info ).c_str() );
            QCTensorProps_t tensorProperties = info.properties;
            tensorProperties.dims[0] = tensorProperties.dims[0] * batchMultiplier;
            ret = m_inputBuffers[i].Allocate( &tensorProperties );
            if ( QC_STATUS_OK == ret )
            {
                if ( i < inputs.size() )
                {
                    auto inputSize = m_inputBuffers[i].size;
                    printf( "[%s] load real input for input %" PRIu64 "\n", name.c_str(), i );
                    if ( inputs[i].size == inputSize )
                    {
                        memcpy( m_inputBuffers[i].data(), inputs[i].pData, inputSize );
                    }
                    else if ( ( inputs[i].size == ( inputSize * sizeof( float ) ) ) &&
                              ( info.quantScale != 0 ) )
                    {   // guess an pFile32 input for DMA UINT8 IO
                        printf( "[%s] quantize it\n", name.c_str() );
                        auto scale = info.quantScale;
                        auto offset = info.quantOffset;
                        float *fData = (float *) inputs[i].pData;
                        uint8_t *pData = (uint8_t *) m_inputBuffers[i].data();
                        for ( size_t j = 0; j < inputSize; j++ )
                        {
                            pData[j] = (uint8_t) std::min(
                                    std::max( std::round( fData[j] / scale - offset ), 0.0f ),
                                    255.0f );
                        }
                        if ( false == s_bDisableDumpingOutputs )
                        {
                            SaveRaw( name + "quantize.raw", pData, inputSize );
                        }
                    }
                    else
                    {
                        printf( "input size not correct, abort test\n" );
                        ret = QC_STATUS_FAIL;
                    }
                }
            }
            else
            {
                printf( "[%s] Failed to allocate buffer for input %" PRIu64 "\n", name.c_str(), i );
                ret = QC_STATUS_NOMEM;
            }
        }

        for ( size_t i = 0; ( i < m_outputInfoList.num ) && ( QC_STATUS_OK == ret ); i++ )
        {
            auto &info = m_outputInfoList.pInfo[i];
            printf( "[%s] output %" PRIu64 " name=%s %s\n", name.c_str(), i, info.pName,
                    GetTensorInfoStr( info ).c_str() );
            QCTensorProps_t tensorProperties = info.properties;
            tensorProperties.dims[0] = tensorProperties.dims[0] * batchMultiplier;
            ret = m_outputBuffers[i].Allocate( &tensorProperties );
            if ( QC_STATUS_OK != ret )
            {
                printf( "[%s] Failed to allocate buffer for output %" PRIu64 "\n", name.c_str(),
                        i );
                ret = QC_STATUS_NOMEM;
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = m_qnn.Start();
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = m_qnn.EnablePerf();
        }

        return ret;
    }

    QCStatus_e Run()
    {
        QCStatus_e ret = QC_STATUS_OK;
        auto &name = m_params.name;
        auto processor = m_params.processor;
        auto &inputs = m_params.inputs;
        QnnRuntime_Perf_t perf = { 0, 0, 0, 0 };

        if ( 0 == m_iter )
        {
            m_start = std::chrono::high_resolution_clock::now();
        }

        auto begin = std::chrono::high_resolution_clock::now();
        ret = m_qnn.Execute( m_inputBuffers.data(), m_inputBuffers.size(), m_outputBuffers.data(),
                             m_outputBuffers.size() );
        if ( QC_STATUS_OK != ret )
        {
            printf( "[%s] Failed to run, error is %d\n", name.c_str(), ret );
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        if ( QC_STATUS_OK == ret )
        {
            ret = m_qnn.GetPerf( &perf );
            m_total += cost;
            if ( QC_STATUS_OK == ret )
            {
                m_totalQnn += perf.entireExecTime;
                m_totalRpc += perf.rpcExecTimeCPU;
                m_totalQnnAcc += perf.rpcExecTimeHTP;
                m_totalAcc += perf.rpcExecTimeAcc;
            }
            printf( "[%s-%s-%d] %d: %.2f ms, perf(us): QNN=%" PRIu64 " RPC=%" PRIu64
                    " QNN ACC=%" PRIu64 " ACC=%" PRIu64 "\n",
                    name.c_str(), s_processorName[processor], m_params.tid, m_iter,
                    (float) cost / 1000.0, perf.entireExecTime, perf.rpcExecTimeCPU,
                    perf.rpcExecTimeHTP, perf.rpcExecTimeAcc );
            if ( ( inputs.size() > 0 ) && ( false == s_bDisableDumpingOutputs ) )
            {
                for ( size_t i = 0; i < m_outputBuffers.size(); i++ )
                {
                    std::string path = name + "_output" + std::to_string( i ) + ".raw";
                    SaveRaw( path, m_outputBuffers[i].data(), m_outputBuffers[i].size );
                    auto &info = m_outputInfoList.pInfo[i];
                    // enhanced way to dump the output for accuracy analyze
                    uint8_t *pData = (uint8_t *) m_outputBuffers[i].data();
                    float *fData = new float[m_outputBuffers[i].size];
                    for ( size_t j = 0; j < m_outputBuffers[i].size; j++ )
                    {
                        fData[j] = info.quantScale * ( pData[j] + info.quantOffset );
                    }
                    path = name + "_" + std::string( info.pName ) + ".raw";
                    SaveRaw( path, fData, m_outputBuffers[i].size * sizeof( float ) );
                    delete[] fData;
                }
            }

            m_iter++;


            if ( 0 == ( m_iter % m_params.nLoops ) )
            {
                auto Tend = std::chrono::high_resolution_clock::now();
                cost = std::chrono::duration_cast<std::chrono::microseconds>( Tend - m_start )
                               .count();
                float FPS = m_iter / ( cost / 1000000.f );
                printf( "[%s-%s-%d-%d] CPU: avg %.2f ms, %.2f FPS;"
                        " QNN: avg %.2f ms %.2f FPS;"
                        " RPC: avg %.2f ms %.2f FPS;"
                        " QNN ACC: avg %.2f ms %.2f FPS;"
                        " ACC: avg %.2f ms %.2f FPS\n",
                        name.c_str(), s_processorName[processor], m_params.tid, m_iter,
                        (float) m_total / m_iter / 1000.0, FPS,
                        (float) m_totalQnn / m_iter / 1000.0,
                        (float) 1000000.0 / ( m_totalQnn / m_iter ),
                        (float) m_totalRpc / m_iter / 1000.0,
                        (float) 1000000.0 / ( m_totalRpc / m_iter ),
                        (float) m_totalQnnAcc / m_iter / 1000.0,
                        (float) 1000000.0 / ( m_totalQnnAcc / m_iter ),
                        (float) m_totalAcc / m_iter / 1000.0,
                        (float) 1000000.0 / ( m_totalAcc / m_iter ) );
            }
        }
        return ret;
    }

    void Deinit()
    {
        auto name = m_params.name;
        auto begin = std::chrono::high_resolution_clock::now();
        m_qnn.Stop();
        m_qnn.Deinit();
        auto end = std::chrono::high_resolution_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        printf( "[%s] Deinit cost %.2f ms\n", name.c_str(), (float) cost / 1000.0 );
    }

    int GetnLoops() { return m_params.nLoops; }
    int GetTID() { return m_params.tid; }

    int GetDelay() { return m_params.delayMs; }
    int GetPeriod() { return m_params.periodMs; }

private:
    QnnTest_Parameters_t m_params;
    QnnRuntime_Config_t m_config;
    QnnRuntime m_qnn;
    QnnRuntime_TensorInfoList_t m_inputInfoList;
    QnnRuntime_TensorInfoList_t m_outputInfoList;
    std::vector<QCSharedBuffer_t> m_inputBuffers;
    std::vector<QCSharedBuffer_t> m_outputBuffers;
    uint64_t m_total = 0;
    uint64_t m_totalQnn = 0;
    uint64_t m_totalRpc = 0;
    uint64_t m_totalQnnAcc = 0;
    uint64_t m_totalAcc = 0;
    int m_iter = 0;

    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

bool ThreadMain( int nLoops, std::vector<std::shared_ptr<QnnTestRunner>> runners )
{
    int tid = runners[0]->GetTID();
    int delayMs = runners[0]->GetDelay();
    uint64_t periodUs = (uint64_t) runners[0]->GetPeriod() * 1000;
    uint64_t total = 0;

    std::this_thread::sleep_for( std::chrono::milliseconds( delayMs ) );
    for ( int i = 0; i < nLoops; i++ )
    {
        auto begin = std::chrono::high_resolution_clock::now();
        for ( auto &runner : runners )
        {
            runner->Run();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto cost = std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count();
        total += cost;
        if ( (uint64_t) cost < periodUs )
        {
            std::this_thread::sleep_for( std::chrono::microseconds( periodUs - cost ) );
        }

        if ( runners.size() > 1 )
        {
            printf( "[%d] %d nLoops cost %.2f ms\n", tid, i, (float) cost / 1000.0 );
        }
    }
    if ( runners.size() > 1 )
    {
        printf( "[%d] avg cost %.2f ms\n", tid, (float) total / nLoops / 1000.0 );
    }

    return QC_STATUS_OK;
}

int Usage( char *prog, int error )
{
    printf( "usage: %s"
            " -n name -m model_path -p processor [-b batch_multiplier][-t tid] [-l nLoops] [-i "
            "input.raw]* [-P period] "
            "[-S start] [-d]\n"
            "  With below options to create a QNN tester:\n"
            "    -n name: The unique QNN tester name, must be before options -m/-p/-l/-i/-P/-S\n"
            "    -m model_path: The QNN model path, for HTP, it was context bin, others, it was\n"
            "      shared library\n"
            "    -t tid: The thread id to run the QNN model, those tester with the same tid run\n"
            "      sequentially in the same thread\n"
            "    -p processor: The processor, 0: HTP0, 1: HTP1, 2: CPU, 3:GPU\n"
            "    -b batch_multiplier:  Specifies the value with which the batch value in input and "
            "output tensors dimensions will be multiplied.\n"
            "    -u udo: Specifies udo lib path and interface provider name."
            " e.g. libQnnAutoAiswOpPackage.so:AutoAiswOpPackageInterfaceProvider\n"
            "    -l nLoops: optional, specify the iterations that to call QNN Execute\n"
            "    -i input.raw: optional, specify the input raw file to the QNN model, repeat this\n"
            "      option if the QNN model has multiple inputs\n"
            "    -P period: optional, the period in ms of the QNN Execute call\n"
            "    -S start: optional, the delay time in ms to start to do the QNN Execute call\n"
            "  Repeat above options to create multiple testers\n"
            "  Other miscellaneous options:\n"
            "    -d: disable dumping outputs even if the input raw files specified\n",
            prog );

    return error;
}

int main( int argc, char *argv[] )
{
    std::vector<QnnTest_Parameters_t> paramsList;

    int flags, opt;
    while ( ( opt = getopt( argc, argv, "n:m:p:b:u:t:l:i:P:S:dh" ) ) != -1 )
    {
        switch ( opt )
        {
            case 'n':
            {
                QnnTest_Parameters_t params;
                params.name = optarg;
                params.tid = paramsList.size();
                paramsList.push_back( params );
                break;
            }
            case 'm':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                params.modelPath = optarg;
                break;
            }
            case 'p':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                params.processor = (QCProcessorType_e) atoi( optarg );
                if ( ( params.processor >= QC_PROCESSOR_MAX ) ||
                     ( params.processor < QC_PROCESSOR_HTP0 ) )
                {
                    printf( "invalid processor %s for %s\n", optarg, params.name.c_str() );
                    return -1;
                }
                break;
            }
            case 'b':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                params.batchMultiplier = atoi( optarg );
                break;
            }
            case 'u':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                std::vector<std::string> opPackagePaths;
                split( opPackagePaths, optarg, ',' );
                params.opPackagePaths.resize( opPackagePaths.size() );
                for ( int i = 0; i < opPackagePaths.size(); ++i )
                {
                    static std::vector<std::string> opPackage;
                    split( opPackage, opPackagePaths[i], ':' );
                    if ( opPackage.size() != 2 )
                    {
                        printf( "invalid opPackage params: %s\n", opPackagePaths[i].c_str() );
                        return -1;
                    }
                    params.opPackagePaths[i].udoLibPath = opPackage[0].c_str();
                    params.opPackagePaths[i].interfaceProvider = opPackage[1].c_str();
                    printf( "opPackage params %d, udoLibPath: %s, interfaceProvider: %s\n", i,
                            params.opPackagePaths[i].udoLibPath,
                            params.opPackagePaths[i].interfaceProvider );
                }
                break;
            }
            case 't':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                params.tid = atoi( optarg );
                break;
            }
            case 'l':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                params.nLoops = atoi( optarg );
                break;
            }
            case 'i':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                QnnTest_Buffer_t buffer;
                buffer.pData = LoadRaw( optarg, buffer.size );
                if ( nullptr == buffer.pData )
                {
                    return -1;
                }
                params.inputs.push_back( buffer );
                break;
            }
            case 'P':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                params.periodMs = atoi( optarg );
                break;
            }
            case 'S':
            {
                QnnTest_Parameters_t &params = paramsList.back();
                params.delayMs = atoi( optarg );
                break;
            }
            case 'd':
                s_bDisableDumpingOutputs = true;
                break;
            case 'h':
                return Usage( argv[0], 0 );
                break;
            default:
                return Usage( argv[0], -1 );
                break;
        }
    }
    std::map<int, std::vector<std::shared_ptr<QnnTestRunner>>> runnersMap;
    for ( auto &params : paramsList )
    {
        printf( "Test case %s model_path=%s nLoops=%d processor=%s tid=%d with %d inputs, delay %d "
                "ms, period %d ms\n",
                params.name.c_str(), params.modelPath.c_str(), params.nLoops,
                s_processorName[params.processor], params.tid, (int) params.inputs.size(),
                params.delayMs, params.periodMs );
        auto runner = std::make_shared<QnnTestRunner>();
        auto ret = runner->Init( params );
        if ( QC_STATUS_OK != ret )
        {
            return -1;
        }
        auto tid = params.tid;
        auto it = runnersMap.find( tid );
        if ( it == runnersMap.end() )
        {
            runnersMap[tid] = { runner };
        }
        else
        {
            runnersMap[tid].push_back( runner );
        }
    }

    std::vector<std::shared_ptr<std::thread>> threads;
    for ( auto &kv : runnersMap )
    {
        auto runners = kv.second;
        auto nLoops = runners[0]->GetnLoops();
        auto th = std::make_shared<std::thread>( ThreadMain, nLoops, runners );
        threads.push_back( th );
    }

    for ( auto &th : threads )
    {
        th->join();
    }

    for ( auto &kv : runnersMap )
    {
        auto runners = kv.second;
        for ( auto runner : runners )
        {
            runner->Deinit();
        }
    }

    return 0;
}
