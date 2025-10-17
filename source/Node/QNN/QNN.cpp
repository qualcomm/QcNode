// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/QNN.hpp"
#include "QnnImpl.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

using namespace QC::Memory;

Qnn::Qnn()
    : m_pQnnImpl( new QnnImpl( m_nodeId, m_logger ) ),
      m_configIfs( m_logger, m_pQnnImpl ),
      m_monitorIfs( m_logger, m_pQnnImpl ) {};

Qnn::~Qnn()
{
    delete m_pQnnImpl;
}

QCStatus_e Qnn::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    bool bNodeBaseInitDone = false;

    if ( QC_OBJECT_STATE_INITIAL != GetState() )
    {
        QC_ERROR( "QNN not in initial state!" );
        status = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_configIfs.VerifyAndSet( config.config, errors );
        if ( QC_STATUS_OK == status )
        {
            status = NodeBase::Init( cfg.nodeId );
        }
        else
        {
            QC_ERROR( "config error: %s", errors.c_str() );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        bNodeBaseInitDone = true;
        status = m_pQnnImpl->Initialize( config.callback, config.buffers );
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

QCStatus_e Qnn::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = m_pQnnImpl->DeInitialize();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
    }

    status2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK != status2 )
    {
        status = status2;
    }

    return status;
}

QCStatus_e Qnn::Start()
{
    return m_pQnnImpl->Start();
}

QCStatus_e Qnn::Stop()
{
    return m_pQnnImpl->Stop();
}

QCStatus_e Qnn::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    return m_pQnnImpl->ProcessFrameDescriptor( frameDesc );
}

QCObjectState_e Qnn::GetState()
{
    return m_pQnnImpl->GetState();
}

}   // namespace Node
}   // namespace QC
