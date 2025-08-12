// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"

#include <fcntl.h>
#include <linux/dma-heap.h>
#include <mutex>
#include <plat_dmabuf.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace QC
{
namespace Memory
{

static std::mutex s_heapMutex;
static int s_heapDevFd[ID_DMA_BUF_HEAP_CMA] = { -1, -1 };

static int GetLinuxHeapDevFd( heap_type heapType )
{
    int devFd = -1;
    std::lock_guard<std::mutex> guard( s_heapMutex );
    if ( -1 == s_heapDevFd[heapType] )
    {
        devFd = dmabufheap_init( heapType );
        if ( devFd >= 0 )
        {
            s_heapDevFd[heapType] = devFd;
        }
    }
    else
    {
        devFd = s_heapDevFd[heapType];
    }

    return devFd;
}

static void __attribute__( ( destructor ) ) LinuxHeapDevFdCleanUp( void )
{
    /* no mutex lock as the s_heapMutex maybe destroied */
    /* clean up */
    if ( s_heapDevFd[ID_DMA_BUF_HEAP_CACHED] >= 0 )
    {
        dmabufheap_release( s_heapDevFd[ID_DMA_BUF_HEAP_CACHED] );
        s_heapDevFd[ID_DMA_BUF_HEAP_CACHED] = -1;
    }

    if ( s_heapDevFd[ID_DMA_BUF_HEAP_UNCACHED] >= 0 )
    {
        dmabufheap_release( s_heapDevFd[ID_DMA_BUF_HEAP_UNCACHED] );
        s_heapDevFd[ID_DMA_BUF_HEAP_UNCACHED] = -1;
    }
}

QCStatus_e QCDmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size, QCBufferFlags_t flags,
                          QCBufferUsage_e usage )
{
    QCStatus_e ret = QC_STATUS_OK;
    heap_type heapType = ID_DMA_BUF_HEAP_UNCACHED;
    void *pAddr = nullptr;
    int fd = -1;
    int devFd = -1;
    int rc = 0;
    (void) usage;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) )
    {
        QC_LOG_ERROR( "DmaAllocate with pData or pDmaHandle is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        /* convert ride hal flags to the dma buf heap type */
        if ( 0 != ( flags & QC_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            heapType = ID_DMA_BUF_HEAP_CACHED;
        }

        if ( ( usage < QC_BUFFER_USAGE_MAX ) && ( usage >= QC_BUFFER_USAGE_DEFAULT ) )
        {
            /* usage OK but not used */
        }
        else
        {
            QC_LOG_ERROR( "DmaAllocate with invalid usage: %d", usage );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        devFd = GetLinuxHeapDevFd( heapType );
        if ( devFd < 0 )
        {
            QC_LOG_ERROR( "DmaAllocate failed to do dmabuf heap init: %d", devFd );
            ret = QC_STATUS_UNSUPPORTED;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        rc = dmabufheap_alloc( devFd, size, 0, &fd );
        if ( rc < 0 )
        {
            QC_LOG_ERROR( "DmaAllocate failed to do dmabuf heap alloc: %d", rc );
            ret = QC_STATUS_NOMEM;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        pAddr = mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
        if ( ( nullptr != pAddr ) && ( MAP_FAILED != pAddr ) )
        {
            *pData = pAddr;
            *pDmaHandle = static_cast<uint64_t>( fd );
        }
        else
        {
            QC_LOG_ERROR( "DmaAllocate failed to mmap: %d", errno );
            ret = QC_STATUS_FAIL;
            close( fd );
        }
    }


    return ret;
}

QCStatus_e QCDmaFree( void *pData, uint64_t dmaHandle, size_t size )
{
    QCStatus_e ret = QC_STATUS_OK;
    int fd = static_cast<int>( dmaHandle );
    int rc = 0;

    if ( nullptr == pData )
    {
        QC_LOG_ERROR( "DmaFree with pData is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( fd < 0 )
    {
        QC_LOG_ERROR( "DmaFree with invalid dmaHandle" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        rc = munmap( pData, size );
        if ( 0 != rc )
        {
            QC_LOG_ERROR( "DmaFree failed to do munmap for buffer %p: %d", pData, rc );
            ret = QC_STATUS_FAIL;
        }

        rc = close( fd );
        if ( 0 != rc )
        {
            QC_LOG_ERROR( "DmaFree failed to close buffer %" PRIu64 ": %d", dmaHandle, rc );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e QCDmaImport( void **pData, uint64_t *pDmaHandle, pid_t pid, uint64_t dmaHandle,
                        size_t size, QCBufferFlags_t flags, QCBufferUsage_e usage )
{
    int rc = 0;
    heap_type heapType = ID_DMA_BUF_HEAP_UNCACHED;
    int devFd = -1;
    QCStatus_e ret = QC_STATUS_OK;
    int fd = static_cast<int>( dmaHandle );
    int newFd = -1;
    void *pAddr = nullptr;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) || ( fd < 0 ) )
    {
        QC_LOG_ERROR( "DmaImport with invalid pData or dmaHandle" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        /* convert ride hal flags to the dma buf heap type */
        if ( 0 != ( flags & QC_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            heapType = ID_DMA_BUF_HEAP_CACHED;
        }

        if ( ( usage < QC_BUFFER_USAGE_MAX ) && ( usage >= QC_BUFFER_USAGE_DEFAULT ) )
        {
            /* usage OK but not used */
        }
        else
        {
            QC_LOG_ERROR( "DmaImport with invalid usage: %d", usage );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        devFd = GetLinuxHeapDevFd( heapType );
        if ( devFd < 0 )
        {
            QC_LOG_ERROR( "DmaImport failed to do dmabuf heap init: %d", devFd );
            ret = QC_STATUS_UNSUPPORTED;
        }
    }
    if ( QC_STATUS_OK == ret )
    {
        if ( 0 != pid )
        {
            newFd = dmabufheap_import( static_cast<int>( pid ), fd );
            if ( newFd < 0 )
            {
                QC_LOG_ERROR( "DmaImport failed to import dma-buf pid %d fd %d: %d",
                              static_cast<int>( pid ), fd );
                ret = QC_STATUS_FAIL;
            }
        }
        else
        { /* pid is 0, a case that the fd is already imported one */
            newFd = fd;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        pAddr = mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, newFd, 0 );
        if ( ( nullptr == pAddr ) || ( MAP_FAILED == pAddr ) )
        {
            QC_LOG_ERROR( "DmaImport map failed: %d", errno );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            *pData = pAddr;
            *pDmaHandle = static_cast<uint64_t>( newFd );
        }
    }

    return ret;
}

QCStatus_e QCDmaUnImport( void *pData, uint64_t dmaHandle, size_t size )
{
    QCStatus_e ret = QC_STATUS_OK;
    int fd = static_cast<int>( dmaHandle );
    int rc = 0;

    if ( nullptr == pData )
    {
        QC_LOG_ERROR( "DmaUnImport with pData is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( fd < 0 )
    {
        QC_LOG_ERROR( "DmaUnImport with invalid dmaHandle" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        rc = munmap( pData, size );
        if ( 0 != rc )
        {
            QC_LOG_ERROR( "DmaUnImport failed to do munmap for buffer %p: %d", pData, rc );
            ret = QC_STATUS_FAIL;
        }

        rc = close( fd );
        if ( 0 != rc )
        {
            QC_LOG_ERROR( "DmaUnImport failed to close buffer %" PRIu64 ": %d", dmaHandle, rc );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}
}   // namespace Memory
}   // namespace QC
