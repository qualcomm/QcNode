// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "include/CL2DPipelineBase.hpp"

namespace ridehal
{
namespace component
{

CL2DPipelineBase::CL2DPipelineBase() {}

CL2DPipelineBase::~CL2DPipelineBase() {}

void CL2DPipelineBase::InitLogger( const char *pName, Logger_Level_e level )
{
    m_name = pName;
    RideHalError_e ret = RIDEHAL_LOGGER_INIT( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to create logger for component %s: ret = %d\n",
                        m_name.c_str(), ret );
    }
    /* ignore logger init error */
}

void CL2DPipelineBase::DeinitLogger()
{
    RideHalError_e ret = RIDEHAL_LOGGER_DEINIT();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to deinit logger for component %s: ret = %d\n",
                        m_name.c_str(), ret );
    }
    /* ignore logger deinit error */
}

}   // namespace component
}   // namespace ridehal
