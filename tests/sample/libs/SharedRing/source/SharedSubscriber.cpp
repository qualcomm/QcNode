// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/shared_ring/SharedSubscriber.hpp"
#include <chrono>
using namespace std::chrono_literals;

namespace ridehal
{
namespace sample
{
namespace shared_ring
{

#define WAIT_PUBLISHER_ONLINE_TIMEOUT_MS ( 100 * 1000 )

SharedSubscriber::SharedSubscriber() {}
SharedSubscriber::~SharedSubscriber() {}

RideHalError_e SharedSubscriber::Init( std::string name, std::string topicName,
                                       uint32_t queueDepth )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_name = name;
    m_shName = topicName;
    m_queueDepth = queueDepth;

    std::replace( m_shName.begin(), m_shName.end(), '/', '_' );

    ret = RIDEHAL_LOGGER_INIT( name.c_str(), LOGGER_LEVEL_INFO );
    ret = RIDEHAL_ERROR_NONE; /* ignore logger init error */

    return ret;
}

RideHalError_e SharedSubscriber::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t timeout = WAIT_PUBLISHER_ONLINE_TIMEOUT_MS;
    bool bShmemOpened = false;
    bool bSemFreeOpened = false;

    if ( m_shName.empty() )
    {
        RIDEHAL_ERROR( "Not initialized" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        while ( timeout > 0 )
        {
            ret = m_shmem.Open( m_shName );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                timeout--;
                std::this_thread::sleep_for( 1ms );
            }
            else
            {
                bShmemOpened = true;
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_pRingMem = (SharedRing_Memory_t *) m_shmem.Data();
        while ( SHARED_RING_MEMORY_INITIALIZED != m_pRingMem->status )
        {
            if ( timeout > 0 )
            {
                timeout--;
                std::this_thread::sleep_for( 1ms );
            }
            else
            {
                RIDEHAL_ERROR( "shared memory %s not in initialized status", m_shName.c_str() );
                ret = RIDEHAL_ERROR_TIMEOUT;
            }
        }
    }
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( m_shmem.Size() != sizeof( SharedRing_Memory_t ) )
        {
            RIDEHAL_ERROR( "shared memory %s size is not correct: %" PRIu64 "!=%" PRIu64,
                           m_shName.c_str(), m_shmem.Size(), sizeof( SharedRing_Memory_t ) );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_semFree.Open( m_shName + "_free" );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to open semaphore %s_free", m_shName.c_str() );
        }
        else
        {
            bSemFreeOpened = true;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( int32_t i = 0; i < SHARED_RING_NUM_SUBSCRIBERS; i++ )
        {
            SharedRing_Ring_t *pUsedRing = &m_pRingMem->used[i];
            if ( false == __atomic_exchange_n( &pUsedRing->reserved, true, __ATOMIC_ACQUIRE ) )
            {
                m_slotId = i;
                m_pUsedRing = pUsedRing;
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_semUsed.Open( m_shName + "_used" + std::to_string( m_slotId ) );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to open semaphore %s_used%d", m_shName.c_str(), m_slotId );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "publisher %s online for %s, slot %d", m_pRingMem->name, m_shName.c_str(),
                      m_slotId );
        m_pUsedRing->SetName( m_name );
        m_pUsedRing->queueDepth = m_queueDepth;
        __atomic_store_n( &m_pUsedRing->status, SHARED_RING_USED_INITIALIZED, __ATOMIC_RELAXED );
        m_bStarted = true;
        m_thread = std::thread( &SharedSubscriber::ThreadMain, this );
    }
    else
    {
        /* do error cleanup */
        if ( nullptr != m_pUsedRing )
        {
            __atomic_store_n( &m_pUsedRing->reserved, false, __ATOMIC_RELEASE );
            m_pUsedRing = nullptr;
            m_slotId = -1;
        }

        if ( bSemFreeOpened )
        {
            (void) m_semFree.Close();
        }

        if ( bShmemOpened )
        {
            m_shmem.Close();
        }
    }

    return ret;
}

void SharedSubscriber::ThreadMain()
{
    while ( m_bStarted )
    {
        std::this_thread::sleep_for( 10ms );
    }
}

void SharedSubscriber::ReleaseDesc( uint16_t idx )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    SharedRing_Desc_t *pDesc = &m_pRingMem->descs[idx];

    RIDEHAL_DEBUG( "release idx %d", idx );
    int32_t ref = __atomic_sub_fetch( &pDesc->ref, 1, __ATOMIC_RELAXED );
    if ( 0 == ref )
    {
        RIDEHAL_DEBUG( "free idx %d", idx );
        ret = m_pRingMem->free.Push( idx );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = m_semFree.Post();
            if ( false == m_bStarted )
            {
                ret = RIDEHAL_ERROR_NONE; /* ignore semaphore post error after stop */
            }
        }
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "failed to release idx %d", idx );
        }
    }
    else if ( ref < 0 )
    {
        RIDEHAL_ERROR( "desc %d ref underflow", idx );
        /* Note: this is a diaster if reach here, this error now was not handled */
    }
    else
    {
        /* still hold by others */
    }
}

void SharedSubscriber::ReleaseSharedBuffer( SharedBuffer_t *pSharedBuffer )
{
    uint16_t idx = (uint16_t) pSharedBuffer->pubHandle;
    std::lock_guard<std::mutex> l( m_lock );
    auto it = m_dataFrames.find( idx );
    if ( it != m_dataFrames.end() )
    {
        int32_t ref = __atomic_sub_fetch( &it->second.ref, 1, __ATOMIC_RELAXED );
        if ( 0 == ref )
        {
            ReleaseDesc( idx );
        }
        else if ( ref < 0 )
        {
            RIDEHAL_ERROR( "data frame %d ref underflow", idx );
            /* Note: this is a diaster if reach here, this error now was not handled */
        }
        else
        {
            /* some of the shared buffer still in use */
        }
    }
    else
    {
        RIDEHAL_ERROR( "idx %d not found", idx );
    }
}

RideHalError_e SharedSubscriber::Receive( DataFrames_t &dataFrames, uint32_t timeoutMs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint16_t idx = 0;

    if ( false == m_bStarted )
    {
        RIDEHAL_ERROR( "Not started" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_semUsed.Wait( timeoutMs );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pUsedRing->Pop( idx );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            if ( ( idx >= SHARED_RING_NUM_DESC ) || ( idx < 0 ) )
            {
                RIDEHAL_ERROR( "get an invalid idx: %d", idx );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "idx = %d received for %s", idx, m_shName.c_str() );
            SharedRing_Desc_t *pDesc = &m_pRingMem->descs[idx];
            {
                std::lock_guard<std::mutex> l( m_lock );
                m_dataFrames[idx] = { (int32_t) pDesc->numDataFrames, {} };
                m_dataFrames[idx].bufs.resize( pDesc->numDataFrames );
            }
            for ( uint32_t i = 0; i < pDesc->numDataFrames; i++ )
            {
                DataFrame_t frame;
                RideHal_SharedBuffer_t sbuf;
                uint64_t dmaHandle = pDesc->dataFrames[i].buf.buffer.dmaHandle;
                auto it = m_buffers.find( dmaHandle );
                if ( m_buffers.end() == it )
                {
                    ret = sbuf.Import( &pDesc->dataFrames[i].buf );
                    if ( RIDEHAL_ERROR_NONE == ret )
                    {
                        m_buffers[dmaHandle] = sbuf;
                    }
                    else
                    {
                        RIDEHAL_ERROR( "Failed to mmap for %s: %d", m_shName.c_str(), ret );
                        /* Note: a disaster here if it failed, now this error was not handled. */
                        break;
                    }
                }
                else
                {
                    sbuf = it->second;
                }

                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    SharedBuffer_t *pSharedBuffer;
                    {
                        std::lock_guard<std::mutex> l( m_lock );
                        pSharedBuffer = &m_dataFrames[idx].bufs[i];
                    }
                    pSharedBuffer->sharedBuffer = sbuf;
                    pSharedBuffer->pubHandle = idx;
                    std::shared_ptr<SharedBuffer_t> buffer(
                            pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
                                ReleaseSharedBuffer( pSharedBuffer );
                            } );
                    frame.frameId = pDesc->dataFrames[i].frameId;
                    frame.buffer = buffer;
                    frame.timestamp = pDesc->dataFrames[i].timestamp;
                    frame.quantScale = pDesc->dataFrames[i].quantScale;
                    frame.quantOffset = pDesc->dataFrames[i].quantOffset;
                    frame.name = pDesc->dataFrames[i].name;
                    dataFrames.Add( frame );
                }
            }
        }
    }

    return ret;
}

RideHalError_e SharedSubscriber::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHalError_e ret2;

    if ( false == m_bStarted )
    {
        RIDEHAL_ERROR( "Not started" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        m_bStarted = false;
        if ( m_thread.joinable() )
        {
            m_thread.join();
        }

        do
        {
            uint16_t idx = 0;
            ret2 = m_pUsedRing->Pop( idx );
            if ( RIDEHAL_ERROR_NONE == ret2 )
            {
                ReleaseDesc( idx );
            }
        } while ( RIDEHAL_ERROR_NONE == ret2 );
    }

    return ret;
}


RideHalError_e SharedSubscriber::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHalError_e ret2;

    if ( m_bStarted )
    {
        RIDEHAL_ERROR( "Not stopped" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        /* depends on the publisher's server thread to detect this and do the resource reclaim if
         * there is any left */
        __atomic_store_n( &m_pUsedRing->status, SHARED_RING_USED_DESTROYED, __ATOMIC_RELAXED );

        ret2 = m_semUsed.Close();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "Failed to close semaphore %s_used%d", m_shName.c_str(), m_slotId );
            ret = ret2;
        }

        ret2 = m_semFree.Close();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "Failed to close semaphore %s_free", m_shName.c_str() );
            ret = ret2;
        }

        ret2 = m_shmem.Close();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "Failed to close shared memory %s", m_shName.c_str() );
            ret = ret2;
        }

        m_pRingMem = nullptr;
        m_pUsedRing = nullptr;
        m_slotId = -1;
        (void) RIDEHAL_LOGGER_DEINIT();
        m_name = "";
        m_shName = "";
    }

    return ret;
}

}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal
