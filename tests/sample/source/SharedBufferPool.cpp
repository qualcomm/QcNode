// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SharedBufferPool.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC;

namespace QC
{
namespace sample
{

SharedBufferPool::SharedBufferPool() {}

QCStatus_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number )
{
    QCStatus_e ret = QC_STATUS_OK;

    (void) QC_LOGGER_INIT( name.c_str(), level );

    m_name = name;
    m_queue.resize( number );
    QC_DEBUG( "Pool %s inited with queue size %u", m_name.c_str(), number );
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
        QC_ERROR( "(Should not be here) %s pool is not inited", m_name.c_str() );
        return nullptr;
    }

    uint32_t idx = 0;
    auto it = m_queue.begin();
    for ( ; it != m_queue.end() && __atomic_load_n( &it->dirty, __ATOMIC_RELAXED ); it++, idx++ )
    {
    }

    if ( it == m_queue.end() )
    {
        QC_ERROR( "(Should not be here) All buffer are in use. Pool name %s", m_name.c_str() );
        return nullptr;
    }
    __atomic_store_n( &it->dirty, true, __ATOMIC_RELAXED );

    QC_DEBUG( "Marked %s buffer %u in use", m_name.c_str(), idx );

    std::shared_ptr<SharedBuffer_t> ptr( &it->sharedBuffer,
                                         [&]( SharedBuffer_t *p ) { Deleter( p ); } );

    return ptr;
}

void SharedBufferPool::Deleter( SharedBuffer_t *ptrToDelete )
{
    if ( !ptrToDelete )
    {
        QC_ERROR( "(Should not be here) Found invalid pointerf for %s", m_name.c_str() );
        return;
    }

    if ( ptrToDelete->pubHandle >= m_queue.size() )
    {
        QC_ERROR( "(Should not be here) %s index out of range", m_name.c_str() );
        return;
    }

    __atomic_store_n( &m_queue[ptrToDelete->pubHandle].dirty, false, __ATOMIC_RELAXED );

    QC_DEBUG( "Marked %s buffer %llu available", m_name.c_str(), ptrToDelete->pubHandle );
}

QCStatus_e SharedBufferPool::Register( void )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::vector<QCSharedBuffer_t> buffers;
    buffers.resize( m_queue.size() );
    ret = GetBuffers( buffers.data(), m_queue.size() );
    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::RegisterBuffers( m_name, buffers.data(), m_queue.size() );
    }
    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                   uint32_t width, uint32_t height, QCImageFormat_e format,
                                   QCBufferUsage_e usage, QCBufferFlags_t flags )
{
    QCStatus_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        QCSharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( width, height, format, usage, flags );
        QC_DEBUG( "%s image[%u] %ux%u allocated with size=%u data=%p handle=%llu ret=%d\n",
                  m_name.c_str(), idx, width, height, sharedBuffer.size, sharedBuffer.data(),
                  sharedBuffer.buffer.dmaHandle, ret );
        m_queue[idx].sharedBuffer.imgDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.imgDesc = sharedBuffer;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.imgDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                   uint32_t batchSize, uint32_t width, uint32_t height,
                                   QCImageFormat_e format, QCBufferUsage_e usage,
                                   QCBufferFlags_t flags )
{
    QCStatus_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        QCSharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( batchSize, width, height, format, usage, flags );
        QC_DEBUG( "%s image[%u] %u %ux%u allocated with size=%u data=%p handle=%llu ret=%d\n",
                  m_name.c_str(), idx, batchSize, width, height, sharedBuffer.size,
                  sharedBuffer.data(), sharedBuffer.buffer.dmaHandle, ret );
        m_queue[idx].sharedBuffer.imgDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.imgDesc = sharedBuffer;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.imgDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                   const QCImageProps_t &imageProps, QCBufferUsage_e usage,
                                   QCBufferFlags_t flags )
{
    QCStatus_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        QCSharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( &imageProps, usage, flags );
        QC_DEBUG( "%s image[%u] allocated with size=%u data=%p handle=%llu ret=%d\n",
                  m_name.c_str(), idx, sharedBuffer.size, sharedBuffer.data(),
                  sharedBuffer.buffer.dmaHandle, ret );
        m_queue[idx].sharedBuffer.imgDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.imgDesc = sharedBuffer;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.imgDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, Logger_Level_e level, uint32_t number,
                                   const QCTensorProps_t &tensorProps, QCBufferUsage_e usage,
                                   QCBufferFlags_t flags )
{
    QCStatus_e ret = Init( name, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        QCSharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        ret = sharedBuffer.Allocate( &tensorProps, usage, flags );
        QC_DEBUG( "%s tensor[%u] allocated with size=%u data=%p handle=%llu ret=%d\n",
                  m_name.c_str(), idx, sharedBuffer.size, sharedBuffer.data(),
                  sharedBuffer.buffer.dmaHandle, ret );
        m_queue[idx].sharedBuffer.tensorDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.tensorDesc = sharedBuffer;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.tensorDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::GetBuffers( QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bIsInited )
    {
        QC_ERROR( "%s pool is not inited", m_name.c_str() );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "pBuffers is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_queue.size() != (size_t) numBuffers )
    {
        QC_ERROR( "numBuffers is not equal to %" PRIu64, m_queue.size() );
        ret = QC_STATUS_BAD_ARGUMENTS;
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

QCStatus_e SharedBufferPool::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2;

    if ( false == m_bIsInited )
    {
        QC_ERROR( "%s pool is not inited", m_name.c_str() );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        (void) SampleIF::DeRegisterBuffers( m_name );
        for ( uint32_t idx = 0; idx < m_queue.size(); idx++ )
        {
            QCSharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
            ret2 = sharedBuffer.Free();
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "free %u failed: %d", idx, ret2 );
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
        QCSharedBuffer_t &sharedBuffer = m_queue[idx].sharedBuffer.sharedBuffer;
        (void) sharedBuffer.Free();
    }
}
}   // namespace sample
}   // namespace QC
