// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/infras/memory/BufferManager.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"
#include <string.h>
#include <unistd.h>

namespace QC
{
namespace common
{

void QCSharedBuffer::Init()
{
    (void) memset( this, 0, sizeof( *this ) );
    this->buffer.pData = nullptr;
    this->buffer.dmaHandle = 0;
    this->buffer.size = 0;
    this->buffer.id = 0;
    this->buffer.pid = static_cast<uint64_t>( getpid() );
    this->buffer.usage = QC_BUFFER_USAGE_DEFAULT;
    this->buffer.flags = 0;
    this->size = 0;
    this->offset = 0;
    this->type = QC_BUFFER_TYPE_RAW;
}

QCSharedBuffer::QCSharedBuffer()
{
    Init();
}

QCSharedBuffer::QCSharedBuffer( const QCSharedBuffer &rhs )
{
    this->buffer = rhs.buffer;
    this->size = rhs.size;
    this->offset = rhs.offset;
    this->type = rhs.type;
    switch ( type )
    {
        case QC_BUFFER_TYPE_IMAGE:
            this->imgProps = rhs.imgProps;
            break;
        case QC_BUFFER_TYPE_TENSOR:
            this->tensorProps = rhs.tensorProps;
            break;
        default: /* do nothing for RAW type */
            break;
    }
}

QCSharedBuffer &QCSharedBuffer::operator=( const QCSharedBuffer &rhs )
{
    this->buffer = rhs.buffer;
    this->size = rhs.size;
    this->offset = rhs.offset;
    this->type = rhs.type;
    switch ( type )
    {
        case QC_BUFFER_TYPE_IMAGE:
            this->imgProps = rhs.imgProps;
            break;
        case QC_BUFFER_TYPE_TENSOR:
            this->tensorProps = rhs.tensorProps;
            break;
        default: /* do nothing for RAW type */
            break;
    }
    return *this;
}

QCSharedBuffer::~QCSharedBuffer() {}

QCStatus_e QCSharedBuffer::Allocate( size_t size, QCBufferUsage_e usage, QCBufferFlags_t flags )
{
    QCStatus_e ret = QC_STATUS_OK;
    void *pData = nullptr;
    uint64_t dmaHandle = 0;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();

    if ( nullptr == pBufferManager )
    {
        QC_LOG_ERROR( "Failed to get buffer manager" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr != this->buffer.pData )
    {
        QC_LOG_ERROR( "buffer is already allocated" );
        ret = QC_STATUS_ALREADY;
    }
    else
    {
        ret = QCDmaAllocate( &pData, &dmaHandle, size, flags, usage );

        if ( QC_STATUS_OK == ret )
        {
            this->buffer.pData = pData;
            this->buffer.dmaHandle = dmaHandle;
            this->buffer.size = size;
            this->buffer.pid = static_cast<uint64_t>( getpid() );
            this->buffer.usage = usage;
            this->buffer.flags = flags;
            this->size = size;
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = pBufferManager->Register( this );
        }

        if ( QC_STATUS_OK != ret )
        {
            if ( nullptr != pData )
            {
                (void) QCDmaFree( pData, dmaHandle, size );
            }
            Init();
        }
    }

    return ret;
}

QCStatus_e QCSharedBuffer::Free()
{
    QCStatus_e ret = QC_STATUS_OK;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    int pid = getpid();

    if ( nullptr == pBufferManager )
    {
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == this->buffer.pData )
    {
        QC_LOG_ERROR( "buffer not allocated" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( pid != this->buffer.pid )
    {
        QC_LOG_ERROR( "buffer not allocated by self, can't do free" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = pBufferManager->Deregister( this->buffer.id );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = QCDmaFree( this->buffer.pData, this->buffer.dmaHandle, this->buffer.size );
    }

    if ( QC_STATUS_OK == ret )
    {
        Init();
    }

    return ret;
}

QCStatus_e QCSharedBuffer::Import( const QCSharedBuffer *pSharedBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;
    void *pData = nullptr;
    uint64_t dmaHandle = 0;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    int pid = getpid();

    if ( nullptr == pBufferManager )
    {
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pSharedBuffer )
    {
        QC_LOG_ERROR( "pSharedBuffer is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == pSharedBuffer->buffer.dmaHandle )
    {
        QC_LOG_ERROR( "invalid dma buffer handle" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == pSharedBuffer->buffer.size )
    {
        QC_LOG_ERROR( "invalid dma buffer size" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( pid == pSharedBuffer->buffer.pid )
    {
        QC_LOG_ERROR( "buffer allocated by self, no need to do memory map" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        ret = QCDmaImport( &pData, &dmaHandle, pSharedBuffer->buffer.pid,
                           pSharedBuffer->buffer.dmaHandle, pSharedBuffer->buffer.size,
                           pSharedBuffer->buffer.flags, pSharedBuffer->buffer.usage );
        if ( QC_STATUS_OK == ret )
        {
            *this = *pSharedBuffer;
            this->buffer.pData = pData;
            this->buffer.dmaHandle = dmaHandle;
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = pBufferManager->Register( this );
        }

        if ( QC_STATUS_OK != ret )
        {
            if ( nullptr != pData )
            {
                (void) QCDmaUnImport( pData, dmaHandle, pSharedBuffer->buffer.size );
            }
            Init();
        }
    }

    return ret;
}


QCStatus_e QCSharedBuffer::UnImport()
{
    QCStatus_e ret = QC_STATUS_OK;
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    int pid = getpid();

    if ( nullptr == pBufferManager )
    {
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == this->buffer.pData )
    {
        QC_LOG_ERROR( "buffer not mapped" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( pid == this->buffer.pid )
    {
        QC_LOG_ERROR( "buffer allocated by self, no need to do memory unmap" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = pBufferManager->Deregister( this->buffer.id );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = QCDmaUnImport( this->buffer.pData, this->buffer.dmaHandle, this->buffer.size );
    }

    if ( QC_STATUS_OK == ret )
    {
        Init();
    }

    return ret;
}

}   // namespace common
}   // namespace QC
