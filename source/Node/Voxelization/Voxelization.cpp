// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Node/Voxelization.hpp"
#include "VoxelizationImpl.hpp"

namespace QC
{
namespace Node
{

using namespace QC::Memory;

Voxelization::Voxelization()
    : m_pVoxelImpl( new VoxelizationImpl( m_nodeId, m_logger ) ),
      m_configIfs( m_logger, m_pVoxelImpl ),
      m_monitor( m_logger, m_pVoxelImpl )
{}

Voxelization::~Voxelization()
{
    delete m_pVoxelImpl;
    m_pVoxelImpl = nullptr;
}

QCStatus_e Voxelization::Initialize( QCNodeInit_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    bool bNodeBaseInitDone = false;

    ret = m_configIfs.VerifyAndSet( config.config, errors );
    if ( QC_STATUS_OK == ret )
    {
        ret = NodeBase::Init( cfg.nodeId );
    }
    else
    {
        QC_ERROR( "config error: %s", errors.c_str() );
    }

    if ( QC_STATUS_OK == ret )
    {
        bNodeBaseInitDone = true;
        ret = m_pVoxelImpl->Initialize( config.callback, config.buffers );
    }

    if ( QC_STATUS_OK != ret )
    {
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    return ret;
}

QCStatus_e Voxelization::DeInitialize()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2 = m_pVoxelImpl->DeInitialize();
    if ( QC_STATUS_OK != ret2 )
    {
        ret = ret2;
        QC_ERROR( "Failed to deinitialize voxelization" );
    }

    ret2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK != ret2 )
    {
        ret = ret2;
        QC_ERROR( "Failed to deinitialize NodeBase" );
    }

    return ret;
}

QCStatus_e Voxelization::Start()
{
    return m_pVoxelImpl->Start();
}

QCStatus_e Voxelization::Stop()
{
    return m_pVoxelImpl->Stop();
}

QCStatus_e Voxelization::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    return m_pVoxelImpl->ProcessFrameDescriptor( frameDesc );
}

QCObjectState_e Voxelization::GetState()
{
    return m_pVoxelImpl->GetState();
}

}   // namespace Node
}   // namespace QC

