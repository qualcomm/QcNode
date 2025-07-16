// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/QNN.hpp"
#include "QnnImpl.hpp"
#include "QnnTypeMacros.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

DataTree QnnConfig::ConvertTensorInfoToJson( const Qnn_Tensor_t &info )
{
    DataTree dt;
    uint32_t rank = QNN_TENSOR_GET_RANK( &info );
    uint32_t *dimensions = QNN_TENSOR_GET_DIMENSIONS( &info );
    std::vector<uint32_t> dims( dimensions, dimensions + rank );

    dt.Set<std::string>( "name", std::string( QNN_TENSOR_GET_NAME( &info ) ) );
    dt.SetTensorType( "type",
                      m_pQnnImpl->SwitchFromQnnDataType( QNN_TENSOR_GET_DATA_TYPE( &info ) ) );
    dt.Set<std::uint32_t>( "dims", dims );

    Qnn_QuantizeParams_t quantizeParams = QNN_TENSOR_GET_QUANT_PARAMS( &info );
    if ( QNN_QUANTIZATION_ENCODING_SCALE_OFFSET == quantizeParams.quantizationEncoding )
    {
        dt.Set<std::string>( "quantType", "scale_offset" );
        dt.Set<float>( "quantScale", quantizeParams.scaleOffsetEncoding.scale );
        dt.Set<int32_t>( "quantOffset", quantizeParams.scaleOffsetEncoding.offset );
    }
    else
    {
        dt.Set<std::string>( "quantType", "none" );
        dt.Set<float>( "quantScale", 1.0 );
        dt.Set<int32_t>( "quantOffset", 0 );
    }

    return dt;
}

std::vector<DataTree>
QnnConfig::ConvertTensorInfoListToJson( const std::vector<Qnn_Tensor_t> &infoList )
{
    std::vector<DataTree> dts;

    for ( auto &info : infoList )
    {
        DataTree dt = ConvertTensorInfoToJson( info );
        dts.push_back( dt );
    }

    return dts;
}

QCStatus_e QnnConfig::VerifyStaticConfig( DataTree &dt, std::string &errors )
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

    std::string processorType = dt.Get<std::string>( "processorType", "htp0" );
    if ( ( "htp0" != processorType ) && ( "htp1" != processorType ) &&
         ( "htp2" != processorType ) && ( "htp3" != processorType ) && ( "cpu" != processorType ) &&
         ( "gpu" != processorType ) )
    {
        errors += "the processorType " + processorType + " is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string loadType = dt.Get<std::string>( "loadType", "binary" );
    if ( ( "binary" == loadType ) || ( "library" == loadType ) )
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


QCStatus_e QnnConfig::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    QnnImplConfig_t &config = m_pQnnImpl->GetConifg();

    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        config.nodeId.name = dt.Get<std::string>( "name", "" );
        config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );
        std::string processorType = dt.Get<std::string>( "processorType", "htp0" );
        if ( "htp0" == processorType )
        {
            config.processorType = QNN_PROCESSOR_HTP0;
        }
        else if ( "htp1" == processorType )
        {
            config.processorType = QNN_PROCESSOR_HTP1;
        }
        else if ( "htp2" == processorType )
        {
            config.processorType = QNN_PROCESSOR_HTP2;
        }
        else if ( "htp3" == processorType )
        {
            config.processorType = QNN_PROCESSOR_HTP3;
        }
        else if ( "cpu" == processorType )
        {
            config.processorType = QNN_PROCESSOR_CPU;
        }
        else
        {
            config.processorType = QNN_PROCESSOR_GPU;
        }
        config.coreIds = dt.Get<std::vector<uint32_t>>( "coreIds", std::vector<uint32_t>( { 0 } ) );
        std::string loadType = dt.Get<std::string>( "loadType", "binary" );
        if ( "binary" == loadType )
        {
            config.modelPath = dt.Get<std::string>( "modelPath", "" );
            config.loadType = QNN_LOAD_CONTEXT_BIN_FROM_FILE;
            QC_DEBUG( "QNN binary model: %s", config.modelPath.c_str() );
        }
        else if ( "library" == loadType )
        {
            config.modelPath = dt.Get<std::string>( "modelPath", "" );
            config.loadType = QNN_LOAD_SHARED_LIBRARY_FROM_FILE;
            QC_DEBUG( "QNN library model: %s", config.modelPath.c_str() );
        }
        else /* buffer */
        {
            config.contextBufferId = dt.Get<uint32_t>( "contextBufferId", UINT32_MAX );
            config.modelPath = "";
            config.loadType = QNN_LOAD_CONTEXT_BIN_FROM_BUFFER;
        }

        std::string priority = dt.Get<std::string>( "priority", "normal" );
        if ( "low" == priority )
        {
            config.priority = QNN_PRIORITY_LOW;
        }
        else if ( "normal" == priority )
        {
            config.priority = QNN_PRIORITY_NORMAL;
        }
        else if ( "normal_high" == priority )
        {
            config.priority = QNN_PRIORITY_NORMAL_HIGH;
        }
        else
        {
            config.priority = QNN_PRIORITY_HIGH;
        }

        config.bufferIds = dt.Get<uint32_t>( "bufferIds", std::vector<uint32_t>{} );

        std::vector<DataTree> dts;
        (void) dt.Get( "udoPackages", dts );
        config.udoPackages.resize( dts.size() );
        uint32_t idx = 0;
        for ( DataTree &udt : dts )
        {
            config.udoPackages[idx].udoLibPath = udt.Get<std::string>( "udoLibPath", "" );
            config.udoPackages[idx].interfaceProvider =
                    udt.Get<std::string>( "interfaceProvider", "" );
            idx++;
        }

        std::vector<DataTree> globalBufferIdMap;
        (void) dt.Get( "globalBufferIdMap", globalBufferIdMap );
        config.globalBufferIdMap.resize( globalBufferIdMap.size() );
        idx = 0;
        for ( DataTree &gbm : globalBufferIdMap )
        {
            config.globalBufferIdMap[idx].name = gbm.Get<std::string>( "name", "" );
            config.globalBufferIdMap[idx].globalBufferId = gbm.Get<uint32_t>( "id", UINT32_MAX );
            idx++;
        }

        config.bDeRegisterAllBuffersWhenStop =
                dt.Get<bool>( "deRegisterAllBuffersWhenStop", false );
    }

    return status;
}

QCStatus_e QnnConfig::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( dt.Exists( "enablePerf" ) )
    {
        bool bEnablePerf = dt.Get<bool>( "enablePerf", false );
        if ( bEnablePerf )
        {
            status = m_pQnnImpl->EnablePerf();
        }
        else
        {
            status = m_pQnnImpl->DisablePerf();
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

QCStatus_e QnnConfig::VerifyAndSet( const std::string config, std::string &errors )
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

const std::string &QnnConfig::GetOptions()
{

    QCStatus_e status = QC_STATUS_OK;

    std::vector<Qnn_Tensor_t> inputTensors;
    std::vector<Qnn_Tensor_t> outputTensors;
    DataTree dt;

    if ( false == m_bOptionsBuilt )
    {
        status = m_pQnnImpl->GetInputTensors( inputTensors );
        if ( QC_STATUS_OK == status )
        {
            status = m_pQnnImpl->GetOutputTensors( outputTensors );
        }

        if ( QC_STATUS_OK == status )
        {
            std::vector<DataTree> inputDts = ConvertTensorInfoListToJson( inputTensors );
            std::vector<DataTree> outputDts = ConvertTensorInfoListToJson( outputTensors );
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

const QCNodeConfigBase_t &QnnConfig::Get()
{
    return m_pQnnImpl->GetConifg();
}

}   // namespace Node
}   // namespace QC
