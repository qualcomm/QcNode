// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/shared_ring/SpinLock.hpp"
#include "ridehal/common/Logger.hpp"
#include <chrono>


using namespace ridehal::common;

namespace ridehal
{
namespace sample
{
namespace shared_ring
{
#define SPINLOCK_UNLOCKED 0
#define SPINLOCK_LOCKED 1

#if SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_INT32
RideHalError_e RideHal_SpinLockInit( int32_t *pSpinlock )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        *pSpinlock = SPINLOCK_UNLOCKED;
    }

    return ret;
}

/* refer: https://rigtorp.se/spinlock/ */
RideHalError_e RideHal_SpinLock( int32_t *pSpinlock, uint32_t timeoutMs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        int32_t expected = SPINLOCK_LOCKED;
        while ( __atomic_exchange_n( pSpinlock, SPINLOCK_LOCKED, __ATOMIC_ACQUIRE ) )
        {
            auto begin = std::chrono::high_resolution_clock::now();
            while ( __atomic_load_n( pSpinlock, __ATOMIC_RELAXED ) )
            {
                if ( SPINLOCK_WAIT_INFINITE != timeoutMs )
                {
                    auto now = std::chrono::high_resolution_clock::now();
                    auto elapsed =
                            std::chrono::duration_cast<std::chrono::milliseconds>( now - begin )
                                    .count();
                    if ( elapsed > timeoutMs )
                    {
                        RIDEHAL_LOG_ERROR( "SpinLock %p timeout", pSpinlock );
                        ret = RIDEHAL_ERROR_TIMEOUT;
                        break;
                    }
                }
            }
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            if ( SPINLOCK_LOCKED != __atomic_load_n( pSpinlock, __ATOMIC_RELAXED ) )
            {
                RIDEHAL_LOG_ERROR( "pSpinlock %p lock failed ", pSpinlock );
                ret = RIDEHAL_ERROR_BAD_STATE;
            }
        }
    }

    return ret;
}

RideHalError_e RideHal_SpinUnLock( int32_t *pSpinlock )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( SPINLOCK_LOCKED != __atomic_load_n( pSpinlock, __ATOMIC_RELAXED ) )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock %p is not in lock state", pSpinlock );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        __atomic_store_n( pSpinlock, SPINLOCK_UNLOCKED, __ATOMIC_RELAXED );
    }

    return ret;
}
#endif /* SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_INT32 */

#if SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_SPIN
RideHalError_e RideHal_SpinLockInit( RideHal_SpinLock_t *pSpinlock )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        int rv = pthread_spin_init( pSpinlock, PTHREAD_PROCESS_SHARED );
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "pSpinlock init failed: %d", rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e RideHal_SpinLock( RideHal_SpinLock_t *pSpinlock, uint32_t timeoutMs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
#if 1
        auto begin = std::chrono::high_resolution_clock::now();
        int rv = pthread_spin_trylock( pSpinlock );
        while ( 0 != rv )
        {
            if ( SPINLOCK_WAIT_INFINITE != timeoutMs )
            {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( now - begin )
                                       .count();
                if ( elapsed > timeoutMs )
                {
                    RIDEHAL_LOG_ERROR( "SpinLock %p timeout", pSpinlock );
                    ret = RIDEHAL_ERROR_TIMEOUT;
                    break;
                }
            }
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
            rv = pthread_spin_trylock( pSpinlock );
        }
#else
        (void) timeoutMs; /* not used */
        int rv = pthread_spin_lock( pSpinlock );
#endif
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "SpinLock %p lock failed: %d", pSpinlock, rv );
        }
    }

    return ret;
}

RideHalError_e RideHal_SpinUnLock( RideHal_SpinLock_t *pSpinlock )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        int rv = pthread_spin_unlock( pSpinlock );
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "SpinLock %p unlock failed: %d", pSpinlock, rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}
#endif /* SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_SPIN */

#if SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_MUTEX
RideHalError_e RideHal_SpinLockInit( RideHal_SpinLock_t *pSpinlock )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    pthread_mutexattr_t attr;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        pthread_mutexattr_init( &attr );
        pthread_mutexattr_setrobust( &attr, PTHREAD_MUTEX_STALLED );
        pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
        int rv = pthread_mutex_init( pSpinlock, &attr );
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "pSpinlock init failed: %d", rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e RideHal_SpinLock( RideHal_SpinLock_t *pSpinlock, uint32_t timeoutMs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
#if 0
        auto begin = std::chrono::high_resolution_clock::now();
        int rv = pthread_mutex_trylock( pSpinlock );
        while ( 0 != rv )
        {
            if ( SPINLOCK_WAIT_INFINITE != timeoutMs )
            {
                auto now = std::chrono::high_resolution_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( now - begin )
                                       .count();
                if ( elapsed > timeoutMs )
                {
                    RIDEHAL_LOG_ERROR( "SpinLock %p timeout", pSpinlock );
                    ret = RIDEHAL_ERROR_TIMEOUT;
                    break;
                }
            }
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
            rv = pthread_mutex_trylock( pSpinlock );
        }
#else
        (void) timeoutMs; /* not used */
        int rv = pthread_mutex_lock( pSpinlock );
#endif
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "SpinLock %p lock failed: %d", pSpinlock, rv );
        }
    }

    return ret;
}

RideHalError_e RideHal_SpinUnLock( RideHal_SpinLock_t *pSpinlock )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSpinlock )
    {
        RIDEHAL_LOG_ERROR( "pSpinlock is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        int rv = pthread_mutex_unlock( pSpinlock );
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "SpinLock %p unlock failed: %d", pSpinlock, rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}
#endif /* SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_MUTEX */
}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal
