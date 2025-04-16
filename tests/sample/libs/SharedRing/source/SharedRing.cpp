// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/shared_ring/SharedRing.hpp"
#include "QC/sample/shared_ring/SpinLock.hpp"

namespace QC
{
namespace sample
{
namespace shared_ring
{

#define SPINLOCK_TIMEOUT 100

QCStatus_e SharedRing_Ring::Push( uint16_t idx )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = QCSpinLock( &spinlock, SPINLOCK_TIMEOUT );
    if ( QC_STATUS_OK == ret )
    {
        ring[writeIdx % SHARED_RING_NUM_DESC] = idx;
        writeIdx++;
        ret = QCSpinUnLock( &spinlock );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_LOG_ERROR( "ring push %d to %s failed", idx, name );
    }

    return ret;
}

QCStatus_e SharedRing_Ring::Pop( uint16_t &idx )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2;

    ret = QCSpinLock( &spinlock, SPINLOCK_TIMEOUT );
    if ( QC_STATUS_OK == ret )
    {
        if ( readIdx != writeIdx )
        {
            idx = ring[readIdx % SHARED_RING_NUM_DESC];
            readIdx++;
        }
        else
        {
            ret = QC_STATUS_OUT_OF_BOUND;
        }
        ret2 = QCSpinUnLock( &spinlock );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_LOG_ERROR( "ring pop %d from %s unlock failed", idx, name );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        QC_LOG_ERROR( "ring pop from %s lock failed", name );
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
}   // namespace QC