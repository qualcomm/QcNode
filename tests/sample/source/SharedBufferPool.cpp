// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/sample/SharedBufferPool.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal;

namespace ridehal
{
namespace sample
{

SharedBufferPool::SharedBufferPool() {}

RideHalError_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    (void) RIDEHAL_LOGGER_INIT( name.c_str(), level );

    m_name = name;
    m_queue.resize( number );
    memset( m_queue.data(), 0, m_queue.size() * sizeof( SharedBufferInfo ) );
    RIDEHAL_DEBUG( "Pool %s inited with queue size %u", m_name.c_str(), number );
    for ( uint32_t idx = 0; idx < number; idx++ )
    {
        m_queue[idx].sharedBuffer.pubHandle = idx;
        m_queue[idx].dirty = false;
    }

    return ret;
}

std::shared_ptr<SharedBuffer_t> SharedBufferPool::Get()
{
    if ( false == m_bIsInited )
    {
        RIDEHAL_ERROR( "(Should not be here) %s pool is not inited", m_name.c_str() );
        return nullptr;
    }

    uint32_t idx = 0;
    auto it = m_queue.begin();
    for ( ; it != m_queue.end() && __atomic_load_n( &it->dirty, __ATOMIC_RELAXED ); it++, idx++ )
    {
    }

    if ( it == m_queue.end() )
    {
        RIDEHAL_ERROR( "(Should not be here) All buffer are in use. Pool name %s", m_name.c_str() );
        return nullptr;
    }
    __atomic_store_n( &it->dirty, true, __ATOMIC_RELAXED );

    RIDEHAL_DEBUG( "Marked %s buffer %u in use", m_name.c_str(), idx );

    std::shared_ptr<SharedBuffer_t> ptr( &it->sharedBuffer,
                                         [&]( SharedBuffer_t *p ) { Deleter( p ); } );

    return ptr;
}

void SharedBufferPool::Deleter( SharedBuffer_t *ptrToDelete )
{
    if ( !ptrToDelete )
    {
        RIDEHAL_ERROR( "(Should not be here) Found invalid pointerf for %s", m_name.c_str() );
        return;
    }

    if ( ptrToDelete->pubHandle >= m_queue.size() )
    {
        RIDEHAL_ERROR( "(Should not be here) %s index out of range", m_name.c_str() );
        return;
    }

    __atomic_store_n( &m_queue[ptrToDelete->pubHandle].dirty, false, __ATOMIC_RELAXED );

    RIDEHAL_DEBUG( "Marked %s buffer %llu available", m_name.c_str(), ptrToDelete->pubHandle );
}

RideHalError_e SharedBufferPool::Register( void )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    std::vector<RideHal_SharedBuffer_t> buffers;
    buffers.resize( m_queue.size() );
    ret = GetBuffers( buffers.data(), m_queue.size() );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = SampleIF::RegisterBuffers( m_name, buffers.data(), m_queue.size() );
    }
    return ret;
}

RideHalError_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                       uint32_t width, uint32_t height,
                                       RideHal_ImageFormat_e format, RideHal_BufferUsage_e usage,
                                       RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( RIDEHAL_ERROR_NONE == ret ); idx++ )
    {
        RideHal_SharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( width, height, format, usage, flags );
        RIDEHAL_DEBUG( "%s image[%u] %ux%u allocated with size=%u data=%p handle=%llu ret=%d\n",
                       m_name.c_str(), idx, width, height, sharedBuffer.size, sharedBuffer.data(),
                       sharedBuffer.buffer.dmaHandle, ret );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

RideHalError_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                       uint32_t batchSize, uint32_t width, uint32_t height,
                                       RideHal_ImageFormat_e format, RideHal_BufferUsage_e usage,
                                       RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( RIDEHAL_ERROR_NONE == ret ); idx++ )
    {
        RideHal_SharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( batchSize, width, height, format, usage, flags );
        RIDEHAL_DEBUG( "%s image[%u] %u %ux%u allocated with size=%u data=%p handle=%llu ret=%d\n",
                       m_name.c_str(), idx, batchSize, width, height, sharedBuffer.size,
                       sharedBuffer.data(), sharedBuffer.buffer.dmaHandle, ret );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

RideHalError_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                       const RideHal_ImageProps_t &imageProps,
                                       RideHal_BufferUsage_e usage, RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( RIDEHAL_ERROR_NONE == ret ); idx++ )
    {
        RideHal_SharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( &imageProps, usage, flags );
        RIDEHAL_DEBUG( "%s image[%u] allocated with size=%u data=%p handle=%llu ret=%d\n",
                       m_name.c_str(), idx, sharedBuffer.size, sharedBuffer.data(),
                       sharedBuffer.buffer.dmaHandle, ret );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

RideHalError_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                       const RideHal_TensorProps_t &tensorProps,
                                       RideHal_BufferUsage_e usage, RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( RIDEHAL_ERROR_NONE == ret ); idx++ )
    {
        RideHal_SharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( &tensorProps, usage, flags );
        RIDEHAL_DEBUG( "%s tensor[%u] allocated with size=%u data=%p handle=%llu ret=%d\n",
                       m_name.c_str(), idx, sharedBuffer.size, sharedBuffer.data(),
                       sharedBuffer.buffer.dmaHandle, ret );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

RideHalError_e SharedBufferPool::GetBuffers( RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( false == m_bIsInited )
    {
        RIDEHAL_ERROR( "%s pool is not inited", m_name.c_str() );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "pBuffers is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_queue.size() != (size_t) numBuffers )
    {
        RIDEHAL_ERROR( "numBuffers is not equal to %" PRIu64, m_queue.size() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t idx = 0; idx < numBuffers; idx++ )
        {
            pBuffers[idx] = m_queue[idx].sharedBuffer.sharedBuffer;
        }
    }

    return ret;
}

RideHalError_e SharedBufferPool::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHalError_e ret2;

    if ( false == m_bIsInited )
    {
        RIDEHAL_ERROR( "%s pool is not inited", m_name.c_str() );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        (void) SampleIF::DeRegisterBuffers( m_name );
        for ( uint32_t idx = 0; idx < m_queue.size(); idx++ )
        {
            RideHal_SharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
            ret2 = sharedBuffer.Free();
            if ( RIDEHAL_ERROR_NONE != ret2 )
            {
                RIDEHAL_ERROR( "free %u failed: %d", idx, ret2 );
                ret = ret2;
            }
        }
        m_queue.clear();
        m_bIsInited = false;
    }

    return ret;
}

SharedBufferPool::~SharedBufferPool()
{
    for ( uint32_t idx = 0; idx < m_queue.size(); idx++ )
    {
        /* It's safe to call Free without Allocate, just ignore the return error */
        RideHal_SharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        (void) sharedBuffer.Free();
    }
}
}   // namespace sample
}   // namespace ridehal
