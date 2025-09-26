// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/sample/SharedBufferPool.hpp"
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

SharedBufferPool::SharedBufferPool() {}

QCStatus_e SharedBufferPool::Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level,
                                   uint32_t number )
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

    m_pBufMgr = BufferManager::Get( nodeId, level );
    if ( nullptr == m_pBufMgr )
    {
        QC_ERROR( "Failed to get buffer manager for %s %d: %s!", nodeId.name.c_str(), nodeId.id,
                  name.c_str() );
        ret = QC_STATUS_FAIL;
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
    std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> buffers;
    ret = GetBuffers( buffers );
    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::RegisterBuffers( m_name, buffers );
    }
    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level,
                                   uint32_t number, uint32_t width, uint32_t height,
                                   QCImageFormat_e format, QCMemoryAllocator_e allocatorType,
                                   QCAllocationCache_e cache )
{
    QCStatus_e ret = Init( name, nodeId, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        ImageDescriptor_t &imgDesc = m_queue[idx].sharedBuffer.imgDesc;
        ret = m_pBufMgr->Allocate( ImageBasicProps_t( width, height, format, allocatorType, cache ),
                                   imgDesc );
        QC_DEBUG( "%s image[%u] %ux%u %d allocated with size=%u data=%p handle=%llu ret=%d",
                  m_name.c_str(), idx, width, height, format, imgDesc.size, imgDesc.pBuf,
                  imgDesc.dmaHandle, ret );
        m_queue[idx].sharedBuffer.imgDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.sharedBuffer = imgDesc;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.imgDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level,
                                   uint32_t number, uint32_t batchSize, uint32_t width,
                                   uint32_t height, QCImageFormat_e format,
                                   QCMemoryAllocator_e allocatorType, QCAllocationCache_e cache )
{
    QCStatus_e ret = Init( name, nodeId, level, number );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        ImageDescriptor_t &imgDesc = m_queue[idx].sharedBuffer.imgDesc;
        ret = m_pBufMgr->Allocate(
                ImageBasicProps_t( batchSize, width, height, format, allocatorType, cache ),
                imgDesc );
        QC_DEBUG( "%s image[%u] %u %ux%u %d allocated with size=%u data=%p handle=%llu ret=%d",
                  m_name.c_str(), idx, batchSize, width, height, format, imgDesc.size, imgDesc.pBuf,
                  imgDesc.dmaHandle, ret );
        m_queue[idx].sharedBuffer.imgDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.sharedBuffer = imgDesc;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.imgDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level,
                                   uint32_t number, const QCImageProps_t &imageProps,
                                   QCMemoryAllocator_e allocatorType, QCAllocationCache_e cache )
{
    ImageProps_t imgProp;
    QCStatus_e ret = Init( name, nodeId, level, number );

    imgProp.format = imageProps.format;
    imgProp.batchSize = imageProps.batchSize;
    imgProp.width = imageProps.width;
    imgProp.height = imageProps.height;
    imgProp.allocatorType = allocatorType;
    imgProp.cache = cache;
    std::copy( imageProps.stride, imageProps.stride + imageProps.numPlanes, imgProp.stride );
    std::copy( imageProps.actualHeight, imageProps.actualHeight + imageProps.numPlanes,
               imgProp.actualHeight );
    std::copy( imageProps.planeBufSize, imageProps.planeBufSize + imageProps.numPlanes,
               imgProp.planeBufSize );
    imgProp.numPlanes = imageProps.numPlanes;

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        ImageDescriptor_t &imgDesc = m_queue[idx].sharedBuffer.imgDesc;
        ret = m_pBufMgr->Allocate( imgProp, imgDesc );
        QC_DEBUG( "%s image[%u] %u %ux%u %d allocated with size=%u data=%p handle=%llu ret=%d",
                  m_name.c_str(), idx, imgProp.batchSize, imgProp.width, imgProp.height,
                  imgProp.format, imgDesc.size, imgDesc.pBuf, imgDesc.dmaHandle, ret );
        m_queue[idx].sharedBuffer.imgDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.sharedBuffer = imgDesc;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.imgDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level,
                                   uint32_t number, const QCTensorProps_t &tensorProps,
                                   QCMemoryAllocator_e allocatorType, QCAllocationCache_e cache )
{
    TensorProps_t tensorProp;
    QCStatus_e ret = Init( name, nodeId, level, number );
    tensorProp.tensorType = tensorProps.type;
    tensorProp.numDims = tensorProps.numDims;
    tensorProp.allocatorType = allocatorType;
    tensorProp.cache = cache;
    std::copy( tensorProps.dims, tensorProps.dims + tensorProps.numDims, tensorProp.dims );

    for ( uint32_t idx = 0; ( idx < m_queue.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        TensorDescriptor_t &tensorDesc = m_queue[idx].sharedBuffer.tensorDesc;
        ret = m_pBufMgr->Allocate( tensorProp, tensorDesc );
        QC_DEBUG( "%s tensor[%u] allocated with size=%u data=%p handle=%llu ret=%d\n",
                  m_name.c_str(), idx, tensorDesc.size, tensorDesc.pBuf, tensorDesc.dmaHandle,
                  ret );
        m_queue[idx].sharedBuffer.tensorDesc.name = name + "." + std::to_string( idx );
        m_queue[idx].sharedBuffer.sharedBuffer = tensorDesc;
        m_queue[idx].sharedBuffer.buffer = m_queue[idx].sharedBuffer.tensorDesc;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bIsInited = true;
        (void) Register();
    }

    return ret;
}

QCStatus_e SharedBufferPool::GetBuffers(
        std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bIsInited )
    {
        QC_ERROR( "%s pool is not inited", m_name.c_str() );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        for ( size_t idx = 0; idx < m_queue.size(); idx++ )
        {
            buffers.push_back( m_queue[idx].sharedBuffer.GetBuffer() );
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
            QCBufferDescriptorBase_t &bufDesc = m_queue[idx].sharedBuffer.buffer;
            ret2 = m_pBufMgr->Free( bufDesc );
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "free %u failed: %d", idx, ret2 );
                ret = ret2;
            }
        }
        m_queue.clear();
        m_bIsInited = false;
        BufferManager::Put( m_pBufMgr );
        m_pBufMgr = nullptr;
    }

    return ret;
}

SharedBufferPool::~SharedBufferPool()
{
    for ( uint32_t idx = 0; idx < m_queue.size(); idx++ )
    {
        /* It's safe to call Free without Allocate, just ignore the return error */
        QCBufferDescriptorBase_t &bufDesc = m_queue[idx].sharedBuffer.buffer;
        if ( nullptr != m_pBufMgr )
        {
            (void) m_pBufMgr->Free( bufDesc );
        }
    }

    if ( nullptr != m_pBufMgr )
    {
        BufferManager::Put( m_pBufMgr );
    }
}
}   // namespace sample
}   // namespace QC
