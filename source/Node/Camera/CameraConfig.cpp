// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "CameraImpl.hpp"
#include "QC/Node/Camera.hpp"

namespace QC
{
namespace Node
{
QCStatus_e CameraConfig::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<DataTree> streamConfigs;
    std::vector<uint32_t> bufferIds;

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
        uint32_t inputId = dt.Get<uint32_t>( "inputId", UINT32_MAX );
        if ( UINT32_MAX == inputId )
        {
            errors += "the inputId is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t clientId = dt.Get<uint32_t>( "clientId", UINT32_MAX );
        if ( UINT32_MAX == clientId )
        {
            errors += "the clientId is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t srcId = dt.Get<uint32_t>( "srcId", UINT32_MAX );
        if ( UINT32_MAX == srcId )
        {
            errors += "the srcId is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t inputMode = dt.Get<uint32_t>( "inputMode", UINT32_MAX );
        if ( UINT32_MAX == inputMode )
        {
            errors += "the inputMode is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t ispUseCase = dt.Get<uint32_t>( "ispUseCase", UINT32_MAX );
        if ( UINT32_MAX == ispUseCase )
        {
            errors += "the ispUseCase is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t camFrameDropPattern = dt.Get<uint32_t>( "camFrameDropPattern", UINT32_MAX );
        if ( UINT32_MAX == camFrameDropPattern )
        {
            errors += "the camFrameDropPattern is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t camFrameDropPeriod = dt.Get<uint32_t>( "camFrameDropPeriod", UINT32_MAX );
        if ( UINT32_MAX == camFrameDropPeriod )
        {
            errors += "the camFrameDropPeriod is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t opMode = dt.Get<uint32_t>( "opMode", UINT32_MAX );
        if ( UINT32_MAX == opMode )
        {
            errors += "the opMode is empty, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        status = dt.Get( "streamConfigs", streamConfigs );
        if ( QC_STATUS_OK != status )
        {
            errors += "the streamConfigs is invalid";
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t numStream = streamConfigs.size();
        if ( numStream > MAX_CAMERA_STREAM )
        {
            errors += "the numStream is larger than maximum, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        for ( uint32_t i = 0; i < streamConfigs.size(); i++ )
        {
            DataTree streamConfig = streamConfigs[i];

            uint32_t streamId = streamConfig.Get<uint32_t>( "streamId", UINT32_MAX );
            if ( UINT32_MAX == streamId )
            {
                errors += "the streamId for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            uint32_t bufCnt = streamConfig.Get<uint32_t>( "bufCnt", UINT32_MAX );
            if ( UINT32_MAX == bufCnt )
            {
                errors += "the bufCnt for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            uint32_t width = streamConfig.Get<uint32_t>( "width", UINT32_MAX );
            if ( UINT32_MAX == width )
            {
                errors += "the width for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            uint32_t height = streamConfig.Get<uint32_t>( "height", UINT32_MAX );
            if ( UINT32_MAX == height )
            {
                errors += "the height for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            uint32_t submitRequestPattern =
                    streamConfig.Get<uint32_t>( "submitRequestPattern", UINT32_MAX );
            if ( UINT32_MAX == submitRequestPattern )
            {
                errors += "the submitRequestPattern for stream " + std::to_string( i ) +
                          " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            QCImageFormat_e format = streamConfig.GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
            if ( QC_IMAGE_FORMAT_MAX == format )
            {
                errors += "the format for stream " + std::to_string( i ) + " is empty, ";
                status = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( QC_STATUS_OK != status )
            {
                QC_ERROR( "Config verification failed for stream %u, error: %s", i, errors );
                break;
            }
        }
    }
    else
    {
        QC_ERROR( "Config verification failed for CameraConfigIfs, error: %s", errors );
    }

    return status;
}

QCStatus_e CameraConfig::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<DataTree> streamConfigs;
    CameraImplConfig_t &config = m_pCamImpl->GetConifg();

    status = VerifyStaticConfig( dt, errors );
    if ( QC_STATUS_OK == status )
    {
        config.nodeId.name = dt.Get<std::string>( "name", "" );
        config.nodeId.id = dt.Get<uint32_t>( "id", UINT32_MAX );
        config.inputId = dt.Get<uint32_t>( "inputId", UINT32_MAX );
        config.clientId = dt.Get<uint32_t>( "clientId", UINT32_MAX );
        config.srcId = dt.Get<uint32_t>( "srcId", UINT32_MAX );
        config.inputMode = dt.Get<uint32_t>( "inputMode", UINT32_MAX );
        config.ispUseCase = dt.Get<uint32_t>( "ispUseCase", UINT32_MAX );
        config.camFrameDropPattern = dt.Get<uint32_t>( "camFrameDropPattern", UINT32_MAX );
        config.camFrameDropPeriod = dt.Get<uint32_t>( "camFrameDropPeriod", UINT32_MAX );
        config.opMode = dt.Get<uint32_t>( "opMode", UINT32_MAX );
        status = dt.Get( "streamConfigs", streamConfigs );
    }

    if ( QC_STATUS_OK == status )
    {
        config.numStream = streamConfigs.size();
        for ( uint32_t i = 0; i < streamConfigs.size(); i++ )
        {
            DataTree streamConfig = streamConfigs[i];

            config.streamConfigs[i].streamId = streamConfig.Get<uint32_t>( "streamId", UINT32_MAX );
            config.streamConfigs[i].bufCnt = streamConfig.Get<uint32_t>( "bufCnt", UINT32_MAX );
            config.streamConfigs[i].width = streamConfig.Get<uint32_t>( "width", UINT32_MAX );
            config.streamConfigs[i].height = streamConfig.Get<uint32_t>( "height", UINT32_MAX );
            config.streamConfigs[i].submitRequestPattern =
                    streamConfig.Get<uint32_t>( "submitRequestPattern", UINT32_MAX );
            config.streamConfigs[i].format =
                    streamConfig.GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        config.bRequestMode = dt.Get<bool>( "requestMode", true );
        config.bPrimary = dt.Get<bool>( "primary", true );
        config.bRecovery = dt.Get<bool>( "recovery", true );
    }

    return status;
}

QCStatus_e CameraConfig::VerifyAndSet( const std::string config, std::string &errors )
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

const std::string &CameraConfig::GetOptions()
{
    QCStatus_e status = QC_STATUS_OK;
    return m_options;
}

const QCNodeConfigBase_t &CameraConfig::Get()
{
    return m_pCamImpl->GetConifg();
}

}   // namespace Node
}   // namespace QC

