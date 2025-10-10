// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "pipeline/CL2DPipelineBase.hpp"

namespace QC
{
namespace Node
{

CL2DPipelineBase::CL2DPipelineBase() {}

CL2DPipelineBase::~CL2DPipelineBase() {}

void CL2DPipelineBase::InitLogger( const char *pName, Logger_Level_e level )
{
    m_name = pName;
    QCStatus_e ret = QC_LOGGER_INIT( pName, level );
    if ( QC_STATUS_OK != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to create logger for component %s: ret = %d\n",
                        m_name.c_str(), ret );
    }
    /* ignore logger init error */
}

void CL2DPipelineBase::DeinitLogger()
{
    QCStatus_e ret = QC_LOGGER_DEINIT();
    if ( QC_STATUS_OK != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to deinit logger for component %s: ret = %d\n",
                        m_name.c_str(), ret );
    }
    /* ignore logger deinit error */
}

}   // namespace Node
}   // namespace QC
