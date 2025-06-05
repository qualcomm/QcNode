// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/node/OpticalFlow.hpp"
#include "QC/component/OpticalFlow.hpp"
#include <unistd.h>

namespace QC
{
namespace node
{

QCStatus_e OpticalFlowConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    std::string evaModeStr = dt.Get<std::string>( "eva_mode", "dsp" );
    if ( "dsp" != evaModeStr && "cpu" != evaModeStr && "disable" != evaModeStr )
    {
        errors += "invalid eva mode, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string dirStr = dt.Get<std::string>( "direction", "forward" );
    if ( "forward" != dirStr && "backward" != dirStr )
    {
        errors += "invalid direction, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if (QC_IMAGE_FORMAT_MAX == dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 ))
    {
        errors += "invalid format, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "width", 0 ) == 0 )
    {
        errors += "the width is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "height", 0 ) == 0 )
    {
        errors += "the height is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}
QCStatus_e OpticalFlowConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = VerifyStaticConfig( dt, errors );

    if ( QC_STATUS_OK == status )
    {
        std::string evaModeStr = dt.Get<std::string>( "eva_mode", "dsp" );
        if ( "dsp" == evaModeStr )
        {
            m_config.config.filterOperationMode = EVA_OF_MODE_DSP;
        }
        else if ( "cpu" == evaModeStr )
        {
            m_config.config.filterOperationMode = EVA_OF_MODE_CPU;
        }
        else if ( "disable" == evaModeStr )
        {
            m_config.config.filterOperationMode = EVA_OF_MODE_DISABLE;
        }
        else
        {
            QC_ERROR( "invalid eva_mode %s\n", evaModeStr.c_str() );
            status = QC_STATUS_FAIL;
        }

        std::string dirStr = dt.Get<std::string>( "direction", "forward" );
        if ( "forward" == dirStr )
        {
            m_config.config.direction = EVA_OF_FORWARD_DIRECTION;
        }
        else if ( "backward" == dirStr )
        {
            m_config.config.direction = EVA_OF_BACKWARD_DIRECTION;
        }
        else
        {
            QC_ERROR( "invalid direction %s\n", dirStr.c_str() );
            status = QC_STATUS_FAIL;
        }

        m_config.config.format =
                dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 );

        if (QC_IMAGE_FORMAT_MAX == m_config.config.format)
        {
            QC_ERROR( "invalid direction %d\n", m_config.config.format );
            status = QC_STATUS_FAIL;
        }

        m_config.config.width = dt.Get<uint32_t>( "width", 1920 );
        m_config.config.height = dt.Get<uint32_t>( "height", 1024 );
        m_config.config.frameRate = dt.Get<uint32_t>( "fps", 30 );
        m_config.config.amFilter.nStepSize = dt.Get<uint32_t>( "step_size", 1 );
    }

    return status;
}

QCStatus_e OpticalFlowConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e OpticalFlowConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    // Stage 1 setting logger level (need to be moved to monitoring interface)
    // Loading std::string into parser
    QCStatus_e status = NodeConfigIfs::VerifyAndSet( config, errors );

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

const std::string &OpticalFlowConfigIfs::GetOptions()
{
    return NodeConfigIfs::s_QC_STATUS_UNSUPPORTED;
}

QCStatus_e OpticalFlow::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;

    status = GetConfigurationIfs().VerifyAndSet( config.config, errors );
    if ( QC_STATUS_OK == status )
    {
        status = NodeBase::Init( GetConfigurationIfs().Get().nodeId );
    }
    else
    {
        QC_ERROR( "config error: %s", errors.c_str() );
    }

    if ( QC_STATUS_OK == status )
    {
        const OpticalFlowConfig_t &configuration =
                dynamic_cast<const OpticalFlowConfig_t &>( GetConfigurationIfs().Get() );
        status = m_of.Init( GetConfigurationIfs().Get().nodeId.name.c_str(),
                            &configuration.config, LOGGER_LEVEL_VERBOSE );
    }

    return status;
}

QCStatus_e OpticalFlow::DeInitialize()
{
    QCStatus_e status = m_of.Deinit();

    QCStatus_e status2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
    }

    return status;
}

QCStatus_e OpticalFlow::Start()
{
    return m_of.Start();
}

QCStatus_e OpticalFlow::Stop()
{
    return m_of.Stop();
}

QCStatus_e OpticalFlow::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    const OpticalFlowConfig_t &configuration =
            dynamic_cast<const OpticalFlowConfig_t &>( GetConfigurationIfs().Get() );
    const OpticalFlowBufferMapping_t &bufferMap = configuration.bufferMap;

    QCSharedBufferDescriptor_t *pRefImg = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.referenceImageBufferId ) );

    if (nullptr == pRefImg) {
        QC_ERROR( "invalid shared buffer refImg (id %u)", bufferMap.referenceImageBufferId);
        status = QC_STATUS_FAIL;
    }

    QCSharedBufferDescriptor_t *pCurImg = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.currentImageBufferId ) );

    if (nullptr == pCurImg) {
        QC_ERROR( "invalid shared buffer curImg (id %u)", bufferMap.currentImageBufferId);
        status = QC_STATUS_FAIL;
    }

    QCSharedBufferDescriptor_t *pMv = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.monionVectorsBufferId ) );

    if (nullptr == pMv) {
        QC_ERROR( "invalid shared buffer mv (id %u)", bufferMap.monionVectorsBufferId);
        status = QC_STATUS_FAIL;
    }

    QCSharedBufferDescriptor_t *pMvConf = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.confidenceBufferId ) );

    if (nullptr == pMvConf) {
        QC_ERROR( "invalid shared buffer mvConf (id %u)", bufferMap.confidenceBufferId);
        status = QC_STATUS_FAIL;
    }

    if (QC_STATUS_OK == status) {
        status = m_of.Execute( &pRefImg->buffer,
                               &pCurImg->buffer,
                               &pMv->buffer,
                               &pMvConf->buffer );
    }

    return status;
}

}   // namespace node
}   // namespace QC
