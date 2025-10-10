// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/Node/DepthFromStereo.hpp"
#include "QC/component/DepthFromStereo.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

QCStatus_e DepthFromStereoConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    std::string dirStr = dt.Get<std::string>( "direction", "l2r" );
    if ( "l2r" != dirStr && "r2l" != dirStr )
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

    if ( dt.Get<uint32_t>( "fps", 0 ) == 0 )
    {
        errors += "the frame rate is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

QCStatus_e DepthFromStereoConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = VerifyStaticConfig( dt, errors );

    if ( QC_STATUS_OK == status )
    {
        std::string dirStr = dt.Get<std::string>( "direction", "l2r" );
        if ( "l2r" == dirStr )
        {
            m_config.config.dfsSearchDir = EVA_DFS_SEARCH_L2R;
        }
        else if ( "r2l" == dirStr )
        {
            m_config.config.dfsSearchDir = EVA_DFS_SEARCH_R2L;
        }
        else
        {
            QC_ERROR( "invalid direction %s\n", dirStr.c_str() );
            if ( status == QC_STATUS_OK )
            {
                status = QC_STATUS_BAD_ARGUMENTS;
            }
        }

        m_config.config.format =
                dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 );

        if (QC_IMAGE_FORMAT_MAX == m_config.config.format)
        {
            QC_ERROR( "invalid format %d\n", m_config.config.format );
            status = QC_STATUS_FAIL;
        }

        m_config.config.width = dt.Get<uint32_t>( "width", 1280 );
        m_config.config.height = dt.Get<uint32_t>( "height", 416 );
        m_config.config.frameRate = dt.Get<uint32_t>( "fps", 30 );
    }

    return status;
}

QCStatus_e DepthFromStereoConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e DepthFromStereoConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
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

const std::string &DepthFromStereoConfigIfs::GetOptions()
{
    return NodeConfigIfs::s_QC_STATUS_UNSUPPORTED;
}

QCStatus_e DepthFromStereo::Initialize( QCNodeInit_t &config )
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
        const DepthFromStereoConfig_t &configuration =
                dynamic_cast<const DepthFromStereoConfig_t &>( GetConfigurationIfs().Get() );
        status = m_dfs.Init( GetConfigurationIfs().Get().nodeId.name.c_str(),
                             &configuration.config, LOGGER_LEVEL_VERBOSE );
    }

    return status;
}

QCStatus_e DepthFromStereo::DeInitialize()
{
    QCStatus_e status = static_cast<QCStatus_e>( m_dfs.Deinit() );

    QCStatus_e status2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
    }

    return status;
}

QCStatus_e DepthFromStereo::Start()
{
    return static_cast<QCStatus_e>( m_dfs.Start() );
}

QCStatus_e DepthFromStereo::Stop()
{
    return static_cast<QCStatus_e>( m_dfs.Stop() );
}

QCStatus_e DepthFromStereo::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    const DepthFromStereoConfig_t &configuration =
            dynamic_cast<const DepthFromStereoConfig_t &>( GetConfigurationIfs().Get() );
    const DepthFromStereoBufferMapping_t &bufferMap = configuration.bufferMap;

    QCSharedBufferDescriptor_t *pPrImg = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.primaryImageBufferId ) );

    if (nullptr == pPrImg)
    {
        QC_ERROR( "invalid primary image buffer (id %u)", bufferMap.primaryImageBufferId );
        status = QC_STATUS_FAIL;
    }

    QCSharedBufferDescriptor_t *pAuxImg = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.auxilaryImageBufferId ) );

    if (nullptr == pAuxImg)
    {
        QC_ERROR( "invalid auxilary image buffer (id %u)", bufferMap.primaryImageBufferId );
        status = QC_STATUS_FAIL;
    }

    QCSharedBufferDescriptor_t *pDispMap = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.disparityMapBufferId ) );

    if (nullptr == pDispMap)
    {
        QC_ERROR( "invalid disparity map buffer (id %u)", bufferMap.primaryImageBufferId );
        status = QC_STATUS_FAIL;
    }

    QCSharedBufferDescriptor_t *pDispConf = dynamic_cast<QCSharedBufferDescriptor_t *>(
            &frameDesc.GetBuffer( bufferMap.disparityConfidenceMapBufferId ) );

    if (nullptr == pDispConf)
    {
        QC_ERROR( "invalid disparity confidence map buffer (id %u)",
                  bufferMap.primaryImageBufferId );
        status = QC_STATUS_FAIL;
    }

    if (QC_STATUS_OK == status)
    {
        status = m_dfs.Execute( &pPrImg->buffer,
                                &pAuxImg->buffer,
                                &pDispMap->buffer,
                                &pDispConf->buffer );
    }

    return status;
}

}   // namespace Node
}   // namespace QC
