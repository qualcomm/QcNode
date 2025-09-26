// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "QC/sample/shared_ring/SpinLock.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include <chrono>


using namespace QC;

namespace QC
{
namespace sample
{
namespace shared_ring
{
#define SPINLOCK_UNLOCKED 0
#define SPINLOCK_LOCKED 1

#if SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_INT32
QCStatus_e QCSpinLockInit( int32_t *pSpinlock )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        *pSpinlock = SPINLOCK_UNLOCKED;
    }

    return ret;
}

/* refer: https://rigtorp.se/spinlock/ */
QCStatus_e QCSpinLock( int32_t *pSpinlock, uint32_t timeoutMs )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
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
                        QC_LOG_ERROR( "SpinLock %p timeout", pSpinlock );
                        ret = QC_STATUS_TIMEOUT;
                        break;
                    }
                }
            }
            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            if ( SPINLOCK_LOCKED != __atomic_load_n( pSpinlock, __ATOMIC_RELAXED ) )
            {
                QC_LOG_ERROR( "pSpinlock %p lock failed ", pSpinlock );
                ret = QC_STATUS_BAD_STATE;
            }
        }
    }

    return ret;
}

QCStatus_e QCSpinUnLock( int32_t *pSpinlock )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( SPINLOCK_LOCKED != __atomic_load_n( pSpinlock, __ATOMIC_RELAXED ) )
    {
        QC_LOG_ERROR( "pSpinlock %p is not in lock state", pSpinlock );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        __atomic_store_n( pSpinlock, SPINLOCK_UNLOCKED, __ATOMIC_RELAXED );
    }

    return ret;
}
#endif /* SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_INT32 */

#if SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_SPIN
QCStatus_e QCSpinLockInit( QCSpinLock_t *pSpinlock )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        int rv = pthread_spin_init( pSpinlock, PTHREAD_PROCESS_SHARED );
        if ( 0 != rv )
        {
            QC_LOG_ERROR( "pSpinlock init failed: %d", rv );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e QCSpinLock( QCSpinLock_t *pSpinlock, uint32_t timeoutMs )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
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
                    QC_LOG_ERROR( "SpinLock %p timeout", pSpinlock );
                    ret = QC_STATUS_TIMEOUT;
                    break;
                }
            }
            if ( QC_STATUS_OK != ret )
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
            QC_LOG_ERROR( "SpinLock %p lock failed: %d", pSpinlock, rv );
        }
    }

    return ret;
}

QCStatus_e QCSpinUnLock( QCSpinLock_t *pSpinlock )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        int rv = pthread_spin_unlock( pSpinlock );
        if ( 0 != rv )
        {
            QC_LOG_ERROR( "SpinLock %p unlock failed: %d", pSpinlock, rv );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}
#endif /* SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_SPIN */

#if SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_MUTEX
QCStatus_e QCSpinLockInit( QCSpinLock_t *pSpinlock )
{
    QCStatus_e ret = QC_STATUS_OK;
    pthread_mutexattr_t attr;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        pthread_mutexattr_init( &attr );
        pthread_mutexattr_setrobust( &attr, PTHREAD_MUTEX_STALLED );
        pthread_mutexattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
        int rv = pthread_mutex_init( pSpinlock, &attr );
        if ( 0 != rv )
        {
            QC_LOG_ERROR( "pSpinlock init failed: %d", rv );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e QCSpinLock( QCSpinLock_t *pSpinlock, uint32_t timeoutMs )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
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
                    QC_LOG_ERROR( "SpinLock %p timeout", pSpinlock );
                    ret = QC_STATUS_TIMEOUT;
                    break;
                }
            }
            if ( QC_STATUS_OK != ret )
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
            QC_LOG_ERROR( "SpinLock %p lock failed: %d", pSpinlock, rv );
        }
    }

    return ret;
}

QCStatus_e QCSpinUnLock( QCSpinLock_t *pSpinlock )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSpinlock )
    {
        QC_LOG_ERROR( "pSpinlock is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        int rv = pthread_mutex_unlock( pSpinlock );
        if ( 0 != rv )
        {
            QC_LOG_ERROR( "SpinLock %p unlock failed: %d", pSpinlock, rv );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}
#endif /* SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_MUTEX */
}   // namespace shared_ring
}   // namespace sample
}   // namespace QC
