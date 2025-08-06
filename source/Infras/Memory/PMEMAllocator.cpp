// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/PMEMAllocator.hpp"
#include <pmem.h>
#include <unistd.h>

namespace QC
{
namespace Memory
{


PMEMAllocator::PMEMAllocator( const QCMemoryAllocatorConfigInit_t &config,
                              const QCMemoryAllocator_e allocator )
    : QCMemoryAllocatorIfs( config, allocator )
{
    (void) QC_LOGGER_INIT( GetConfiguration().name.c_str(), LOGGER_LEVEL_VERBOSE );
    pmem_init();
}


PMEMAllocator::~PMEMAllocator()
{
    (void) QC_LOGGER_DEINIT();
    pmem_deinit();
}


QCStatus_e PMEMAllocator::Allocate( const QCBufferPropBase_t &request,
                                    QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    uint32_t pmemId = 0;

    uint32_t pmemFlags = PMEM_FLAGS_CACHE_NONE | PMEM_FLAGS_PHYS_NON_CONTIG | PMEM_FLAGS_SHMEM;
    pmem_handle_t pmemHandle = nullptr;

    if ( QC_CACHEABLE_NON != request.cache )
    {
        pmemFlags |= PMEM_FLAGS_CACHE_WB_WA;
    }

    if ( 0 == request.size )
    {
        QC_LOG_ERROR( "0 == request.size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( QC_STATUS_OK != AllocaotrorToPMEM_ID( GetConfiguration().type, pmemId ) )
    {
        QC_LOG_ERROR( "AllocaotrorToPMEM_ID failed" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == status )
    {
        response.pBuf = pmem_malloc_ext_v2( request.size, pmemId, pmemFlags, request.alignment, 0x0,
                                            &pmemHandle, NULL );

        if ( nullptr == response.pBuf )
        {
            QC_LOG_ERROR( "pmem_malloc_ext_v2 allocate failed" );
            status = QC_STATUS_NOMEM;
        }
        else
        {
            response.dmaHandle = static_cast<uint64_t>( (uintptr_t) pmemHandle );
            response.size = request.size;
            response.allocatorType = GetConfiguration().type;
            response.alignment = request.alignment;
            response.cache = request.cache;
            response.pid = getpid();
            response.name = GetConfiguration().name + "-" + std::to_string( response.dmaHandle );
        }
    }

    return status;
}

QCStatus_e PMEMAllocator::Free( const QCBufferDescriptorBase_t &BufferDescriptor )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( nullptr == BufferDescriptor.pBuf )
    {
        status = QC_STATUS_NULL_PTR;
        QC_ERROR( "nullptr == BufferDescriptor.pBuf" );
    }
    else if ( 0 == BufferDescriptor.dmaHandle )
    {
        QC_ERROR( "0 == BufferDescriptor.dmaHandle" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == BufferDescriptor.size )
    {
        QC_ERROR( "BufferDescriptor.size != 0" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( BufferDescriptor.allocatorType != GetConfiguration().type )
    {
        status = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "BufferDescriptor.allocatorType !=GetConfiguration().type" );
    }
    else
    {
        int rc = pmem_free( BufferDescriptor.pBuf );
        if ( 0 != rc )
        {
            QC_LOG_ERROR( "Free failed to do free for buffer %p: %d", BufferDescriptor.pBuf, rc );
            status = QC_STATUS_FAIL;
        }
    }
    return status;
}

QCStatus_e PMEMAllocator::AllocaotrorToPMEM_ID( const QCMemoryAllocator_e &allocatorType,
                                                uint32_t &pmemId )
{
    QCStatus_e status = QC_STATUS_OK;
    switch ( allocatorType )
    {
        case QC_MEMORY_ALLOCATOR_DMA:
            pmemId = PMEM_DMA_ID;
            break;
        case QC_MEMORY_ALLOCATOR_DMA_CAMERA:
            pmemId = PMEM_CAMERA_ID;
            break;
        case QC_MEMORY_ALLOCATOR_DMA_GPU:
            pmemId = PMEM_GRAPHICS_FRAMEBUFFER_ID;
            break;
        case QC_MEMORY_ALLOCATOR_DMA_VPU:
            pmemId = PMEM_VIDEO_ID;
            break;
        case QC_MEMORY_ALLOCATOR_DMA_EVA:
            pmemId = PMEM_EVA_ID;
            break;
        case QC_MEMORY_ALLOCATOR_DMA_HTP:
            pmemId = PMEM_DSP_ID;
            break;
        default:
            status = QC_STATUS_BAD_ARGUMENTS;
            break;
    }
    return status;
}


}   // namespace Memory
}   // namespace QC
