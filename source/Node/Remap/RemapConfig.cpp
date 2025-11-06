// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/Remap.hpp"
#include "RemapImpl.hpp"
#include <unistd.h>


namespace QC
{
namespace Node
{

QCStatus_e RemapConfig::VerifyStaticConfig( DataTree &dt, std::string &errors )
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

QCStatus_e RemapConfig::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    RemapImplConfig_t &config = m_pRemapImpl->GetConifg();
    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        config.nodeId.name = dt.Get<std::string>( "name", "" );
        config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );

        config.params.numOfInputs = m_numOfInputs;
        config.params.processor = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
        config.params.outputWidth = dt.Get<uint32_t>( "outputWidth", 1024 );
        config.params.outputHeight = dt.Get<uint32_t>( "outputHeight", 1024 );
        config.params.outputFormat = dt.GetImageFormat( "outputFormat", QC_IMAGE_FORMAT_RGB888 );
        config.params.bEnableUndistortion = dt.Get<bool>( "bEnableUndistortion", false );
        config.params.bEnableNormalize = dt.Get<bool>( "bEnableNormalize", false );
        if ( true == config.params.bEnableNormalize )
        {
            config.params.normlzR.sub = dt.Get<float>( "RSub", 0.0f );
            config.params.normlzR.mul = dt.Get<float>( "RMul", 1.0f );
            config.params.normlzR.add = dt.Get<float>( "RAdd", 0.0f );
            config.params.normlzG.sub = dt.Get<float>( "GSub", 0.0f );
            config.params.normlzG.mul = dt.Get<float>( "GMul", 1.0f );
            config.params.normlzG.add = dt.Get<float>( "GAdd", 0.0f );
            config.params.normlzB.sub = dt.Get<float>( "BSub", 0.0f );
            config.params.normlzB.mul = dt.Get<float>( "BMul", 1.0f );
            config.params.normlzB.add = dt.Get<float>( "BAdd", 0.0f );
        }

        std::vector<DataTree> inputDts;
        (void) dt.Get( "inputs", inputDts );
        uint32_t inputId = 0;
        for ( DataTree &idt : inputDts )
        {
            config.params.inputConfigs[inputId].inputWidth =
                    idt.Get<uint32_t>( "inputWidth", 1024 );
            config.params.inputConfigs[inputId].inputHeight =
                    idt.Get<uint32_t>( "inputHeight", 1024 );
            config.params.inputConfigs[inputId].inputFormat =
                    idt.GetImageFormat( "inputFormat", QC_IMAGE_FORMAT_NV12 );
            config.params.inputConfigs[inputId].mapWidth = idt.Get<uint32_t>( "mapWidth", 1024 );
            config.params.inputConfigs[inputId].mapHeight = idt.Get<uint32_t>( "mapHeight", 1024 );
            config.params.inputConfigs[inputId].ROI.x = idt.Get<uint32_t>( "roiX", 0 );
            config.params.inputConfigs[inputId].ROI.y = idt.Get<uint32_t>( "roiY", 0 );
            config.params.inputConfigs[inputId].ROI.width = idt.Get<uint32_t>( "roiWidth", 0 );
            config.params.inputConfigs[inputId].ROI.height = idt.Get<uint32_t>( "roiHeight", 0 );

            if ( true == config.params.bEnableUndistortion )
            {
                config.params.inputConfigs[inputId].remapTable.mapXBufferId =
                        idt.Get<uint32_t>( "mapXBufferId", UINT32_MAX );
                config.params.inputConfigs[inputId].remapTable.mapYBufferId =
                        idt.Get<uint32_t>( "mapYBufferId", UINT32_MAX );
            }

            inputId++;
        }

        config.bufferIds = dt.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );

        std::vector<DataTree> globalBufferIdMap;
        (void) dt.Get( "globalBufferIdMap", globalBufferIdMap );
        config.globalBufferIdMap.resize( globalBufferIdMap.size() );
        uint32_t idx = 0;
        for ( DataTree &gbm : globalBufferIdMap )
        {
            config.globalBufferIdMap[idx].name = gbm.Get<std::string>( "name", "" );
            config.globalBufferIdMap[idx].globalBufferId = gbm.Get<uint32_t>( "id", UINT32_MAX );
            idx++;
        }

        config.bDeRegisterAllBuffersWhenStop =
                dt.Get<bool>( "deRegisterAllBuffersWhenStop", false );
    }
    else
    {
        QC_ERROR( "VerifyStaticConfig failed!" );
    }

    return status;
}

QCStatus_e RemapConfig::VerifyAndSet( const std::string config, std::string &errors )
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

const std::string &RemapConfig::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;

    DataTree dt;
    dt.Set<uint32_t>( "version", QCNODE_REMAP_VERSION );
    m_options = dt.Dump();

    return m_options;
}

const QCNodeConfigBase_t &RemapConfig::Get()
{
    return m_pRemapImpl->GetConifg();
}

}   // namespace Node
}   // namespace QC