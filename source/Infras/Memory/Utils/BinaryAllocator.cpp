// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/Utils/BinaryAllocator.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include <unistd.h>

namespace QC
{
namespace Memory
{

BinaryAllocator::BinaryAllocator( const std::string &name ) : QCMemoryAllocatorIfs( {name}, QC_MEMORY_ALLOCATOR_DMA )
{
    (void) QC_LOGGER_INIT( GetConfiguration().name.c_str(), LOGGER_LEVEL_ERROR );
};

BinaryAllocator::~BinaryAllocator()
{
    (void) QC_LOGGER_DEINIT();
};

QCStatus_e BinaryAllocator::Allocate( const QCBufferPropBase_t &request,
                                      QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    void *pData;
    uint64_t dmaHandle;
    QCBufferFlags_t flags = 0;
    const BufferProps_t *pBufferProps = dynamic_cast<const BufferProps_t *>( &request );
    BufferDescriptor_t *pBufferDescriptor = dynamic_cast<BufferDescriptor_t *>( &response );

    if ( ( nullptr != pBufferProps ) && ( nullptr != pBufferDescriptor ) )
    {
        if ( QC_CACHEABLE_NON != request.cache )
        {
            flags |= QC_BUFFER_FLAGS_CACHE_WB_WA;
        }

        // TODO: replace flags with QCAllocationAttribute_e
        // Do this at the end of phase 2 when to remove RideHal_SharedBuffer_t definition.
        status =
                QCDmaAllocate( &pData, &dmaHandle, pBufferProps->size, flags, pBufferProps->usage );
        if ( QC_STATUS_OK == status )
        {
            pBufferDescriptor->name = GetConfiguration().name + "-" + std::to_string( dmaHandle );
            pBufferDescriptor->pBuf = pData;
            pBufferDescriptor->size = pBufferProps->size;
            pBufferDescriptor->type = QC_BUFFER_TYPE_RAW;
            pBufferDescriptor->pBufBase = pData;
            pBufferDescriptor->dmaHandle = dmaHandle;
            pBufferDescriptor->dmaSize = pBufferProps->size;
            pBufferDescriptor->offset = 0;
            pBufferDescriptor->id = 0; /* TODO: talk with Vadiam about the new buffer manager */
            pBufferDescriptor->pid = getpid();
            pBufferDescriptor->usage = pBufferProps->usage;
            pBufferDescriptor->cache = pBufferProps->cache;
        }
    }
    else
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

QCStatus_e BinaryAllocator::Free( const QCBufferDescriptorBase_t &buffer )
{
    QCStatus_e status = QC_STATUS_OK;
    const BufferDescriptor_t *pBufferDescriptor =
            dynamic_cast<const BufferDescriptor_t *>( &buffer );
    pid_t pid = getpid();

    if ( nullptr == pBufferDescriptor )
    {
        QC_ERROR( "buffer is invalid" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pBufferDescriptor->pBufBase )
    {
        QC_ERROR( "buffer not allocated" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( static_cast<uint64_t>( pid ) != pBufferDescriptor->pid )
    {
        QC_ERROR( "buffer not allocated by self, can't do free" );
        status = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == status )
    {
        status = QCDmaFree( pBufferDescriptor->pBufBase, pBufferDescriptor->dmaHandle,
                            pBufferDescriptor->dmaSize );
    }

    return status;
}
}   // namespace Memory

}   // namespace QC
