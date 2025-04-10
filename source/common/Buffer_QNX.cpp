// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/common/BufferManager.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include <pmem.h>

namespace ridehal
{
namespace common
{

static uint32_t s_usageToPMemID[RIDEHAL_BUFFER_USAGE_MAX] = {
        PMEM_DMA_ID,                  /* RIDEHAL_BUFFER_USAGE_DEFAULT */
        PMEM_CAMERA_ID,               /* RIDEHAL_BUFFER_USAGE_CAMERA */
        PMEM_GRAPHICS_FRAMEBUFFER_ID, /* RIDEHAL_BUFFER_USAGE_GPU */
        PMEM_VIDEO_ID,                /* RIDEHAL_BUFFER_USAGE_VPU */
        PMEM_EVA_ID,                  /* RIDEHAL_BUFFER_USAGE_EVA */
        PMEM_DSP_ID                   /* RIDEHAL_BUFFER_USAGE_HTP */
};

static void __attribute__( ( constructor ) ) QnxPMemHeapInit( void )
{
    /* for pmem_map_handle_v2, must ensure pmem_init is done */
    (void) pmem_init();
}

RideHalError_e RideHal_DmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size,
                                    RideHal_BufferFlags_t flags, RideHal_BufferUsage_e usage )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    uint32_t pmemID = PMEM_DMA_ID;
    pmem_handle_t pmemHandle = nullptr;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) )
    {
        RIDEHAL_LOG_ERROR( "DmaAllocate with pData or pDmaHandle is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        /* convert ride hal flags to the PMEM flags */
        if ( 0 != ( flags & RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            pmemFlags |= PMEM_FLAGS_CACHE_WB_WA;
        }

        /* convert ride hal usage to the PMEM ID */
        if ( ( usage < RIDEHAL_BUFFER_USAGE_MAX ) && ( usage >= RIDEHAL_BUFFER_USAGE_DEFAULT ) )
        {
            pmemID = s_usageToPMemID[usage];
        }
        else
        {
            RIDEHAL_LOG_ERROR( "DmaAllocate with invalid usage: %d", usage );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        *pData = pmem_malloc_ext_v2( size, pmemID, pmemFlags, PMEM_ALIGNMENT_4K, 0x0, &pmemHandle,
                                     NULL );
        if ( nullptr == *pData )
        {
            RIDEHAL_LOG_ERROR( "DmaAllocate allocate failed" );
            ret = RIDEHAL_ERROR_NOMEM;
        }
        else
        {
            *pDmaHandle = static_cast<uint64_t>( (uintptr_t) pmemHandle );
        }
    }

    return ret;
}

RideHalError_e RideHal_DmaFree( void *pData, uint64_t dmaHandle, size_t size )
{
    int rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pData )
    {
        RIDEHAL_LOG_ERROR( "DmaFree with pData is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        rc = pmem_free( pData );
        if ( 0 != rc )
        {
            RIDEHAL_LOG_ERROR( "DmaFree failed to do free for buffer %p: %d", pData, rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e RideHal_DmaImport( void **pData, uint64_t *pDmaHandle, uint64_t pid,
                                  uint64_t dmaHandle, size_t size, RideHal_BufferFlags_t flags,
                                  RideHal_BufferUsage_e usage )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    uint32_t pmemID = PMEM_DMA_ID;
    uint32_t pemeSize = 0;
    void *pAddr;
    pmem_handle_t pmemHandle = (pmem_handle_t) dmaHandle;

    (void) pid;

    if ( ( nullptr == pData ) || ( nullptr == pDmaHandle ) || ( 0 == dmaHandle ) )
    {
        RIDEHAL_LOG_ERROR( "DmaImport with invalid pData or dmaHandle" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        /* convert ride hal flags to the PMEM flags */
        if ( 0 != ( flags & RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            pmemFlags |= PMEM_FLAGS_CACHE_WB_WA;
        }

        /* convert ride hal usage to the PMEM ID */
        if ( ( usage < RIDEHAL_BUFFER_USAGE_MAX ) && ( usage >= RIDEHAL_BUFFER_USAGE_DEFAULT ) )
        {
            pmemID = s_usageToPMemID[usage];
        }
        else
        {
            RIDEHAL_LOG_ERROR( "DmaImport with invalid usage: %d", usage );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        pAddr = pmem_map_handle_v2( pmemHandle, &pemeSize, pmemFlags, pmemID );
        if ( nullptr == pAddr )
        {
            RIDEHAL_LOG_ERROR( "DmaImport map failed" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else if ( pemeSize < size )
        {
            RIDEHAL_LOG_ERROR( "DmaImport size mismatch: %" PRIu32 " < %" PRIu64, pemeSize, size );
            ret = RIDEHAL_ERROR_FAIL;
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

RideHalError_e RideHal_DmaUnImport( void *pData, uint64_t dmaHandle, size_t size )
{
    int rc = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    pmem_handle_t pmemHandle = (pmem_handle_t) dmaHandle;

    if ( nullptr == pData )
    {
        RIDEHAL_LOG_ERROR( "DmaUnImport with pData is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        rc = pmem_unmap_handle( pmemHandle, pData );
        if ( 0 != rc )
        {
            RIDEHAL_LOG_ERROR( "DmaUnImport failed to do unmap for buffer %p: %d", pData, rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}
}   // namespace common
}   // namespace ridehal
