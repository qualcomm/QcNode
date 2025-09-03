// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "CameraImpl.hpp"
#include "QC/Node/Camera.hpp"

namespace QC
{
namespace Node
{

QCStatus_e CameraMonitor::VerifyAndSet( const std::string config, std::string &errors )
{
    return QC_STATUS_UNSUPPORTED;
}

const std::string &CameraMonitor::GetOptions()
{
    m_options = "{}";
    return m_options;
}

const QCNodeMonitoringBase_t &CameraMonitor::Get()
{
    return m_pCamImpl->GetMonitorConifg();
}

}   // namespace Node
}   // namespace QC

