// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Node/Voxelization.hpp"
#include "VoxelizationImpl.hpp"

namespace QC
{
namespace Node
{

QCStatus_e VoxelizationMonitor::VerifyAndSet( const std::string config, std::string &errors )
{
    return QC_STATUS_UNSUPPORTED;
}

const std::string &VoxelizationMonitor::GetOptions()
{
    m_options = "{}";
    return m_options;
}

const QCNodeMonitoringBase_t &VoxelizationMonitor::Get()
{
    return m_pVoxelImpl->GetMonitorConifg();
}

}   // namespace Node
}   // namespace QC
