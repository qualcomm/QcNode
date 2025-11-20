// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/Camera.hpp"
#include "CameraImpl.hpp"

namespace QC
{
namespace Node
{

using namespace QC::Memory;

Camera::Camera()
    : m_pCamImpl( new CameraImpl( m_nodeId, m_logger ) ),
      m_configIfs( m_logger, m_pCamImpl ),
      m_monitor( m_logger, m_pCamImpl ) {};

Camera::~Camera()
{
    delete m_pCamImpl;
    m_pCamImpl = nullptr;
}

QCStatus_e Camera::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;

    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    bool bNodeBaseInitDone = false;

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
        status = m_pCamImpl->Initialize( config.callback, config.buffers );
    }

    if ( QC_STATUS_OK != status )
    {
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e Camera::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2 = m_pCamImpl->DeInitialize();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
        QC_ERROR( "Failed to deinitialize camera" );
    }

    status2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
        QC_ERROR( "Failed to deinitialize NodeBase" );
    }

    return status;
}

QCStatus_e Camera::Start()
{
    return m_pCamImpl->Start();
}

QCStatus_e Camera::Stop()
{
    return m_pCamImpl->Stop();
}

QCStatus_e Camera::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    return m_pCamImpl->ProcessFrameDescriptor( frameDesc );
}

QCObjectState_e Camera::GetState()
{
    return m_pCamImpl->GetState();
}

}   // namespace Node
}   // namespace QC
