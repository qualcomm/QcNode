// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Node/QNN.hpp"
#include "QnnImpl.hpp"
#include <unistd.h>

namespace QC
{
namespace Node
{

QCStatus_e QnnMonitor::VerifyAndSet( const std::string config, std::string &errors )
{
    return QC_STATUS_UNSUPPORTED;
}

const std::string &QnnMonitor::GetOptions()
{
    m_options = "{}";
    return m_options;
}

const QCNodeMonitoringBase_t &QnnMonitor::Get()
{
    return m_pQnnImpl->GetMonitorConfig();
}


uint32_t QnnMonitor::GetMaximalSize()
{
    return sizeof( Qnn_Perf_t );
}

uint32_t QnnMonitor::GetCurrentSize()
{
    return sizeof( Qnn_Perf_t );
}

QCStatus_e QnnMonitor::Place( void *pData, uint32_t &size )
{
    QCStatus_e status = QC_STATUS_OK;
    Qnn_Perf_t perf;

    if ( nullptr == pData )
    {
        QC_ERROR( "Place with invalid size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( size != sizeof( Qnn_Perf_t ) )
    {
        QC_ERROR( "Place with invalid size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        status = m_pQnnImpl->GetPerf( perf );
        if ( QC_STATUS_OK == status )
        {
            memcpy( pData, &perf, sizeof( Qnn_Perf_t ) );
        }
    }

    return status;
}

}   // namespace Node
}   // namespace QC
