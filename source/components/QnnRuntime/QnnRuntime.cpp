// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/component/QnnRuntime.hpp"
#include "DataUtil.hpp"
#include "HTP/QnnHtpCommon.h"
#include "HTP/QnnHtpMem.h"
#include "HTP/QnnHtpProfile.h"
#include "Logger.hpp"
#include "QnnProfile.h"
#include "QnnSampleAppUtils.hpp"
#include "QnnSdkBuildId.h"
#include "QnnTypeMacros.hpp"
#include <libgen.h>
#include <sstream>
#include <unistd.h>

using namespace qnn;
using namespace qnn::tools;

extern "C"
{
#include "fastrpc_api.h"
}
#include "rpcmem.h"
#pragma weak remote_session_control
#include "remote.h"

namespace ridehal
{
namespace component
{

std::mutex QnnRuntime::s_lock[RIDEHAL_PROCESSOR_MAX];
std::map<void *, int> QnnRuntime::s_dmaMemRefMap[RIDEHAL_PROCESSOR_MAX];

static std::map<RideHal_ProcessorType_e, const char *> s_Backends = {
        { RIDEHAL_PROCESSOR_HTP0, "libQnnHtp.so" },
        { RIDEHAL_PROCESSOR_HTP1, "libQnnHtp.so" },
        { RIDEHAL_PROCESSOR_CPU, "libQnnCpu.so" },
        { RIDEHAL_PROCESSOR_GPU, "libQnnGpu.so" } };

QnnRuntime::QnnRuntime() {}

void QnnLog_Callback( const char *fmt, QnnLog_Level_t logLevel, uint64_t timestamp, va_list args )
{
#ifndef DISABLE_RIDEHAL_LOG
    switch ( logLevel )
    {
        case QNN_LOG_LEVEL_VERBOSE:
            Logger::GetDefault().Log( LOGGER_LEVEL_VERBOSE, fmt, args );
            break;
        case QNN_LOG_LEVEL_DEBUG:
            Logger::GetDefault().Log( LOGGER_LEVEL_DEBUG, fmt, args );
            break;
        case QNN_LOG_LEVEL_INFO:
            Logger::GetDefault().Log( LOGGER_LEVEL_INFO, fmt, args );
            break;
        case QNN_LOG_LEVEL_WARN:
            Logger::GetDefault().Log( LOGGER_LEVEL_WARN, fmt, args );
            break;
        case QNN_LOG_LEVEL_ERROR:
            Logger::GetDefault().Log( LOGGER_LEVEL_ERROR, fmt, args );
            break;
        default:
            Logger::GetDefault().Log( LOGGER_LEVEL_VERBOSE, fmt, args );
            break;
    }
#endif
}

RideHalError_e QnnRuntime::CreateFromModelSo( std::string modelFile )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    Qnn_ErrorHandle_t retVal = m_qnnFunctionPointers.qnnInterface.contextCreate(
            m_backendHandle, m_deviceHandle, (const QnnContext_Config_t **) m_contextConfig,
            &m_context );
    if ( QNN_CONTEXT_NO_ERROR != retVal )
    {
        RIDEHAL_ERROR( "Could not create context, error is %" PRIu64, retVal );
        ret = RIDEHAL_ERROR_FAIL;
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        qnn_wrapper_api::ModelError_t retME = m_qnnFunctionPointers.composeGraphsFnHandle(
                m_backendHandle, m_qnnFunctionPointers.qnnInterface, m_context,
                (const qnn_wrapper_api::GraphConfigInfo_t **) m_graphConfigsInfo,
                m_graphConfigsInfoCount, &m_graphsInfo, &m_graphsCount, false, &QnnLog_Callback,
                QNN_LOG_LEVEL_ERROR );
        if ( qnn_wrapper_api::ModelError_t::MODEL_NO_ERROR != retME )
        {
            RIDEHAL_ERROR( "Failed in composeGraphs(), error is %d", retME );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_graphsCount != 1 )
        {
            RIDEHAL_ERROR( "too much graphs" );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.graphFinalize(
                ( *m_graphsInfo )[0].graph, m_profileBackendHandle, nullptr );
        if ( QNN_GRAPH_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Failed in graphFinalize()" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        Qnn_ContextBinarySize_t binaryBufferSize;
        retVal = m_qnnFunctionPointers.qnnInterface.contextGetBinarySize( m_context,
                                                                          &binaryBufferSize );
        if ( QNN_GRAPH_NO_ERROR == retVal )
        {
            std::unique_ptr<uint8_t[]> saveBuffer( new uint8_t[binaryBufferSize] );
            Qnn_ContextBinarySize_t writtenBufferSize;

            if ( QNN_GRAPH_NO_ERROR == m_qnnFunctionPointers.qnnInterface.contextGetBinary(
                                               m_context,
                                               reinterpret_cast<void *>( saveBuffer.get() ),
                                               binaryBufferSize, &writtenBufferSize ) )
            {
                RIDEHAL_INFO( "saving cached binary(size = %llu)", writtenBufferSize );
                std::string fileDir = dirname( (char *) modelFile.c_str() );
                auto dataUtilStatus = tools::datautil::writeBinaryToFile(
                        fileDir, "programGenFromSo.bin", (uint8_t *) saveBuffer.get(),
                        writtenBufferSize );
                if ( tools::datautil::StatusCode::SUCCESS != dataUtilStatus )
                {
                    RIDEHAL_ERROR( "Error while writing binary to file." );
                    ret = RIDEHAL_ERROR_FAIL;
                }
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::CreateFromBinaryBuffer( uint8_t *pBuffer, uint64_t bufferSize )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    const QnnSystemContext_BinaryInfo_t *binaryInfo{ nullptr };
    Qnn_ContextBinarySize_t binaryInfoSize{ 0 };
    Qnn_ErrorHandle_t retVal = m_qnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo(
            m_systemContext, static_cast<void *>( pBuffer ), bufferSize, &binaryInfo,
            &binaryInfoSize );
    if ( QNN_SUCCESS != retVal )
    {
        RIDEHAL_ERROR( "Failed to get context binary info." );
        ret = RIDEHAL_ERROR_FAIL;
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        bool bOK = copyMetadataToGraphsInfo( binaryInfo, m_graphsInfo, m_graphsCount );
        if ( false == bOK )
        {
            RIDEHAL_ERROR( "Failed to copy metadata." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == m_qnnFunctionPointers.qnnInterface.contextCreateFromBinary )
        {
            RIDEHAL_ERROR( "contextCreateFromBinaryFnHandle is nullptr." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.contextCreateFromBinary(
                m_backendHandle, m_deviceHandle, (const QnnContext_Config_t **) m_contextConfig,
                static_cast<void *>( pBuffer ), bufferSize, &m_context, m_profileBackendHandle );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Could not create context from binary. Error is %d ", retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    for ( size_t graphIdx = 0; graphIdx < m_graphsCount; graphIdx++ )
    {
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            if ( nullptr == m_qnnFunctionPointers.qnnInterface.graphRetrieve )
            {
                RIDEHAL_ERROR( "graphRetrieveFnHandle is nullptr." );
                ret = RIDEHAL_ERROR_FAIL;
                break;
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            retVal = m_qnnFunctionPointers.qnnInterface.graphRetrieve(
                    m_context, ( *m_graphsInfo )[graphIdx].graphName,
                    &( ( *m_graphsInfo )[graphIdx].graph ) );
            if ( QNN_SUCCESS != retVal )
            {
                RIDEHAL_ERROR( "Unable to retrieve graph handle for graph Idx: %d, error is %d",
                               graphIdx, (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}


RideHalError_e QnnRuntime::CreateFromBinaryFile( std::string modelFile )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( -1 == access( modelFile.c_str(), F_OK ) )
    {
        RIDEHAL_ERROR( "No existing file: %s", modelFile.c_str() );
        ret = RIDEHAL_ERROR_FAIL;
    }

    size_t bufferSize{ 0 };
    uint8_t *pBuffer = nullptr;
    // read serialized binary into a byte buffer
    tools::datautil::StatusCode status{ tools::datautil::StatusCode::SUCCESS };
    std::tie( status, bufferSize ) = tools::datautil::getFileSize( modelFile );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( 0 == bufferSize )
        {
            RIDEHAL_ERROR( "Received path to an empty file. Nothing to deserialize." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        pBuffer = (uint8_t *) malloc( bufferSize );
        if ( nullptr == pBuffer )
        {
            RIDEHAL_ERROR( "Failed to allocate memory." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        status = tools::datautil::readBinaryFromFile( modelFile, pBuffer, bufferSize );
        if ( status != tools::datautil::StatusCode::SUCCESS )
        {
            RIDEHAL_ERROR( "Failed to read binary data." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = CreateFromBinaryBuffer( pBuffer, bufferSize );
    }

    if ( nullptr != pBuffer )
    {
        free( pBuffer );
    }

    return ret;
}

RideHalError_e QnnRuntime::LoadOpPackages( QnnRuntime_UdoPackage_t *pUdoPackages,
                                           int numOfUdoPackages )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;

    for ( size_t i = 0; i < numOfUdoPackages; ++i )
    {
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            QnnRuntime_UdoPackage_t *pUdoPackage = &pUdoPackages[i];
            RIDEHAL_INFO( "Registered Op Package: %s and interface provider: %s",
                          pUdoPackage->udoLibPath, pUdoPackage->interfaceProvider );
            retVal = m_qnnFunctionPointers.qnnInterface.backendRegisterOpPackage(
                    m_backendHandle, (char *) pUdoPackage->udoLibPath,
                    (char *) pUdoPackage->interfaceProvider, nullptr );
            if ( QNN_BACKEND_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "Could not register Op Package: %s and interface provider: %s, "
                               "error is %d",
                               pUdoPackage->udoLibPath, pUdoPackage->interfaceProvider,
                               (int) retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
            RIDEHAL_INFO( "Registered Op Package: %s and interface provider: %s",
                          pUdoPackage->udoLibPath, pUdoPackage->interfaceProvider );
        }
    }
    return ret;
}

QnnLog_Level_t QnnRuntime::GetQnnLogLevel( Logger_Level_e level )
{
    QnnLog_Level_t qnnLogLevel = QNN_LOG_LEVEL_WARN;
    Logger_Level_e lvl = level;

#ifndef DISABLE_RIDEHAL_LOG
    lvl = m_logger.GetLevel();
#endif
    switch ( lvl )
    {
        case LOGGER_LEVEL_VERBOSE:
            qnnLogLevel = QNN_LOG_LEVEL_VERBOSE;
            break;
        case LOGGER_LEVEL_DEBUG:
            qnnLogLevel = QNN_LOG_LEVEL_DEBUG;
            break;
        case LOGGER_LEVEL_INFO:
            qnnLogLevel = QNN_LOG_LEVEL_INFO;
            break;
        case LOGGER_LEVEL_WARN:
            qnnLogLevel = QNN_LOG_LEVEL_WARN;
            break;
        case LOGGER_LEVEL_ERROR:
            qnnLogLevel = QNN_LOG_LEVEL_ERROR;
            break;
        default: /* warn level */
            break;
    }

    return qnnLogLevel;
}

RideHalError_e QnnRuntime::Init( const char *pName, const QnnRuntime_Config_t *pConfig,
                                 Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    bool bIFInitOK = false;
    Qnn_ErrorHandle_t retVal;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        bIFInitOK = true;
        if ( nullptr == pConfig )
        {
            RIDEHAL_ERROR( "QnnRuntime Config is nullptr!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( ( QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER != pConfig->loadType ) &&
                  ( nullptr == pConfig->modelPath ) )
        {
            RIDEHAL_ERROR( "modelPath is null" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( ( QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER == pConfig->loadType ) &&
                  ( ( nullptr == pConfig->contextBuffer ) || ( 0 == pConfig->contextSize ) ) )
        {
            RIDEHAL_ERROR( "contextBuffer is null or 0 size" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( ( pConfig->numOfUdoPackages > 0 ) && ( nullptr == pConfig->pUdoPackages ) )
        {
            RIDEHAL_ERROR( "pUdoPackages is null" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( pConfig->numOfUdoPackages < 0 )
        {
            RIDEHAL_ERROR( "UdoPackages size is less than 0: %d", pConfig->numOfUdoPackages );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            /* OK */
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_backendType = pConfig->processorType;
        if ( m_backendType == RIDEHAL_PROCESSOR_HTP1 )
        {
            m_backendCoreId = 1;
        }
        else
        {
            m_backendCoreId = 0;
        }
    }

    std::string modelPath;
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        modelPath = pName; /* intend to set path with instance name */
        m_bLoadFromCachedBinary =
                ( ( QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE == pConfig->loadType ) ||
                  ( QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER == pConfig->loadType ) );
        if ( QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER != pConfig->loadType )
        {
            modelPath = std::string( pConfig->modelPath );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        auto statusCode = dynamicloadutil::getQnnFunctionPointers(
                s_Backends[m_backendType], modelPath, &m_qnnFunctionPointers, &m_backendHandle,
                !m_bLoadFromCachedBinary, &m_modelHandle );
        if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
        {
            RIDEHAL_ERROR( "failed to get qnn function pointers from model %s(%s), error is %d",
                           modelPath.c_str(), s_Backends[m_backendType], statusCode );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        QnnLog_Error_t logError;
        auto logLevel = GetQnnLogLevel( level );

        (void) qnn::log::Logger::createLogger( &QnnLog_Callback, logLevel, &logError );

        retVal = m_qnnFunctionPointers.qnnInterface.logCreate( &QnnLog_Callback, logLevel,
                                                               &m_logHandle );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_WARN( "Unable to initialize logging in the backend. error is %d",
                          (int) retVal );
        }
    }

    auto statusCode = dynamicloadutil::getQnnSystemFunctionPointers( "libQnnSystem.so",
                                                                     &m_qnnFunctionPointers );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( dynamicloadutil::StatusCode::SUCCESS != statusCode )
        {
            RIDEHAL_ERROR( "Error initializing QNN System Function Pointers" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextCreate ) ||
             ( nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo ) ||
             ( nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextFree ) )
        {
            RIDEHAL_ERROR( "QNN System function pointers are not populated." );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        retVal = m_qnnFunctionPointers.qnnSystemInterface.systemContextCreate( &m_systemContext );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Could not create system handle. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.backendCreate(
                m_logHandle, (const QnnBackend_Config_t **) m_backendConfig, &m_backendHandle );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not initialize backend due to error = %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        Qnn_ApiVersion_t version;
        retVal = m_qnnFunctionPointers.qnnInterface.backendGetApiVersion( &version );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not get backend version due to error = %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        RIDEHAL_INFO( "QNN build version: %s", QNN_SDK_BUILD_ID );
        RIDEHAL_INFO( "QNN running core api version: %u.%u.%u, backend api version: %u.%u.%u",
                      version.coreApiVersion.major, version.coreApiVersion.minor,
                      version.coreApiVersion.patch, version.backendApiVersion.major,
                      version.backendApiVersion.minor, version.backendApiVersion.patch );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pConfig->processorType != RIDEHAL_PROCESSOR_CPU )
        {
            retVal = m_qnnFunctionPointers.qnnInterface.deviceGetPlatformInfo( m_logHandle,
                                                                               &m_platformInfo );
            if ( QNN_DEVICE_NO_ERROR != retVal )
            {
                RIDEHAL_ERROR( "Could not get platform information due to error = %" PRIu64,
                               retVal );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pConfig->processorType != RIDEHAL_PROCESSOR_CPU )
        {
            if ( QNN_DEVICE_PLATFORM_INFO_VERSION_1 == m_platformInfo->version )
            {
                RIDEHAL_INFO( "numHwDevices = %u", m_platformInfo->v1.numHwDevices );
                for ( uint32_t i = 0; i < m_platformInfo->v1.numHwDevices; i++ )
                {
                    auto &deviceInfo = m_platformInfo->v1.hwDevices[i].v1;
                    RIDEHAL_INFO( "deviceId = %u deviceType = %u numCores = %u",
                                  deviceInfo.deviceId, deviceInfo.deviceType, deviceInfo.numCores );
                }

                int deviceId = m_backendCoreId;
                if ( deviceId < (int) m_platformInfo->v1.numHwDevices )
                {
                    QnnDevice_HardwareDeviceInfo_t hwDevice =
                            m_platformInfo->v1.hwDevices[deviceId];
                    QnnDevice_PlatformInfo_t platformInfo = {
                            .version = QNN_DEVICE_PLATFORM_INFO_VERSION_1,
                            .v1 =
                                    {
                                            .numHwDevices = 1,
                                            .hwDevices = &hwDevice,
                                    },
                    };
                    QnnDevice_Config_t deviceConfig = {
                            .option = QNN_DEVICE_CONFIG_OPTION_PLATFORM_INFO,
                            .hardwareInfo = &platformInfo,
                    };
                    const QnnDevice_Config_t *configs[] = { &deviceConfig, nullptr };
                    retVal = m_qnnFunctionPointers.qnnInterface.deviceCreate( m_logHandle, configs,
                                                                              &m_deviceHandle );
                    if ( QNN_BACKEND_NO_ERROR != retVal )
                    {
                        RIDEHAL_ERROR( "Could not create device due to error = %" PRIu64, retVal );
                        ret = RIDEHAL_ERROR_FAIL;
                    }
                }
                else
                {
                    RIDEHAL_ERROR( "invalid backend device id = %d", deviceId );
                    ret = RIDEHAL_ERROR_FAIL;
                }
            }
        }
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pConfig->numOfUdoPackages == 0 )
        {
            RIDEHAL_INFO( "no op package" );
        }
        else
        {
            ret = LoadOpPackages( pConfig->pUdoPackages, pConfig->numOfUdoPackages );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "fail to load package" );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( RIDEHAL_PROCESSOR_HTP0 == m_backendType ) ||
             ( RIDEHAL_PROCESSOR_HTP1 == m_backendType ) )
        {   // set up context priority
            m_contextConfigArray[0].option = QNN_CONTEXT_CONFIG_OPTION_PRIORITY;
            m_contextConfigArray[0].priority = pConfig->priority;
            m_contextConfig[0] = &m_contextConfigArray[0];
            m_contextConfig[1] = nullptr;
            RIDEHAL_INFO( "set context priority = %d", pConfig->priority );
        }

        switch ( pConfig->loadType )
        {
            case QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE:
            {
                ret = CreateFromModelSo( modelPath );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "fail to create from model so." );
                }
                break;
            }
            case QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER:
            {
                ret = CreateFromBinaryBuffer( pConfig->contextBuffer, pConfig->contextSize );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to create from binary buffer %d", pConfig->contextSize );
                }
                break;
            }

            case QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE:
            default:
            {
                ret = CreateFromBinaryFile( modelPath );
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "fail to create from binary file." );
                }
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetInputInfo();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = GetOutputInfo();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "init %s with backend %s\n", modelPath.c_str(), s_Backends[m_backendType] );
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    { /* do error clean up */
        (void) Destroy();
        if ( true == bIFInitOK )
        {
            (void) ComponentIF::Deinit();
        }
    }
    else
    {
        m_outputCb = nullptr;
        m_errorCb = nullptr;
        m_pAppPriv = nullptr;
        m_notifyParamQ.Init();
    }

    return ret;
}

QnnRuntime::~QnnRuntime() {}

RideHalError_e QnnRuntime::GetInputInfo()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_graphsInfo == nullptr )
    {
        RIDEHAL_ERROR( "m_graphsInfo is nullptr" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_inputTensorNum = m_graphsInfo[0]->numInputTensors;
    }

    if ( m_inputTensorNum != 0 )
    {
        m_pInputTensor = new QnnRuntime_TensorInfo_t[m_inputTensorNum];
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_inputs.resize( m_inputTensorNum );
        for ( uint32_t i = 0; i < m_inputTensorNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            auto tensor = &m_graphsInfo[0]->inputTensors[i];
            m_inputs[i] = *tensor;

            m_pInputTensor[i].pName = QNN_TENSOR_GET_NAME( tensor );

            auto rank = QNN_TENSOR_GET_RANK( tensor );
            auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            std::stringstream ss;
            ss << "[ ";
            for ( uint32_t j = 0; j < rank; j++ )
            {
                tensorProp.dims[j] = dimensions[j];
                ss << tensorProp.dims[j] << ", ";
            }
            ss << "]";
            tensorProp.numDims = rank;

            auto quantizeParams = QNN_TENSOR_GET_QUANT_PARAMS( tensor );
            if ( QNN_QUANTIZATION_ENCODING_SCALE_OFFSET == quantizeParams.quantizationEncoding )
            {
                m_pInputTensor[i].quantScale = quantizeParams.scaleOffsetEncoding.scale;
                m_pInputTensor[i].quantOffset = quantizeParams.scaleOffsetEncoding.offset;
            }
            else
            {
                RIDEHAL_WARN( "input %s: quantize encoding %d not supported",
                              m_pInputTensor[i].pName, quantizeParams.quantizationEncoding );
            }
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            tensorProp.type = SwitchFromQnnDataType( dataType );
            m_pInputTensor[i].properties = tensorProp;

            RIDEHAL_INFO( "input %s: shape = %s, scale=%f, offset=%d, type=%x\n",
                          m_pInputTensor[i].pName, ss.str().c_str(), m_pInputTensor[i].quantScale,
                          m_pInputTensor[i].quantOffset, dataType );
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetInputInfo( QnnRuntime_TensorInfoList_t *pList )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pList )
    {
        RIDEHAL_ERROR( "pList is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_pInputTensor == nullptr )
        {
            RIDEHAL_ERROR( "Input tensor is nullptr!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            pList->pInfo = m_pInputTensor;
            pList->num = m_inputTensorNum;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetOutputInfo()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_graphsInfo == nullptr )
    {
        RIDEHAL_ERROR( "m_graphsInfo is nullptr" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_outputTensorNum = m_graphsInfo[0]->numOutputTensors;
    }

    if ( m_outputTensorNum != 0 )
    {
        m_pOutputTensor = new QnnRuntime_TensorInfo_t[m_outputTensorNum];
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_outputs.resize( m_outputTensorNum );
        for ( uint32_t i = 0; i < m_outputTensorNum; ++i )
        {
            RideHal_TensorProps_t tensorProp;
            auto tensor = &m_graphsInfo[0]->outputTensors[i];
            m_outputs[i] = *tensor;

            m_pOutputTensor[i].pName = QNN_TENSOR_GET_NAME( tensor );

            auto rank = QNN_TENSOR_GET_RANK( tensor );
            auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            std::stringstream ss;
            ss << "[ ";
            for ( uint32_t j = 0; j < rank; j++ )
            {
                tensorProp.dims[j] = dimensions[j];
                ss << tensorProp.dims[j] << ", ";
            }
            ss << "]";
            tensorProp.numDims = rank;

            auto quantizeParams = QNN_TENSOR_GET_QUANT_PARAMS( tensor );
            if ( QNN_QUANTIZATION_ENCODING_SCALE_OFFSET == quantizeParams.quantizationEncoding )
            {
                m_pOutputTensor[i].quantScale = quantizeParams.scaleOffsetEncoding.scale;
                m_pOutputTensor[i].quantOffset = quantizeParams.scaleOffsetEncoding.offset;
            }
            else
            {
                RIDEHAL_WARN( "input %s: quantize encoding %d not supported",
                              m_pOutputTensor[i].pName, quantizeParams.quantizationEncoding );
            }
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            tensorProp.type = SwitchFromQnnDataType( dataType );
            m_pOutputTensor[i].properties = tensorProp;

            RIDEHAL_INFO( "output %s: shape = %s, scale=%f, offset=%d, type=%x\n",
                          m_pOutputTensor[i].pName, ss.str().c_str(), m_pOutputTensor[i].quantScale,
                          m_pOutputTensor[i].quantOffset, dataType );
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetOutputInfo( QnnRuntime_TensorInfoList_t *pList )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pList )
    {
        RIDEHAL_ERROR( "pList is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_pOutputTensor == nullptr )
        {
            RIDEHAL_ERROR( "Input tensor is nullptr!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            pList->pInfo = m_pOutputTensor;
            pList->num = m_outputTensorNum;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::RemoteRegisterBuf( const RideHal_SharedBuffer_t *pSharedBuffer, int &fd )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    int rpcFd = rpcmem_to_fd( pSharedBuffer->buffer.pData );
    if ( rpcFd < 0 )
    {
        std::lock_guard<std::mutex> l( s_lock[m_backendType] );
        int domain = CDSP_DOMAIN_ID;
        if ( 1 == m_backendCoreId )
        {
            domain = CDSP1_DOMAIN_ID;
        }
        int client = 0;   // NOTE: default is 0
        int extDomainId = get_extended_domains_id( domain, client );

#if defined( __QNXNTO__ )
        remote_register_buf_v2( extDomainId, pSharedBuffer->buffer.pData,
                                pSharedBuffer->buffer.size, 0 );
#else
        remote_register_buf_v2( extDomainId, pSharedBuffer->buffer.pData,
                                pSharedBuffer->buffer.size, (int) pSharedBuffer->buffer.dmaHandle );
#endif
        rpcFd = rpcmem_to_fd( pSharedBuffer->buffer.pData );
        if ( rpcFd >= 0 )
        {
            s_dmaMemRefMap[m_backendType][pSharedBuffer->buffer.pData] = 1;
            fd = rpcFd;
        }
        else
        {
            RIDEHAL_ERROR( "remote register failed for buffer %p(%" PRIu64 ", %" PRIu64
                           ") for core %d!",
                           pSharedBuffer->buffer.pData, pSharedBuffer->buffer.size,
                           pSharedBuffer->offset, m_backendCoreId );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        std::lock_guard<std::mutex> l( s_lock[m_backendType] );
        auto it = s_dmaMemRefMap[m_backendType].find( pSharedBuffer->buffer.pData );
        if ( it != s_dmaMemRefMap[m_backendType].end() )
        {
            it->second++;
        }
        else
        {
            /* buffer is possbile that remote_register_buf_v2 by others, so do nothing */
        }
        fd = rpcFd;
    }

    return ret;
}

RideHalError_e QnnRuntime::RemoteDeRegisterBuf( void *pData, size_t size )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    constexpr int client = 0;   // NOTE: default is 0
    int domain = CDSP_DOMAIN_ID;

    if ( 1 == m_backendCoreId )
    {
        domain = CDSP1_DOMAIN_ID;
    }
    int extDomainId = get_extended_domains_id( domain, client );
    std::lock_guard<std::mutex> l( s_lock[m_backendType] );
    auto it = s_dmaMemRefMap[m_backendType].find( pData );
    if ( it != s_dmaMemRefMap[m_backendType].end() )
    {
        if ( it->second > 0 )
        {
            it->second--;
        }
        if ( 0 == it->second )
        {
            remote_register_buf_v2( extDomainId, (void *) pData, size, -1 );
            s_dmaMemRefMap[m_backendType].erase( it );
        }
    }
    else
    {
        /* buffer is possbile that remote_register_buf_v2 by others */
        RIDEHAL_INFO( "Can't find buffer %p(%" PRIu64 ") in dma ref map", pData, size );
    }

    return ret;
}

RideHalError_e QnnRuntime::RegisterBufferToHTP( const RideHal_SharedBuffer_t *pSharedBuffer,
                                                Qnn_MemHandle_t *pMemHandle )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;

#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
#else
    /* QnnRuntime build with old version QNN SDK that do not support DMA buffer with offset. */
    if ( 0 != pSharedBuffer->offset )
    {
        RIDEHAL_ERROR( "Tensor offset is not zero in qnn ealier version!" );
        ret = RIDEHAL_ERROR_FAIL;
    }
#endif

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::lock_guard<std::mutex> l( m_lock );
        auto it = m_dmaMemInfoMap.find( (uint8_t *) pSharedBuffer->data() );
        if ( it == m_dmaMemInfoMap.end() )
        {
            int fd;
            ret = RemoteRegisterBuf( pSharedBuffer, fd );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                Qnn_MemDescriptor_t desc;
                desc.memShape.numDim = pSharedBuffer->tensorProps.numDims;
                desc.memShape.dimSize = (uint32_t *) pSharedBuffer->tensorProps.dims;
                desc.memShape.shapeConfig = nullptr;
                desc.dataType = SwitchToQnnDataType( pSharedBuffer->tensorProps.type );

#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
                QnnMemHtp_Descriptor_t htpDesc;
                htpDesc.type = QNN_HTP_MEM_SHARED_BUFFER;
                htpDesc.size = pSharedBuffer->buffer.size;
                htpDesc.sharedBufferConfig.fd = fd;
                htpDesc.sharedBufferConfig.offset = pSharedBuffer->offset;

                desc.memType = QNN_MEM_TYPE_CUSTOM;
                desc.customInfo = &htpDesc;
#else
                desc.memType = QNN_MEM_TYPE_ION;
                desc.ionInfo.fd = fd;
#endif

                retVal = m_qnnFunctionPointers.qnnInterface.memRegister( m_context, &desc, 1,
                                                                         pMemHandle );
                if ( QNN_SUCCESS == retVal )
                {
                    QnnRuntime::DmaMemInfo_t info;
                    info.memHandle = *pMemHandle;
                    info.size = pSharedBuffer->buffer.size;
                    info.fd = fd;
                    m_dmaMemInfoMap[(uint8_t *) pSharedBuffer->data()] = info;
                    RIDEHAL_INFO( "succeed to register map buffer %p(%d, %" PRIu64 ", %" PRIu64
                                  ") as %p for core %d",
                                  pSharedBuffer->buffer.pData, fd, pSharedBuffer->buffer.size,
                                  pSharedBuffer->offset, *pMemHandle, m_backendCoreId );
                }
                else
                {
                    (void) RemoteDeRegisterBuf( pSharedBuffer->buffer.pData,
                                                pSharedBuffer->buffer.size );
                    RIDEHAL_ERROR( "failed to map buffer %p(%d, %" PRIu64 ", %" PRIu64
                                   ") for core %d, error %d\n",
                                   pSharedBuffer->buffer.pData, fd, pSharedBuffer->buffer.size,
                                   pSharedBuffer->offset, m_backendCoreId, retVal );
                    ret = RIDEHAL_ERROR_FAIL;
                }
            }
        }
        else
        {
            auto &info = it->second;
            *pMemHandle = info.memHandle;
            RIDEHAL_DEBUG( "already register map buffer %p(%d, %" PRIu64 ", %" PRIu64
                           ") as %p for core %d",
                           pSharedBuffer->buffer.pData, info.fd, pSharedBuffer->size,
                           pSharedBuffer->offset, *pMemHandle, m_backendCoreId );
        }
    }

    return ret;
}


RideHalError_e QnnRuntime::RegisterBuffers( const RideHal_SharedBuffer_t *pSharedBuffers,
                                            uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pSharedBuffers )
    {
        RIDEHAL_ERROR( "pSharedBuffers is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 0 == numBuffers )
    {
        RIDEHAL_ERROR( "numBuffers is 0" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_PROCESSOR_HTP0 != m_backendType ) &&
              ( RIDEHAL_PROCESSOR_HTP1 != m_backendType ) )
    {
        RIDEHAL_ERROR( "not supported for processor %d", m_backendType );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }
    else
    {
        /* OK */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numBuffers; ++i )
        {
            Qnn_MemHandle_t memHandle = nullptr;
            ret = RegisterBufferToHTP( &pSharedBuffers[i], &memHandle );
            if ( ret != RIDEHAL_ERROR_NONE )
            {
                break;
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetMemHandle( const RideHal_SharedBuffer_t *pSharedBuffer,
                                         Qnn_MemHandle_t *pMemHandle )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    *pMemHandle = nullptr;

    if ( ( RIDEHAL_PROCESSOR_HTP0 == m_backendType ) ||
         ( RIDEHAL_PROCESSOR_HTP1 == m_backendType ) )
    {
        ret = RegisterBufferToHTP( pSharedBuffer, pMemHandle );
    }

    return ret;
}

RideHalError_e QnnRuntime::ExtractProfilingEvent( QnnProfile_EventId_t profileEventId,
                                                  QnnRuntime_Perf_t *pPerf, bool &bPerfDataValid )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    QnnProfile_EventData_t eventData;
    Qnn_ErrorHandle_t retVal;

    retVal = m_qnnFunctionPointers.qnnInterface.profileGetEventData( profileEventId, &eventData );
    if ( QNN_PROFILE_NO_ERROR != retVal )
    {
        RIDEHAL_ERROR( "Failure in profile get event type. error is %" PRIu64, retVal );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Printing Event Info - Event Type: [%d], Event Value: [%" PRIu64
                       "], Event Identifier: [%s], Event Unit: [%d]",
                       eventData.type, eventData.value, eventData.identifier, eventData.unit );
        switch ( eventData.type )
        {
            case QNN_PROFILE_EVENTTYPE_EXECUTE:
                bPerfDataValid = true;
                pPerf->entireExecTime = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HOST_RPC_TIME_MICROSEC:
                bPerfDataValid = true;
                pPerf->rpcExecTimeCPU = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HTP_RPC_TIME_MICROSEC:
                bPerfDataValid = true;
                pPerf->rpcExecTimeHTP = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_ACCEL_TIME_MICROSEC:
                bPerfDataValid = true;
                pPerf->rpcExecTimeAcc = eventData.value;
                break;
            default:
            {
                // Unsupported qnn event profile type
                break;
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                                    const RideHal_SharedBuffer_t *pOutputs, uint32_t numOutputs,
                                    void *pOutputPriv )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;
    NotifyParam_t *pNotifyParam = nullptr;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( ( nullptr != pOutputPriv ) && ( nullptr == m_outputCb ) )
    {
        RIDEHAL_ERROR( "ASYNC execute request without callback registerred!" );
        ret = RIDEHAL_ERROR_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }

    // Check input tensors
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = CheckInputTensors( pInputs, numInputs );
    }

    // Check output tensors
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = CheckOutputTensors( pOutputs, numOutputs );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_inputTensorNum; i++ )
        {
            Qnn_MemHandle_t memHandle = nullptr;
            ret = GetMemHandle( &pInputs[i], &memHandle );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                QNN_TENSOR_SET_DIMENSIONS( &m_inputs[i], (uint32_t *) pInputs[i].tensorProps.dims );
                if ( nullptr != memHandle )
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_inputs[i], QNN_TENSORMEMTYPE_MEMHANDLE );
                    QNN_TENSOR_SET_MEM_HANDLE( &m_inputs[i], memHandle );
                }
                else
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_inputs[i], QNN_TENSORMEMTYPE_RAW );
                    Qnn_ClientBuffer_t clientBuffer = { (uint8_t *) pInputs[i].data(),
                                                        (uint32_t) pInputs[i].size };
                    QNN_TENSOR_SET_CLIENT_BUF( &m_inputs[i], clientBuffer );
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < m_outputTensorNum; i++ )
        {
            Qnn_MemHandle_t memHandle = nullptr;
            ret = GetMemHandle( &pOutputs[i], &memHandle );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                QNN_TENSOR_SET_DIMENSIONS( &m_outputs[i],
                                           (uint32_t *) pOutputs[i].tensorProps.dims );
                if ( nullptr != memHandle )
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_outputs[i], QNN_TENSORMEMTYPE_MEMHANDLE );
                    QNN_TENSOR_SET_MEM_HANDLE( &m_outputs[i], memHandle );
                }
                else
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_outputs[i], QNN_TENSORMEMTYPE_RAW );
                    Qnn_ClientBuffer_t clientBuffer = { (uint8_t *) pOutputs[i].data(),
                                                        (uint32_t) pOutputs[i].size };
                    QNN_TENSOR_SET_CLIENT_BUF( &m_outputs[i], clientBuffer );
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        const auto graphInfo = m_graphsInfo[0];
        if ( nullptr == pOutputPriv )
        {
            retVal = m_qnnFunctionPointers.qnnInterface.graphExecute(
                    graphInfo->graph, m_inputs.data(), m_inputs.size(), m_outputs.data(),
                    m_outputs.size(), m_profileBackendHandle, nullptr );
        }
        else
        {
            pNotifyParam = m_notifyParamQ.Pop();
            if ( nullptr != pNotifyParam )
            {
                pNotifyParam->self = this;
                pNotifyParam->pOutputPriv = pOutputPriv;
                retVal = m_qnnFunctionPointers.qnnInterface.graphExecuteAsync(
                        graphInfo->graph, m_inputs.data(), m_inputs.size(), m_outputs.data(),
                        m_outputs.size(), m_profileBackendHandle, nullptr, QnnNotifyFn,
                        pNotifyParam );
            }
            else
            {
                RIDEHAL_ERROR( "notifyParamQ is empty!" );
                retVal = QNN_GRAPH_ERROR_MEM_ALLOC;
            }
        }
        if ( QNN_GRAPH_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "QNN failed %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    return ret;
}

void QnnRuntime::QnnNotifyFn( void *pNotifyParam, Qnn_NotifyStatus_t notifyStatus )
{
    NotifyParam_t *pNotifyParam2 = (NotifyParam_t *) pNotifyParam;

    pNotifyParam2->self->QnnNotifyFn( pNotifyParam2, notifyStatus );
}

void QnnRuntime::QnnNotifyFn( NotifyParam_t *pNotifyParam, Qnn_NotifyStatus_t notifyStatus )
{
    if ( QNN_SUCCESS == notifyStatus.error )
    {
        if ( m_outputCb != nullptr )
        {
            m_outputCb( m_pAppPriv, pNotifyParam->pOutputPriv );
        }
        else
        {
            RIDEHAL_ERROR( "output callback is nullptr!" );
        }
    }
    else
    {
        if ( m_errorCb != nullptr )
        {
            m_errorCb( m_pAppPriv, pNotifyParam->pOutputPriv, notifyStatus );
        }
        else
        {
            RIDEHAL_ERROR( "error callback is nullptr!" );
        }
    }

    m_notifyParamQ.Push( pNotifyParam );
}

RideHalError_e QnnRuntime::DeRegisterAllBuffers()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;
    std::lock_guard<std::mutex> l( m_lock );
    for ( auto &kv : m_dmaMemInfoMap )
    {
        auto pData = kv.first;
        auto &info = kv.second;
        retVal = m_qnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Failed to DeRegister memory. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "succeed to deregister buffer %p(%d, %" PRIu64 ") as %p for core %d",
                          pData, info.fd, info.size, info.memHandle, m_backendCoreId );
        }
        if ( ( RIDEHAL_PROCESSOR_HTP0 == m_backendType ) ||
             ( RIDEHAL_PROCESSOR_HTP1 == m_backendType ) )
        {
            RideHalError_e ret2 = RemoteDeRegisterBuf( pData, info.size );
            if ( RIDEHAL_ERROR_NONE != ret2 )
            {
                ret = ret2;
            }
        }
    }
    m_dmaMemInfoMap.clear();

    return ret;
}

RideHalError_e QnnRuntime::DeRegisterBuffers( const RideHal_SharedBuffer_t *pSharedBuffers,
                                              uint32_t numBuffers )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pSharedBuffers )
    {
        RIDEHAL_ERROR( "pSharedBuffers is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 0 == numBuffers )
    {
        RIDEHAL_ERROR( "numBuffers is 0" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        constexpr int client = 0;   // NOTE: default is 0
        int domain = CDSP_DOMAIN_ID;
        if ( 1 == m_backendCoreId )
        {
            domain = CDSP1_DOMAIN_ID;
        }
        int extDomainId = get_extended_domains_id( domain, client );

        std::lock_guard<std::mutex> l( m_lock );
        for ( size_t i = 0; i < numBuffers; ++i )
        {
            auto it = m_dmaMemInfoMap.find( pSharedBuffers[i].data() );
            if ( it != m_dmaMemInfoMap.end() )
            {
                auto pData = it->first;
                auto &info = it->second;
                retVal = m_qnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
                if ( QNN_SUCCESS != retVal )
                {
                    RIDEHAL_ERROR( "Failed to DeRegister memory. error is %" PRIu64, retVal );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else
                {
                    if ( ( RIDEHAL_PROCESSOR_HTP0 == m_backendType ) ||
                         ( RIDEHAL_PROCESSOR_HTP1 == m_backendType ) )
                    {
                        ret = RemoteDeRegisterBuf( pData, info.size );
                    }
                }

                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    RIDEHAL_INFO( "succeed to deregister buffer %p(%d, %" PRIu64 ", %" PRIu64
                                  ") as %p for core %d",
                                  pSharedBuffers[i].buffer.pData, info.fd, pSharedBuffers[i].size,
                                  pSharedBuffers[i].offset, info.memHandle, m_backendCoreId );
                    (void) m_dmaMemInfoMap.erase( it );
                }
            }
            else
            {
                RIDEHAL_ERROR( "buffer hasn't been registered yet %p(%" PRIu64 ", %" PRIu64
                               ") for core %d",
                               pSharedBuffers[i].buffer.pData, pSharedBuffers[i].size,
                               pSharedBuffers[i].offset, m_backendCoreId );
                ret = RIDEHAL_ERROR_OUT_OF_BOUND;
            }

            if ( ret != RIDEHAL_ERROR_NONE )
            {
                break;
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::Destroy()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal = QNN_SUCCESS;

    ret = DeRegisterAllBuffers();

    if ( nullptr != m_profileBackendHandle )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.profileFree( m_profileBackendHandle );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not free backend profile handle. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( nullptr != m_context )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.contextFree( m_context, nullptr );
        if ( QNN_CONTEXT_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not free context. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        m_context = nullptr;
    }

    if ( nullptr != m_systemContext )
    {
        retVal = m_qnnFunctionPointers.qnnSystemInterface.systemContextFree( m_systemContext );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Failed to free system context. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( nullptr != m_deviceHandle )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.deviceFree( m_deviceHandle );
        if ( QNN_CONTEXT_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not free device handle. Error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        m_deviceHandle = nullptr;
    }

    if ( nullptr != m_platformInfo )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.deviceFreePlatformInfo( m_logHandle,
                                                                            m_platformInfo );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_ERROR( "Failed to free device platform info. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        m_platformInfo = nullptr;
    }

    if ( nullptr != m_backendHandle )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.backendFree( m_backendHandle );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not terminate backend. Error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        m_backendHandle = nullptr;
    }

    if ( m_logHandle != nullptr )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.logFree( m_logHandle );
        if ( QNN_SUCCESS != retVal )
        {
            RIDEHAL_WARN( "Unable to terminate logging in the backend. Error is %" PRIu64, retVal );
        }
    }

    if ( m_bLoadFromCachedBinary && ( nullptr != m_graphsInfo ) )
    {
        RIDEHAL_DEBUG( "Cleaning up graph Info structures." );
        const qnn_wrapper_api::ModelError_t retVal2 =
                qnn_wrapper_api::freeGraphsInfo( &m_graphsInfo, m_graphsCount );
        if ( qnn_wrapper_api::MODEL_NO_ERROR != retVal2 )
        {
            RIDEHAL_ERROR( "failed to free graph info. Error is %" PRIu64, retVal2 );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( m_pInputTensor != nullptr )
    {
        delete[] m_pInputTensor;
    }

    if ( m_pOutputTensor != nullptr )
    {
        delete[] m_pOutputTensor;
    }

    return ret;
}

RideHalError_e QnnRuntime::Deinit()
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal = QNN_SUCCESS;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = Destroy();

        RideHalError_e ret2 = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            ret = ret2;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::RegisterCallback( QnnRuntime_OutputCallback_t outputCb,
                                             QnnRuntime_ErrorCallback_t errorCb, void *pAppPriv )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Register Callback failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( ( nullptr == outputCb ) || ( nullptr == errorCb ) || ( nullptr == pAppPriv ) )
    {
        RIDEHAL_ERROR( "Register Callback with nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        m_outputCb = outputCb;
        m_errorCb = errorCb;
        m_pAppPriv = pAppPriv;
    }

    return ret;
}


RideHalError_e QnnRuntime::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        RIDEHAL_ERROR( "QnnRuntime component start failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e QnnRuntime::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state ) ||
         ( RIDEHAL_COMPONENT_STATE_ERROR == m_state ) )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else
    {
        RIDEHAL_ERROR( "QnnRuntime component stop failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e QnnRuntime::EnablePerf()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr != m_profileBackendHandle )
    {
        RIDEHAL_ERROR( "perf prifiler already created!" );
        ret = RIDEHAL_ERROR_ALREADY;
    }
    else
    {
        retVal = m_qnnFunctionPointers.qnnInterface.profileCreate(
                m_backendHandle, QNN_PROFILE_LEVEL_BASIC, &m_profileBackendHandle );
        if ( QNN_GRAPH_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "failed to create profile. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::DisablePerf()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == m_profileBackendHandle )
    {
        RIDEHAL_ERROR( "perf prifiler is not created!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        retVal = m_qnnFunctionPointers.qnnInterface.profileFree( m_profileBackendHandle );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Could not free backend profile handle. error is %" PRIu64, retVal );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            m_profileBackendHandle = nullptr;
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::GetPerf( QnnRuntime_Perf_t *pPerf )
{
    bool bPerfDataValid = false;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Qnn_ErrorHandle_t retVal;
    const QnnProfile_EventId_t *profileEvents{ nullptr };
    uint32_t numEvents{ 0 };

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "QnnRuntime component not in running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pPerf )
    {
        RIDEHAL_ERROR( "pPerf is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == m_profileBackendHandle )
    {
        RIDEHAL_ERROR( "perf was not enabled!" );
        ret = RIDEHAL_ERROR_OUT_OF_BOUND;
    }
    else
    {
        (void) memset( pPerf, 0, sizeof( QnnRuntime_Perf_t ) );

        retVal = m_qnnFunctionPointers.qnnInterface.profileGetEvents( m_profileBackendHandle,
                                                                      &profileEvents, &numEvents );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            RIDEHAL_ERROR( "Failure in profile get events." );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "ProfileEvents: numEvents: [%u]", numEvents );
            for ( uint32_t event = 0; event < numEvents; event++ )
            {
                (void) ExtractProfilingEvent( profileEvents[event], pPerf, bPerfDataValid );
            }

            if ( false == bPerfDataValid )
            {
                RIDEHAL_ERROR( "no valid perf data!" );
                ret = RIDEHAL_ERROR_OUT_OF_BOUND;
            }
        }
    }

    return ret;
}

RideHal_TensorType_e QnnRuntime::SwitchFromQnnDataType( Qnn_DataType_t dataType )
{
    RideHal_TensorType_e tensorType = RIDEHAL_TENSOR_TYPE_MAX;
    switch ( dataType )
    {
        case QNN_DATATYPE_INT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_8;
            break;

        case QNN_DATATYPE_INT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_16;
            break;

        case QNN_DATATYPE_INT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_32;
            break;

        case QNN_DATATYPE_INT_64:
            tensorType = RIDEHAL_TENSOR_TYPE_INT_64;
            break;

        case QNN_DATATYPE_UINT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_8;
            break;

        case QNN_DATATYPE_UINT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_16;
            break;

        case QNN_DATATYPE_UINT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_32;
            break;

        case QNN_DATATYPE_UINT_64:
            tensorType = RIDEHAL_TENSOR_TYPE_UINT_64;
            break;

        case QNN_DATATYPE_FLOAT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_FLOAT_16;
            break;

        case QNN_DATATYPE_FLOAT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_FLOAT_32;
            break;

        case QNN_DATATYPE_FLOAT_64:
            tensorType = RIDEHAL_TENSOR_TYPE_FLOAT_64;
            break;

        case QNN_DATATYPE_SFIXED_POINT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8;
            break;

        case QNN_DATATYPE_SFIXED_POINT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16;
            break;

        case QNN_DATATYPE_SFIXED_POINT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32;
            break;

        case QNN_DATATYPE_UFIXED_POINT_8:
            tensorType = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8;
            break;

        case QNN_DATATYPE_UFIXED_POINT_16:
            tensorType = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16;
            break;

        case QNN_DATATYPE_UFIXED_POINT_32:
            tensorType = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32;
            break;

        default:
            RIDEHAL_ERROR( "unsupported qnn data type: %d", (int) dataType );
            break;
    }
    return tensorType;
}

Qnn_DataType_t QnnRuntime::SwitchToQnnDataType( RideHal_TensorType_e tensorType )
{
    Qnn_DataType_t dataType = QNN_DATATYPE_UNDEFINED;
    switch ( tensorType )
    {
        case RIDEHAL_TENSOR_TYPE_INT_8:
            dataType = QNN_DATATYPE_INT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_INT_16:
            dataType = QNN_DATATYPE_INT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_INT_32:
            dataType = QNN_DATATYPE_INT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_INT_64:
            dataType = QNN_DATATYPE_INT_64;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_8:
            dataType = QNN_DATATYPE_UINT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_16:
            dataType = QNN_DATATYPE_UINT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_32:
            dataType = QNN_DATATYPE_UINT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_UINT_64:
            dataType = QNN_DATATYPE_UINT_64;
            break;

        case RIDEHAL_TENSOR_TYPE_FLOAT_16:
            dataType = QNN_DATATYPE_FLOAT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_FLOAT_32:
            dataType = QNN_DATATYPE_FLOAT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_FLOAT_64:
            dataType = QNN_DATATYPE_FLOAT_64;
            break;

        case RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8:
            dataType = QNN_DATATYPE_SFIXED_POINT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16:
            dataType = QNN_DATATYPE_SFIXED_POINT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32:
            dataType = QNN_DATATYPE_SFIXED_POINT_32;
            break;

        case RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8:
            dataType = QNN_DATATYPE_UFIXED_POINT_8;
            break;

        case RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16:
            dataType = QNN_DATATYPE_UFIXED_POINT_16;
            break;

        case RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32:
            dataType = QNN_DATATYPE_UFIXED_POINT_32;
            break;

        default:
            RIDEHAL_ERROR( "unsupported ridehal tensor type: %d", (int) tensorType );
            break;
    }
    return dataType;
}

RideHalError_e QnnRuntime::CheckInputTensors( const RideHal_SharedBuffer_t *pInputs,
                                              uint32_t numInputs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_inputTensorNum != numInputs )
    {
        RIDEHAL_ERROR( "Input tensors number is not equal to model input tensors number. Input "
                       "tensors number: %u, model input tensors number: %u",
                       numInputs, m_inputTensorNum );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < numInputs; ++i )
        {
            const auto tensor = &m_graphsInfo[0]->inputTensors[i];
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            const uint32_t rank = QNN_TENSOR_GET_RANK( tensor );
            const auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );

            if ( pInputs[i].buffer.pData == nullptr )
            {
                RIDEHAL_ERROR( "buffer %u data pointer is nullptr", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( pInputs[i].tensorProps.type != SwitchFromQnnDataType( dataType ) )
            {
                RIDEHAL_ERROR( "Unmatched data type. tensor name: %s, shared buffer data type: %u, "
                               "QNN data type: %x",
                               m_pInputTensor[i].pName, pInputs[i].tensorProps.type, dataType );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( rank != pInputs[i].tensorProps.numDims )
            {
                RIDEHAL_ERROR( "Input tensors dim is not equal to model input tensors dim. tensor "
                               "name: %s, Input dim: %u, model input dim: %u",
                               m_pInputTensor[i].pName, pInputs[i].tensorProps.numDims, rank );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                /* OK */
            }

            for ( size_t j = 1; j < rank; ++j )
            {
                if ( dimensions[j] != pInputs[i].tensorProps.dims[j] )
                {
                    RIDEHAL_ERROR( "Input tensor index %u 's shape  is not equal to model's. "
                                   "Shape layer: %u, input shape: %u, model shape: %u.",
                                   i, j, pInputs[i].tensorProps.dims[j], dimensions[j] );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    break;
                }
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
    }

    return ret;
}

RideHalError_e QnnRuntime::CheckOutputTensors( const RideHal_SharedBuffer_t *pOutputs,
                                               uint32_t numOutputs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_outputTensorNum != numOutputs )
    {
        RIDEHAL_ERROR( "Output tensors number is not equal to model output tensors number. Output "
                       "tensors number: %u, model output tensors number: %u",
                       numOutputs, m_outputTensorNum );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( uint32_t i = 0; i < numOutputs; ++i )
        {
            const auto tensor = &m_graphsInfo[0]->outputTensors[i];
            const auto dataType = QNN_TENSOR_GET_DATA_TYPE( tensor );
            const uint32_t rank = QNN_TENSOR_GET_RANK( tensor );
            const auto dimensions = QNN_TENSOR_GET_DIMENSIONS( tensor );
            if ( pOutputs[i].buffer.pData == nullptr )
            {
                RIDEHAL_ERROR( "buffer %u data pointer is nullptr", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( pOutputs[i].tensorProps.type != SwitchFromQnnDataType( dataType ) )
            {
                RIDEHAL_ERROR( "Unmatched data type. tensor name: %s, shared buffer data type: %u, "
                               "QNN data type: %x",
                               m_pOutputTensor[i].pName, pOutputs[i].tensorProps.type, dataType );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( rank != pOutputs[i].tensorProps.numDims )
            {
                RIDEHAL_ERROR( "Output tensors dim is not equal to model output tensors dim. "
                               "tensor name: %s, Output dim: %u, model output dim: %u",
                               m_pOutputTensor[i].pName, pOutputs[i].tensorProps.numDims, rank );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                /* OK */
            }

            for ( size_t j = 1; j < rank; ++j )
            {
                if ( dimensions[j] != pOutputs[i].tensorProps.dims[j] )
                {
                    RIDEHAL_ERROR( "Output tensor index %u 's shape  is not equal to model's. "
                                   "Shape layer: %u, output shape: %u, model shape: %u.",
                                   i, j, pOutputs[i].tensorProps.dims[j], dimensions[j] );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    break;
                }
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
    }

    return ret;
}


void QnnRuntime::NotifyParamQueue::Init()
{
    uint16_t idx;
    pushIdx = 0;
    popIdx = 0;

    for ( idx = 0; idx < QNNRUNTIME_NOTIFY_PARAM_NUM; idx++ )
    {
        ring[pushIdx % QNNRUNTIME_NOTIFY_PARAM_NUM] = idx;
        pushIdx++;
    }
}

void QnnRuntime::NotifyParamQueue::Push( NotifyParam_t *pNotifyParam )
{
    uint16_t idx;

    if ( ( pNotifyParam >= &notifyParam[0] ) &&
         ( pNotifyParam <= &notifyParam[QNNRUNTIME_NOTIFY_PARAM_NUM - 1] ) )
    {
        idx = pNotifyParam - &notifyParam[0];
        ring[pushIdx % QNNRUNTIME_NOTIFY_PARAM_NUM] = idx;
        pushIdx++;
    }
    else
    {
        RIDEHAL_LOG_ERROR( "QnnRuntime NotifyParamQueue Push with invalid param!" );
    }
}

QnnRuntime::NotifyParam_t *QnnRuntime::NotifyParamQueue::Pop()
{
    NotifyParam_t *pNotifyParam = nullptr;
    uint16_t idx;

    if ( pushIdx != popIdx )
    {
        idx = ring[popIdx % QNNRUNTIME_NOTIFY_PARAM_NUM];
        popIdx++;
        pNotifyParam = &notifyParam[idx];
    }

    return pNotifyParam;
}

}   // namespace component
}   // namespace ridehal