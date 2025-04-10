// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/common/BufferManager.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include <string.h>
#include <unistd.h>

namespace ridehal
{
namespace common
{

void RideHal_SharedBuffer::Init()
{
    (void) memset( this, 0, sizeof( *this ) );
    this->buffer.pData = nullptr;
    this->buffer.dmaHandle = 0;
    this->buffer.size = 0;
    this->buffer.id = 0;
    this->buffer.pid = static_cast<uint64_t>( getpid() );
    this->buffer.usage = RIDEHAL_BUFFER_USAGE_DEFAULT;
    this->buffer.flags = 0;
    this->size = 0;
    this->offset = 0;
    this->type = RIDEHAL_BUFFER_TYPE_RAW;
}

RideHal_SharedBuffer::RideHal_SharedBuffer()
{
    Init();
}

RideHal_SharedBuffer::RideHal_SharedBuffer( const RideHal_SharedBuffer &rhs )
{
    this->buffer = rhs.buffer;
    this->size = rhs.size;
    this->offset = rhs.offset;
    this->type = rhs.type;
    switch ( type )
    {
        case RIDEHAL_BUFFER_TYPE_IMAGE:
            this->imgProps = rhs.imgProps;
            break;
        case RIDEHAL_BUFFER_TYPE_TENSOR:
            this->tensorProps = rhs.tensorProps;
            break;
        default: /* do nothing for RAW type */
            break;
    }
}

RideHal_SharedBuffer &RideHal_SharedBuffer::operator=( const RideHal_SharedBuffer &rhs )
{
    this->buffer = rhs.buffer;
    this->size = rhs.size;
    this->offset = rhs.offset;
    this->type = rhs.type;
    switch ( type )
    {
        case RIDEHAL_BUFFER_TYPE_IMAGE:
            this->imgProps = rhs.imgProps;
            break;
        case RIDEHAL_BUFFER_TYPE_TENSOR:
            this->tensorProps = rhs.tensorProps;
            break;
        default: /* do nothing for RAW type */
            break;
    }
    return *this;
}

RideHal_SharedBuffer::~RideHal_SharedBuffer() {}

RideHalError_e RideHal_SharedBuffer::Allocate( size_t size, RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    void *pData = nullptr;
    uint64_t dmaHandle = 0;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();

    if ( nullptr == pBufferManager )
    {
        RIDEHAL_LOG_ERROR( "Failed to get buffer manager" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr != this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "buffer is already allocated" );
        ret = RIDEHAL_ERROR_ALREADY;
    }
    else
    {
        ret = RideHal_DmaAllocate( &pData, &dmaHandle, size, flags, usage );

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            this->buffer.pData = pData;
            this->buffer.dmaHandle = dmaHandle;
            this->buffer.size = size;
            this->buffer.pid = static_cast<uint64_t>( getpid() );
            this->buffer.usage = usage;
            this->buffer.flags = flags;
            this->size = size;
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = pBufferManager->Register( this );
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            if ( nullptr != pData )
            {
                (void) RideHal_DmaFree( pData, dmaHandle, size );
            }
            Init();
        }
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::Free()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    int pid = getpid();

    if ( nullptr == pBufferManager )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "buffer not allocated" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( pid != this->buffer.pid )
    {
        RIDEHAL_LOG_ERROR( "buffer not allocated by self, can't do free" );
        ret = RIDEHAL_ERROR_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = pBufferManager->Deregister( this->buffer.id );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = RideHal_DmaFree( this->buffer.pData, this->buffer.dmaHandle, this->buffer.size );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        Init();
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::Import( const RideHal_SharedBuffer *pSharedBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    void *pData = nullptr;
    uint64_t dmaHandle = 0;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    int pid = getpid();

    if ( nullptr == pBufferManager )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pSharedBuffer )
    {
        RIDEHAL_LOG_ERROR( "pSharedBuffer is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 0 == pSharedBuffer->buffer.dmaHandle )
    {
        RIDEHAL_LOG_ERROR( "invalid dma buffer handle" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 0 == pSharedBuffer->buffer.size )
    {
        RIDEHAL_LOG_ERROR( "invalid dma buffer size" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( pid == pSharedBuffer->buffer.pid )
    {
        RIDEHAL_LOG_ERROR( "buffer allocated by self, no need to do memory map" );
        ret = RIDEHAL_ERROR_OUT_OF_BOUND;
    }
    else
    {
        ret = RideHal_DmaImport( &pData, &dmaHandle, pSharedBuffer->buffer.pid,
                                 pSharedBuffer->buffer.dmaHandle, pSharedBuffer->buffer.size,
                                 pSharedBuffer->buffer.flags, pSharedBuffer->buffer.usage );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            *this = *pSharedBuffer;
            this->buffer.pData = pData;
            this->buffer.dmaHandle = dmaHandle;
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = pBufferManager->Register( this );
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            if ( nullptr != pData )
            {
                (void) RideHal_DmaUnImport( pData, dmaHandle, pSharedBuffer->buffer.size );
            }
            Init();
        }
    }

    return ret;
}


RideHalError_e RideHal_SharedBuffer::UnImport()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    int pid = getpid();

    if ( nullptr == pBufferManager )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "buffer not mapped" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( pid == this->buffer.pid )
    {
        RIDEHAL_LOG_ERROR( "buffer allocated by self, no need to do memory unmap" );
        ret = RIDEHAL_ERROR_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = pBufferManager->Deregister( this->buffer.id );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = RideHal_DmaUnImport( this->buffer.pData, this->buffer.dmaHandle, this->buffer.size );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        Init();
    }

    return ret;
}

}   // namespace common
}   // namespace ridehal
