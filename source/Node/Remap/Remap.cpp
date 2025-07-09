// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/Remap.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

QCStatus_e RemapConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
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

    if ( dt.Exists( "bufferIds" ) )
    {
        std::vector<uint32_t> bufferIds = dt.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );
        if ( 0 == bufferIds.size() )
        {
            errors += "the bufferIds is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    std::vector<DataTree> inputDts;
    (void) dt.Get( "inputs", inputDts );
    m_numOfInputs = 0;
    for ( DataTree &idt : inputDts )
    {
        m_numOfInputs++;
    }

    if ( QC_MAX_INPUTS < m_numOfInputs )
    {
        errors += "inputs number invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
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

QCStatus_e RemapConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );

        m_config.params.numOfInputs = m_numOfInputs;
        m_config.params.processor = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
        m_config.params.outputWidth = dt.Get<uint32_t>( "outputWidth", 1024 );
        m_config.params.outputHeight = dt.Get<uint32_t>( "outputHeight", 1024 );
        m_config.params.outputFormat = dt.GetImageFormat( "outputFormat", QC_IMAGE_FORMAT_RGB888 );
        m_config.params.bEnableUndistortion = dt.Get<bool>( "bEnableUndistortion", false );
        m_config.params.bEnableNormalize = dt.Get<bool>( "bEnableNormalize", false );
        if ( true == m_config.params.bEnableNormalize )
        {
            m_config.params.normlzR.sub = dt.Get<float>( "RSub", 0.0f );
            m_config.params.normlzR.mul = dt.Get<float>( "RMul", 1.0f );
            m_config.params.normlzR.add = dt.Get<float>( "RAdd", 0.0f );
            m_config.params.normlzG.sub = dt.Get<float>( "GSub", 0.0f );
            m_config.params.normlzG.mul = dt.Get<float>( "GMul", 1.0f );
            m_config.params.normlzG.add = dt.Get<float>( "GAdd", 0.0f );
            m_config.params.normlzB.sub = dt.Get<float>( "BSub", 0.0f );
            m_config.params.normlzB.mul = dt.Get<float>( "BMul", 1.0f );
            m_config.params.normlzB.add = dt.Get<float>( "BAdd", 0.0f );
        }

        std::vector<DataTree> inputDts;
        (void) dt.Get( "inputs", inputDts );
        uint32_t inputId = 0;
        for ( DataTree &idt : inputDts )
        {
            m_config.params.inputConfigs[inputId].inputWidth =
                    idt.Get<uint32_t>( "inputWidth", 1024 );
            m_config.params.inputConfigs[inputId].inputHeight =
                    idt.Get<uint32_t>( "inputHeight", 1024 );
            m_config.params.inputConfigs[inputId].inputFormat =
                    idt.GetImageFormat( "inputFormat", QC_IMAGE_FORMAT_NV12 );
            m_config.params.inputConfigs[inputId].mapWidth = idt.Get<uint32_t>( "mapWidth", 1024 );
            m_config.params.inputConfigs[inputId].mapHeight =
                    idt.Get<uint32_t>( "mapHeight", 1024 );
            m_config.params.inputConfigs[inputId].ROI.x = idt.Get<uint32_t>( "roiX", 0 );
            m_config.params.inputConfigs[inputId].ROI.y = idt.Get<uint32_t>( "roiY", 0 );
            m_config.params.inputConfigs[inputId].ROI.width = idt.Get<uint32_t>( "roiWidth", 0 );
            m_config.params.inputConfigs[inputId].ROI.height = idt.Get<uint32_t>( "roiHeight", 0 );

            if ( true == m_config.params.bEnableUndistortion )
            {
                m_mapXBufferIds[inputId] = idt.Get<uint32_t>( "mapX", UINT32_MAX );
                m_mapYBufferIds[inputId] = idt.Get<uint32_t>( "mapY", UINT32_MAX );
            }

            inputId++;
        }

        m_config.bufferIds = dt.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );

        std::vector<DataTree> globalBufferIdMap;
        (void) dt.Get( "globalBufferIdMap", globalBufferIdMap );
        m_config.globalBufferIdMap.resize( globalBufferIdMap.size() );
        uint32_t idx = 0;
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

QCStatus_e RemapConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
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
            QC_ERROR( "Remap only support static config" );
        }
    }

    return status;
}

const std::string &RemapConfigIfs::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;
    // empty function for Remap node
    return m_options;
}

QCStatus_e Remap::SetupGlobalBufferIdMap( const RemapConfig_t &cfg )
{
    QCStatus_e status = QC_STATUS_OK;

    m_inputNum = cfg.params.numOfInputs;
    m_outputNum = 1;
    if ( cfg.globalBufferIdMap.size() > 0 )
    {
        if ( ( m_inputNum + m_outputNum ) != cfg.globalBufferIdMap.size() )
        {
            QC_ERROR( "global buffer map size is not correct: expect %" PRIu32,
                      m_inputNum + m_outputNum + 1 );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            m_globalBufferIdMap = cfg.globalBufferIdMap;
        }
    }
    else
    { /* create a default global buffer index map */
        m_globalBufferIdMap.resize( m_inputNum + m_outputNum );
        uint32_t globalBufferId = 0;
        for ( uint32_t i = 0; i < m_inputNum; i++ )
        {
            m_globalBufferIdMap[globalBufferId].name = "Input" + std::to_string( i );
            m_globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
            globalBufferId++;
        }
        for ( uint32_t i = 0; i < m_outputNum; i++ )
        {
            m_globalBufferIdMap[globalBufferId].name = "Output";
            m_globalBufferIdMap[globalBufferId].globalBufferId = globalBufferId;
            globalBufferId++;
        }
    }
    return status;
}

QCStatus_e Remap::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const RemapConfig_t *pConfig = dynamic_cast<const RemapConfig_t *>( &cfg );
    Remap_Config_t params;
    bool bNodeBaseInitDone = false;
    bool bRemapInitDone = false;

    status = m_configIfs.VerifyAndSet( config.config, errors );

    for ( uint32_t inputId = 0; inputId < m_configIfs.m_config.params.numOfInputs; inputId++ )
    {
        if ( true == m_configIfs.m_config.params.bEnableUndistortion )
        {
            uint32_t mapXBufferId = m_configIfs.m_mapXBufferIds[inputId];
            QCBufferDescriptorBase_t &mapXBufferDesc = config.buffers[mapXBufferId];
            QCSharedBufferDescriptor_t *pMapXBufferDesc =
                    dynamic_cast<QCSharedBufferDescriptor_t *>( &mapXBufferDesc );
            m_configIfs.m_config.params.inputConfigs[inputId].remapTable.pMapX =
                    &( pMapXBufferDesc->buffer );

            uint32_t mapYBufferId = m_configIfs.m_mapYBufferIds[inputId];
            QCBufferDescriptorBase_t &mapYBufferDesc = config.buffers[mapYBufferId];
            QCSharedBufferDescriptor_t *pMapYBufferDesc =
                    dynamic_cast<QCSharedBufferDescriptor_t *>( &mapYBufferDesc );
            m_configIfs.m_config.params.inputConfigs[inputId].remapTable.pMapY =
                    &( pMapYBufferDesc->buffer );
        }
    }

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
        status = m_remap.Init( m_nodeId.name.c_str(), &params );
    }

    if ( QC_STATUS_OK == status )
    {
        status = SetupGlobalBufferIdMap( *pConfig );
    }

    if ( QC_STATUS_OK == status )
    {
        bRemapInitDone = true;
    }


    if ( QC_STATUS_OK == status )
    {   // do buffer register during initialization
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
                    status = m_remap.RegisterBuffers( &( pSharedBuffer->buffer ), 1,
                                                      FADAS_BUF_TYPE_INOUT );
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
        if ( bRemapInitDone )
        {
            (void) m_remap.Deinit();
        }
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e Remap::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = m_remap.Deinit();
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

QCStatus_e Remap::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_remap.Start();

    return status;
}

QCStatus_e Remap::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_remap.Stop();

    return status;
}

QCStatus_e Remap::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
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
        status = m_remap.Execute( inputs.data(), inputs.size(), outputs.data() );
    }

    return status;
}

}   // namespace Node
}   // namespace QC
