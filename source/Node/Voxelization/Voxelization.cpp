// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/Node/Voxelization.hpp"

namespace QC
{
namespace Node
{

QCStatus_e VoxelizationConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2 = QC_STATUS_OK;

    std::string name = dt.Get<std::string>( "name", "" );
    if ( "" == name )
    {
        errors += "the name is empty, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t id = dt.Get<uint32_t>( "id", UINT32_MAX );
        if ( UINT32_MAX == id )
        {
            errors += "the id is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        QCProcessorType_e processorType = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
        if ( QC_PROCESSOR_MAX == processorType )
        {
            errors += "the processorType is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t maxPointNum = dt.Get<uint32_t>( "maxPointNum", 0 );
        if ( 0 == maxPointNum )
        {
            errors += "the maxPointNum is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t maxPlrNum = dt.Get<uint32_t>( "maxPlrNum", 0 );
        if ( 0 == maxPlrNum )
        {
            errors += "the maxPlrNum is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t maxPointNumPerPlr = dt.Get<uint32_t>( "maxPointNumPerPlr", 0 );
        if ( 0 == maxPointNumPerPlr )
        {
            errors += "the maxPointNumPerPlr is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        std::string inputMode = dt.Get<std::string>( "inputMode", "" );
        if ( "" == inputMode )
        {
            errors += "the inputMode is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( ( inputMode != "xyzr" ) && ( inputMode != "xyzrt" ) )
        {
            errors += "the inputMode is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t outputFeatureDimNum = dt.Get<uint32_t>( "outputFeatureDimNum", 0 );
        if ( 0 == outputFeatureDimNum )
        {
            errors += "the outputFeatureDimNum is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( dt.Exists( "inputBufferIds" ) )
    {
        std::vector<uint32_t> inputBufferIds =
                dt.Get<uint32_t>( "inputBufferIds", std::vector<uint32_t>{} );
        if ( 0 == inputBufferIds.size() )
        {
            errors += "the inputBufferIds is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( dt.Exists( "outputPlrBufferIds" ) )
    {
        std::vector<uint32_t> outputPlrBufferIds =
                dt.Get<uint32_t>( "outputPlrBufferIds", std::vector<uint32_t>{} );
        if ( 0 == outputPlrBufferIds.size() )
        {
            errors += "the outputPlrBufferIds is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( dt.Exists( "outputFeatureBufferIds" ) )
    {
        std::vector<uint32_t> outputFeatureBufferIds =
                dt.Get<uint32_t>( "outputFeatureBufferIds", std::vector<uint32_t>{} );
        if ( 0 == outputFeatureBufferIds.size() )
        {
            errors += "the outputFeatureBufferIds is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
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

QCStatus_e VoxelizationConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );
        m_config.params.processor = dt.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
        m_config.params.pillarXSize = dt.Get<float>( "Xsize", 0.0f );
        m_config.params.pillarYSize = dt.Get<float>( "Ysize", 0.0f );
        m_config.params.pillarZSize = dt.Get<float>( "Zsize", 0.0f );
        m_config.params.minXRange = dt.Get<float>( "Xmin", 0.0f );
        m_config.params.minYRange = dt.Get<float>( "Ymin", 0.0f );
        m_config.params.minZRange = dt.Get<float>( "Zmin", 0.0f );
        m_config.params.maxXRange = dt.Get<float>( "Xmax", 0.0f );
        m_config.params.maxYRange = dt.Get<float>( "Ymax", 0.0f );
        m_config.params.maxZRange = dt.Get<float>( "Zmax", 0.0f );
        m_config.params.maxNumInPts = dt.Get<uint32_t>( "maxPointNum", UINT32_MAX );
        m_config.params.maxNumPlrs = dt.Get<uint32_t>( "maxPlrNum", UINT32_MAX );
        m_config.params.maxNumPtsPerPlr = dt.Get<uint32_t>( "maxPointNumPerPlr", UINT32_MAX );
        m_config.params.numOutFeatureDim = dt.Get<uint32_t>( "outputFeatureDimNum", UINT32_MAX );

        std::string inputMode = dt.Get<std::string>( "inputMode", "" );
        m_config.params.inputMode = GetInputMode( inputMode );
        if ( VOXELIZATION_INPUT_XYZR == m_config.params.inputMode )
        {
            m_config.params.numInFeatureDim = 4;
        }
        else
        {
            m_config.params.numInFeatureDim = 5;
        }

        m_config.inputBufferIds = dt.Get<uint32_t>( "inputBufferIds", std::vector<uint32_t>{} );
        m_config.outputPlrBufferIds =
                dt.Get<uint32_t>( "outputPlrBufferIds", std::vector<uint32_t>{} );
        m_config.outputFeatureBufferIds =
                dt.Get<uint32_t>( "outputFeatureBufferIds", std::vector<uint32_t>{} );

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

Voxelization_InputMode_e VoxelizationConfigIfs::GetInputMode( std::string &mode )
{
    Voxelization_InputMode_e inputMode;
    if ( mode == "xyzr" )
    {
        inputMode = VOXELIZATION_INPUT_XYZR;
    }
    else if ( mode == "xyzrt" )
    {
        inputMode = VOXELIZATION_INPUT_XYZRT;
    }

    return inputMode;
}

QCStatus_e VoxelizationConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    DataTree dt;

    status = NodeConfigIfs::VerifyAndSet( config, errors );
    if ( QC_STATUS_OK == status )
    {
        status = m_dataTree.Get( "static", dt );
    }

    if ( QC_STATUS_OK == status )
    {
        status = ParseStaticConfig( dt, errors );
    }

    return status;
}

const std::string &VoxelizationConfigIfs::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;
    return m_options;
}

const QCNodeConfigBase_t &VoxelizationConfigIfs::Get()
{
    QCStatus_e status = QC_STATUS_OK;
    return m_config;
}

QCStatus_e Voxelization::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    bool bNodeBaseInitDone = false;
    bool bVoxelInitDone = false;

    std::string errors;
    Voxelization_Config_t params;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const VoxelizationConfig_t *pConfig = dynamic_cast<const VoxelizationConfig_t *>( &cfg );

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
        status = (QCStatus_e) m_voxel.Init( m_nodeId.name.c_str(), &params );
    }

    if ( QC_STATUS_OK == status )
    {
        bVoxelInitDone = true;
    }
    else
    {
        QC_ERROR( "Failed to initialize Voxelization component" );
    }

    if ( QC_STATUS_OK == status )
    {
        // Register input buffers
        for ( uint32_t bufferId : pConfig->inputBufferIds )
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
                    status = m_voxel.RegisterBuffers( &( pSharedBuffer->buffer ), 1,
                                                      FADAS_BUF_TYPE_IN );
                }
            }
            else
            {
                QC_ERROR( "input buffer index out of range" );
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
        // Register output pillar buffers
        for ( uint32_t bufferId : pConfig->outputPlrBufferIds )
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
                    status = m_voxel.RegisterBuffers( &( pSharedBuffer->buffer ), 1,
                                                      FADAS_BUF_TYPE_OUT );
                }
            }
            else
            {
                QC_ERROR( "output pillar buffer index out of range" );
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
        // Register output feature buffers
        for ( uint32_t bufferId : pConfig->outputFeatureBufferIds )
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
                    status = m_voxel.RegisterBuffers( &( pSharedBuffer->buffer ), 1,
                                                      FADAS_BUF_TYPE_OUT );
                }
            }
            else
            {
                QC_ERROR( "output feature buffer index out of range" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( status != QC_STATUS_OK )
            {
                break;
            }
        }
    }

    if ( QC_STATUS_OK != status )
    {
        if ( bVoxelInitDone )
        {
            (void) m_voxel.Deinit();
        }
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e Voxelization::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    status = (QCStatus_e) m_voxel.Start();
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to start Voxelization component" );
    }

    return status;
}

QCStatus_e Voxelization::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    QCBufferDescriptorBase_t &inputBufDesc = frameDesc.GetBuffer( 0 );
    const QCSharedBufferDescriptor_t *pInputBuffer =
            dynamic_cast<const QCSharedBufferDescriptor_t *>( &inputBufDesc );
    if ( nullptr == pInputBuffer )
    {
        status = QC_STATUS_INVALID_BUF;
    }

    QCBufferDescriptorBase_t &outputPlrBufDesc = frameDesc.GetBuffer( 1 );
    const QCSharedBufferDescriptor_t *pOutputPillarBuffer =
            dynamic_cast<const QCSharedBufferDescriptor_t *>( &outputPlrBufDesc );
    if ( nullptr == pOutputPillarBuffer )
    {
        status = QC_STATUS_INVALID_BUF;
    }

    QCBufferDescriptorBase_t &outputFeatureBufDesc = frameDesc.GetBuffer( 2 );
    const QCSharedBufferDescriptor_t *pOutputFeatureBuffer =
            dynamic_cast<const QCSharedBufferDescriptor_t *>( &outputFeatureBufDesc );
    if ( nullptr == pOutputFeatureBuffer )
    {
        status = QC_STATUS_INVALID_BUF;
    }

    if ( QC_STATUS_OK == status )
    {
        status = m_voxel.Execute( &pInputBuffer->buffer, &pOutputPillarBuffer->buffer,
                                  &pOutputFeatureBuffer->buffer );
    }

    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to process voxelization frame" );
    }

    return status;
}

QCStatus_e Voxelization::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = (QCStatus_e) m_voxel.Stop();
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to stop voxelization component" );
    }

    return status;
}

QCStatus_e Voxelization::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;

    status = (QCStatus_e) m_voxel.Deinit();
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to deinitialize voxelization component" );
    }
    else
    {
        status = NodeBase::DeInitialize();
    }

    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to deinitialize NodeBase" );
    }

    return status;
}

}   // namespace Node
}   // namespace QC

