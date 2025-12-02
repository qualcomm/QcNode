// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/DMABUFFUtils.hpp"
#include <linux/dma-heap.h>
#include <plat_dmabuf.h>

namespace QC
{
namespace Memory
{

DMABUFFUtils::DMABUFFUtils()
{
    (void) QC_LOGGER_INIT( "DMABUFFUtils", LOGGER_LEVEL_VERBOSE );
}

DMABUFFUtils::~DMABUFFUtils()
{
    (void) (void) QC_LOGGER_DEINIT();
}

QCStatus_e DMABUFFUtils::MemoryMap( const QCBufferDescriptorBase_t &orig,
                                    QCBufferDescriptorBase_t &mapped )
{
    heap_type heapType = ID_DMA_BUF_HEAP_UNCACHED;
    int devFd = -1;
    QCStatus_e status = QC_STATUS_OK;

    mapped = orig;
    mapped.pBuf = nullptr;
    mapped.size = 0;

    if ( 0 == orig.size )
    {
        QC_ERROR( "0 == buff.size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
    }

    if ( QC_STATUS_OK == status )
    {
        if ( 0 != orig.pid )
        {
            int fd = static_cast<int>( orig.dmaHandle );
            int newFd = dmabufheap_import( static_cast<int>( orig.pid ), fd );
            if ( newFd < 0 )
            {
                QC_ERROR( "DmaImport failed to import dma-buf pid %d fd %d: %d",
                          static_cast<int>( orig.pid ), fd );
                status = QC_STATUS_FAIL;
            }
            else
            {
                mapped.dmaHandle = newFd;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        void *pAddr = mmap( NULL, orig.size, PROT_READ | PROT_WRITE, MAP_SHARED,
                            static_cast<int>( mapped.dmaHandle ), 0 );
        if ( ( nullptr == pAddr ) || ( MAP_FAILED == pAddr ) )
        {
            QC_LOG_ERROR( "mmap failed: %d", errno );
            status = QC_STATUS_FAIL;
        }
        else
        {
            mapped.pBuf = pAddr;
        }
    }

    return status;
}

QCStatus_e DMABUFFUtils::MemoryUnMap( const QCBufferDescriptorBase_t &buff )
{

    QCStatus_e status = QC_STATUS_OK;
    int fd = static_cast<int>( buff.dmaHandle );

    if ( nullptr == buff.pBuf )
    {
        QC_ERROR( "buff.pBuf is nullptr" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( fd < 0 )
    {
        QC_ERROR( "invalid dmaHandle" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == buff.size )
    {
        QC_ERROR( "0 == buff.size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        int rc = munmap( buff.pBuf, buff.size );
        if ( 0 != rc )
        {
            QC_ERROR( "failed to do munmap for buffer %p: %d", buff.pBuf, rc );
            status = QC_STATUS_FAIL;
        }

        rc = close( fd );
        if ( 0 != rc )
        {
            QC_ERROR( "failed to close buffer %" PRIu64 ": %d", buff.dmaHandle, rc );
            status = QC_STATUS_FAIL;
        }
    }

    return status;
}

}   // namespace Memory
}   // namespace QC
