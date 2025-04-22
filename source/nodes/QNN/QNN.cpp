// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/node/QNN.hpp"
#include <unistd.h>

namespace QC
{
namespace node
{

QCStatus_e QnnConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;
    std::string name = dt.Get<std::string>( "name", "" );
    if ( "" == name )
    {
        errors += "the name is empty, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t id = dt.Get<uint32_t>( "id", UINT32_MAX );
    if ( UINT32_MAX == id )
    {
        errors += "the id is empty, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    QCProcessorType_e processorType = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
    if ( QC_PROCESSOR_MAX == processorType )
    {
        errors += "the processorType is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string loadType = dt.Get<std::string>( "loadType", "binary" );
    if ( ( "binary" == loadType ) || ( "libaray" == loadType ) )
    {
        std::string modelPath = dt.Get<std::string>( "modelPath", "" );
        if ( "" == modelPath )
        {
            errors += "the modelPath is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            int err = access( modelPath.c_str(), F_OK );
            if ( 0 != err )
            {
                errors += "the modelPath <" + modelPath + "> is invalid, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }
    else if ( "buffer" == loadType )
    {
        uint32_t contextBufferId = dt.Get<uint32_t>( "contextBufferId", UINT32_MAX );
        if ( UINT32_MAX == contextBufferId )
        {
            errors += "the contextBufferId is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }
    else
    {
        errors += "the loadType <" + loadType + "> is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Exists( "bufferIds" ) )
    {
        std::vector<uint32_t> bufferIds = dt.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );
        if ( 0 == bufferIds.size() )
        {
            errors += "the bufferIds is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    std::string priority = dt.Get<std::string>( "priority", "normal" );
    if ( ( "low" != priority ) && ( "normal" != priority ) && ( "normal_high" != priority ) &&
         ( "high" != priority ) )
    {
        errors += "the priority is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::vector<DataTree> udos;
    status2 = dt.Get( "udoPackages", udos );
    if ( QC_STATUS_OUT_OF_BOUND == status2 )
    {
        /* OK if not configured */
    }
    else if ( QC_STATUS_OK != status2 )
    {
        errors += "the udoPackages is invalid, ";
        status = status2;
    }
    else
    {
        uint32_t idx = 0;
        for ( DataTree &udo : udos )
        {
            std::string udoLibPath = udo.Get<std::string>( "udoLibPath", "" );
            std::string interfaceProvider = udo.Get<std::string>( "interfaceProvider", "" );
            if ( "" == udoLibPath )
            {
                errors += "the udo " + std::to_string( idx ) + " library path is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            if ( "" == interfaceProvider )
            {
                errors += "the udo " + std::to_string( idx ) + " interface is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            idx++;
        }
    }

    std::vector<DataTree> globalBufferIdMap;
    status2 = dt.Get( "globalBufferIdMap", globalBufferIdMap );
    if ( QC_STATUS_OUT_OF_BOUND == status2 )
    {
        /* OK if not configured */
    }
    else if ( QC_STATUS_OK != status2 )
    {
        errors += "the globalBufferIdMap is invalid, ";
        status = status2;
    }
    else
    {
        uint32_t idx = 0;
        for ( DataTree &gbm : globalBufferIdMap )
        {
            std::string name = gbm.Get<std::string>( "name", "" );
            uint32_t index = gbm.Get<uint32_t>( "id", UINT32_MAX );
            if ( "" == name )
            {
                errors += "the globalIdMap " + std::to_string( idx ) + " name is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( UINT32_MAX == index )
            {
                errors += "the globalIdMap " + std::to_string( idx ) + " id is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            idx++;
        }
    }

    return status;
}


QCStatus_e QnnConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );
        m_config.params.processorType = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
        std::string loadType = dt.Get<std::string>( "loadType", "binary" );
        if ( "binary" == loadType )
        {
            m_modelPath = dt.Get<std::string>( "modelPath", "" );
            m_config.params.contextBuffer = nullptr;
            m_config.params.contextSize = 0;
            m_config.params.modelPath = m_modelPath.c_str();
            m_config.params.loadType = QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE;
            QC_DEBUG( "QNN binary model: %s", m_config.params.modelPath );
        }
        else if ( "libaray" == loadType )
        {
            m_modelPath = dt.Get<std::string>( "modelPath", "" );
            m_config.params.contextBuffer = nullptr;
            m_config.params.contextSize = 0;
            m_config.params.modelPath = m_modelPath.c_str();
            m_config.params.loadType = QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE;
            QC_DEBUG( "QNN library model: %s", m_config.params.modelPath );
        }
        else /* buffer */
        {
            m_config.params.contextBuffer = nullptr;
            m_config.params.contextSize = 0;
            m_config.contextBufferId = dt.Get<uint32_t>( "contextBufferId", UINT32_MAX );
            m_config.params.modelPath = nullptr;
            m_config.params.loadType = QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER;
            QC_DEBUG( "QNN library model: %s", m_config.params.modelPath );
        }

        std::string priority = dt.Get<std::string>( "priority", "normal" );
        if ( "low" == priority )
        {
            m_config.params.priority = QNN_PRIORITY_LOW;
        }
        else if ( "normal" == priority )
        {
            m_config.params.priority = QNN_PRIORITY_NORMAL;
        }
        else if ( "normal_high" == priority )
        {
            m_config.params.priority = QNN_PRIORITY_NORMAL_HIGH;
        }
        else
        {
            m_config.params.priority = QNN_PRIORITY_HIGH;
        }

        m_config.bufferIds = dt.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );

        std::vector<DataTree> dts;
        (void) dt.Get( "udoPackages", dts );
        m_udoPkgs.resize( dts.size() );
        m_udoPackages.resize( dts.size() );
        uint32_t idx = 0;
        for ( DataTree &udt : dts )
        {
            m_udoPkgs[idx].udoLibPath = udt.Get<std::string>( "udoLibPath", "" );
            m_udoPkgs[idx].interfaceProvider = udt.Get<std::string>( "interfaceProvider", "" );
            m_udoPackages[idx].udoLibPath = m_udoPkgs[idx].udoLibPath.c_str();
            m_udoPackages[idx].interfaceProvider = m_udoPkgs[idx].interfaceProvider.c_str();
            idx++;
        }

        if ( m_udoPackages.size() > 0 )
        {
            m_config.params.pUdoPackages = m_udoPackages.data();
            m_config.params.numOfUdoPackages = static_cast<int>( m_udoPackages.size() );
        }
        else
        {
            m_config.params.pUdoPackages = nullptr;
            m_config.params.numOfUdoPackages = 0;
        }

        std::vector<DataTree> globalBufferIdMap;
        (void) dt.Get( "globalBufferIdMap", globalBufferIdMap );
        m_config.globalBufferIdMap.resize( globalBufferIdMap.size() );
        idx = 0;
        for ( DataTree &gbm : globalBufferIdMap )
        {
            m_config.globalBufferIdMap[idx].name = gbm.Get<std::string>( "name", "" );
            m_config.globalBufferIdMap[idx].globalBufferId = gbm.Get<uint32_t>( "id", UINT32_MAX );
            idx++;
        }

        m_config.bDeRegisterAllBuffersWhenStop =
                dt.Get<bool>( "deRegisterAllBuffersWhenStop", false );
    }

    return status;
}

QCStatus_e QnnConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( dt.Exists( "enablePerf" ) )
    {
        bool bEnablePerf = dt.Get<bool>( "enablePerf", false );
        if ( bEnablePerf )
        {
            status = m_qnn.EnablePerf();
        }
        else
        {
            status = m_qnn.DisablePerf();
        }

        if ( QC_STATUS_OK != status )
        {
            QC_ERROR( "Failed to %s perf: %d", bEnablePerf ? "Enable" : "Disable", status );
        }
        else
        {
            QC_DEBUG( "%s perf OK", bEnablePerf ? "Enable" : "Disable" );
        }
    }


    return status;
}

QCStatus_e QnnConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    status = NodeConfigIfs::VerifyAndSet( config, errors );
    if ( QC_STATUS_OK == status )
    {
        DataTree dt;
        status = m_dataTree.Get( "static", dt );
        if ( QC_STATUS_OK == status )
        {
            status = ParseStaticConfig( dt, errors );
        }
        else
        {
            status = m_dataTree.Get( "dynamic", dt );
            if ( QC_STATUS_OK == status )
            {
                status = ApplyDynamicConfig( dt, errors );
            }
        }
    }

    return status;
}

DataTree QnnConfigIfs::ConvertTensorInfoToJson( const QnnRuntime_TensorInfo_t &info )
{
    DataTree dt;
    std::vector<uint32_t> dims( info.properties.dims,
                                info.properties.dims + info.properties.numDims );

    dt.Set<std::string>( "name", std::string( info.pName ) );
    dt.SetTensorType( "type", info.properties.type );
    dt.Set<std::uint32_t>( "dims", dims );
    dt.Set<float>( "quantScale", info.quantScale );
    dt.Set<int32_t>( "quantOffset", info.quantOffset );

    return dt;
}

std::vector<DataTree>
QnnConfigIfs::ConvertTensorInfoListToJson( const QnnRuntime_TensorInfoList_t &infoList )
{
    std::vector<DataTree> dts;

    for ( uint32_t i = 0; i < infoList.num; i++ )
    {
        DataTree dt = ConvertTensorInfoToJson( infoList.pInfo[i] );
        dts.push_back( dt );
    }

    return dts;
}

const std::string &QnnConfigIfs::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;
    QnnRuntime_TensorInfoList_t inputInfoList;
    QnnRuntime_TensorInfoList_t outputInfoList;
    DataTree dt;

    if ( false == m_bOptionsBuilt )
    {
        status = m_qnn.GetInputInfo( &inputInfoList );
        if ( QC_STATUS_OK == status )
        {
            status = m_qnn.GetOutputInfo( &outputInfoList );
        }

        if ( QC_STATUS_OK == status )
        {
            std::vector<DataTree> inputDts = ConvertTensorInfoListToJson( inputInfoList );
            std::vector<DataTree> outputDts = ConvertTensorInfoListToJson( outputInfoList );
            dt.Set( "model.inputs", inputDts );
            dt.Set( "model.outputs", outputDts );
            m_options = dt.Dump();
            m_bOptionsBuilt = true;
        }
        else
        {
            QC_ERROR( "Failed to get QNN input output information" );
            m_options = "{}";
        }
    }
    return m_options;
}

uint32_t QnnMonitoringIfs::GetMaximalSize()
{
    return sizeof( QnnRuntime_Perf_t );
}

uint32_t QnnMonitoringIfs::GetCurrentSize()
{
    return sizeof( QnnRuntime_Perf_t );
}

QCStatus_e QnnMonitoringIfs::Place( void *pData, uint32_t &size )
{
    QCStatus_e status = QC_STATUS_OK;
    QnnRuntime_Perf_t perf;
    QnnRuntime_Perf_t *pPerf = reinterpret_cast<QnnRuntime_Perf_t *>( pData );

    if ( nullptr == pData )
    {
        QC_ERROR( "Place with invalid size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( size != sizeof( QnnRuntime_Perf_t ) )
    {
        QC_ERROR( "Place with invalid size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        status = m_qnn.GetPerf( pPerf );
    }

    return status;
}

void Qnn::OutputCallback( void *pAppPriv, void *pOutputPriv )
{
    Qnn *self = reinterpret_cast<Qnn *>( pAppPriv );
    if ( nullptr != self )
    {
        self->OutputCallback( pOutputPriv );
    }
    else
    {
        QC_LOG_ERROR( "QNN: pAppPriv is invalid" );
    }
}

void Qnn::ErrorCallback( void *pAppPriv, void *pOutputPriv, Qnn_NotifyStatus_t notifyStatus )
{
    Qnn *self = reinterpret_cast<Qnn *>( pAppPriv );
    if ( nullptr != self )
    {
        self->ErrorCallback( pOutputPriv, notifyStatus );
    }
    else
    {
        QC_LOG_ERROR( "QNN: pAppPriv is invalid" );
    }
}

void Qnn::OutputCallback( void *pOutputPriv )
{
    uint64_t ticketId = reinterpret_cast<uint64_t>( pOutputPriv );
    std::lock_guard<std::mutex> l( m_lock );
    auto it = m_frameDescMap.find( ticketId );
    if ( it != m_frameDescMap.end() )
    {
        QCFrameDescriptorNodeIfs *pFrameDesc = it->second;
        if ( nullptr != m_callback )
        {
            QCNodeEventInfo_t info( *pFrameDesc, m_nodeId, QC_STATUS_OK, m_qnn.GetState() );
            m_callback( info );
            m_pFrameDescPool->Put( *pFrameDesc );
        }
        else
        {
            QC_ERROR( "callback is invalid" );
        }
    }
    else
    {
        QC_ERROR( "invalid ticket Id=%" PRIu64, ticketId );
    }
}

void Qnn::ErrorCallback( void *pOutputPriv, Qnn_NotifyStatus_t notifyStatus )
{
    QCStatus_e status;
    uint64_t ticketId = reinterpret_cast<uint64_t>( pOutputPriv );
    std::lock_guard<std::mutex> l( m_lock );
    auto it = m_frameDescMap.find( ticketId );
    if ( it != m_frameDescMap.end() )
    {
        QCFrameDescriptorNodeIfs *pFrameDesc = it->second;
        if ( nullptr != m_callback )
        {
            QCBufferDescriptorBase_t errDesc;
            uint32_t globalBufferId = m_globalBufferIdMap[m_inputNum + m_outputNum].globalBufferId;
            errDesc.name = "QNN ERROR";
            errDesc.pBuf = &notifyStatus;
            errDesc.size = sizeof( notifyStatus );
            status = pFrameDesc->SetBuffer( globalBufferId, errDesc );
            if ( QC_STATUS_OK == status )
            {
                QCNodeEventInfo_t info( *pFrameDesc, m_nodeId, QC_STATUS_FAIL, m_qnn.GetState() );
                m_callback( info );
            }
            else
            {
                QC_ERROR( "Failed to set error buffer descriptor" );
            }
        }
        else
        {
            QC_ERROR( "callback is invalid" );
        }
        m_pFrameDescPool->Put( *pFrameDesc );
    }
    else
    {
        QC_ERROR( "invalid ticket Id=%" PRIu64, ticketId );
    }
}

QCStatus_e Qnn::SetupGlobalBufferIdMap( const QnnConfig_t &cfg )
{
    QCStatus_e status = QC_STATUS_OK;
    QnnRuntime_TensorInfoList_t inputInfoList;
    QnnRuntime_TensorInfoList_t outputInfoList;
    status = m_qnn.GetInputInfo( &inputInfoList );
    if ( QC_STATUS_OK == status )
    {
        status = m_qnn.GetOutputInfo( &outputInfoList );
    }
    if ( QC_STATUS_OK == status )
    {
        m_inputNum = inputInfoList.num;
        m_outputNum = outputInfoList.num;
        m_bufferDescNum = m_inputNum + m_outputNum + 1;
        if ( cfg.globalBufferIdMap.size() > 0 )
        {
            if ( ( m_inputNum + m_outputNum + 1 ) == cfg.globalBufferIdMap.size() )
            {
                uint32_t globalBufferId = 0;
                for ( uint32_t i = 0; i < m_inputNum; i++ )
                {
                    const QnnRuntime_TensorInfo_t *pInfo = &inputInfoList.pInfo[i];
                    if ( cfg.globalBufferIdMap[globalBufferId].name != pInfo->pName )
                    {
                        QC_ERROR( "global buffer map[%" PRIu32 "] name %s != %s", globalBufferId,
                                  cfg.globalBufferIdMap[globalBufferId].name.c_str(),
                                  pInfo->pName );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    if ( QC_STATUS_OK != status )
                    {
                        break;
                    }
                    /* In scenarios where the same FrameDescriptor is passed through all nodes,
                     * the actual size of the buffer descriptor exceeds the total number of QNN
                     * inputs and outputs. */
                    if ( cfg.globalBufferIdMap[globalBufferId].globalBufferId > m_bufferDescNum )
                    {
                        m_bufferDescNum = cfg.globalBufferIdMap[globalBufferId].globalBufferId + 1;
                    }
                    globalBufferId++;
                }
                if ( QC_STATUS_OK == status )
                {
                    for ( uint32_t i = 0; i < m_outputNum; i++ )
                    {
                        const QnnRuntime_TensorInfo_t *pInfo = &outputInfoList.pInfo[i];
                        if ( cfg.globalBufferIdMap[globalBufferId].name != pInfo->pName )
                        {
                            QC_ERROR( "global buffer map[%" PRIu32 "] name %s != %s",
                                      globalBufferId,
                                      cfg.globalBufferIdMap[globalBufferId].name.c_str(),
                                      pInfo->pName );
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
                QC_ERROR( "global buffer map size is not correct: expect %" PRIu32,
                          m_inputNum + m_outputNum + 1 );
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( QC_STATUS_OK == status )
            {
                m_globalBufferIdMap = cfg.globalBufferIdMap;
            }
        }
        else
        { /* create a default global buffer index map */
            m_globalBufferIdMap.resize( m_inputNum + m_outputNum + 1 );
            uint32_t globalBufferId = 0;
            for ( uint32_t i = 0; i < m_inputNum; i++ )
            {
                const QnnRuntime_TensorInfo_t *pInfo = &inputInfoList.pInfo[i];
                m_globalBufferIdMap[globalBufferId].name = pInfo->pName;
                m_globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
                globalBufferId++;
            }
            for ( uint32_t i = 0; i < m_outputNum; i++ )
            {
                const QnnRuntime_TensorInfo_t *pInfo = &outputInfoList.pInfo[i];
                m_globalBufferIdMap[globalBufferId].name = pInfo->pName;
                m_globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
                globalBufferId++;
            }

            m_globalBufferIdMap[globalBufferId].name = "QNN ASYNC ERROR";
            m_globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
        }
    }
    return status;
}

QCStatus_e Qnn::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const QnnConfig_t *pConfig = dynamic_cast<const QnnConfig_t *>( &cfg );
    QnnRuntime_Config_t params;
    bool bNodeBaseInitDone = false;
    bool bQnnInitDone = false;

    status = m_configIfs.VerifyAndSet( config.config, errors );
    if ( QC_STATUS_OK == status )
    {
        status = NodeBase::Init( cfg.nodeId );
    }
    else
    {
        QC_ERROR( "config error: %s", errors.c_str() );
    }

    if ( QC_STATUS_OK == status )
    {
        bNodeBaseInitDone = true;
        params = pConfig->params;
        if ( QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER == params.loadType )
        {
            uint32_t contextBufferId = pConfig->contextBufferId;
            if ( contextBufferId < config.buffers.size() )
            {
                const QCBufferDescriptorBase_t &bufData = config.buffers[contextBufferId];
                params.contextBuffer = static_cast<uint8_t *>( bufData.pBuf );
                params.contextSize = bufData.size;
            }
            else
            {
                QC_ERROR( "context buffer index out of range" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        status = m_qnn.Init( m_nodeId.name.c_str(), &params );
    }

    if ( QC_STATUS_OK == status )
    {
        status = SetupGlobalBufferIdMap( *pConfig );
    }

    if ( QC_STATUS_OK == status )
    {
        bQnnInitDone = true;
        m_callback = config.callback;
        if ( nullptr != m_callback )
        {
            m_ticketId = 1;
            m_frameDescMap.clear();
            status = m_qnn.RegisterCallback( OutputCallback, ErrorCallback, this );
            if ( QC_STATUS_OK == status )
            {
                m_pFrameDescPool = new QCSharedFrameDescriptorNodePool( QNNRUNTIME_NOTIFY_PARAM_NUM,
                                                                        m_bufferDescNum );
                if ( nullptr == m_pFrameDescPool )
                {
                    QC_ERROR( "Allocate Frame Descriptor Node Pool failed" );
                    status = QC_STATUS_NOMEM;
                }
            }
        }
    }


    if ( QC_STATUS_OK == status )
    {   // do buffer register during initialization
        m_bDeRegisterAllBuffersWhenStop = pConfig->bDeRegisterAllBuffersWhenStop;
        for ( uint32_t bufferId : pConfig->bufferIds )
        {
            if ( bufferId < config.buffers.size() )
            {
                const QCBufferDescriptorBase_t &bufDesc = config.buffers[bufferId];
                const QCSharedBufferDescriptor_t *pSharedBuffer =
                        dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
                if ( nullptr == pSharedBuffer )
                {
                    QC_ERROR( "buffer %" PRIu32 "is invalid", bufferId );
                    status = QC_STATUS_INVALID_BUF;
                }
                else
                {
                    status = m_qnn.RegisterBuffers( &( pSharedBuffer->buffer ), 1 );
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

    if ( QC_STATUS_OK != status )
    { /* do error clean up */
        if ( nullptr != m_pFrameDescPool )
        {
            delete m_pFrameDescPool;
            m_pFrameDescPool = nullptr;
        }

        if ( bQnnInitDone )
        {
            (void) m_qnn.Deinit();
        }
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e Qnn::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    if ( nullptr != m_pFrameDescPool )
    {
        delete m_pFrameDescPool;
        m_pFrameDescPool = nullptr;
    }

    status2 = m_qnn.Deinit();
    if ( QC_STATUS_OK == status2 )
    {
        status = status2;
    }

    status2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK == status2 )
    {
        status = status2;
    }

    return status;
}

QCStatus_e Qnn::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_qnn.Start();

    return status;
}

QCStatus_e Qnn::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_qnn.Stop();

    if ( QC_STATUS_OK == status )
    {
        if ( m_bDeRegisterAllBuffersWhenStop )
        {
            status = m_qnn.DeRegisterAllBuffers();
        }
    }

    return status;
}

QCStatus_e Qnn::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<QCSharedBuffer_t> inputs;
    std::vector<QCSharedBuffer_t> outputs;

    inputs.reserve( m_inputNum );
    for ( uint32_t i = 0; i < m_inputNum; i++ )
    {
        uint32_t globalBufferId = m_globalBufferIdMap[i].globalBufferId;
        QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( globalBufferId );
        const QCSharedBufferDescriptor_t *pSharedBuffer =
                dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
        if ( nullptr == pSharedBuffer )
        {
            status = QC_STATUS_INVALID_BUF;
            break;
        }
        else
        {
            inputs.push_back( pSharedBuffer->buffer );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        outputs.reserve( m_outputNum );
        for ( uint32_t i = m_inputNum; i < m_inputNum + m_outputNum; i++ )
        {
            uint32_t globalBufferId = m_globalBufferIdMap[i].globalBufferId;
            QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( globalBufferId );
            const QCSharedBufferDescriptor_t *pSharedBuffer =
                    dynamic_cast<const QCSharedBufferDescriptor_t *>( &bufDesc );
            if ( nullptr == pSharedBuffer )
            {
                status = QC_STATUS_INVALID_BUF;
                break;
            }
            else
            {
                outputs.push_back( pSharedBuffer->buffer );
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        if ( nullptr == m_callback )
        {
            status = m_qnn.Execute( inputs.data(), inputs.size(), outputs.data(), outputs.size() );
        }
        else
        {
            std::lock_guard<std::mutex> l( m_lock );
            QCReturn<QCFrameDescriptorNodeIfs> ret = m_pFrameDescPool->Get();
            if ( QC_STATUS_OK == ret.status )
            {
                QCFrameDescriptorNodeIfs &fd = ret.obj;
                fd = frameDesc;
                m_frameDescMap[m_ticketId] = &fd;
                status = m_qnn.Execute( inputs.data(), inputs.size(), outputs.data(),
                                        outputs.size(), (void *) m_ticketId );
                if ( QC_STATUS_OK == status )
                {
                    m_ticketId++;
                    if ( 0 == m_ticketId )
                    {
                        m_ticketId = 1;
                    }
                }
                else
                {
                    m_frameDescMap.erase( m_ticketId );
                    m_pFrameDescPool->Put( fd );
                }
            }
            else
            {
                QC_ERROR( "Frame Desc Pool is Full" );
                status = ret.status;
            }
        }
    }

    return status;
}

}   // namespace node
}   // namespace QC
