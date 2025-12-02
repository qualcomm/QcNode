// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/sample/shared_ring/SharedPublisher.hpp"
#include <chrono>
using namespace std::chrono_literals;

namespace QC
{
namespace sample
{
namespace shared_ring
{

SharedPublisher::SharedPublisher() {}
SharedPublisher::~SharedPublisher() {}

QCStatus_e SharedPublisher::Init( std::string name, std::string topicName )
{
    QCStatus_e ret = QC_STATUS_OK;
    bool bShmemCreated = false;
    bool bSemFreeCreated = false;
    bool bSemUsedCreated[SHARED_RING_NUM_SUBSCRIBERS] = {
            false,
    };

    m_shName = topicName;
    std::replace( m_shName.begin(), m_shName.end(), '/', '_' );

    ret = QC_LOGGER_INIT( name.c_str(), LOGGER_LEVEL_INFO );

    ret = m_shmem.Create( m_shName, sizeof( SharedRing_Memory_t ) );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to create shared memory %s", m_shName.c_str() );
    }
    else
    {
        bShmemCreated = true;
        m_pRingMem = (SharedRing_Memory_t *) m_shmem.Data();
    }

    if ( QC_STATUS_OK == ret )
    {
        m_pRingMem->avail.SetName( name + ".avail" );
        ret = QCSpinLockInit( &m_pRingMem->avail.spinlock );
        if ( QC_STATUS_OK == ret )
        {
            m_pRingMem->free.SetName( name + ".free" );
            ret = QCSpinLockInit( &m_pRingMem->free.spinlock );
        }
        if ( QC_STATUS_OK == ret )
        {
            for ( int i = 0; i < SHARED_RING_NUM_SUBSCRIBERS; i++ )
            {
                ret = QCSpinLockInit( &m_pRingMem->used[i].spinlock );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_semFree.Create( m_shName + "_free", 0 );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to create semaphore %s_free", m_shName.c_str() );
        }
        else
        {
            bSemFreeCreated = true;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( int i = 0; i < SHARED_RING_NUM_SUBSCRIBERS; i++ )
        {
            ret = m_semUsed[i].Create( m_shName + "_used" + std::to_string( i ), 0 );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to create semaphore %s_used%d", m_shName.c_str(), i );
                break;
            }
            else
            {
                bSemUsedCreated[i] = true;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_pRingMem->SetName( name );
        for ( uint16_t idx = 0; idx < SHARED_RING_NUM_DESC; idx++ )
        {
            ret = m_pRingMem->avail.Push( idx );
            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_pRingMem->status = SHARED_RING_MEMORY_INITIALIZED;
        QC_INFO( "shared ring %s online", m_shName.c_str() );
    }

    if ( QC_STATUS_OK != ret )
    { /* do error clean up */
        for ( int i = 0; i < SHARED_RING_NUM_SUBSCRIBERS; i++ )
        {
            if ( bSemUsedCreated[i] )
            {
                (void) m_semUsed[i].Destroy();
            }
        }

        if ( bSemFreeCreated )
        {
            (void) m_semFree.Destroy();
        }

        if ( bShmemCreated )
        {
            (void) m_shmem.Destroy();
        }

        m_pRingMem = nullptr;
        m_shName = "";
    }


    return ret;
}

QCStatus_e SharedPublisher::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( m_shName.empty() )
    {
        QC_ERROR( "Not initialized" );
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bStarted = true;
        m_thread = std::thread( &SharedPublisher::ThreadMain, this );
    }

    return ret;
}

void SharedPublisher::ReleaseDesc( uint16_t idx )
{
    QCStatus_e ret = QC_STATUS_OK;
    SharedRing_Desc_t *pDesc = &m_pRingMem->descs[idx];

    QC_DEBUG( "release idx %d", idx );
    int32_t ref = __atomic_sub_fetch( &pDesc->ref, 1, __ATOMIC_RELAXED );
    if ( 0 == ref )
    {
        QC_DEBUG( "free idx %d", idx );
        ret = m_pRingMem->free.Push( idx );
        if ( QC_STATUS_OK == ret )
        {
            ret = m_semFree.Post();
        }
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "failed to release idx %d", idx );
        }
    }
    else if ( ref < 0 )
    {
        QC_ERROR( "desc %d ref underflow", idx );
        /* Note: this is a diaster if reach here, this error now was not handled */
    }
    else
    {
        /* still hold by others */
    }
}

void SharedPublisher::ReclaimResource( int32_t slotId )
{
    QCStatus_e ret = QC_STATUS_OK;
    SharedRing_Ring_t *pUsedRing = &m_pRingMem->used[slotId];
    do
    {
        uint16_t idx = 0;
        ret = pUsedRing->Pop( idx );
        if ( QC_STATUS_OK == ret )
        {
            ReleaseDesc( idx );
        }
    } while ( QC_STATUS_OK == ret );
    m_semUsed[slotId].Reset();
}

void SharedPublisher::ThreadMain()
{
    QCStatus_e ret = QC_STATUS_OK;
    uint16_t idx = 0;

    while ( m_bStarted )
    {
        for ( int32_t slotId = 0; slotId < SHARED_RING_NUM_SUBSCRIBERS; slotId++ )
        {
            SharedRing_Ring_t *pUsedRing = &m_pRingMem->used[slotId];
            if ( true == pUsedRing->reserved )
            {
                if ( SHARED_RING_USED_INITIALIZED == pUsedRing->status )
                {
                    std::lock_guard<std::mutex> l( m_lock );
                    auto it = m_subscribers.find( slotId );
                    if ( m_subscribers.end() == it )
                    {
                        QC_INFO( "subscriber %s online for %s, slot %d", pUsedRing->name,
                                 m_shName.c_str(), slotId );
                        m_subscribers[slotId] = pUsedRing;
                    }
                }
                else if ( SHARED_RING_USED_DESTROYED == pUsedRing->status )
                {
                    std::lock_guard<std::mutex> l( m_lock );
                    auto it = m_subscribers.find( slotId );
                    if ( m_subscribers.end() != it )
                    {
                        QC_INFO( "subscriber %s offline for %s, slot %d", pUsedRing->name,
                                 m_shName.c_str(), slotId );
                        ReclaimResource( slotId );
                        m_subscribers.erase( slotId );
                    }
                    __atomic_store_n( &pUsedRing->status, SHARED_RING_USED_UNINITIALIZED,
                                      __ATOMIC_RELAXED );
                    __atomic_store_n( &pUsedRing->reserved, false, __ATOMIC_RELEASE );
                }
                else
                {
                    /* do nothing */
                }
            }
        }

        (void) m_semFree.Wait( 10 );
        /* m_semFree is used for sync, always check on the free ring queue */
        ret = m_pRingMem->free.Pop( idx );
        if ( QC_STATUS_OK == ret )
        {
            std::lock_guard<std::mutex> l( m_lock );
            auto it = m_dataFrames.find( idx );
            if ( it != m_dataFrames.end() )
            {
                m_dataFrames.erase( idx );
            }
            else
            {
                QC_ERROR( "idx %d not found", idx );
            }
            QC_DEBUG( "release idx = %d back", idx, m_shName.c_str() );
            auto ret2 = m_pRingMem->avail.Push( idx );
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "Failed to release %d back when free", idx );
            }
        }
    }
}

size_t SharedPublisher::GetNumberOfSubscribers()
{
    std::lock_guard<std::mutex> l( m_lock );
    return m_subscribers.size();
}

QCStatus_e SharedPublisher::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bStarted )
    {
        QC_ERROR( "Not started" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        m_bStarted = false;
        if ( m_thread.joinable() )
        {
            m_thread.join();
        }
    }

    return ret;
}

QCStatus_e SharedPublisher::Publish( DataFrames_t &data )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint16_t idx = 0;

    if ( false == m_bStarted )
    {
        QC_ERROR( "Not started" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        std::lock_guard<std::mutex> l( m_lock );
        ret = m_pRingMem->avail.Pop( idx );
        if ( QC_STATUS_OK == ret )
        {
            if ( ( idx >= SHARED_RING_NUM_DESC ) || ( idx < 0 ) )
            {
                QC_ERROR( "get an invalid idx: %d", idx );
                ret = QC_STATUS_FAIL;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        std::lock_guard<std::mutex> l( m_lock );
        if ( m_subscribers.size() > 0 )
        {
            m_dataFrames[idx] = data;
            QC_DEBUG( "idx = %d publish for %s", idx, m_shName.c_str() );
            SharedRing_Desc_t *pDesc = &m_pRingMem->descs[idx];
            pDesc->ref = (int32_t) m_subscribers.size();
            pDesc->numDataFrames = (uint32_t) data.frames.size();
            for ( uint32_t i = 0; i < pDesc->numDataFrames; i++ )
            {
                pDesc->dataFrames[i].bufDesc = data.GetBuffer( i );
                pDesc->dataFrames[i].frameId = data.FrameId( i );
                pDesc->dataFrames[i].timestamp = data.Timestamp( i );
                pDesc->dataFrames[i].quantScale = data.QuantScale( i );
                pDesc->dataFrames[i].quantOffset = data.QuantOffset( i );
                pDesc->dataFrames[i].SetName( data.Name( i ) );
            }
            for ( auto &it : m_subscribers )
            {
                int32_t slotId = it.first;
                SharedRing_Ring_t *pUsedRing = it.second;
                ret = pUsedRing->Push( idx );
                if ( QC_STATUS_OK == ret )
                {
                    if ( pUsedRing->Size() > pUsedRing->queueDepth )
                    { /* reach the queue depth, release the old one */
                        uint16_t idx2 = 0;
                        QCStatus_e ret2 = pUsedRing->Pop( idx2 );
                        if ( QC_STATUS_OK == ret2 )
                        {
                            ReleaseDesc( idx2 );
                        }
                    }
                    else
                    {
                        ret = m_semUsed[slotId].Post();
                    }
                }
                if ( QC_STATUS_OK != ret )
                {
                    ReleaseDesc( idx );
                    QC_ERROR( "Failed to publish to subscriber %s(%d) for %s: %d", pUsedRing->name,
                              slotId, m_shName.c_str(), ret );
                    /* Note: a disaster here if it failed, now this error was not handled.
                     * When here, it was generally because of the remote subscriber hold the
                     * spinlock and crashed. */
                    break;
                }
            }
        }
        else
        {
            /* release idx back as no subscribers */
            auto ret2 = m_pRingMem->avail.Push( idx );
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "Failed to release %d back as no subscribers", idx );
            }
        }
    }

    return ret;
}

QCStatus_e SharedPublisher::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2;

    m_pRingMem->status = SHARED_RING_MEMORY_DESTROYED;

    for ( int i = 0; i < SHARED_RING_NUM_SUBSCRIBERS; i++ )
    {
        ret2 = m_semUsed[i].Destroy();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "Failed to destroy semaphore %s_used%d", m_shName.c_str(), i );
            ret = ret2;
        }
    }

    ret2 = m_semFree.Destroy();
    if ( QC_STATUS_OK != ret2 )
    {
        QC_ERROR( "Failed to destroy semaphore %s_free", m_shName.c_str() );
        ret = ret2;
    }

    ret2 = m_shmem.Destroy();
    if ( QC_STATUS_OK != ret2 )
    {
        QC_ERROR( "Failed to destroy shared memory %s", m_shName.c_str() );
        ret = ret2;
    }

    m_pRingMem = nullptr;

    (void) QC_LOGGER_DEINIT();

    m_shName = "";

    return ret;
}

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC
