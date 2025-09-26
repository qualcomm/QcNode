// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/Infras/Memory/DMABUFFAllocator.hpp"
#include <linux/dma-heap.h>
#include <plat_dmabuf.h>

namespace QC
{
namespace Memory
{

DMABUFFAllocator::DMABUFFAllocator( const QCMemoryAllocatorConfigInit_t &config,
                                    const QCMemoryAllocator_e allocator )
    : QCMemoryAllocatorIfs( config, allocator )
{
    (void) QC_LOGGER_INIT( GetConfiguration().name.c_str(), LOGGER_LEVEL_VERBOSE );
    m_dmaBufDevFdCached = dmabufheap_init( ID_DMA_BUF_HEAP_CACHED );
    if ( m_dmaBufDevFdCached < 0 )
    {
        QC_ERROR( " dmaBufDevFdCached < 0, dmabufheap_init(ID_DMA_BUF_HEAP_CACHED) " );
    }
}

DMABUFFAllocator::~DMABUFFAllocator()
{
    if ( m_dmaBufDevFdCached >= 0 )
    {
        dmabufheap_release( m_dmaBufDevFdCached );
        m_dmaBufDevFdCached = -1;
    }
    (void) QC_LOGGER_DEINIT();
}


QCStatus_e DMABUFFAllocator::Allocate( const QCBufferPropBase_t &request,
                                       QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    response.pBuf = nullptr;
    int fd = -1;

    if ( request.cache != QC_CACHEABLE )
    {
        status = QC_STATUS_UNSUPPORTED;
        QC_ERROR( " request.cache != QC_CACHEABLE, unsupported request" );
    }
    else if ( m_dmaBufDevFdCached < 0 )
    {
        status = QC_STATUS_FAIL;
        QC_ERROR( " m_dmaBufDevFdCached < 0, DMA_BUFF chached device file descriptor not "
                  "initialized" );
    }
    else if ( 0 == request.size )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( " 0 == request.size" );
    }
    else if ( request.alignment > QC_MEMORY_DEFAULT_ALLIGNMENT )
    {
        status = QC_STATUS_UNSUPPORTED;
        QC_ERROR( " request.alignment (%d) > QC_MEMORY_DEFAULT_ALLIGNMENT (%d), lerger than "
                  "supported",
                  request.alignment, QC_MEMORY_DEFAULT_ALLIGNMENT );
    }
    else
    {
        int rc = dmabufheap_alloc( m_dmaBufDevFdCached, request.size, 0, &fd );
        if ( rc < 0 )
        {
            QC_ERROR( "DmaAllocate failed to do dmabuf heap alloc: %d", rc );
            status = QC_STATUS_NOMEM;
        }
        else
        {
            QC_DEBUG( "DmaAllocate allocated fd: %d", fd );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        void *pAddr = mmap( NULL, request.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
        if ( ( nullptr != pAddr ) && ( MAP_FAILED != pAddr ) )
        {
            response.pBuf = pAddr;
            response.dmaHandle = static_cast<uint64_t>( fd );
            response.size = request.size;
            response.cache = QC_CACHEABLE;
            response.allocatorType = GetConfiguration().type;
            response.pid = getpid();
            response.name = GetConfiguration().name + "-" + std::to_string( response.dmaHandle );
            QC_DEBUG( "DmaAllocate allocated fd: %d with pointer %p", fd, pAddr );
        }
        else
        {
            QC_ERROR( "mmap failed to mmap: %d", errno );
            status = QC_STATUS_FAIL;
            close( fd );
        }
    }

    return status;
}

QCStatus_e DMABUFFAllocator::Free( const QCBufferDescriptorBase_t &BufferDescriptor )
{
    QCStatus_e status = QC_STATUS_OK;
    int fd = static_cast<int>( BufferDescriptor.dmaHandle );

    if ( nullptr == BufferDescriptor.pBuf )
    {
        QC_ERROR( "nullptr == BufferDescriptor.pBuf" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_dmaBufDevFdCached < 0 )
    {
        status = QC_STATUS_FAIL;
        QC_ERROR( " m_dmaBufDevFdCached < 0, DMA_BUFF chached device file descriptor not "
                  "initialized" );
    }
    else if ( fd < 0 )
    {
        QC_ERROR( "BufferDescriptor.dmaHandle < 0" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == BufferDescriptor.size )
    {
        QC_ERROR( "BufferDescriptor.size != 0" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        int rc = munmap( BufferDescriptor.pBuf, BufferDescriptor.size );
        if ( 0 != rc )
        {
            QC_ERROR( "munmap failed buffer %p: %d", BufferDescriptor.pBuf, rc );
            status = QC_STATUS_FAIL;
        }

        rc = dmabufheap_free( fd );
        if ( 0 != rc )
        {
            QC_ERROR( "Close fd %ull failed , rc %d", BufferDescriptor.dmaHandle, rc );
            status = QC_STATUS_FAIL;
        }
    }

    return status;
}

}   // namespace Memory
}   // namespace QC
