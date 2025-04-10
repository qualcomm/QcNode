// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/shared_ring/SharedRing.hpp"
#include "ridehal/sample/shared_ring/SpinLock.hpp"

namespace ridehal
{
namespace sample
{
namespace shared_ring
{

#define SPINLOCK_TIMEOUT 100

RideHalError_e SharedRing_Ring::Push( uint16_t idx )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = RideHal_SpinLock( &spinlock, SPINLOCK_TIMEOUT );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ring[writeIdx % SHARED_RING_NUM_DESC] = idx;
        writeIdx++;
        ret = RideHal_SpinUnLock( &spinlock );
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_LOG_ERROR( "ring push %d to %s failed", idx, name );
    }

    return ret;
}

RideHalError_e SharedRing_Ring::Pop( uint16_t &idx )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHalError_e ret2;

    ret = RideHal_SpinLock( &spinlock, SPINLOCK_TIMEOUT );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( readIdx != writeIdx )
        {
            idx = ring[readIdx % SHARED_RING_NUM_DESC];
            readIdx++;
        }
        else
        {
            ret = RIDEHAL_ERROR_OUT_OF_BOUND;
        }
        ret2 = RideHal_SpinUnLock( &spinlock );
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_LOG_ERROR( "ring pop %d from %s unlock failed", idx, name );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_LOG_ERROR( "ring pop from %s lock failed", name );
    }

    return ret;
}

uint32_t SharedRing_Ring::Size()
{
    return ( writeIdx - readIdx ) & 0xFFFF;
}

void SharedRing_DataFrame::SetName( std::string name )
{
    (void) snprintf( this->name, sizeof( this->name ), "%s", name.c_str() );
}

void SharedRing_Ring::SetName( std::string name )
{
    (void) snprintf( this->name, sizeof( this->name ), "%s", name.c_str() );
}

void SharedRing_Memory::SetName( std::string name )
{
    (void) snprintf( this->name, sizeof( this->name ), "%s", name.c_str() );
}

}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal