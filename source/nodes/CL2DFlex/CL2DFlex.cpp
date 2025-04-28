// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/node/CL2DFlex.hpp"
#include <unistd.h>

namespace QC
{
namespace node
{

QCStatus_e CL2DFlexConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
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

        std::string mode = idt.Get<std::string>( "workMode", "resize_nearest" );
        if ( ( "convert" != mode ) && ( "resize_nearest" != mode ) &&
             ( "letterbox_nearest" != mode ) && ( "letterbox_nearest_multiple" != mode ) &&
             ( "resize_nearest_multiple" != mode ) && ( "convert_ubwc" != mode ) &&
             ( "remap_nearest" != mode ) )
        {
            errors += "the mode is invalid, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_MAX_INPUTS < m_numOfInputs )
    {
        errors += "inputs number invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::vector<DataTree> roiDts;
    (void) dt.Get( "ROIs", roiDts );
    for ( DataTree &idt : roiDts )
    {
        m_numOfROIs++;
    }

    if ( QC_CL2DFLEX_ROI_NUMBER_MAX < m_numOfROIs )
    {
        errors += "ROIs number invalid, ";
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

QCStatus_e CL2DFlexConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );

        m_config.params.numOfInputs = m_numOfInputs;
        m_config.params.outputWidth = dt.Get<uint32_t>( "outputWidth", 1024 );
        m_config.params.outputHeight = dt.Get<uint32_t>( "outputHeight", 1024 );
        m_config.params.outputFormat = dt.GetImageFormat( "outputFormat", QC_IMAGE_FORMAT_RGB888 );

        std::vector<DataTree> inputDts;
        (void) dt.Get( "inputs", inputDts );
        uint32_t inputId = 0;
        for ( DataTree &idt : inputDts )
        {
            m_config.params.inputWidths[inputId] = idt.Get<uint32_t>( "inputWidth", 1024 );
            m_config.params.inputHeights[inputId] = idt.Get<uint32_t>( "inputHeight", 1024 );
            m_config.params.inputFormats[inputId] =
                    idt.GetImageFormat( "inputFormat", QC_IMAGE_FORMAT_NV12 );
            m_config.params.ROIs[inputId].x = idt.Get<uint32_t>( "roiX", 0 );
            m_config.params.ROIs[inputId].y = idt.Get<uint32_t>( "roiY", 0 );
            m_config.params.ROIs[inputId].width = idt.Get<uint32_t>( "roiWidth", 0 );
            m_config.params.ROIs[inputId].height = idt.Get<uint32_t>( "roiHeight", 0 );
            std::string mode = idt.Get<std::string>( "workMode", "resize_nearest" );
            if ( "convert" == mode )
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_CONVERT;
            }
            else if ( "resize_nearest" == mode )
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
            }
            else if ( "letterbox_nearest" == mode )
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST;
            }
            else if ( "letterbox_nearest_multiple" == mode )
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE;
            }
            else if ( "resize_nearest_multiple" == mode )
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE;
            }
            else if ( "convert_ubwc" == mode )
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_CONVERT_UBWC;
            }
            else if ( "remap_nearest" == mode )
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_REMAP_NEAREST;
                m_mapXBufferIds[inputId] = idt.Get<uint32_t>( "mapX", UINT32_MAX );
                m_mapYBufferIds[inputId] = idt.Get<uint32_t>( "mapY", UINT32_MAX );
            }
            else
            {
                m_config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
            }

            inputId++;
        }

        std::vector<DataTree> roiDts;
        uint32_t roiId = 0;
        (void) dt.Get( "ROIs", roiDts );
        for ( DataTree &idt : roiDts )
        {
            m_ROIs[roiId].x = idt.Get<uint32_t>( "roiX", 0 );
            m_ROIs[roiId].y = idt.Get<uint32_t>( "roiY", 0 );
            m_ROIs[roiId].width = idt.Get<uint32_t>( "roiWidth", 0 );
            m_ROIs[roiId].height = idt.Get<uint32_t>( "roiHeight", 0 );
            roiId++;
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

QCStatus_e CL2DFlexConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
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
            QC_ERROR( "CL2D only support static config" );
        }
    }

    return status;
}

const std::string &CL2DFlexConfigIfs::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;
    // empty function for CL2D node
    return m_options;
}

QCStatus_e CL2DFlex::SetupGlobalBufferIdMap( const CL2DFlexConfig_t &cfg )
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

QCStatus_e CL2DFlex::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const CL2DFlexConfig_t *pConfig = dynamic_cast<const CL2DFlexConfig_t *>( &cfg );
    CL2DFlex_Config_t params;
    bool bNodeBaseInitDone = false;
    bool bCL2DFlexInitDone = false;

    status = m_configIfs.VerifyAndSet( config.config, errors );

    for ( uint32_t inputId = 0; inputId < m_configIfs.m_config.params.numOfInputs; inputId++ )
    {
        if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == m_configIfs.m_config.params.workModes[inputId] )
        {
            uint32_t mapXBufferId = m_configIfs.m_mapXBufferIds[inputId];
            QCBufferDescriptorBase_t &mapXBufferDesc = config.buffers[mapXBufferId];
            QCSharedBufferDescriptor_t *pMapXBufferDesc =
                    dynamic_cast<QCSharedBufferDescriptor_t *>( &mapXBufferDesc );
            m_configIfs.m_config.params.remapTable[inputId].pMapX = &( pMapXBufferDesc->buffer );

            uint32_t mapYBufferId = m_configIfs.m_mapYBufferIds[inputId];
            QCBufferDescriptorBase_t &mapYBufferDesc = config.buffers[mapYBufferId];
            QCSharedBufferDescriptor_t *pMapYBufferDesc =
                    dynamic_cast<QCSharedBufferDescriptor_t *>( &mapYBufferDesc );
            m_configIfs.m_config.params.remapTable[inputId].pMapY = &( pMapYBufferDesc->buffer );
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
        status = m_cl2d.Init( m_nodeId.name.c_str(), &params );
    }

    if ( QC_STATUS_OK == status )
    {
        status = SetupGlobalBufferIdMap( *pConfig );
    }

    if ( QC_STATUS_OK == status )
    {
        bCL2DFlexInitDone = true;
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
                    status = m_cl2d.RegisterBuffers( &( pSharedBuffer->buffer ), 1 );
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
        if ( bCL2DFlexInitDone )
        {
            (void) m_cl2d.Deinit();
        }
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e CL2DFlex::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = m_cl2d.Deinit();
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

QCStatus_e CL2DFlex::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_cl2d.Start();

    return status;
}

QCStatus_e CL2DFlex::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_cl2d.Stop();

    return status;
}

QCStatus_e CL2DFlex::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
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
        if ( 0 == m_configIfs.m_numOfROIs )
        {
            status = m_cl2d.Execute( inputs.data(), inputs.size(), outputs.data() );
        }
        else
        {
            status = m_cl2d.ExecuteWithROI( inputs.data(), outputs.data(), m_configIfs.m_ROIs,
                                            m_configIfs.m_numOfROIs );
        }
    }

    return status;
}

}   // namespace node
}   // namespace QC
