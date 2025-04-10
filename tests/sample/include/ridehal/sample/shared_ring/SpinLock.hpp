// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_SPIN_LOCK_HPP
#define RIDEHAL_SAMPLE_SPIN_LOCK_HPP

#include "ridehal/common/Types.hpp"
#include <cinttypes>

#include <pthread.h>

using namespace ridehal::common;

namespace ridehal
{
namespace sample
{
namespace shared_ring
{

#define SPINLOCK_OVER_INT32 0
#define SPINLOCK_OVER_PTHREAD_SPIN 1
#define SPINLOCK_OVER_PTHREAD_MUTEX 2

#define SPINLOCK_IMPL_METHOD SPINLOCK_OVER_INT32

#if SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_INT32
typedef int32_t RideHal_SpinLock_t;
#elif SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_SPIN
typedef pthread_spinlock_t RideHal_SpinLock_t;
#elif SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_MUTEX
typedef pthread_mutex_t RideHal_SpinLock_t;
#else
#error SPINLOCK_IMPL_METHOD is invalid
#endif

#define SPINLOCK_WAIT_INFINITE ( (uint32_t) 0xFFFFFFFF )

RideHalError_e RideHal_SpinLockInit( RideHal_SpinLock_t *pSpinlock );

RideHalError_e RideHal_SpinLock( RideHal_SpinLock_t *pSpinlock,
                                 uint32_t timeoutMs = SPINLOCK_WAIT_INFINITE );

RideHalError_e RideHal_SpinUnLock( RideHal_SpinLock_t *pSpinlock );

}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_SPIN_LOCK_HPP
