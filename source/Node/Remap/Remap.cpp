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

Remap::Remap()
    : m_pRemapImpl( new RemapImpl( m_nodeId, m_logger ) ),
      m_configIfs( m_logger, m_pRemapImpl ),
      m_monitorIfs( m_logger, m_pRemapImpl ) {};

Remap::~Remap()
{
    delete m_pRemapImpl;
}

QCStatus_e Remap::Initialize( QCNodeInit_t &config )
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
        status = m_pRemapImpl->Initialize( config.buffers );
    }

    if ( QC_STATUS_OK != status )
    { /* do error clean up */
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e Remap::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = m_pRemapImpl->DeInitialize();
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

QCStatus_e Remap::Start()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_pRemapImpl->Start();

    return status;
}

QCStatus_e Remap::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_pRemapImpl->Stop();

    return status;
}

QCStatus_e Remap::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    return m_pRemapImpl->ProcessFrameDescriptor( frameDesc );
}

QCObjectState_e Remap::GetState()
{
    return m_pRemapImpl->GetState();
}

}   // namespace Node
}   // namespace QC
