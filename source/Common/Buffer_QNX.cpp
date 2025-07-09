// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#if defined( __QNXNTO__ )
#include "QC/Infras/Memory/BufferManager.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include <pmem.h>

namespace QC
{

static uint32_t s_usageToPMemID[QC_BUFFER_USAGE_MAX] = {
        PMEM_DMA_ID,                  /* QC_BUFFER_USAGE_DEFAULT */
        PMEM_CAMERA_ID,               /* QC_BUFFER_USAGE_CAMERA */
        PMEM_GRAPHICS_FRAMEBUFFER_ID, /* QC_BUFFER_USAGE_GPU */
        PMEM_VIDEO_ID,                /* QC_BUFFER_USAGE_VPU */
        PMEM_EVA_ID,                  /* QC_BUFFER_USAGE_EVA */
        PMEM_DSP_ID                   /* QC_BUFFER_USAGE_HTP */
};

static void __attribute__( ( constructor ) ) QnxPMemHeapInit( void )
{
    /* for pmem_map_handle_v2, must ensure pmem_init is done */
    (void) pmem_init();
}

QCStatus_e QCDmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size, QCBufferFlags_t flags,
                          QCBufferUsage_e usage )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    uint32_t pmemID = PMEM_DMA_ID;
    pmem_handle_t pmemHandle = nullptr;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) )
    {
        QC_LOG_ERROR( "DmaAllocate with pData or pDmaHandle is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        /* convert ride hal flags to the PMEM flags */
        if ( 0 != ( flags & QC_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            pmemFlags |= PMEM_FLAGS_CACHE_WB_WA;
        }

        /* convert ride hal usage to the PMEM ID */
        if ( ( usage < QC_BUFFER_USAGE_MAX ) && ( usage >= QC_BUFFER_USAGE_DEFAULT ) )
        {
            pmemID = s_usageToPMemID[usage];
        }
        else
        {
            QC_LOG_ERROR( "DmaAllocate with invalid usage: %d", usage );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        *pData = pmem_malloc_ext_v2( size, pmemID, pmemFlags, PMEM_ALIGNMENT_4K, 0x0, &pmemHandle,
                                     NULL );
        if ( nullptr == *pData )
        {
            QC_LOG_ERROR( "DmaAllocate allocate failed" );
            ret = QC_STATUS_NOMEM;
        }
        else
        {
            *pDmaHandle = static_cast<uint64_t>( (uintptr_t) pmemHandle );
        }
    }

    return ret;
}

QCStatus_e QCDmaFree( void *pData, uint64_t dmaHandle, size_t size )
{
    int rc = 0;
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pData )
    {
        QC_LOG_ERROR( "DmaFree with pData is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        rc = pmem_free( pData );
        if ( 0 != rc )
        {
            QC_LOG_ERROR( "DmaFree failed to do free for buffer %p: %d", pData, rc );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e QCDmaImport( void **pData, uint64_t *pDmaHandle, uint64_t pid, uint64_t dmaHandle,
                        size_t size, QCBufferFlags_t flags, QCBufferUsage_e usage )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    uint32_t pmemID = PMEM_DMA_ID;
    uint32_t pemeSize = 0;
    void *pAddr;
    pmem_handle_t pmemHandle = (pmem_handle_t) dmaHandle;

    (void) pid;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) || ( 0 == dmaHandle ) )
    {
        QC_LOG_ERROR( "DmaImport with invalid pData or dmaHandle" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        /* convert ride hal flags to the PMEM flags */
        if ( 0 != ( flags & QC_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            pmemFlags |= PMEM_FLAGS_CACHE_WB_WA;
        }

        /* convert ride hal usage to the PMEM ID */
        if ( ( usage < QC_BUFFER_USAGE_MAX ) && ( usage >= QC_BUFFER_USAGE_DEFAULT ) )
        {
            pmemID = s_usageToPMemID[usage];
        }
        else
        {
            QC_LOG_ERROR( "DmaImport with invalid usage: %d", usage );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        pAddr = pmem_map_handle_v2( pmemHandle, &pemeSize, pmemFlags, pmemID );
        if ( nullptr == pAddr )
        {
            QC_LOG_ERROR( "DmaImport map failed" );
            ret = QC_STATUS_FAIL;
        }
        else if ( pemeSize < size )
        {
            QC_LOG_ERROR( "DmaImport size mismatch: %" PRIu32 " < %" PRIu64, pemeSize, size );
            ret = QC_STATUS_FAIL;
            (void) pmem_unmap_handle( pmemHandle, pAddr );
        }
        else
        {
            *pData = pAddr;
            *pDmaHandle = dmaHandle;
        }
    }

    return ret;
}

QCStatus_e QCDmaUnImport( void *pData, uint64_t dmaHandle, size_t size )
{
    int rc = 0;
    QCStatus_e ret = QC_STATUS_OK;
    pmem_handle_t pmemHandle = (pmem_handle_t) dmaHandle;

    if ( nullptr == pData )
    {
        QC_LOG_ERROR( "DmaUnImport with pData is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        rc = pmem_unmap_handle( pmemHandle, pData );
        if ( 0 != rc )
        {
            QC_LOG_ERROR( "DmaUnImport failed to do unmap for buffer %p: %d", pData, rc );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}
}   // namespace QC

#endif /*  __QNXNTO__ */
