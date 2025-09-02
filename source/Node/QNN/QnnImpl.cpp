// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
#include "HTP/QnnHtpCommon.h"
#include "HTP/QnnHtpMem.h"
#include "HTP/QnnHtpProfile.h"
#include "QnnProfile.h"
#include "QnnSdkBuildId.h"
#include "QnnTypeMacros.hpp"

#include <dlfcn.h>
#include <libgen.h>
#include <sstream>
#include <stdio.h>
#include <unistd.h>

#include "QnnImpl.hpp"

extern "C"
{
#include "fastrpc_api.h"
}
#include "rpcmem.h"
#pragma weak remote_session_control
#include "remote.h"

namespace QC
{
namespace Node
{

#if ( QC_TARGET_SOC == 8295 ) || ( QC_TARGET_SOC == 8620 ) || ( QC_TARGET_SOC == 8650 )
#define QC_USE_REMOTE_REGISTER_V2
#endif

std::mutex QnnImpl::s_lock[QNN_PROCESSOR_MAX];
std::map<void *, int> QnnImpl::s_dmaMemRefMap[QNN_PROCESSOR_MAX];

std::map<Qnn_ProcessorType_e, std::string> QnnImpl::s_Backends = {
        { QNN_PROCESSOR_HTP0, "libQnnHtp.so" }, { QNN_PROCESSOR_HTP1, "libQnnHtp.so" },
        { QNN_PROCESSOR_HTP2, "libQnnHtp.so" }, { QNN_PROCESSOR_HTP3, "libQnnHtp.so" },
        { QNN_PROCESSOR_CPU, "libQnnCpu.so" },  { QNN_PROCESSOR_GPU, "libQnnGpu.so" } };


typedef Qnn_ErrorHandle_t ( *QnnInterfaceGetProvidersFn_t )( const QnnInterface_t ***providerList,
                                                             uint32_t *numProviders );

typedef Qnn_ErrorHandle_t ( *QnnSystemInterfaceGetProvidersFn_t )(
        const QnnSystemInterface_t ***providerList, uint32_t *numProviders );

static void QnnLog_Callback( const char *fmt, QnnLog_Level_t logLevel, uint64_t timestamp,
                             va_list args )
{
#ifndef DISABLE_QC_LOG
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

static size_t memscpy( void *dst, size_t dstSize, const void *src, size_t copySize )
{
    size_t minSize = 0;
    if ( ( nullptr == dst ) || ( nullptr == src ) || ( 0 == dstSize ) || ( 0 == copySize ) )
    {
        minSize = 0;
    }
    else
    {
        size_t minSize = copySize;
        if ( dstSize < copySize )
        {
            minSize = dstSize;
        }

        (void) memcpy( dst, src, minSize );
    }

    return minSize;
}

QCStatus_e QnnImpl::GetQnnFunctionPointers( std::string backendPath, std::string modelPath,
                                            bool loadModelLib )
{
    QCStatus_e status = QC_STATUS_OK;
    void *libModelHandle = nullptr;
    void *libBackendHandle =
            dlopen( backendPath.c_str(), (int) ( (uint32_t) RTLD_NOW | (uint32_t) RTLD_GLOBAL ) );
    if ( nullptr == libBackendHandle )
    {
        QC_ERROR( "Unable to load backend. dlerror(): %s", dlerror() );
        status = QC_STATUS_FAIL;
    }
    // Get QNN Interface
    QnnInterfaceGetProvidersFn_t getInterfaceProviders{ nullptr };
    if ( QC_STATUS_OK == status )
    {
        getInterfaceProviders = (QnnInterfaceGetProvidersFn_t) dlsym( libBackendHandle,
                                                                      "QnnInterface_getProviders" );
        if ( nullptr == getInterfaceProviders )
        {
            QC_ERROR( "no symbol QnnInterface_getProviders." );
            status = QC_STATUS_FAIL;
        }
    }
    QnnInterface_t **interfaceProviders{ nullptr };
    uint32_t numProviders{ 0 };
    if ( QC_STATUS_OK == status )
    {
        if ( QNN_SUCCESS != getInterfaceProviders( (const QnnInterface_t ***) &interfaceProviders,
                                                   &numProviders ) )
        {
            QC_ERROR( "Failed to get interface providers." );
            status = QC_STATUS_FAIL;
        }
        if ( nullptr == interfaceProviders )
        {
            QC_ERROR( "Failed to get interface providers: null interface providers received." );
            status = QC_STATUS_FAIL;
        }
        if ( 0 == numProviders )
        {
            QC_ERROR( "Failed to get interface providers: 0 interface providers." );
            status = QC_STATUS_FAIL;
        }
    }
    bool foundValidInterface{ false };
    if ( QC_STATUS_OK == status )
    {
        QC_DEBUG( "Looking QNN interface compatible with version %u.%u.%u", QNN_API_VERSION_MAJOR,
                  QNN_API_VERSION_MINOR, QNN_API_VERSION_PATCH );
        for ( size_t pIdx = 0; pIdx < numProviders; pIdx++ )
        {
            QC_DEBUG( "Found QNN interface with version %u.%u.%u",
                      interfaceProviders[pIdx]->apiVersion.coreApiVersion.major,
                      interfaceProviders[pIdx]->apiVersion.coreApiVersion.minor,
                      interfaceProviders[pIdx]->apiVersion.coreApiVersion.patch );
            if ( QNN_API_VERSION_MAJOR ==
                         interfaceProviders[pIdx]->apiVersion.coreApiVersion.major &&
                 QNN_API_VERSION_MINOR <=
                         interfaceProviders[pIdx]->apiVersion.coreApiVersion.minor )
            {
                foundValidInterface = true;
                m_qnnFunctionPointers.qnnInterface =
                        interfaceProviders[pIdx]->QNN_INTERFACE_VER_NAME;
                break;
            }
        }

        if ( false == foundValidInterface )
        {
            QC_ERROR( "Unable to find a valid interface." );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( true == loadModelLib )
        {
            QC_INFO( "Loading model shared library (%s)", modelPath.c_str() );
            libModelHandle = dlopen( modelPath.c_str(),
                                     (int) ( (uint32_t) RTLD_NOW | (uint32_t) RTLD_GLOBAL ) );
            if ( nullptr == libModelHandle )
            {
                QC_ERROR( "Unable to load model. dlerror(): %s", dlerror() );
                status = QC_STATUS_FAIL;
            }
            else
            {
                std::string modelPrepareFunc = "QnnModel_composeGraphs";
                m_qnnFunctionPointers.composeGraphsFnHandle =
                        (QnnImplComposeGraphsFnHandleType_t) dlsym( libModelHandle,
                                                                    modelPrepareFunc.c_str() );
                if ( nullptr == m_qnnFunctionPointers.composeGraphsFnHandle )
                {
                    QC_ERROR( "no symbol QnnModel_composeGraphs." );
                    status = QC_STATUS_FAIL;
                }
            }

            if ( QC_STATUS_OK == status )
            {
                std::string modelFreeFunc = "QnnModel_freeGraphsInfo";
                m_qnnFunctionPointers.freeGraphInfoFnHandle =
                        (QnnImplFreeGraphInfoFnHandleType_t) dlsym( libModelHandle,
                                                                    modelFreeFunc.c_str() );
                if ( nullptr == m_qnnFunctionPointers.freeGraphInfoFnHandle )
                {
                    QC_ERROR( "no symbol QnnModel_composeGraphs." );
                    status = QC_STATUS_FAIL;
                }
            }
        }
        else
        {
            QC_INFO( "Model wasn't loaded from a shared library." );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        m_backendLibraryHandle = libBackendHandle;
        if ( true == loadModelLib )
        {
            m_modelLibraryHandle = libModelHandle;
        }
    }
    else
    {
        if ( nullptr != libModelHandle )
        {
            (void) dlclose( libModelHandle );
        }
        if ( nullptr != libBackendHandle )
        {
            (void) dlclose( libBackendHandle );
        }
    }
    return status;
}

QCStatus_e QnnImpl::GetQnnSystemFunctionPointers( std::string systemLibraryPath )
{
    QCStatus_e status = QC_STATUS_OK;
    void *systemLibraryHandle = nullptr;
    QnnSystemInterfaceGetProvidersFn_t getSystemInterfaceProviders{ nullptr };

    systemLibraryHandle = dlopen( systemLibraryPath.c_str(),
                                  (int) ( (uint32_t) RTLD_NOW | (uint32_t) RTLD_LOCAL ) );
    if ( nullptr == systemLibraryHandle )
    {
        QC_ERROR( "Unable to load system library. dlerror(): %s", dlerror() );
        status = QC_STATUS_FAIL;
    }
    else
    {
        getSystemInterfaceProviders = (QnnSystemInterfaceGetProvidersFn_t) dlsym(
                systemLibraryHandle, "QnnSystemInterface_getProviders" );
        if ( nullptr == getSystemInterfaceProviders )
        {
            QC_ERROR( "no symbol QnnSystemInterface_getProviders." );
            status = QC_STATUS_FAIL;
        }
    }
    QnnSystemInterface_t **systemInterfaceProviders{ nullptr };
    uint32_t numProviders{ 0 };
    if ( QC_STATUS_OK == status )
    {
        if ( QNN_SUCCESS !=
             getSystemInterfaceProviders(
                     (const QnnSystemInterface_t ***) &systemInterfaceProviders, &numProviders ) )
        {
            QC_ERROR( "Failed to get system interface providers." );
            status = QC_STATUS_FAIL;
        }
        if ( nullptr == systemInterfaceProviders )
        {
            QC_ERROR( "Failed to get system interface providers: null interface providers "
                      "received." );
            status = QC_STATUS_FAIL;
        }
        if ( 0 == numProviders )
        {
            QC_ERROR( "Failed to get interface providers: 0 interface providers." );
            status = QC_STATUS_FAIL;
        }
        bool foundValidSystemInterface{ false };
        if ( QC_STATUS_OK == status )
        {
            QC_DEBUG( "Looking system interface compatible with version %u.%u.%u",
                      QNN_SYSTEM_API_VERSION_MAJOR, QNN_SYSTEM_API_VERSION_MINOR,
                      QNN_SYSTEM_API_VERSION_PATCH );
            for ( size_t pIdx = 0; pIdx < numProviders; pIdx++ )
            {
                QC_DEBUG( "Found system interface with version %u.%u.%u",
                          systemInterfaceProviders[pIdx]->systemApiVersion.major,
                          systemInterfaceProviders[pIdx]->systemApiVersion.minor,
                          systemInterfaceProviders[pIdx]->systemApiVersion.patch );
                if ( QNN_SYSTEM_API_VERSION_MAJOR ==
                             systemInterfaceProviders[pIdx]->systemApiVersion.major &&
                     QNN_SYSTEM_API_VERSION_MINOR <=
                             systemInterfaceProviders[pIdx]->systemApiVersion.minor )
                {
                    foundValidSystemInterface = true;
                    m_qnnFunctionPointers.qnnSystemInterface =
                            systemInterfaceProviders[pIdx]->QNN_SYSTEM_INTERFACE_VER_NAME;
                    break;
                }
            }
            if ( false == foundValidSystemInterface )
            {
                QC_ERROR( "Unable to find a valid system interface." );
                status = QC_STATUS_FAIL;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        m_systemLibraryHandle = systemLibraryHandle;
    }
    else
    {
        if ( nullptr != systemLibraryHandle )
        {
            (void) dlclose( systemLibraryHandle );
        }
    }

    return status;
}

QCStatus_e QnnImpl::DeepCopyQnnTensorInfo( Qnn_Tensor_t *dst, const Qnn_Tensor_t *src )
{
    QCStatus_e status = QC_STATUS_OK;
    if ( ( nullptr == dst ) || ( nullptr == src ) )
    {
        QC_ERROR( "Received nullptr" );
        status = QC_STATUS_FAIL;
    }
    else
    {
        // set tensor.version before using QNN_TENSOR_SET macros, as they require the version to be
        // set to correctly assign values
        dst->version = src->version;
        const char *tensorName = QNN_TENSOR_GET_NAME( src );
        if ( nullptr == tensorName )
        {
            QNN_TENSOR_SET_NAME( dst, nullptr );
        }
        else
        {
            QNN_TENSOR_SET_NAME( dst, strndup( tensorName, strlen( tensorName ) ) );
        }
        QNN_TENSOR_SET_ID( dst, QNN_TENSOR_GET_ID( src ) );
        QNN_TENSOR_SET_TYPE( dst, QNN_TENSOR_GET_TYPE( src ) );
        QNN_TENSOR_SET_DATA_FORMAT( dst, QNN_TENSOR_GET_DATA_FORMAT( src ) );
        QNN_TENSOR_SET_DATA_TYPE( dst, QNN_TENSOR_GET_DATA_TYPE( src ) );
        Qnn_QuantizeParams_t qParams = QNN_QUANTIZE_PARAMS_INIT;
        qParams.encodingDefinition = QNN_TENSOR_GET_QUANT_PARAMS( src ).encodingDefinition;
        qParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
        if ( QNN_TENSOR_GET_QUANT_PARAMS( src ).quantizationEncoding ==
             QNN_QUANTIZATION_ENCODING_SCALE_OFFSET )
        {
            qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS( src ).quantizationEncoding;
            qParams.scaleOffsetEncoding = QNN_TENSOR_GET_QUANT_PARAMS( src ).scaleOffsetEncoding;
        }
        else if ( QNN_TENSOR_GET_QUANT_PARAMS( src ).quantizationEncoding ==
                  QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET )
        {
            qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS( src ).quantizationEncoding;
            qParams.axisScaleOffsetEncoding.axis =
                    QNN_TENSOR_GET_QUANT_PARAMS( src ).axisScaleOffsetEncoding.axis;
            qParams.axisScaleOffsetEncoding.numScaleOffsets =
                    QNN_TENSOR_GET_QUANT_PARAMS( src ).axisScaleOffsetEncoding.numScaleOffsets;
            if ( QNN_TENSOR_GET_QUANT_PARAMS( src ).axisScaleOffsetEncoding.numScaleOffsets > 0 )
            {
                qParams.axisScaleOffsetEncoding.scaleOffset = (Qnn_ScaleOffset_t *) malloc(
                        QNN_TENSOR_GET_QUANT_PARAMS( src ).axisScaleOffsetEncoding.numScaleOffsets *
                        sizeof( Qnn_ScaleOffset_t ) );
                if ( nullptr != qParams.axisScaleOffsetEncoding.scaleOffset )
                {
                    for ( size_t idx = 0; idx < QNN_TENSOR_GET_QUANT_PARAMS( src )
                                                        .axisScaleOffsetEncoding.numScaleOffsets;
                          idx++ )
                    {
                        qParams.axisScaleOffsetEncoding.scaleOffset[idx].scale =
                                QNN_TENSOR_GET_QUANT_PARAMS( src )
                                        .axisScaleOffsetEncoding.scaleOffset[idx]
                                        .scale;
                        qParams.axisScaleOffsetEncoding.scaleOffset[idx].offset =
                                QNN_TENSOR_GET_QUANT_PARAMS( src )
                                        .axisScaleOffsetEncoding.scaleOffset[idx]
                                        .offset;
                    }
                }
                else
                {
                    QC_ERROR( "failed to allocate scaleOffset" );
                    status = QC_STATUS_NOMEM;
                }
            }
        }
        else if ( QNN_QUANTIZATION_ENCODING_UNDEFINED ==
                  QNN_TENSOR_GET_QUANT_PARAMS( src ).quantizationEncoding )
        {
            /* OK for no quantization */
        }
        else
        {
            QC_ERROR( "unsupported quantization encoding %d",
                      QNN_TENSOR_GET_QUANT_PARAMS( src ).quantizationEncoding );
            status = QC_STATUS_UNSUPPORTED;
        }
        QNN_TENSOR_SET_QUANT_PARAMS( dst, qParams );
        QNN_TENSOR_SET_RANK( dst, QNN_TENSOR_GET_RANK( src ) );
        QNN_TENSOR_SET_DIMENSIONS( dst, nullptr );
        if ( QNN_TENSOR_GET_RANK( src ) > 0 )
        {
            QNN_TENSOR_SET_DIMENSIONS(
                    dst, (uint32_t *) malloc( QNN_TENSOR_GET_RANK( src ) * sizeof( uint32_t ) ) );
            if ( nullptr != QNN_TENSOR_GET_DIMENSIONS( dst ) )
            {
                memscpy( QNN_TENSOR_GET_DIMENSIONS( dst ),
                         QNN_TENSOR_GET_RANK( src ) * sizeof( uint32_t ),
                         QNN_TENSOR_GET_DIMENSIONS( src ),
                         QNN_TENSOR_GET_RANK( src ) * sizeof( uint32_t ) );
            }
            else
            {
                QC_ERROR( "failed to allocate dimensions" );
                status = QC_STATUS_NOMEM;
            }
            if ( QNN_TENSOR_GET_IS_DYNAMIC_DIMENSIONS( src ) )
            {
                QNN_TENSOR_SET_IS_DYNAMIC_DIMENSIONS(
                        dst, (uint8_t *) malloc( QNN_TENSOR_GET_RANK( src ) * sizeof( uint8_t ) ) );
                if ( nullptr != QNN_TENSOR_GET_IS_DYNAMIC_DIMENSIONS( dst ) )
                {
                    memscpy( QNN_TENSOR_GET_IS_DYNAMIC_DIMENSIONS( dst ),
                             QNN_TENSOR_GET_RANK( src ) * sizeof( uint8_t ),
                             QNN_TENSOR_GET_IS_DYNAMIC_DIMENSIONS( src ),
                             QNN_TENSOR_GET_RANK( src ) * sizeof( uint8_t ) );
                }
                else
                {
                    QC_ERROR( "failed to allocate is dynamic dimensions" );
                    status = QC_STATUS_NOMEM;
                }
            }
        }
        QNN_TENSOR_SET_SPARSE_PARAMS( dst, QNN_TENSOR_GET_SPARSE_PARAMS( src ) );
    }
    return status;
}

QCStatus_e QnnImpl::CopyTensorsInfo( const Qnn_Tensor_t *tensorsInfoSrc,
                                     Qnn_Tensor_t *&tensorWrappers, uint32_t tensorsCount )
{
    QCStatus_e status = QC_STATUS_OK;
    tensorWrappers = (Qnn_Tensor_t *) calloc( tensorsCount, sizeof( Qnn_Tensor_t ) );
    if ( nullptr == tensorWrappers )
    {
        QC_ERROR( "Failed to allocate memory for tensorWrappers." );
        status = QC_STATUS_FAIL;
    }
    else
    {
        for ( size_t tIdx = 0; tIdx < tensorsCount; tIdx++ )
        {
            QC_DEBUG( "Extracting tensorInfo for tensor Idx: %d", tIdx );
            tensorWrappers[tIdx] = QNN_TENSOR_INIT;
            status = DeepCopyQnnTensorInfo( &tensorWrappers[tIdx], &tensorsInfoSrc[tIdx] );
            if ( QC_STATUS_OK != status )
            {
                break;
            }
        }
    }
    return status;
}

QCStatus_e QnnImpl::CopyGraphsInfoV1( const QnnSystemContext_GraphInfoV1_t *graphInfoSrc,
                                      qnn_wrapper_api::GraphInfo_t *graphInfoDst )
{
    QCStatus_e status = QC_STATUS_OK;
    graphInfoDst->graphName = nullptr;
    if ( graphInfoSrc->graphName )
    {
        graphInfoDst->graphName =
                strndup( graphInfoSrc->graphName, strlen( graphInfoSrc->graphName ) );
        if ( nullptr == graphInfoDst->graphName )
        {
            QC_ERROR( "Failed to copy graphName." );
            status = QC_STATUS_FAIL;
        }
    }
    if ( QC_STATUS_OK == status )
    {
        graphInfoDst->inputTensors = nullptr;
        graphInfoDst->numInputTensors = 0;
        if ( graphInfoSrc->graphInputs )
        {
            status = CopyTensorsInfo( graphInfoSrc->graphInputs, graphInfoDst->inputTensors,
                                      graphInfoSrc->numGraphInputs );
            if ( QC_STATUS_OK == status )
            {
                graphInfoDst->numInputTensors = graphInfoSrc->numGraphInputs;
            }
        }
    }
    if ( QC_STATUS_OK == status )
    {
        graphInfoDst->outputTensors = nullptr;
        graphInfoDst->numOutputTensors = 0;
        if ( graphInfoSrc->graphOutputs )
        {
            status = CopyTensorsInfo( graphInfoSrc->graphOutputs, graphInfoDst->outputTensors,
                                      graphInfoSrc->numGraphOutputs );
            if ( QC_STATUS_OK == status )
            {
                graphInfoDst->numOutputTensors = graphInfoSrc->numGraphOutputs;
            }
        }
    }
    return status;
}

QCStatus_e QnnImpl::CopyGraphsInfoV3( const QnnSystemContext_GraphInfoV3_t *graphInfoSrc,
                                      qnn_wrapper_api::GraphInfo_t *graphInfoDst )
{
    QCStatus_e status = QC_STATUS_OK;
    graphInfoDst->graphName = nullptr;
    if ( graphInfoSrc->graphName )
    {
        graphInfoDst->graphName =
                strndup( graphInfoSrc->graphName, strlen( graphInfoSrc->graphName ) );
        if ( nullptr == graphInfoDst->graphName )
        {
            QC_ERROR( "Failed to copy graphName." );
            status = QC_STATUS_FAIL;
        }
    }
    if ( QC_STATUS_OK == status )
    {
        graphInfoDst->inputTensors = nullptr;
        graphInfoDst->numInputTensors = 0;
        if ( graphInfoSrc->graphInputs )
        {
            status = CopyTensorsInfo( graphInfoSrc->graphInputs, graphInfoDst->inputTensors,
                                      graphInfoSrc->numGraphInputs );
            if ( QC_STATUS_OK == status )
            {
                graphInfoDst->numInputTensors = graphInfoSrc->numGraphInputs;
            }
        }
    }
    if ( QC_STATUS_OK == status )
    {
        graphInfoDst->outputTensors = nullptr;
        graphInfoDst->numOutputTensors = 0;
        if ( graphInfoSrc->graphOutputs )
        {
            status = CopyTensorsInfo( graphInfoSrc->graphOutputs, graphInfoDst->outputTensors,
                                      graphInfoSrc->numGraphOutputs );
            if ( QC_STATUS_OK == status )
            {
                graphInfoDst->numOutputTensors = graphInfoSrc->numGraphOutputs;
            }
        }
    }
    return status;
}

QCStatus_e QnnImpl::CopyGraphsInfo( const QnnSystemContext_GraphInfo_t *graphsInput,
                                    const uint32_t numGraphs,
                                    qnn_wrapper_api::GraphInfo_t **&graphsInfo )
{
    QCStatus_e status = QC_STATUS_OK;
    qnn_wrapper_api::GraphInfo_t *graphInfoArr = nullptr;

    if ( nullptr == graphsInput )
    {
        QC_ERROR( "Received nullptr for graphsInput." );
        status = QC_STATUS_FAIL;
    }
    else
    {
        graphsInfo = (qnn_wrapper_api::GraphInfo_t **) calloc(
                numGraphs, sizeof( qnn_wrapper_api::GraphInfo_t * ) );
        graphInfoArr = (qnn_wrapper_api::GraphInfo_t *) calloc(
                numGraphs, sizeof( qnn_wrapper_api::GraphInfo_t ) );
        if ( ( nullptr == graphsInfo ) || ( nullptr == graphInfoArr ) )
        {
            QC_ERROR( "Failure to allocate memory for *graphInfo" );
            status = QC_STATUS_FAIL;
        }
    }
    if ( QC_STATUS_OK == status )
    {
        for ( size_t gIdx = 0; gIdx < numGraphs; gIdx++ )
        {
            QC_INFO( "Extracting graphsInfo for graph Idx: %d", gIdx );
            if ( graphsInput[gIdx].version == QNN_SYSTEM_CONTEXT_GRAPH_INFO_VERSION_1 )
            {
                status = CopyGraphsInfoV1( &graphsInput[gIdx].graphInfoV1, &graphInfoArr[gIdx] );
            }
            else if ( graphsInput[gIdx].version == QNN_SYSTEM_CONTEXT_GRAPH_INFO_VERSION_3 )
            {
                status = CopyGraphsInfoV3( &graphsInput[gIdx].graphInfoV3, &graphInfoArr[gIdx] );
            }
            else
            {
                QC_ERROR( "unsupported graph version %d", graphsInput[gIdx].version );
                status = QC_STATUS_UNSUPPORTED;
            }
            if ( QC_STATUS_OK == status )
            {
                graphsInfo[gIdx] = graphInfoArr + gIdx;
            }
            else
            {
                break;
            }
        }
    }
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Received an ERROR during extractGraphsInfo. Freeing resources." );
        if ( nullptr != graphsInfo )
        {
            for ( uint32_t gIdx = 0; gIdx < numGraphs; gIdx++ )
            {
                if ( graphsInfo[gIdx] )
                {
                    if ( nullptr != graphsInfo[gIdx]->graphName )
                    {
                        free( graphsInfo[gIdx]->graphName );
                        graphsInfo[gIdx]->graphName = nullptr;
                    }
                    FreeQnnTensors( graphsInfo[gIdx]->inputTensors,
                                    graphsInfo[gIdx]->numInputTensors );
                    FreeQnnTensors( graphsInfo[gIdx]->outputTensors,
                                    graphsInfo[gIdx]->numOutputTensors );
                }
            }
            free( *graphsInfo );
        }
        free( graphsInfo );
        graphsInfo = nullptr;
        status = QC_STATUS_FAIL;
    }
    return status;
}

QCStatus_e QnnImpl::CopyMetadataToGraphsInfo( const QnnSystemContext_BinaryInfo_t *binaryInfo,
                                              qnn_wrapper_api::GraphInfo_t **&graphsInfo,
                                              uint32_t &graphsCount )
{
    QCStatus_e status = QC_STATUS_OK;
    if ( nullptr == binaryInfo )
    {
        QC_ERROR( "binaryInfo is nullptr." );
        status = QC_STATUS_FAIL;
    }
    else
    {
        graphsCount = 0;
        if ( binaryInfo->version == QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_1 )
        {
            if ( binaryInfo->contextBinaryInfoV1.graphs )
            {
                status = CopyGraphsInfo( binaryInfo->contextBinaryInfoV1.graphs,
                                         binaryInfo->contextBinaryInfoV1.numGraphs, graphsInfo );
                if ( QC_STATUS_OK == status )
                {
                    graphsCount = binaryInfo->contextBinaryInfoV1.numGraphs;
                }
            }
        }
        else if ( binaryInfo->version == QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_2 )
        {
            if ( binaryInfo->contextBinaryInfoV2.graphs )
            {
                status = CopyGraphsInfo( binaryInfo->contextBinaryInfoV2.graphs,
                                         binaryInfo->contextBinaryInfoV2.numGraphs, graphsInfo );
                if ( QC_STATUS_OK == status )
                {
                    graphsCount = binaryInfo->contextBinaryInfoV2.numGraphs;
                }
            }
        }
        else if ( binaryInfo->version == QNN_SYSTEM_CONTEXT_BINARY_INFO_VERSION_3 )
        {
            if ( binaryInfo->contextBinaryInfoV3.graphs )
            {
                status = CopyGraphsInfo( binaryInfo->contextBinaryInfoV3.graphs,
                                         binaryInfo->contextBinaryInfoV3.numGraphs, graphsInfo );
                if ( QC_STATUS_OK == status )
                {
                    graphsCount = binaryInfo->contextBinaryInfoV3.numGraphs;
                }
            }
        }
    }
    return status;
}

QCStatus_e QnnImpl::FreeQnnTensor( Qnn_Tensor_t &tensor )
{
    QCStatus_e status = QC_STATUS_OK;
    // free all pointer allocations in struct
    free( (void *) QNN_TENSOR_GET_NAME( tensor ) );
    free( QNN_TENSOR_GET_DIMENSIONS( tensor ) );
    if ( QNN_TENSOR_GET_IS_DYNAMIC_DIMENSIONS( tensor ) )
    {
        free( QNN_TENSOR_GET_IS_DYNAMIC_DIMENSIONS( tensor ) );
    }
    auto quant = QNN_TENSOR_GET_QUANT_PARAMS( tensor );
    auto encoding = quant.quantizationEncoding;
    if ( encoding == QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET )
    {
        if ( quant.axisScaleOffsetEncoding.scaleOffset != nullptr )
        {
            free( quant.axisScaleOffsetEncoding.scaleOffset );
        }
    }
    return status;
}

QCStatus_e QnnImpl::FreeQnnTensors( Qnn_Tensor_t *&tensors, uint32_t numTensors )
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;
    // free all pointer allocations in struct
    for ( size_t i = 0; i < numTensors; i++ )
    {
        status2 = FreeQnnTensor( tensors[i] );
        if ( status2 != QC_STATUS_OK )
        {
            status = status2;
        }
    }
    free( tensors );
    return status;
}

QCStatus_e QnnImpl::FreeGraphsInfo( qnn_wrapper_api::GraphInfoPtr_t **graphsInfo,
                                    uint32_t numGraphs )
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;
    if ( ( graphsInfo == nullptr ) || ( *graphsInfo == nullptr ) )
    {
        status = QC_STATUS_FAIL;
    }
    else
    {
        for ( uint32_t i = 0; i < numGraphs; i++ )
        {
            free( ( *graphsInfo )[i]->graphName );
            status2 = FreeQnnTensors( ( *graphsInfo )[i]->inputTensors,
                                      ( *graphsInfo )[i]->numInputTensors );
            if ( status2 != QC_STATUS_OK )
            {
                status = status2;
            }
            status2 = FreeQnnTensors( ( *graphsInfo )[i]->outputTensors,
                                      ( *graphsInfo )[i]->numOutputTensors );
            if ( status2 != QC_STATUS_OK )
            {
                status = status2;
            }
        }
        free( **graphsInfo );
        free( *graphsInfo );
        *graphsInfo = nullptr;
    }
    return status;
}

QnnLog_Level_t QnnImpl::GetQnnLogLevel( Logger_Level_e level )
{
    QnnLog_Level_t qnnLogLevel = QNN_LOG_LEVEL_WARN;

    switch ( level )
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

uint32_t QnnImpl::GetQnnDeviceId( Qnn_ProcessorType_e processorType )
{
    uint32_t deviceId = UINT32_MAX;
    switch ( processorType )
    {
        case QNN_PROCESSOR_HTP0:
            deviceId = 0;
            break;
        case QNN_PROCESSOR_HTP1:
            deviceId = 1;
            break;
        case QNN_PROCESSOR_HTP2:
            deviceId = 2;
            break;
        case QNN_PROCESSOR_HTP3:
            deviceId = 3;
            break;
        case QNN_PROCESSOR_CPU:
            deviceId = 0;
            break;
        case QNN_PROCESSOR_GPU:
            deviceId = 0;
            break;
        default:
            break;
    }
    return deviceId;
}

QCStatus_e QnnImpl::LoadOpPackages( std::vector<Qnn_UdoPackage_t> &udoPackages )
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;

    for ( Qnn_UdoPackage_t &udoPackage : udoPackages )
    {
        QC_INFO( "Registered Op Package: %s and interface provider: %s",
                 udoPackage.udoLibPath.c_str(), udoPackage.interfaceProvider.c_str() );
        retVal = m_qnnFunctionPointers.qnnInterface.backendRegisterOpPackage(
                m_backendHandle, udoPackage.udoLibPath.c_str(),
                udoPackage.interfaceProvider.c_str(), nullptr );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not register Op Package: %s and interface provider: %s, "
                      "error is %" PRIu64,
                      udoPackage.udoLibPath.c_str(), udoPackage.interfaceProvider.c_str(), retVal );
            status = QC_STATUS_FAIL;
            break;
        }
        else
        {
            QC_INFO( "Registered Op Package: %s and interface provider: %s",
                     udoPackage.udoLibPath.c_str(), udoPackage.interfaceProvider.c_str() );
        }
    }
    return status;
}

QCStatus_e QnnImpl::CreateFromModelSo( std::string modelFile )
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;

    retVal = m_qnnFunctionPointers.qnnInterface.contextCreate(
            m_backendHandle, m_deviceHandle, (const QnnContext_Config_t **) m_contextConfig,
            &m_context );
    if ( QNN_CONTEXT_NO_ERROR != retVal )
    {
        QC_ERROR( "Could not create context, error is %" PRIu64, retVal );
        status = QC_STATUS_FAIL;
    }

    if ( QC_STATUS_OK == status )
    {
        qnn_wrapper_api::ModelError_t retME = m_qnnFunctionPointers.composeGraphsFnHandle(
                m_backendHandle, m_qnnFunctionPointers.qnnInterface, m_context,
                (const qnn_wrapper_api::GraphConfigInfo_t **) m_graphConfigsInfo,
                m_graphConfigsInfoCount, &m_graphsInfo, &m_graphsCount, false, &QnnLog_Callback,
                QNN_LOG_LEVEL_ERROR );
        if ( qnn_wrapper_api::MODEL_NO_ERROR != retME )
        {
            QC_ERROR( "Failed in composeGraphs(), error is %d", retME );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( m_graphsCount != 1 )
        {
            QC_ERROR( "too much graphs" );
            status = QC_STATUS_UNSUPPORTED;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.graphFinalize(
                ( *m_graphsInfo )[0].graph, m_profileBackendHandle, nullptr );
        if ( QNN_GRAPH_NO_ERROR != retVal )
        {
            QC_ERROR( "Failed in graphFinalize()" );
            status = QC_STATUS_FAIL;
        }
    }

    return status;
}

QCStatus_e QnnImpl::CreateFromBinaryBuffer( QCBufferDescriptorBase &bufDesc )
{
    QCStatus_e status = QC_STATUS_OK;
    const QnnSystemContext_BinaryInfo_t *binaryInfo{ nullptr };
    Qnn_ContextBinarySize_t binaryInfoSize{ 0 };
    Qnn_ErrorHandle_t retVal = m_qnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo(
            m_systemContext, static_cast<void *>( bufDesc.pBuf ), bufDesc.size, &binaryInfo,
            &binaryInfoSize );
    if ( QNN_SUCCESS != retVal )
    {
        QC_ERROR( "Failed to get context binary info." );
        status = QC_STATUS_FAIL;
    }

    if ( QC_STATUS_OK == status )
    {
        status = CopyMetadataToGraphsInfo( binaryInfo, m_graphsInfo, m_graphsCount );
        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "Failed to copy metadata." );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( nullptr == m_qnnFunctionPointers.qnnInterface.contextCreateFromBinary )
        {
            QC_ERROR( "contextCreateFromBinaryFnHandle is nullptr." );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.contextCreateFromBinary(
                m_backendHandle, m_deviceHandle, (const QnnContext_Config_t **) m_contextConfig,
                static_cast<void *>( bufDesc.pBuf ), bufDesc.size, &m_context,
                m_profileBackendHandle );
        if ( QNN_SUCCESS != retVal )
        {
            QC_ERROR( "Could not create context from binary. Error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
    }

    for ( size_t graphIdx = 0; graphIdx < m_graphsCount; graphIdx++ )
    {
        if ( QC_STATUS_OK == status )
        {
            if ( nullptr == m_qnnFunctionPointers.qnnInterface.graphRetrieve )
            {
                QC_ERROR( "graphRetrieveFnHandle is nullptr." );
                status = QC_STATUS_FAIL;
                break;
            }
        }

        if ( QC_STATUS_OK == status )
        {
            retVal = m_qnnFunctionPointers.qnnInterface.graphRetrieve(
                    m_context, ( *m_graphsInfo )[graphIdx].graphName,
                    &( ( *m_graphsInfo )[graphIdx].graph ) );
            if ( QNN_SUCCESS != retVal )
            {
                QC_ERROR( "Unable to retrieve graph handle for graph Idx: %" PRIu64
                          ", error is %" PRIu64,
                          graphIdx, retVal );
                status = QC_STATUS_FAIL;
            }
        }
    }
    return status;
}

QCStatus_e QnnImpl::CreateFromBinaryFile( std::string modelFile )
{
    QCStatus_e status = QC_STATUS_OK;
    QCBufferDescriptorBase bufDesc;
    uint32_t bufferSize = 0;
    FILE *pFile = fopen( modelFile.c_str(), "rb" );
    if ( nullptr == pFile )
    {
        QC_ERROR( "No existing file: %s", modelFile.c_str() );
        status = QC_STATUS_FAIL;
    }
    else
    {
        fseek( pFile, 0, SEEK_END );
        bufferSize = static_cast<uint32_t>( ftell( pFile ) );
        fseek( pFile, 0, SEEK_SET );
        if ( 0 == bufferSize )
        {
            QC_ERROR( "Received path to an empty file. Nothing to deserialize." );
            status = QC_STATUS_FAIL;
        }
    }

    uint8_t *pBuffer = nullptr;
    if ( QC_STATUS_OK == status )
    {
        pBuffer = (uint8_t *) malloc( bufferSize );
        if ( nullptr == pBuffer )
        {
            QC_ERROR( "Failed to allocate memory." );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        size_t readSize = fread( pBuffer, 1, bufferSize, pFile );
        if ( readSize != bufferSize )
        {
            QC_ERROR( "Failed to read binary data." );
            status = QC_STATUS_FAIL;
        }
        (void) fclose( pFile );
    }

    if ( QC_STATUS_OK == status )
    {
        bufDesc.name = modelFile;
        bufDesc.pBuf = pBuffer;
        bufDesc.size = bufferSize;
        bufDesc.type = QC_BUFFER_TYPE_RAW;
        status = CreateFromBinaryBuffer( bufDesc );
    }

    if ( nullptr != pBuffer )
    {
        free( pBuffer );
    }

    return status;
}


static const int sg_lowerLatency = 40;      // Should be used on V66 and above only
static const int sg_lowLatency = 100;       // This will limit sleep modes available while running
static const int sg_mediumLatency = 1000;   // This will limit sleep modes available while running

QCStatus_e QnnImpl::SetHtpPerformanceMode()
{
    QCStatus_e status = QC_STATUS_OK;
    QnnDevice_Infrastructure_t deviceInfra = nullptr;
    Qnn_ErrorHandle_t retVal;
    QnnHtpPerfInfrastructure_PowerConfig_t powerConfig;
    memset( &powerConfig, 0, sizeof( powerConfig ) );

#if ( QC_TARGET_SOC == 8797 )
    QnnHtpPerfInfrastructure_PowerConfig_t powerHmxConfig;
    memset( &powerHmxConfig, 0, sizeof( powerHmxConfig ) );
#endif

    QC_INFO( "set HTP perf mode: %d", m_config.perfProfile );

    retVal = m_qnnFunctionPointers.qnnInterface.deviceGetInfrastructure( &deviceInfra );
    if ( QNN_SUCCESS != retVal )
    {
        QC_ERROR( "Failure in deviceGetInfrastructure() = %" PRIu64, retVal );
        status = QC_STATUS_FAIL;
    }

    if ( QC_STATUS_OK == status )
    {
        QnnHtpDevice_Infrastructure_t *htpInfra =
                static_cast<QnnHtpDevice_Infrastructure_t *>( deviceInfra );
        m_perfInfra = &( htpInfra->perfInfra );
        m_powerConfigIds.reserve( m_config.coreIds.size() );
        for ( size_t i = 0; i < m_config.coreIds.size(); i++ )
        {
            uint32_t deviceId = GetQnnDeviceId( m_config.processorType );
            uint32_t coreId = m_config.coreIds[i];
            QnnDevice_HardwareDeviceInfo_t &hwDevice = m_platformInfo->v1.hwDevices[deviceId];
            deviceId = hwDevice.v1.deviceId;
            coreId = hwDevice.v1.cores->v1.coreId;
            uint32_t powerConfigId = UINT32_MAX;
            retVal = m_perfInfra->createPowerConfigId( deviceId, coreId, &powerConfigId );
            if ( QNN_SUCCESS != retVal )
            {
                QC_ERROR( "Failure in createPowerConfigId() for device %u core %u = %" PRIu64,
                          deviceId, coreId, retVal );
                status = QC_STATUS_FAIL;
                break;
            }
            else
            {
                m_powerConfigIds.push_back( powerConfigId );
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        powerConfig.option = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_DCVS_V3;
        // keep dcvs enabled for powerMode to take effect
        powerConfig.dcvsV3Config.dcvsEnable = 0;
        powerConfig.dcvsV3Config.setDcvsEnable = 1;

        // during init & inferencing, vote in performance mode
        powerConfig.dcvsV3Config.powerMode = QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_PERFORMANCE_MODE;
        powerConfig.dcvsV3Config.setSleepLatency = 1;
        powerConfig.dcvsV3Config.setBusParams = 1;
        powerConfig.dcvsV3Config.setCoreParams = 1;
        powerConfig.dcvsV3Config.sleepDisable = 0;
        powerConfig.dcvsV3Config.setSleepDisable = 0;

#if ( QC_TARGET_SOC == 8797 )
        powerHmxConfig.option = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_HMX_V2;
        powerHmxConfig.hmxV2Config.hmxPickDefault = 0;
#endif

        switch ( m_config.perfProfile )
        {
            case QNN_PERF_PROFILE_BURST:
                powerConfig.dcvsV3Config.sleepLatency = sg_lowerLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin =
                        DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
                powerConfig.dcvsV3Config.busVoltageCornerTarget =
                        DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
                powerConfig.dcvsV3Config.busVoltageCornerMax =
                        DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
                powerConfig.dcvsV3Config.coreVoltageCornerMin =
                        DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget =
                        DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
                powerConfig.dcvsV3Config.coreVoltageCornerMax =
                        DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_HIGH;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_MAX;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_MAX;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_MAX;
#endif
                break;
            case QNN_PERF_PROFILE_SUSTAINED_HIGH_PERFORMANCE:
            case QNN_PERF_PROFILE_HIGH_PERFORMANCE:
                powerConfig.dcvsV3Config.sleepLatency = sg_lowLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_VCORNER_TURBO;
                powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_TURBO;
                powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_VCORNER_TURBO;
                powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_VCORNER_TURBO;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_TURBO;
                powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_VCORNER_TURBO;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_HIGH;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_TUR;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_TUR;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_TUR;
#endif
                break;
            case QNN_PERF_PROFILE_POWER_SAVER:
                powerConfig.dcvsV3Config.powerMode =
                        QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_POWER_SAVER_MODE;
                powerConfig.dcvsV3Config.sleepLatency = sg_mediumLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_VCORNER_SVS;
                powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_SVS;
                powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_VCORNER_SVS;
                powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_VCORNER_SVS;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_SVS;
                powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_VCORNER_SVS;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_LOW;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_SVS;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_SVS;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_SVS;
#endif
                break;
            case QNN_PERF_PROFILE_LOW_POWER_SAVER:
                powerConfig.dcvsV3Config.powerMode =
                        QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_POWER_SAVER_MODE;
                powerConfig.dcvsV3Config.sleepLatency = sg_mediumLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_VCORNER_SVS2;
                powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_SVS2;
                powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_VCORNER_SVS2;
                powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_VCORNER_SVS2;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_SVS2;
                powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_VCORNER_SVS2;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_LOW;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_LOW_SVS_D2;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_LOW_SVS_D2;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_LOW_SVS_D2;
#endif
                break;
            case QNN_PERF_PROFILE_HIGH_POWER_SAVER:
                powerConfig.dcvsV3Config.powerMode =
                        QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_POWER_SAVER_MODE;
                powerConfig.dcvsV3Config.sleepLatency = sg_mediumLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_VCORNER_SVS_PLUS;
                powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_SVS_PLUS;
                powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_VCORNER_SVS_PLUS;
                powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_VCORNER_SVS_PLUS;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_SVS_PLUS;
                powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_VCORNER_SVS_PLUS;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_LOW;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_LOW_SVS_D2;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_LOW_SVS_D2;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_LOW_SVS_D2;
#endif
                break;
            case QNN_PERF_PROFILE_EXTREME_POWER_SAVER:
                powerConfig.dcvsV3Config.powerMode =
                        QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_POWER_SAVER_MODE;
                powerConfig.dcvsV3Config.sleepLatency = sg_mediumLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_CORNER_DISABLE;
                powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_CORNER_DISABLE;
                powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_CORNER_DISABLE;
                powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_CORNER_DISABLE;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_CORNER_DISABLE;
                powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_CORNER_DISABLE;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_LOW;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_LOW_SVS_D2;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_LOW_SVS_D2;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_LOW_SVS_D2;
#endif
                break;
            case QNN_PERF_PROFILE_LOW_BALANCED:
                powerConfig.dcvsV3Config.sleepLatency = sg_mediumLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_VCORNER_NOM;
                powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_NOM;
                powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_VCORNER_NOM;
                powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_VCORNER_NOM;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_NOM;
                powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_VCORNER_NOM;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_HIGH;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_NOM;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_NOM;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_NOM;
#endif
                break;
            case QNN_PERF_PROFILE_BALANCED:
                powerConfig.dcvsV3Config.sleepLatency = sg_mediumLatency;
                powerConfig.dcvsV3Config.busVoltageCornerMin = DCVS_VOLTAGE_VCORNER_NOM_PLUS;
                powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_NOM_PLUS;
                powerConfig.dcvsV3Config.busVoltageCornerMax = DCVS_VOLTAGE_VCORNER_NOM_PLUS;
                powerConfig.dcvsV3Config.coreVoltageCornerMin = DCVS_VOLTAGE_VCORNER_NOM_PLUS;
                powerConfig.dcvsV3Config.coreVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_NOM_PLUS;
                powerConfig.dcvsV3Config.coreVoltageCornerMax = DCVS_VOLTAGE_VCORNER_NOM_PLUS;

#if ( QC_TARGET_SOC == 8797 )
                powerHmxConfig.hmxV2Config.hmxPerfMode = QNN_HTP_PERF_INFRASTRUCTURE_CLK_PERF_HIGH;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMin = DCVS_EXP_VCORNER_NOM_L1;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerTarget = DCVS_EXP_VCORNER_NOM_L1;
                powerHmxConfig.hmxV2Config.hmxVoltageCornerMax = DCVS_EXP_VCORNER_NOM_L1;
#endif
                break;
            default:
                QC_ERROR( "Invalid performance profile %d to set power configs",
                          m_config.perfProfile );
                status = QC_STATUS_FAIL;
                break;
        }
    }
    if ( QC_STATUS_OK == status )
    {
        // Set power config with different performance parameters
        for ( uint32_t &powerConfigId : m_powerConfigIds )
        {
            powerConfig.dcvsV3Config.contextId = powerConfigId;
            const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = { &powerConfig,
#if ( QC_TARGET_SOC == 8797 )
                                                                             &powerHmxConfig,
#endif
                                                                             NULL };
            retVal = m_perfInfra->setPowerConfig( powerConfigId, powerConfigs );
            if ( QNN_SUCCESS != retVal )
            {
                QC_ERROR( "Failure in setPowerConfig() = %" PRIu64, retVal );
                status = QC_STATUS_FAIL;
            }
        }
    }


    return status;
}

QCStatus_e QnnImpl::SetPerformanceMode()
{
    QCStatus_e status = QC_STATUS_OK;

    if ( QNN_PERF_PROFILE_DEFAULT != m_config.perfProfile )
    {
        if ( ( QNN_PROCESSOR_HTP0 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP1 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP2 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP3 == m_config.processorType ) )
        {
            status = SetHtpPerformanceMode();
        }
    }

    return status;
}

QCStatus_e
QnnImpl::Initialize( QCNodeEventCallBack_t callback,
                     std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;

    if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        QC_ERROR( "QNN not in initial state!" );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( ( QNN_LOAD_CONTEXT_BIN_FROM_BUFFER != m_config.loadType ) &&
              ( true == m_config.modelPath.empty() ) )
    {
        QC_ERROR( "modelPath is null" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == status )
    {
        if ( QNN_LOAD_CONTEXT_BIN_FROM_BUFFER == m_config.loadType )
        {
            if ( m_config.contextBufferId >= buffers.size() )
            {
                QC_ERROR( "contextBufferId is invalid" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                QCBufferDescriptorBase &bufDesc = buffers[m_config.contextBufferId];
                if ( nullptr == bufDesc.pBuf )
                {
                    QC_ERROR( "contextBuffer is null" );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( 0 == bufDesc.size )
                {
                    QC_ERROR( "contextBuffer size is 0" );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
                else
                {
                    /* OK */
                }
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        m_bLoadFromCachedBinary = ( ( QNN_LOAD_CONTEXT_BIN_FROM_FILE == m_config.loadType ) ||
                                    ( QNN_LOAD_CONTEXT_BIN_FROM_BUFFER == m_config.loadType ) );
    }

    if ( QC_STATUS_OK == status )
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        status = GetQnnFunctionPointers( s_Backends[m_config.processorType], m_config.modelPath,
                                         !m_bLoadFromCachedBinary );
        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "failed to get qnn function pointers from model %s(%s), error is %d",
                      m_config.modelPath.c_str(), s_Backends[m_config.processorType].c_str(),
                      status );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        QnnLog_Error_t logError;
        auto logLevel = GetQnnLogLevel( m_logger.GetLevel() );
        retVal = m_qnnFunctionPointers.qnnInterface.logCreate( &QnnLog_Callback, logLevel,
                                                               &m_logHandle );
        if ( QNN_SUCCESS != retVal )
        {
            QC_WARN( "Unable to initialize logging in the backend. error is %d", (int) retVal );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        status = GetQnnSystemFunctionPointers( "libQnnSystem.so" );
        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "Error initializing QNN System Function Pointers" );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( ( nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextCreate ) ||
             ( nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextGetBinaryInfo ) ||
             ( nullptr == m_qnnFunctionPointers.qnnSystemInterface.systemContextFree ) )
        {
            QC_ERROR( "QNN System function pointers are not populated." );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        retVal = m_qnnFunctionPointers.qnnSystemInterface.systemContextCreate( &m_systemContext );
        if ( QNN_SUCCESS != retVal )
        {
            QC_ERROR( "Could not create system handle. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.backendCreate(
                m_logHandle, (const QnnBackend_Config_t **) m_backendConfig, &m_backendHandle );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not initialize backend due to error = %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        Qnn_ApiVersion_t version;
        retVal = m_qnnFunctionPointers.qnnInterface.backendGetApiVersion( &version );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not get backend version due to error = %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        QC_INFO( "QNN build version: %s", QNN_SDK_BUILD_ID );
        QC_INFO( "QNN running core api version: %u.%u.%u, backend api version: %u.%u.%u",
                 version.coreApiVersion.major, version.coreApiVersion.minor,
                 version.coreApiVersion.patch, version.backendApiVersion.major,
                 version.backendApiVersion.minor, version.backendApiVersion.patch );
    }

    if ( QC_STATUS_OK == status )
    {
        if ( m_config.processorType != QNN_PROCESSOR_CPU )
        {
            retVal = m_qnnFunctionPointers.qnnInterface.deviceGetPlatformInfo( m_logHandle,
                                                                               &m_platformInfo );
            if ( QNN_DEVICE_NO_ERROR != retVal )
            {
                QC_ERROR( "Could not get platform information due to error = %" PRIu64, retVal );
                status = QC_STATUS_FAIL;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( m_config.processorType != QNN_PROCESSOR_CPU )
        {
            if ( QNN_DEVICE_PLATFORM_INFO_VERSION_1 == m_platformInfo->version )
            {
                QC_INFO( "numHwDevices = %u", m_platformInfo->v1.numHwDevices );
                for ( uint32_t i = 0; i < m_platformInfo->v1.numHwDevices; i++ )
                {
                    auto &deviceInfo = m_platformInfo->v1.hwDevices[i].v1;
                    QC_INFO( "deviceId = %u deviceType = %u numCores = %u", deviceInfo.deviceId,
                             deviceInfo.deviceType, deviceInfo.numCores );
                }
                std::vector<QnnDevice_CoreInfo_t> coreInfo;
                uint32_t deviceId = GetQnnDeviceId( m_config.processorType );
                if ( deviceId < m_platformInfo->v1.numHwDevices )
                {
                    QnnDevice_HardwareDeviceInfo_t hwDevice =
                            m_platformInfo->v1.hwDevices[deviceId];
                    coreInfo.resize( m_config.coreIds.size() );
                    for ( uint32_t i = 0; i < m_config.coreIds.size(); i++ )
                    {
                        uint32_t coreId = m_config.coreIds[i];
                        if ( coreId < hwDevice.v1.numCores )
                        {
                            coreInfo[i] = hwDevice.v1.cores[coreId];
                        }
                        else
                        {
                            QC_ERROR( "Invaild coreId = %u", coreId );
                            status = QC_STATUS_FAIL;
                            break;
                        }
                    }
                }
                else
                {
                    QC_ERROR( "invalid backend device id = %d", deviceId );
                    status = QC_STATUS_FAIL;
                }
                if ( QC_STATUS_OK == status )
                {
                    QnnDevice_HardwareDeviceInfo_t hwDevice =
                            m_platformInfo->v1.hwDevices[deviceId];
                    hwDevice.v1.numCores = static_cast<uint32_t>( m_config.coreIds.size() );
                    hwDevice.v1.cores = coreInfo.data();

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
                        QC_ERROR( "Could not create device due to error = %" PRIu64, retVal );
                        status = QC_STATUS_FAIL;
                    }
                }
            }
            else
            {
                QC_ERROR( "platform info version %d not supported", m_platformInfo->version );
                status = QC_STATUS_UNSUPPORTED;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        status = SetPerformanceMode();
    }

    if ( QC_STATUS_OK == status )
    {
        status = LoadOpPackages( m_config.udoPackages );
        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "fail to load package" );
            status = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( ( QNN_PROCESSOR_HTP0 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP1 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP2 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP3 == m_config.processorType ) )
        {   // set up context priority
            m_contextConfigArray[0].option = QNN_CONTEXT_CONFIG_OPTION_PRIORITY;
            m_contextConfigArray[0].priority = m_config.priority;
            m_contextConfig[0] = &m_contextConfigArray[0];
            m_contextConfig[1] = nullptr;
            QC_INFO( "set context priority = %d", m_config.priority );
        }

        switch ( m_config.loadType )
        {
            case QNN_LOAD_SHARED_LIBRARY_FROM_FILE:
            {
                status = CreateFromModelSo( m_config.modelPath );
                if ( QC_STATUS_OK != status )
                {
                    QC_ERROR( "fail to create from model so." );
                }
                break;
            }
            case QNN_LOAD_CONTEXT_BIN_FROM_BUFFER:
            {
                status = CreateFromBinaryBuffer( buffers[m_config.contextBufferId] );
                if ( QC_STATUS_OK != status )
                {
                    QC_ERROR( "Failed to create from binary buffer %u", m_config.contextBufferId );
                }
                break;
            }

            case QNN_LOAD_CONTEXT_BIN_FROM_FILE:
            default:
            {
                status = CreateFromBinaryFile( m_config.modelPath );
                if ( QC_STATUS_OK != status )
                {
                    QC_ERROR( "fail to create from binary file." );
                }
                break;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {   // do buffer register during initialization
        for ( uint32_t bufferId : m_config.bufferIds )
        {
            if ( bufferId < buffers.size() )
            {
                const QCBufferDescriptorBase_t &bufDesc = buffers[bufferId];
                const TensorDescriptor_t *pTensorDesc =
                        dynamic_cast<const TensorDescriptor_t *>( &bufDesc );
                if ( nullptr == pTensorDesc )
                {
                    QC_ERROR( "buffer %" PRIu32 "is invalid", bufferId );
                    status = QC_STATUS_INVALID_BUF;
                }
                else
                {
                    status = RegisterTensor( *pTensorDesc );
                }
            }
            else
            {
                QC_ERROR( "buffer index out of range" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( status != QC_STATUS_OK )
            {
                break;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        status = SetupGlobalBufferIdMap();
    }

    if ( QC_STATUS_OK == status )
    {
        QC_INFO( "init with backend %s", s_Backends[m_config.processorType].c_str() );
        m_callback = callback;
        m_notifyParamQ.Init();
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    { /* do error clean up */
        (void) Destroy();
    }

    return status;
}

QCStatus_e QnnImpl::RemoteRegisterBuf( const TensorDescriptor_t &tensorDesc, int &fd )
{
    QCStatus_e status = QC_STATUS_OK;
#ifdef QC_USE_REMOTE_REGISTER_V2
    constexpr int client = 0;   // NOTE: default is 0
    int domain = CDSP_DOMAIN_ID;
    int extDomainId;
    if ( QNN_PROCESSOR_HTP1 == m_config.processorType )
    {
        domain = CDSP1_DOMAIN_ID;
    }
    extDomainId = get_extended_domains_id( domain, client );
#endif

    int rpcFd = rpcmem_to_fd( tensorDesc.pBuf );
    if ( rpcFd < 0 )
    {
        std::lock_guard<std::mutex> l( s_lock[m_config.processorType] );
#ifdef QC_USE_REMOTE_REGISTER_V2
#if defined( __QNXNTO__ )
        remote_register_buf_v2( extDomainId, tensorDesc.pBuf, static_cast<int>( tensorDesc.size ),
                                0 );
#else
        remote_register_buf_v2( extDomainId, tensorDesc.pBuf, static_cast<int>( tensorDesc.size ),
                                static_cast<int>( tensorDesc.dmaHandle ) );
#endif

#else
#if defined( __QNXNTO__ )
        remote_register_buf( tensorDesc.pBuf, static_cast<int>( tensorDesc.size ), 0 );
#else
        remote_register_buf( tensorDesc.pBuf, static_cast<int>( tensorDesc.size ),
                             static_cast<int>( tensorDesc.dmaHandle ) );
#endif
#endif
        rpcFd = rpcmem_to_fd( tensorDesc.pBuf );
        if ( rpcFd >= 0 )
        {
            s_dmaMemRefMap[m_config.processorType][tensorDesc.pBuf] = 1;
            fd = rpcFd;
        }
        else
        {
            QC_ERROR( "remote register failed for buffer %p(%" PRIu64 ", %" PRIu64 ") for core %d!",
                      tensorDesc.pBuf, tensorDesc.size, tensorDesc.offset, m_config.processorType );
            status = QC_STATUS_FAIL;
        }
    }
    else
    {
        std::lock_guard<std::mutex> l( s_lock[m_config.processorType] );
        auto it = s_dmaMemRefMap[m_config.processorType].find( tensorDesc.pBuf );
        if ( it != s_dmaMemRefMap[m_config.processorType].end() )
        {
            it->second++;
        }
        else
        {
            /* buffer is possbile that remote_register_buf by others, so do nothing */
        }
        fd = rpcFd;
    }

    return status;
}

QCStatus_e QnnImpl::RegisterBufferToHTP( const TensorDescriptor_t &tensorDesc,
                                         Qnn_MemHandle_t &memHandle )
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;
    std::vector<uint32_t> dims( tensorDesc.dims, tensorDesc.dims + tensorDesc.numDims );

#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
#else
    /* QnnRuntime build with old version QNN SDK that do not support DMA buffer with offset. */
    if ( 0 != tensorDesc.offset )
    {
        QC_ERROR( "Tensor offset is not zero in qnn ealier version!" );
        status = QC_STATUS_FAIL;
    }
#endif

    if ( QC_STATUS_OK == status )
    {
        std::lock_guard<std::mutex> l( m_lock );
        auto it = m_dmaMemInfoMap.find( tensorDesc.GetDataPtr() );
        if ( it == m_dmaMemInfoMap.end() )
        {
            int fd;
            status = RemoteRegisterBuf( tensorDesc, fd );
            if ( QC_STATUS_OK == status )
            {
                Qnn_MemDescriptor_t desc;
                desc.memShape.numDim = tensorDesc.numDims;
                desc.memShape.dimSize = dims.data();
                desc.memShape.shapeConfig = nullptr;
                desc.dataType = SwitchToQnnDataType( tensorDesc.tensorType );

#if ( ( QNN_HTP_API_VERSION_MAJOR == 5 ) && ( QNN_HTP_API_VERSION_MINOR >= 16 ) ) ||               \
        ( QNN_HTP_API_VERSION_MAJOR > 5 )
                QnnMemHtp_Descriptor_t htpDesc;
                htpDesc.type = QNN_HTP_MEM_SHARED_BUFFER;
                htpDesc.size = tensorDesc.size;
                htpDesc.sharedBufferConfig.fd = fd;
                htpDesc.sharedBufferConfig.offset = tensorDesc.offset;

                desc.memType = QNN_MEM_TYPE_CUSTOM;
                desc.customInfo = &htpDesc;
#else
                desc.memType = QNN_MEM_TYPE_ION;
                desc.ionInfo.fd = fd;
#endif

                retVal = m_qnnFunctionPointers.qnnInterface.memRegister( m_context, &desc, 1,
                                                                         &memHandle );
                if ( QNN_SUCCESS == retVal )
                {
                    QnnImpl::DmaMemInfo_t info;
                    info.memHandle = memHandle;
                    info.size = tensorDesc.size;
                    info.fd = fd;
                    m_dmaMemInfoMap[tensorDesc.GetDataPtr()] = info;
                    QC_INFO( "succeed to register map buffer %p(%d, %" PRIu64 ", %" PRIu64
                             ") as %p for core %d",
                             tensorDesc.pBuf, fd, tensorDesc.size, tensorDesc.offset, memHandle,
                             m_config.processorType );
                }
                else
                {
                    (void) RemoteDeRegisterBuf( tensorDesc.pBuf, tensorDesc.size );
                    QC_ERROR( "failed to map buffer %p(%d, %" PRIu64 ", %" PRIu64
                              ") for core %d, error %d\n",
                              tensorDesc.pBuf, fd, tensorDesc.size, tensorDesc.offset,
                              m_config.processorType, retVal );
                    status = QC_STATUS_FAIL;
                }
            }
        }
        else
        {
            auto &info = it->second;
            memHandle = info.memHandle;
            QC_DEBUG( "already register map buffer %p(%d, %" PRIu64 ", %" PRIu64
                      ") as %p for core %d",
                      tensorDesc.pBuf, info.fd, tensorDesc.size, tensorDesc.offset, memHandle,
                      m_config.processorType );
        }
    }

    return status;
}

QCStatus_e QnnImpl::GetMemHandle( const TensorDescriptor_t &tensorDesc, Qnn_MemHandle_t &memHandle )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( ( QNN_PROCESSOR_HTP0 == m_config.processorType ) ||
         ( QNN_PROCESSOR_HTP1 == m_config.processorType ) ||
         ( QNN_PROCESSOR_HTP2 == m_config.processorType ) ||
         ( QNN_PROCESSOR_HTP3 == m_config.processorType ) )
    {
        status = RegisterBufferToHTP( tensorDesc, memHandle );
    }

    return status;
}

QCStatus_e QnnImpl::RegisterTensor( const TensorDescriptor_t &tensorDesc )
{
    Qnn_MemHandle_t memHandle = nullptr;
    return GetMemHandle( tensorDesc, memHandle );
}

QCStatus_e QnnImpl::Start()
{
    QCStatus_e status = QC_STATUS_OK;
    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "QNN node start failed due to wrong state!" );
        status = QC_STATUS_BAD_STATE;
    }

    return status;
}

QCStatus_e QnnImpl::ValidateTensor( const TensorDescriptor_t &tensorDesc,
                                    const Qnn_Tensor_t &tensor )
{
    QCStatus_e status = QC_STATUS_OK;
    const Qnn_DataType_t dataType = QNN_TENSOR_GET_DATA_TYPE( &tensor );
    const uint32_t rank = QNN_TENSOR_GET_RANK( &tensor );
    const uint32_t *dimensions = QNN_TENSOR_GET_DIMENSIONS( &tensor );
    const char *pName = QNN_TENSOR_GET_NAME( &tensor );

    QC_DEBUG( "input %s: type %d dims = [%u, %u, %u, %u, %u, %u, %u, %u], numDims = %u",
              tensorDesc.name.c_str() ?: "null", tensorDesc.tensorType, tensorDesc.dims[0],
              tensorDesc.dims[1], tensorDesc.dims[2], tensorDesc.dims[3], tensorDesc.dims[4],
              tensorDesc.dims[5], tensorDesc.dims[6], tensorDesc.dims[7], tensorDesc.numDims );

    if ( tensorDesc.pBuf == nullptr )
    {
        QC_ERROR( "Tensor %s: data pointer is nullptr", pName );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( tensorDesc.tensorType != SwitchFromQnnDataType( dataType ) )
    {
        QC_ERROR( "Tensor %s: Unmatched data type, input data type: %u, QNN data type: %x", pName,
                  tensorDesc.tensorType, dataType );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( rank != tensorDesc.numDims )
    {
        QC_ERROR( "Tensor %s: Unmatched numDims, input numDims: %u, expected numDims: %u", pName,
                  tensorDesc.numDims, rank );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }

    for ( size_t j = 1; j < rank; ++j )
    {
        if ( dimensions[j] != tensorDesc.dims[j] )
        {
            QC_ERROR( "Tensor %s: Unmatched dimension, input dimension: %u, expected dimension: %u",
                      pName, tensorDesc.dims[j], dimensions[j] );
            status = QC_STATUS_BAD_ARGUMENTS;
            break;
        }
    }

    return status;
}

QCStatus_e QnnImpl::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;
    NotifyParam_t *pNotifyParam = nullptr;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "QNN node not in running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == status )
    {
        for ( uint32_t i = 0; i < m_inputTensorNum; i++ )
        {
            Qnn_MemHandle_t memHandle = nullptr;
            uint32_t globalBufferId =
                    m_config.globalBufferIdMap[static_cast<size_t>( i )].globalBufferId;
            QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( globalBufferId );
            const TensorDescriptor_t *pTensor =
                    dynamic_cast<const TensorDescriptor_t *>( &bufDesc );
            if ( nullptr != pTensor )
            {
                status = ValidateTensor( *pTensor, m_graphsInfo[0]->inputTensors[i] );
                if ( QC_STATUS_OK != status )
                {
                    QC_ERROR( "input %u(%u) is not a valid tensor!", i, globalBufferId );
                }
                else
                {
                    status = GetMemHandle( *pTensor, memHandle );
                }
            }
            else
            {
                QC_ERROR( "input %u(%u) is not a tensor!", i, globalBufferId );
                status = QC_STATUS_INVALID_BUF;
            }
            if ( QC_STATUS_OK == status )
            {
                QNN_TENSOR_SET_DIMENSIONS( &m_inputs[i], (uint32_t *) pTensor->dims );
                if ( nullptr != memHandle )
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_inputs[i], QNN_TENSORMEMTYPE_MEMHANDLE );
                    QNN_TENSOR_SET_MEM_HANDLE( &m_inputs[i], memHandle );
                }
                else
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_inputs[i], QNN_TENSORMEMTYPE_RAW );
                    Qnn_ClientBuffer_t clientBuffer = { (uint8_t *) pTensor->GetDataPtr(),
                                                        (uint32_t) pTensor->GetDataSize() };
                    QNN_TENSOR_SET_CLIENT_BUF( &m_inputs[i], clientBuffer );
                }
            }
            else
            {
                break;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        for ( uint32_t i = 0; i < m_outputTensorNum; i++ )
        {
            Qnn_MemHandle_t memHandle = nullptr;
            uint32_t globalBufferId =
                    m_config.globalBufferIdMap[static_cast<size_t>( m_inputTensorNum + i )]
                            .globalBufferId;
            QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( globalBufferId );
            const TensorDescriptor_t *pTensor =
                    dynamic_cast<const TensorDescriptor_t *>( &bufDesc );
            if ( nullptr != pTensor )
            {
                status = ValidateTensor( *pTensor, m_graphsInfo[0]->outputTensors[i] );
                if ( QC_STATUS_OK != status )
                {
                    QC_ERROR( "output %u(%u) is not a valid tensor!", i, globalBufferId );
                }
                else
                {
                    status = GetMemHandle( *pTensor, memHandle );
                }
            }
            else
            {
                QC_ERROR( "output %u(%u) is not a tensor!", i, globalBufferId );
                status = QC_STATUS_INVALID_BUF;
            }
            if ( QC_STATUS_OK == status )
            {
                QNN_TENSOR_SET_DIMENSIONS( &m_outputs[i], (uint32_t *) pTensor->dims );
                if ( nullptr != memHandle )
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_outputs[i], QNN_TENSORMEMTYPE_MEMHANDLE );
                    QNN_TENSOR_SET_MEM_HANDLE( &m_outputs[i], memHandle );
                }
                else
                {
                    QNN_TENSOR_SET_MEM_TYPE( &m_outputs[i], QNN_TENSORMEMTYPE_RAW );
                    Qnn_ClientBuffer_t clientBuffer = { (uint8_t *) pTensor->GetDataPtr(),
                                                        (uint32_t) pTensor->GetDataSize() };
                    QNN_TENSOR_SET_CLIENT_BUF( &m_outputs[i], clientBuffer );
                }
            }
            else
            {
                break;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        const Qnn_GraphHandle_t hGraph = m_graphsInfo[0]->graph;
        if ( nullptr == m_callback )
        {
            retVal = m_qnnFunctionPointers.qnnInterface.graphExecute(
                    hGraph, m_inputs.data(), m_inputs.size(), m_outputs.data(), m_outputs.size(),
                    m_profileBackendHandle, nullptr );
        }
        else
        {
            pNotifyParam = m_notifyParamQ.Pop();
            if ( nullptr != pNotifyParam )
            {
                pNotifyParam->self = this;
                pNotifyParam->pFrameDesc = &frameDesc;
                retVal = m_qnnFunctionPointers.qnnInterface.graphExecuteAsync(
                        hGraph, m_inputs.data(), m_inputs.size(), m_outputs.data(),
                        m_outputs.size(), m_profileBackendHandle, nullptr, QnnNotifyFn,
                        pNotifyParam );
            }
            else
            {
                QC_ERROR( "notifyParamQ is empty!" );
                retVal = QNN_GRAPH_ERROR_MEM_ALLOC;
            }
        }
        if ( QNN_GRAPH_NO_ERROR != retVal )
        {
            QC_ERROR( "QNN failed %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
    }
    return status;
}

QCStatus_e QnnImpl::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_RUNNING == m_state ) || ( QC_OBJECT_STATE_ERROR == m_state ) )
    {
        if ( m_config.bDeRegisterAllBuffersWhenStop )
        {
            status = DeRegisterAllBuffers();
        }
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "QNN node stop failed due to wrong state!" );
        status = QC_STATUS_BAD_STATE;
    }

    return status;
}

QCStatus_e QnnImpl::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal = QNN_SUCCESS;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "QNN node not in ready status!" );
        status = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == status )
    {
        status = Destroy();
    }

    return status;
}


QCStatus_e QnnImpl::EnablePerf()
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "QNN node not in ready or running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr != m_profileBackendHandle )
    {
        QC_ERROR( "perf profiler already created!" );
        status = QC_STATUS_ALREADY;
    }
    else
    {
        retVal = m_qnnFunctionPointers.qnnInterface.profileCreate(
                m_backendHandle, QNN_PROFILE_LEVEL_BASIC, &m_profileBackendHandle );
        if ( QNN_GRAPH_NO_ERROR != retVal )
        {
            QC_ERROR( "failed to create profile. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
    }

    return status;
}

QCStatus_e QnnImpl::ExtractProfilingEvent( QnnProfile_EventId_t profileEventId, Qnn_Perf_t &perf,
                                           bool &bPerfDataValid )
{

    QCStatus_e status = QC_STATUS_OK;
    QnnProfile_EventData_t eventData;
    Qnn_ErrorHandle_t retVal;

    retVal = m_qnnFunctionPointers.qnnInterface.profileGetEventData( profileEventId, &eventData );
    if ( QNN_PROFILE_NO_ERROR != retVal )
    {
        QC_ERROR( "Failure in profile get event type. error is %" PRIu64, retVal );
        status = QC_STATUS_FAIL;
    }

    if ( QC_STATUS_OK == status )
    {
        QC_DEBUG( "Printing Event Info - Event Type: [%d], Event Value: [%" PRIu64
                  "], Event Identifier: [%s], Event Unit: [%d]",
                  eventData.type, eventData.value, eventData.identifier, eventData.unit );
        switch ( eventData.type )
        {
            case QNN_PROFILE_EVENTTYPE_EXECUTE:
                bPerfDataValid = true;
                perf.entireExecTime = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HOST_RPC_TIME_MICROSEC:
                bPerfDataValid = true;
                perf.rpcExecTimeCPU = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_HTP_RPC_TIME_MICROSEC:
                bPerfDataValid = true;
                perf.rpcExecTimeHTP = eventData.value;
                break;
            case QNN_HTP_PROFILE_EVENTTYPE_GRAPH_EXECUTE_ACCEL_TIME_MICROSEC:
                bPerfDataValid = true;
                perf.rpcExecTimeAcc = eventData.value;
                break;
            default:
            {
                // Unsupported qnn event profile type
                break;
            }
        }
    }

    return status;
}

QCStatus_e QnnImpl::GetPerf( Qnn_Perf_t &perf )
{
    bool bPerfDataValid = false;
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;
    const QnnProfile_EventId_t *profileEvents{ nullptr };
    uint32_t numEvents{ 0 };

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "QnnRuntime component not in running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == m_profileBackendHandle )
    {
        QC_ERROR( "perf was not enabled!" );
        status = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        (void) memset( &perf, 0, sizeof( perf ) );
        retVal = m_qnnFunctionPointers.qnnInterface.profileGetEvents( m_profileBackendHandle,
                                                                      &profileEvents, &numEvents );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            QC_ERROR( "Failure in profile get events." );
            status = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "ProfileEvents: numEvents: [%u]", numEvents );
            for ( uint32_t event = 0; event < numEvents; event++ )
            {
                (void) ExtractProfilingEvent( profileEvents[event], perf, bPerfDataValid );
            }

            if ( false == bPerfDataValid )
            {
                QC_ERROR( "no valid perf data!" );
                status = QC_STATUS_OUT_OF_BOUND;
            }
        }
    }

    return status;
}


QCStatus_e QnnImpl::DisablePerf()
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "QNN node not in ready or running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == m_profileBackendHandle )
    {
        QC_ERROR( "perf prifiler is not created!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        retVal = m_qnnFunctionPointers.qnnInterface.profileFree( m_profileBackendHandle );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not free backend profile handle. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        else
        {
            m_profileBackendHandle = nullptr;
        }
    }

    return status;
}

QCStatus_e QnnImpl::RemoteDeRegisterBuf( void *pData, size_t size )
{
    QCStatus_e status = QC_STATUS_OK;
#ifdef QC_USE_REMOTE_REGISTER_V2
    constexpr int client = 0;   // NOTE: default is 0
    int domain = CDSP_DOMAIN_ID;
    int extDomainId;
    if ( QNN_PROCESSOR_HTP1 == m_config.processorType )
    {
        domain = CDSP1_DOMAIN_ID;
    }
    extDomainId = get_extended_domains_id( domain, client );
#endif
    std::lock_guard<std::mutex> l( s_lock[m_config.processorType] );
    auto it = s_dmaMemRefMap[m_config.processorType].find( pData );
    if ( it != s_dmaMemRefMap[m_config.processorType].end() )
    {
        if ( it->second > 0 )
        {
            it->second--;
        }
        if ( 0 == it->second )
        {
#ifdef QC_USE_REMOTE_REGISTER_V2
            remote_register_buf_v2( extDomainId, pData, static_cast<int>( size ), -1 );
#else
            remote_register_buf( pData, static_cast<int>( size ), -1 );
#endif
            s_dmaMemRefMap[m_config.processorType].erase( it );
        }
    }
    else
    {
        /* buffer is possbile that remote_register_buf by others */
        QC_INFO( "Can't find buffer %p(%" PRIu64 ") in dma ref map", pData, size );
    }

    return status;
}

QCStatus_e QnnImpl::DeRegisterAllBuffers()
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal;
    std::lock_guard<std::mutex> l( m_lock );
    for ( auto &kv : m_dmaMemInfoMap )
    {
        auto pData = kv.first;
        auto &info = kv.second;
        retVal = m_qnnFunctionPointers.qnnInterface.memDeRegister( &info.memHandle, 1 );
        if ( QNN_SUCCESS != retVal )
        {
            QC_ERROR( "Failed to DeRegister memory. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        else
        {
            QC_INFO( "succeed to deregister buffer %p(%d, %" PRIu64 ") as %p for core %d", pData,
                     info.fd, info.size, info.memHandle, m_config.processorType );
        }
        if ( ( QNN_PROCESSOR_HTP0 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP1 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP2 == m_config.processorType ) ||
             ( QNN_PROCESSOR_HTP3 == m_config.processorType ) )
        {
            QCStatus_e status2 = RemoteDeRegisterBuf( pData, info.size );
            if ( QC_STATUS_OK != status2 )
            {
                status = status2;
            }
        }
    }
    m_dmaMemInfoMap.clear();

    return status;
}

QCStatus_e QnnImpl::Destroy()
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_ErrorHandle_t retVal = QNN_SUCCESS;

    status = DeRegisterAllBuffers();

    if ( nullptr != m_profileBackendHandle )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.profileFree( m_profileBackendHandle );
        if ( QNN_PROFILE_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not free backend profile handle. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        m_profileBackendHandle = nullptr;
    }

    if ( nullptr != m_context )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.contextFree( m_context, nullptr );
        if ( QNN_CONTEXT_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not free context. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        m_context = nullptr;
    }

    if ( nullptr != m_perfInfra )
    {
        for ( uint32_t &powerConfigId : m_powerConfigIds )
        {
            retVal = m_perfInfra->destroyPowerConfigId( powerConfigId );
            if ( QNN_CONTEXT_NO_ERROR != retVal )
            {
                QC_ERROR( "Could not destroy power config Id. Error is %" PRIu64, retVal );
                status = QC_STATUS_FAIL;
            }
        }
        m_perfInfra = nullptr;
    }

    if ( nullptr != m_deviceHandle )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.deviceFree( m_deviceHandle );
        if ( QNN_CONTEXT_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not free device handle. Error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        m_deviceHandle = nullptr;
    }

    if ( nullptr != m_platformInfo )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.deviceFreePlatformInfo( m_logHandle,
                                                                            m_platformInfo );
        if ( QNN_SUCCESS != retVal )
        {
            QC_ERROR( "Failed to free device platform info. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        m_platformInfo = nullptr;
    }

    if ( nullptr != m_backendHandle )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.backendFree( m_backendHandle );
        if ( QNN_BACKEND_NO_ERROR != retVal )
        {
            QC_ERROR( "Could not terminate backend. Error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        m_backendHandle = nullptr;
    }

    if ( nullptr != m_systemContext )
    {
        retVal = m_qnnFunctionPointers.qnnSystemInterface.systemContextFree( m_systemContext );
        if ( QNN_SUCCESS != retVal )
        {
            QC_ERROR( "Failed to free system context. error is %" PRIu64, retVal );
            status = QC_STATUS_FAIL;
        }
        m_systemContext = nullptr;
    }

    if ( m_logHandle != nullptr )
    {
        retVal = m_qnnFunctionPointers.qnnInterface.logFree( m_logHandle );
        if ( QNN_SUCCESS != retVal )
        {
            QC_WARN( "Unable to terminate logging in the backend. Error is %" PRIu64, retVal );
        }
        m_logHandle = nullptr;
    }

    if ( m_bLoadFromCachedBinary && ( nullptr != m_graphsInfo ) )
    {
        QC_DEBUG( "Cleaning up graph Info structures." );
        QCStatus_e retVal2 = FreeGraphsInfo( &m_graphsInfo, m_graphsCount );
        if ( QC_STATUS_OK != retVal2 )
        {
            QC_ERROR( "failed to free graph info. Error is %d", retVal2 );
            status = QC_STATUS_FAIL;
        }
        m_graphsInfo = nullptr;
    }

    if ( nullptr != m_modelLibraryHandle )
    {
        (void) dlclose( m_modelLibraryHandle );
        m_modelLibraryHandle = nullptr;
    }

    if ( nullptr != m_systemLibraryHandle )
    {
        (void) dlclose( m_systemLibraryHandle );
        m_systemLibraryHandle = nullptr;
    }

    if ( nullptr != m_backendLibraryHandle )
    {
        (void) dlclose( m_backendLibraryHandle );
        m_backendLibraryHandle = nullptr;
    }

    m_state = QC_OBJECT_STATE_INITIAL;

    return status;
}

QCObjectState_e QnnImpl::GetState()
{
    return m_state;
}

void QnnImpl::QnnNotifyFn( void *pNotifyParam, Qnn_NotifyStatus_t notifyStatus )
{
    NotifyParam_t *pNotifyParam2 = static_cast<NotifyParam_t *>( pNotifyParam );

    pNotifyParam2->self->QnnNotifyFn( pNotifyParam2, notifyStatus );
}

void QnnImpl::QnnNotifyFn( NotifyParam_t *pNotifyParam, Qnn_NotifyStatus_t notifyStatus )
{
    if ( QNN_SUCCESS == notifyStatus.error )
    {
        if ( m_callback != nullptr )
        {
            QCFrameDescriptorNodeIfs *pFrameDesc = pNotifyParam->pFrameDesc;
            QCNodeEventInfo_t info( *pFrameDesc, m_nodeId, QC_STATUS_OK, GetState() );
            m_callback( info );
        }
        else
        {
            QC_ERROR( "output callback is nullptr!" );
        }
    }
    else
    {
        if ( m_callback != nullptr )
        {
            QCFrameDescriptorNodeIfs *pFrameDesc = pNotifyParam->pFrameDesc;
            QCBufferDescriptorBase_t errDesc;
            uint32_t globalBufferId =
                    m_config.globalBufferIdMap[static_cast<size_t>( m_inputTensorNum +
                                                                    m_outputTensorNum )]
                            .globalBufferId;
            errDesc.name = "QNN ERROR";
            errDesc.pBuf = &notifyStatus;
            errDesc.size = sizeof( notifyStatus );
            QCStatus_e status = pFrameDesc->SetBuffer( globalBufferId, errDesc );
            if ( QC_STATUS_OK == status )
            {
                QCNodeEventInfo_t info( *pFrameDesc, m_nodeId, QC_STATUS_FAIL, GetState() );
                m_callback( info );
            }
            else
            {
                QC_ERROR( "Failed to set error buffer descriptor" );
            }
        }
        else
        {
            QC_ERROR( "error callback is nullptr!" );
        }
    }

    m_notifyParamQ.Push( pNotifyParam );
}


QCStatus_e QnnImpl::SetupGlobalBufferIdMap()
{
    QCStatus_e status = QC_STATUS_OK;

    m_inputTensorNum = m_graphsInfo[0]->numInputTensors;
    m_outputTensorNum = m_graphsInfo[0]->numOutputTensors;
    m_bufferDescNum = m_inputTensorNum + m_outputTensorNum + 1;
    m_inputs.resize( m_inputTensorNum );
    m_outputs.resize( m_outputTensorNum );
    if ( m_config.globalBufferIdMap.size() > 0 )
    {
        if ( m_bufferDescNum == m_config.globalBufferIdMap.size() )
        {
            uint32_t globalBufferId = 0;
            for ( uint32_t i = 0; i < m_inputTensorNum; i++ )
            {
                Qnn_Tensor_t *pTensor = &m_graphsInfo[0]->inputTensors[i];
                std::string name( QNN_TENSOR_GET_NAME( pTensor ) );
                m_inputs[i] = *pTensor;
                if ( m_config.globalBufferIdMap[globalBufferId].name != name )
                {
                    QC_ERROR( "global buffer map[%" PRIu32 "] name %s != %s", globalBufferId,
                              m_config.globalBufferIdMap[globalBufferId].name.c_str(),
                              name.c_str() );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
                if ( QC_STATUS_OK != status )
                {
                    break;
                }
                /* In scenarios where the same FrameDescriptor is passed through all nodes,
                 * the actual size of the buffer descriptor exceeds the total number of QNN
                 * inputs and outputs. */
                if ( m_config.globalBufferIdMap[globalBufferId].globalBufferId > m_bufferDescNum )
                {
                    m_bufferDescNum = m_config.globalBufferIdMap[globalBufferId].globalBufferId + 1;
                }
                globalBufferId++;
            }
            if ( QC_STATUS_OK == status )
            {
                for ( uint32_t i = 0; i < m_outputTensorNum; i++ )
                {
                    Qnn_Tensor_t *pTensor = &m_graphsInfo[0]->outputTensors[i];
                    std::string name( QNN_TENSOR_GET_NAME( pTensor ) );
                    m_outputs[i] = *pTensor;
                    if ( m_config.globalBufferIdMap[globalBufferId].name != name )
                    {
                        QC_ERROR( "global buffer map[%" PRIu32 "] name %s != %s", globalBufferId,
                                  m_config.globalBufferIdMap[globalBufferId].name.c_str(),
                                  name.c_str() );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    if ( QC_STATUS_OK != status )
                    {
                        break;
                    }
                    globalBufferId++;
                }
            }
        }
        else
        {
            QC_ERROR( "global buffer map size is not correct: expect %" PRIu32, m_bufferDescNum );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }
    else
    { /* create a default global buffer index map */
        m_config.globalBufferIdMap.resize( m_bufferDescNum );
        uint32_t globalBufferId = 0;
        for ( uint32_t i = 0; i < m_inputTensorNum; i++ )
        {
            Qnn_Tensor_t *pTensor = &m_graphsInfo[0]->inputTensors[i];
            m_inputs[i] = *pTensor;
            m_config.globalBufferIdMap[globalBufferId].name = QNN_TENSOR_GET_NAME( pTensor );
            m_config.globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
            globalBufferId++;
        }
        for ( uint32_t i = 0; i < m_outputTensorNum; i++ )
        {
            Qnn_Tensor_t *pTensor = &m_graphsInfo[0]->outputTensors[i];
            m_outputs[i] = *pTensor;
            m_config.globalBufferIdMap[globalBufferId].name = QNN_TENSOR_GET_NAME( pTensor );
            m_config.globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
            globalBufferId++;
        }

        m_config.globalBufferIdMap[globalBufferId].name = "QNN ASYNC ERROR";
        m_config.globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
    }

    return status;
}

QCStatus_e QnnImpl::GetInputTensors( std::vector<Qnn_Tensor_t> &inputTensors )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "QNN Node not in ready or running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {

        inputTensors.resize( m_inputTensorNum );
        for ( uint32_t i = 0; i < m_inputTensorNum; ++i )
        {
            QCTensorProps_t tensorProp;
            inputTensors[i] = m_graphsInfo[0]->inputTensors[i];
        }
    }

    return status;
}


QCStatus_e QnnImpl::GetOutputTensors( std::vector<Qnn_Tensor_t> &outputTensors )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "QNN Node not in ready or running status!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {

        outputTensors.resize( m_outputTensorNum );
        for ( uint32_t i = 0; i < m_outputTensorNum; ++i )
        {
            QCTensorProps_t tensorProp;
            outputTensors[i] = m_graphsInfo[0]->outputTensors[i];
        }
    }

    return status;
}

QCTensorType_e QnnImpl::SwitchFromQnnDataType( Qnn_DataType_t dataType )
{
    QCTensorType_e tensorType = QC_TENSOR_TYPE_MAX;
    switch ( dataType )
    {
        case QNN_DATATYPE_INT_8:
            tensorType = QC_TENSOR_TYPE_INT_8;
            break;

        case QNN_DATATYPE_INT_16:
            tensorType = QC_TENSOR_TYPE_INT_16;
            break;

        case QNN_DATATYPE_INT_32:
            tensorType = QC_TENSOR_TYPE_INT_32;
            break;

        case QNN_DATATYPE_INT_64:
            tensorType = QC_TENSOR_TYPE_INT_64;
            break;

        case QNN_DATATYPE_UINT_8:
            tensorType = QC_TENSOR_TYPE_UINT_8;
            break;

        case QNN_DATATYPE_UINT_16:
            tensorType = QC_TENSOR_TYPE_UINT_16;
            break;

        case QNN_DATATYPE_UINT_32:
            tensorType = QC_TENSOR_TYPE_UINT_32;
            break;

        case QNN_DATATYPE_UINT_64:
            tensorType = QC_TENSOR_TYPE_UINT_64;
            break;

        case QNN_DATATYPE_FLOAT_16:
            tensorType = QC_TENSOR_TYPE_FLOAT_16;
            break;

        case QNN_DATATYPE_FLOAT_32:
            tensorType = QC_TENSOR_TYPE_FLOAT_32;
            break;

        case QNN_DATATYPE_FLOAT_64:
            tensorType = QC_TENSOR_TYPE_FLOAT_64;
            break;

        case QNN_DATATYPE_SFIXED_POINT_8:
            tensorType = QC_TENSOR_TYPE_SFIXED_POINT_8;
            break;

        case QNN_DATATYPE_SFIXED_POINT_16:
            tensorType = QC_TENSOR_TYPE_SFIXED_POINT_16;
            break;

        case QNN_DATATYPE_SFIXED_POINT_32:
            tensorType = QC_TENSOR_TYPE_SFIXED_POINT_32;
            break;

        case QNN_DATATYPE_UFIXED_POINT_8:
            tensorType = QC_TENSOR_TYPE_UFIXED_POINT_8;
            break;

        case QNN_DATATYPE_UFIXED_POINT_16:
            tensorType = QC_TENSOR_TYPE_UFIXED_POINT_16;
            break;

        case QNN_DATATYPE_UFIXED_POINT_32:
            tensorType = QC_TENSOR_TYPE_UFIXED_POINT_32;
            break;

        default:
            QC_ERROR( "unsupported qnn data type: %d", (int) dataType );
            break;
    }
    return tensorType;
}

Qnn_DataType_t QnnImpl::SwitchToQnnDataType( QCTensorType_e tensorType )
{
    Qnn_DataType_t dataType = QNN_DATATYPE_UNDEFINED;
    switch ( tensorType )
    {
        case QC_TENSOR_TYPE_INT_8:
            dataType = QNN_DATATYPE_INT_8;
            break;

        case QC_TENSOR_TYPE_INT_16:
            dataType = QNN_DATATYPE_INT_16;
            break;

        case QC_TENSOR_TYPE_INT_32:
            dataType = QNN_DATATYPE_INT_32;
            break;

        case QC_TENSOR_TYPE_INT_64:
            dataType = QNN_DATATYPE_INT_64;
            break;

        case QC_TENSOR_TYPE_UINT_8:
            dataType = QNN_DATATYPE_UINT_8;
            break;

        case QC_TENSOR_TYPE_UINT_16:
            dataType = QNN_DATATYPE_UINT_16;
            break;

        case QC_TENSOR_TYPE_UINT_32:
            dataType = QNN_DATATYPE_UINT_32;
            break;

        case QC_TENSOR_TYPE_UINT_64:
            dataType = QNN_DATATYPE_UINT_64;
            break;

        case QC_TENSOR_TYPE_FLOAT_16:
            dataType = QNN_DATATYPE_FLOAT_16;
            break;

        case QC_TENSOR_TYPE_FLOAT_32:
            dataType = QNN_DATATYPE_FLOAT_32;
            break;

        case QC_TENSOR_TYPE_FLOAT_64:
            dataType = QNN_DATATYPE_FLOAT_64;
            break;

        case QC_TENSOR_TYPE_SFIXED_POINT_8:
            dataType = QNN_DATATYPE_SFIXED_POINT_8;
            break;

        case QC_TENSOR_TYPE_SFIXED_POINT_16:
            dataType = QNN_DATATYPE_SFIXED_POINT_16;
            break;

        case QC_TENSOR_TYPE_SFIXED_POINT_32:
            dataType = QNN_DATATYPE_SFIXED_POINT_32;
            break;

        case QC_TENSOR_TYPE_UFIXED_POINT_8:
            dataType = QNN_DATATYPE_UFIXED_POINT_8;
            break;

        case QC_TENSOR_TYPE_UFIXED_POINT_16:
            dataType = QNN_DATATYPE_UFIXED_POINT_16;
            break;

        case QC_TENSOR_TYPE_UFIXED_POINT_32:
            dataType = QNN_DATATYPE_UFIXED_POINT_32;
            break;

        default:
            QC_ERROR( "unsupported qcnode tensor type: %d", (int) tensorType );
            break;
    }
    return dataType;
}

void QnnImpl::NotifyParamQueue::Init()
{
    uint16_t idx;
    pushIdx = 0;
    popIdx = 0;

    for ( idx = 0; idx < QNN_NOTIFY_PARAM_NUM; idx++ )
    {
        ring[pushIdx % QNN_NOTIFY_PARAM_NUM] = idx;
        pushIdx++;
    }
}

void QnnImpl::NotifyParamQueue::Push( NotifyParam_t *pNotifyParam )
{
    uint16_t idx;

    if ( ( pNotifyParam >= &notifyParam[0] ) &&
         ( pNotifyParam <= &notifyParam[QNN_NOTIFY_PARAM_NUM - 1] ) )
    {
        idx = static_cast<uint16_t>( pNotifyParam - &notifyParam[0] );
        ring[pushIdx % QNN_NOTIFY_PARAM_NUM] = idx;
        pushIdx++;
    }
    else
    {
        QC_LOG_ERROR( "QnnImpl NotifyParamQueue Push with invalid param!" );
    }
}

QnnImpl::NotifyParam_t *QnnImpl::NotifyParamQueue::Pop()
{
    NotifyParam_t *pNotifyParam = nullptr;
    uint16_t idx;

    if ( pushIdx != popIdx )
    {
        idx = ring[popIdx % QNN_NOTIFY_PARAM_NUM];
        popIdx++;
        pNotifyParam = &notifyParam[idx];
    }

    return pNotifyParam;
}


}   // namespace Node
}   // namespace QC
