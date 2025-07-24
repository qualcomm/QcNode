// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "CL2DFlexImpl.hpp"
#include "QC/Node/CL2DFlex.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

QCStatus_e CL2DFlexConfig::VerifyStaticConfig( DataTree &dt, std::string &errors )
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

    m_pCL2DFlexImpl->m_numOfROIs = 0;
    std::vector<DataTree> roiDts;
    (void) dt.Get( "ROIs", roiDts );
    for ( DataTree &idt : roiDts )
    {
        m_pCL2DFlexImpl->m_numOfROIs++;
    }

    if ( QC_CL2DFLEX_ROI_NUMBER_MAX < m_pCL2DFlexImpl->m_numOfROIs )
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

QCStatus_e CL2DFlexConfig::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    CL2DFlexImplConfig_t &config = m_pCL2DFlexImpl->GetConifg();
    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        config.nodeId.name = dt.Get<std::string>( "name", "" );
        config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );

        config.params.numOfInputs = m_numOfInputs;
        config.params.outputWidth = dt.Get<uint32_t>( "outputWidth", 1024 );
        config.params.outputHeight = dt.Get<uint32_t>( "outputHeight", 1024 );
        config.params.outputFormat = dt.GetImageFormat( "outputFormat", QC_IMAGE_FORMAT_RGB888 );

        std::vector<DataTree> inputDts;
        (void) dt.Get( "inputs", inputDts );
        uint32_t inputId = 0;
        for ( DataTree &idt : inputDts )
        {
            config.params.inputWidths[inputId] = idt.Get<uint32_t>( "inputWidth", 1024 );
            config.params.inputHeights[inputId] = idt.Get<uint32_t>( "inputHeight", 1024 );
            config.params.inputFormats[inputId] =
                    idt.GetImageFormat( "inputFormat", QC_IMAGE_FORMAT_NV12 );
            config.params.ROIs[inputId].x = idt.Get<uint32_t>( "roiX", 0 );
            config.params.ROIs[inputId].y = idt.Get<uint32_t>( "roiY", 0 );
            config.params.ROIs[inputId].width = idt.Get<uint32_t>( "roiWidth", 0 );
            config.params.ROIs[inputId].height = idt.Get<uint32_t>( "roiHeight", 0 );
            std::string mode = idt.Get<std::string>( "workMode", "resize_nearest" );
            if ( "convert" == mode )
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_CONVERT;
            }
            else if ( "resize_nearest" == mode )
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
            }
            else if ( "letterbox_nearest" == mode )
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST;
            }
            else if ( "letterbox_nearest_multiple" == mode )
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE;
            }
            else if ( "resize_nearest_multiple" == mode )
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE;
            }
            else if ( "convert_ubwc" == mode )
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_CONVERT_UBWC;
            }
            else if ( "remap_nearest" == mode )
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_REMAP_NEAREST;
                m_pCL2DFlexImpl->m_mapXBufferIds[inputId] = idt.Get<uint32_t>( "mapX", UINT32_MAX );
                m_pCL2DFlexImpl->m_mapYBufferIds[inputId] = idt.Get<uint32_t>( "mapY", UINT32_MAX );
            }
            else
            {
                config.params.workModes[inputId] = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
            }

            inputId++;
        }

        std::vector<DataTree> roiDts;
        uint32_t roiId = 0;
        (void) dt.Get( "ROIs", roiDts );
        for ( DataTree &idt : roiDts )
        {
            m_pCL2DFlexImpl->m_ROIs[roiId].x = idt.Get<uint32_t>( "roiX", 0 );
            m_pCL2DFlexImpl->m_ROIs[roiId].y = idt.Get<uint32_t>( "roiY", 0 );
            m_pCL2DFlexImpl->m_ROIs[roiId].width = idt.Get<uint32_t>( "roiWidth", 0 );
            m_pCL2DFlexImpl->m_ROIs[roiId].height = idt.Get<uint32_t>( "roiHeight", 0 );
            roiId++;
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

QCStatus_e CL2DFlexConfig::VerifyAndSet( const std::string config, std::string &errors )
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

const std::string &CL2DFlexConfig::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;
    // empty function for CL2D node
    return m_options;
}

const QCNodeConfigBase_t &CL2DFlexConfig::Get()
{
    return m_pCL2DFlexImpl->GetConifg();
}

}   // namespace Node
}   // namespace QC
