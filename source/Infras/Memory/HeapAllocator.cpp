// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

// #include <malloc.h>
#include <memory>
#include <stdlib.h>
#include <string>

#include "QC/Infras/Memory/HeapAllocator.hpp"

namespace QC
{
namespace Memory
{

HeapAllocator::HeapAllocator()
    : QCMemoryAllocatorIfs( { "Heap Allocator" }, QC_MEMORY_ALLOCATOR_HEAP )
{
    (void) QC_LOGGER_INIT( GetConfiguration().name.c_str(), LOGGER_LEVEL_VERBOSE );
}

HeapAllocator::~HeapAllocator()
{
    (void) QC_LOGGER_DEINIT();
};

QCStatus_e HeapAllocator::Allocate( const QCBufferPropBase_t &request,
                                    QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    // check correctness of size request
    if ( 0 == request.size )
    {
        QC_ERROR( "0 == request.size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    // check correctness of cache type request for heap allocation
    else if ( QC_CACHEABLE != request.cache )
    {
        QC_ERROR( "QC_CACHEABLE != request.cache" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        QC_DEBUG(
                "%s: Allocating %zu bytes aligned at %lu byte boundary from the process's heap...",
                GetConfiguration().name.c_str(), request.size, request.alignment );
        uint32_t ret = posix_memalign( &response.pBuf, request.alignment, request.size );
        response.alignment = request.alignment;
        response.cache = request.cache;
        response.allocatorType = GetConfiguration().type;
        response.size = request.size;
        response.name = GetConfiguration().name;
        // check if nullptr is the result
        if ( nullptr == response.pBuf )
        {
            QC_ERROR( "nullptr == response.pBuf" );
            status = QC_STATUS_NULL_PTR;
        }
        else if ( 0 != ret )
        {
            QC_ERROR( "ret = %d ", ret );
            status = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG(
                    "%s: Allocated %zu bytes aligned at %lu byte boundary from the process's heap",
                    GetConfiguration().name.c_str(), request.size, request.alignment );
        }
    }

    return status;
}

QCStatus_e HeapAllocator::Free( const QCBufferDescriptorBase_t &buff )
{
    QCStatus_e status = QC_STATUS_OK;
    if ( buff.allocatorType != QC_MEMORY_ALLOCATOR_HEAP )
    {
        QC_ERROR( "buff.allocatorType != QC_MEMORY_ALLOCATOR_HEAP, %d", buff.allocatorType );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    // Check if the buffer size is valid
    else if ( 0 == buff.size )
    {
        QC_ERROR( "0 == buff.size" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    // Check if the buffer pointer is valid
    else if ( nullptr == buff.pBuf )
    {
        QC_ERROR( "nullptr == buff.pBuf" );
        status = QC_STATUS_NULL_PTR;
    }
    else
    {
        QC_DEBUG( "%s: Freeing the object %p...", GetConfiguration().name.c_str(), buff.pBuf );
        ::free( buff.pBuf );
    }

    return status;
}

}   // namespace Memory
}   // namespace QC
