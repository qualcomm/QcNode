// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/Voxelization.hpp"
#include "VoxelizationImpl.hpp"

namespace QC
{
namespace Node
{
QCStatus_e VoxelizationConfig::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2 = QC_STATUS_OK;

    std::string name = dt.Get<std::string>( "name", "" );
    if ( "" == name )
    {
        errors += "the name is empty, ";
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        uint32_t id = dt.Get<uint32_t>( "id", UINT32_MAX );
        if ( UINT32_MAX == id )
        {
            errors += "the id is empty, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        uint32_t maxPointNum = dt.Get<uint32_t>( "maxPointNum", 0 );
        if ( 0 == maxPointNum )
        {
            errors += "the maxPointNum is empty, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        uint32_t maxPlrNum = dt.Get<uint32_t>( "maxPlrNum", 0 );
        if ( 0 == maxPlrNum )
        {
            errors += "the maxPlrNum is empty, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        uint32_t maxPointNumPerPlr = dt.Get<uint32_t>( "maxPointNumPerPlr", 0 );
        if ( 0 == maxPointNumPerPlr )
        {
            errors += "the maxPointNumPerPlr is empty, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        std::string inputMode = dt.Get<std::string>( "inputMode", "" );
        if ( "" == inputMode )
        {
            errors += "the inputMode is empty, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( ( inputMode != "xyzr" ) && ( inputMode != "xyzrt" ) )
        {
            errors += "the inputMode is invalid, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        uint32_t outputFeatureDimNum = dt.Get<uint32_t>( "outputFeatureDimNum", 0 );
        if ( 0 == outputFeatureDimNum )
        {
            errors += "the outputFeatureDimNum is empty, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( dt.Exists( "outputPlrBufferIds" ) )
    {
        std::vector<uint32_t> outputPlrBufferIds =
                dt.Get<uint32_t>( "outputPlrBufferIds", std::vector<uint32_t>{} );
        if ( 0 == outputPlrBufferIds.size() )
        {
            errors += "the outputPlrBufferIds is invalid, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }
    else
    {
        errors += "the outputPlrBufferIds is not set, ";
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Exists( "outputFeatureBufferIds" ) )
    {
        std::vector<uint32_t> outputFeatureBufferIds =
                dt.Get<uint32_t>( "outputFeatureBufferIds", std::vector<uint32_t>{} );
        if ( 0 == outputFeatureBufferIds.size() )
        {
            errors += "the outputFeatureBufferIds is invalid, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }
    else
    {
        errors += "the outputFeatureBufferIds is not set, ";
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        QCProcessorType_e processorType = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
        if ( QC_PROCESSOR_MAX == processorType )
        {
            errors += "the processorType is invalid, ";
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( QC_PROCESSOR_GPU == processorType )
        {
            if ( false == dt.Exists( "plrPointsBufferId" ) )
            {
                errors += "the plrPointsBufferId is not set, ";
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( false == dt.Exists( "coordToPlrIdxBufferId" ) )
            {
                errors += "the coordToPlrIdxBufferId is not set, ";
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }

    std::vector<DataTree> globalBufferIdMap;
    ret2 = dt.Get( "globalBufferIdMap", globalBufferIdMap );
    if ( QC_STATUS_OUT_OF_BOUND == ret2 )
    {
        /* OK if not configured */
    }
    else if ( QC_STATUS_OK != ret2 )
    {
        errors += "the globalBufferIdMap is invalid, ";
        ret = ret2;
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
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( UINT32_MAX == index )
            {
                errors += "the globalIdMap " + std::to_string( idx ) + " id is empty, ";
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            idx++;
        }
    }

    return ret;
}

QCStatus_e VoxelizationConfig::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e ret = QC_STATUS_OK;

    VoxelizationImplConfig_t &config = m_pVoxelImpl->GetConifg();

    ret = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == ret )
    {
        config.nodeId.name = dt.Get<std::string>( "name", "" );
        config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );
        config.voxelConfig.processor = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
        config.voxelConfig.pillarXSize = dt.Get<float>( "Xsize", 0.0f );
        config.voxelConfig.pillarYSize = dt.Get<float>( "Ysize", 0.0f );
        config.voxelConfig.pillarZSize = dt.Get<float>( "Zsize", 0.0f );
        config.voxelConfig.minXRange = dt.Get<float>( "Xmin", 0.0f );
        config.voxelConfig.minYRange = dt.Get<float>( "Ymin", 0.0f );
        config.voxelConfig.minZRange = dt.Get<float>( "Zmin", 0.0f );
        config.voxelConfig.maxXRange = dt.Get<float>( "Xmax", 0.0f );
        config.voxelConfig.maxYRange = dt.Get<float>( "Ymax", 0.0f );
        config.voxelConfig.maxZRange = dt.Get<float>( "Zmax", 0.0f );
        config.voxelConfig.maxNumInPts = dt.Get<uint32_t>( "maxPointNum", UINT32_MAX );
        config.voxelConfig.maxNumPlrs = dt.Get<uint32_t>( "maxPlrNum", UINT32_MAX );
        config.voxelConfig.maxNumPtsPerPlr = dt.Get<uint32_t>( "maxPointNumPerPlr", UINT32_MAX );
        config.voxelConfig.numOutFeatureDim = dt.Get<uint32_t>( "outputFeatureDimNum", UINT32_MAX );

        config.voxelConfig.inputMode = dt.Get<std::string>( "inputMode", "" );
        if ( "xyzr" == config.voxelConfig.inputMode )
        {
            config.voxelConfig.numInFeatureDim = 4;
        }
        else if ( "xyzrt" == config.voxelConfig.inputMode )
        {
            config.voxelConfig.numInFeatureDim = 5;
        }
        else
        {
            QC_ERROR( "Error input mode" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        config.outputPlrBufferIds =
                dt.Get<uint32_t>( "outputPlrBufferIds", std::vector<uint32_t>{} );
        config.outputFeatureBufferIds =
                dt.Get<uint32_t>( "outputFeatureBufferIds", std::vector<uint32_t>{} );
        config.plrPointsBufferId = dt.Get<uint32_t>( "plrPointsBufferId", UINT32_MAX );
        config.coordToPlrIdxBufferId = dt.Get<uint32_t>( "coordToPlrIdxBufferId", UINT32_MAX );

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

    return ret;
}

QCStatus_e VoxelizationConfig::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e ret = QC_STATUS_OK;
    DataTree dt;

    ret = NodeConfigIfs::VerifyAndSet( config, errors );
    if ( QC_STATUS_OK == ret )
    {
        ret = m_dataTree.Get( "static", dt );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = ParseStaticConfig( dt, errors );
    }

    return ret;
}

const std::string &VoxelizationConfig::GetOptions()
{
    QCStatus_e ret = QC_STATUS_OK;
    return m_options;
}

const QCNodeConfigBase_t &VoxelizationConfig::Get()
{
    return m_pVoxelImpl->GetConifg();
}

}   // namespace Node
}   // namespace QC

