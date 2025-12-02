// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/PMEMUtils.hpp"
#include <pmem.h>

namespace QC
{
namespace Memory
{

PMEMUtils::PMEMUtils()
{
    (void) QC_LOGGER_INIT( "PMEMUtils", LOGGER_LEVEL_VERBOSE );
}

PMEMUtils::~PMEMUtils()
{
    (void) (void) QC_LOGGER_DEINIT();
}

QCStatus_e PMEMUtils::MemoryMap( const QCBufferDescriptorBase_t &orig,
                                 QCBufferDescriptorBase_t &mapped )
{
    QCStatus_e status = QC_STATUS_OK;
    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    uint32_t pmemID = PMEM_DMA_ID;
    mapped = orig;
    mapped.pBuf = nullptr;
    mapped.size = 0;

    if ( ( 0 == orig.dmaHandle ) || ( 0 == orig.size ) )
    {
        QC_LOG_ERROR( "invalid dmaHandle or size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == status )
    {
        /* convert ride hal flags to the PMEM flags */
        if ( ( QC_CACHEABLE == orig.cache ) || ( QC_CACHEABLE_WRITE_THROUGH == orig.cache ) ||
             ( QC_CACHEABLE_WRITE_BACK == orig.cache ) )
        {
            pmemFlags |= PMEM_FLAGS_CACHE_WB_WA;
        }

        /* convert ride hal usage to the PMEM ID */
        if ( ( orig.allocatorType < QC_MEMORY_ALLOCATOR_LAST ) &&
             ( orig.allocatorType >= QC_MEMORY_ALLOCATOR_DMA ) )
        {
            pmemID = static_cast<uint32_t>( orig.allocatorType - QC_MEMORY_ALLOCATOR_DMA );
        }
        else
        {
            QC_ERROR( "Invalid Allocator type : %d", orig.allocatorType );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == status )
    {
        uint32_t pemeSize = 0;
        pmem_handle_t pmemHandle = (pmem_handle_t) orig.dmaHandle;
        void *pAddr = pmem_map_handle_v2( pmemHandle, &pemeSize, pmemFlags, pmemID );
        if ( nullptr == pAddr )
        {
            QC_ERROR( "pmem_map_handle_v2 failed" );
            status = QC_STATUS_FAIL;
        }
        else if ( pemeSize < orig.size )
        {
            QC_LOG_ERROR( " size mismatch: %" PRIu32 " < %" PRIu64, pemeSize, orig.size );
            status = QC_STATUS_FAIL;
            (void) pmem_unmap_handle( pmemHandle, pAddr );
        }
        else
        {
            mapped.pBuf = pAddr;
            mapped.size = pemeSize;
            mapped.dmaHandle = orig.dmaHandle;
        }
    }

    return status;
}

QCStatus_e PMEMUtils::MemoryUnMap( const QCBufferDescriptorBase_t &buff )
{
    QCStatus_e status = QC_STATUS_OK;
    pmem_handle_t pmemHandle = (pmem_handle_t) buff.dmaHandle;

    if ( nullptr == buff.pBuf )
    {
        QC_ERROR( "nullptr == buff.pBuf" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == buff.dmaHandle )
    {
        QC_ERROR( "0 == buff.dmaHandle" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        int rc = pmem_unmap_handle( pmemHandle, buff.pBuf );
        if ( 0 != rc )
        {
            QC_ERROR( "failed to do unmap for buffer %p: %d", buff.pBuf, rc );
            status = QC_STATUS_FAIL;
        }
    }

    return status;
}


}   // namespace Memory
}   // namespace QC
