// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/Node/CL2DFlex.hpp"
#include "CL2DFlexImpl.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

CL2DFlex::CL2DFlex()
    : m_pCL2DFlexImpl( new CL2DFlexImpl( m_nodeId, m_logger ) ),
      m_configIfs( m_logger, m_pCL2DFlexImpl ),
      m_monitorIfs( m_logger, m_pCL2DFlexImpl ) {};

CL2DFlex::~CL2DFlex()
{
    delete m_pCL2DFlexImpl;
}

QCStatus_e CL2DFlex::Initialize( QCNodeInit_t &config )
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
        status = m_pCL2DFlexImpl->Initialize( config.buffers );
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

QCStatus_e CL2DFlex::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = m_pCL2DFlexImpl->DeInitialize();
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

    status = m_pCL2DFlexImpl->Start();

    return status;
}

QCStatus_e CL2DFlex::Stop()
{
    QCStatus_e status = QC_STATUS_OK;

    status = m_pCL2DFlexImpl->Stop();

    return status;
}

QCStatus_e CL2DFlex::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    return m_pCL2DFlexImpl->ProcessFrameDescriptor( frameDesc );
}

QCObjectState_e CL2DFlex::GetState()
{
    return m_pCL2DFlexImpl->GetState();
}

}   // namespace Node
}   // namespace QC
