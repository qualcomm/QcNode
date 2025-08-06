// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/BufferDescriptor.hpp"
#include "QC/Infras/Memory/BufferManager.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include <string.h>
#include <unistd.h>

namespace QC
{
namespace Memory
{

void QCSharedBuffer::Init()
{
    (void) memset( this, 0, sizeof( *this ) );
    this->buffer.pData = nullptr;
    this->buffer.dmaHandle = 0;
    this->buffer.size = 0;
    this->buffer.id = 0;
    this->buffer.pid = getpid();
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

QCSharedBuffer::QCSharedBuffer( const QCBufferDescriptorBase_t &other )
{
    const BufferDescriptor_t *pBuffer = dynamic_cast<const BufferDescriptor_t *>( &other );
    const TensorDescriptor_t *pTensor = dynamic_cast<const TensorDescriptor_t *>( &other );
    const ImageDescriptor_t *pImage = dynamic_cast<const ImageDescriptor_t *>( &other );
    static const QCBufferUsage_e s_Allocator2Usage[] = {
            QC_BUFFER_USAGE_MAX,     /* QC_MEMORY_ALLOCATOR_HEAP */
            QC_BUFFER_USAGE_DEFAULT, /* QC_MEMORY_ALLOCATOR_DMA */
            QC_BUFFER_USAGE_CAMERA,  /* QC_MEMORY_ALLOCATOR_DMA_CAMERA */
            QC_BUFFER_USAGE_GPU,     /* QC_MEMORY_ALLOCATOR_DMA_GPU */
            QC_BUFFER_USAGE_VPU,     /* QC_MEMORY_ALLOCATOR_DMA_VPU */
            QC_BUFFER_USAGE_EVA,     /* QC_MEMORY_ALLOCATOR_DMA_EVA */
            QC_BUFFER_USAGE_HTP,     /* QC_MEMORY_ALLOCATOR_DMA_HTP */
            QC_BUFFER_USAGE_MAX,     /* QC_MEMORY_ALLOCATOR_LAST */
    };
    if ( nullptr != pBuffer )
    {
        this->buffer.pData = pBuffer->pBuf;
        this->buffer.size = pBuffer->size;
        this->buffer.dmaHandle = pBuffer->dmaHandle;
        this->buffer.id = pBuffer->id;
        this->buffer.pid = pBuffer->pid;
        this->buffer.usage = s_Allocator2Usage[pBuffer->allocatorType];
        this->buffer.flags = QC_BUFFER_FLAGS_CACHE_WB_WA;
        this->size = pBuffer->validSize;
        this->offset = pBuffer->offset;
        this->type = pBuffer->type;
    }
    if ( nullptr != pTensor )
    {
        this->tensorProps.type = pTensor->tensorType;
        std::copy( pTensor->dims, pTensor->dims + pTensor->numDims, this->tensorProps.dims );
        this->tensorProps.numDims = pTensor->numDims;
    }

    if ( nullptr != pImage )
    {
        this->imgProps.format = pImage->format;
        this->imgProps.width = pImage->width;
        this->imgProps.height = pImage->height;
        this->imgProps.batchSize = pImage->batchSize;
        std::copy( pImage->stride, pImage->stride + pImage->numPlanes, this->imgProps.stride );
        std::copy( pImage->actualHeight, pImage->actualHeight + pImage->numPlanes,
                   this->imgProps.actualHeight );
        std::copy( pImage->planeBufSize, pImage->planeBufSize + pImage->numPlanes,
                   this->imgProps.planeBufSize );
        this->imgProps.numPlanes = pImage->numPlanes;
    }
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
            this->buffer.pid = getpid();
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
    pid_t pid = getpid();

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
    pid_t pid = getpid();

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
    pid_t pid = getpid();

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

}   // namespace Memory
}   // namespace QC
