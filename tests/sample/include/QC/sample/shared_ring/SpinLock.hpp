// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef QC_SAMPLE_SPIN_LOCK_HPP
#define QC_SAMPLE_SPIN_LOCK_HPP

#include "QC/Common/Types.hpp"
#include <cinttypes>

#include <pthread.h>

using namespace QC;

namespace QC
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
typedef int32_t QCSpinLock_t;
#elif SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_SPIN
typedef pthread_spinlock_t QCSpinLock_t;
#elif SPINLOCK_IMPL_METHOD == SPINLOCK_OVER_PTHREAD_MUTEX
typedef pthread_mutex_t QCSpinLock_t;
#else
#error SPINLOCK_IMPL_METHOD is invalid
#endif

#define SPINLOCK_WAIT_INFINITE ( (uint32_t) 0xFFFFFFFF )

QCStatus_e QCSpinLockInit( QCSpinLock_t *pSpinlock );

QCStatus_e QCSpinLock( QCSpinLock_t *pSpinlock, uint32_t timeoutMs = SPINLOCK_WAIT_INFINITE );

QCStatus_e QCSpinUnLock( QCSpinLock_t *pSpinlock );

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_SPIN_LOCK_HPP
