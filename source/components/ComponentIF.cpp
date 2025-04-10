// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/component/ComponentIF.hpp"
#include <stdio.h>

namespace ridehal
{
namespace component
{

RideHalError_e ComponentIF::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pName )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_COMPONENT_STATE_INITIAL != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        m_name = pName;
        ret = RIDEHAL_LOGGER_INIT( pName, level );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            (void) fprintf( stderr,
                            "WARINING: failed to create logger for component %s: ret = %d\n", pName,
                            ret );
        }
        ret = RIDEHAL_ERROR_NONE;
        RIDEHAL_INFO( "RideHal version %d.%d.%d", RIDEHAL_VERSION_MAJOR, RIDEHAL_VERSION_MINOR,
                      RIDEHAL_VERSION_PATCH );
    }

    return ret;
}

RideHalError_e ComponentIF::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = RIDEHAL_LOGGER_DEINIT();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to deinit logger for component %s: ret = %d\n",
                        GetName(), ret );
    }
    ret = RIDEHAL_ERROR_NONE; /* ignore logger init error */

    m_state = RIDEHAL_COMPONENT_STATE_INITIAL;

    return ret;
}


RideHal_ComponentState_t ComponentIF::GetState()
{
    return m_state;
}

const char *ComponentIF::GetName()
{
    return m_name.c_str();
}

}   // namespace component
}   // namespace ridehal
