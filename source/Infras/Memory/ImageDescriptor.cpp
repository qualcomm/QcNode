// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Log/Logger.hpp"

namespace QC
{
namespace Memory
{

static uint32_t s_qcFormatToBytesPerPixel[QC_IMAGE_FORMAT_MAX] = {
        3, /* QC_IMAGE_FORMAT_RGB888 */
        3, /* QC_IMAGE_FORMAT_BGR888 */
        2, /* QC_IMAGE_FORMAT_UYVY */
        1, /* QC_IMAGE_FORMAT_NV12 */
        2, /* QC_IMAGE_FORMAT_P010 */
        1, /* QC_IMAGE_FORMAT_NV12_UBWC */
        2  /* QC_IMAGE_FORMAT_TP10_UBWC */
};

QCStatus_e ImageDescriptor::ImageToTensor( TensorDescriptor_t &tensorDesc ) const
{
    QCStatus_e status = QC_STATUS_OK;
    uint32_t bpp = 0;
    if ( nullptr == this->pBuf )
    {
        QC_LOG_ERROR( "image not allocated" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != this->type )
    {
        QC_LOG_ERROR( "buffer type %d is not image", this->type );
        status = QC_STATUS_UNSUPPORTED;
    }
    else if ( ( QC_IMAGE_FORMAT_RGB888 == this->format ) ||
              ( QC_IMAGE_FORMAT_BGR888 == this->format ) ||
              ( QC_IMAGE_FORMAT_UYVY == this->format ) )
    { /* for image format with just 1 plane */
        bpp = s_qcFormatToBytesPerPixel[this->format];
        if ( this->stride[0] != ( this->width * bpp ) )
        {
            QC_LOG_ERROR(
                    "image with format %d with extra padding along width is not supported to be "
                    "converted to tensor: stride=%u width=%u",
                    this->format, this->stride[0], this->width );
            status = QC_STATUS_UNSUPPORTED;
        }
        else if ( ( this->batchSize > 1 ) && ( this->actualHeight[0] != this->height ) )
        {
            QC_LOG_ERROR( "image with format %d with extra padding along height is not "
                          "supported to be "
                          "converted to tensor: actualHeight=%u height=%u",
                          this->format, this->actualHeight[0], this->height );
            status = QC_STATUS_UNSUPPORTED;
        }
        else if ( ( this->batchSize > 1 ) &&
                  ( this->planeBufSize[0] != ( this->stride[0] * this->height ) ) )
        {
            QC_LOG_ERROR( "image with format %d with extra padding along height is not "
                          "supported to be "
                          "converted to tensor: planeBufSize=%u %u",
                          this->format, this->planeBufSize[0], this->stride[0] * this->height );
            status = QC_STATUS_UNSUPPORTED;
        }
        else
        {
            /* OK */
        }
    }
    else
    {
        QC_LOG_ERROR( "image with format %d is not supported to be converted to tensor",
                      this->format );
        status = QC_STATUS_UNSUPPORTED;
    }

    if ( QC_STATUS_OK == status )
    {
        tensorDesc = *this;
        tensorDesc.type = QC_BUFFER_TYPE_TENSOR;
        tensorDesc.tensorType = QC_TENSOR_TYPE_UFIXED_POINT_8;
        tensorDesc.numDims = 4;
        tensorDesc.dims[0] = this->batchSize;
        tensorDesc.dims[1] = this->height;
        tensorDesc.dims[2] = this->width;
        tensorDesc.dims[3] = bpp;
        tensorDesc.size = static_cast<size_t>( this->batchSize * this->height * this->width * bpp );
    }
    return status;
}

QCStatus_e ImageDescriptor::ImageToTensor( TensorDescriptor_t &luma,
                                           TensorDescriptor_t &chroma ) const
{
    QCStatus_e status = QC_STATUS_OK;

    if ( nullptr == this->pBuf )
    {
        QC_LOG_ERROR( "image not allocated" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != this->type )
    {
        QC_LOG_ERROR( "buffer type %d is not image", this->type );
        status = QC_STATUS_UNSUPPORTED;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == this->format ) || ( QC_IMAGE_FORMAT_P010 == this->format ) )
    { /* for image format with 2 plane */
        if ( this->stride[0] != ( this->width * s_qcFormatToBytesPerPixel[this->format] ) )
        {
            QC_LOG_ERROR(
                    "image with format %d with extra padding along width is not supported to be "
                    "converted to tensor: stride=%u width=%u",
                    this->format, this->stride[0], this->width );
            status = QC_STATUS_UNSUPPORTED;
        }
        else if ( this->batchSize > 1 )
        {
            QC_LOG_ERROR( "not supported for batched image" );
            status = QC_STATUS_UNSUPPORTED;
        }
        else if ( 0 != ( this->height & 0x1u ) )
        {
            QC_LOG_ERROR( "height is not n times of 2" );
            status = QC_STATUS_UNSUPPORTED;
        }
        else if ( 0 != ( this->width & 0x1u ) )
        {
            QC_LOG_ERROR( "width is not n times of 2" );
            status = QC_STATUS_UNSUPPORTED;
        }
        else
        {
            /* OK */
        }
    }
    else
    {
        QC_LOG_ERROR(
                "image with format %d is not supported to be converted to luma and chroma tensor",
                this->format );
        status = QC_STATUS_UNSUPPORTED;
    }

    if ( QC_STATUS_OK == status )
    {
        luma = *this;
        luma.offset = this->offset;
        luma.type = QC_BUFFER_TYPE_TENSOR;
        if ( QC_IMAGE_FORMAT_NV12 == this->format )
        {
            luma.tensorType = QC_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else
        {
            luma.tensorType = QC_TENSOR_TYPE_UFIXED_POINT_16;
        }
        luma.numDims = 4;
        luma.dims[0] = 1;
        luma.dims[1] = this->height;
        luma.dims[2] = this->width;
        luma.dims[3] = 1;
        luma.size = static_cast<size_t>( this->height * this->width *
                                         s_qcFormatToBytesPerPixel[this->format] );

        chroma = *this;
        chroma.offset = this->offset + this->planeBufSize[0];
        chroma.type = QC_BUFFER_TYPE_TENSOR;
        if ( QC_IMAGE_FORMAT_NV12 == this->format )
        {
            chroma.tensorType = QC_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else
        {
            chroma.tensorType = QC_TENSOR_TYPE_UFIXED_POINT_16;
        }
        chroma.numDims = 4;
        chroma.dims[0] = 1;
        chroma.dims[1] = this->height / 2;
        chroma.dims[2] = this->width / 2;
        chroma.dims[3] = 2;
        chroma.size = static_cast<size_t>( this->height * this->width *
                                           s_qcFormatToBytesPerPixel[this->format] / 2 );
    }

    return status;
}

QCStatus_e ImageDescriptor::GetImageDesc( ImageDescriptor &imageDesc, uint32_t batchOffset,
                                          uint32_t batchSize ) const
{
    QCStatus_e status = QC_STATUS_OK;
    size_t singleImageSize = 0;

    if ( nullptr == this->pBuf )
    {
        QC_LOG_ERROR( "image not allocated" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != this->type )
    {
        QC_LOG_ERROR( "buffer type %d is not image", this->type );
        status = QC_STATUS_UNSUPPORTED;
    }
    else if ( batchOffset >= this->batchSize )
    {
        QC_LOG_ERROR( "buffer batch offset %u(>=%u) out of range", batchOffset, this->batchSize );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( batchOffset + batchSize ) >= this->batchSize )
    {
        QC_LOG_ERROR( "buffer batch size %u out of range", batchSize );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        imageDesc = *this;
    }

    if ( QC_STATUS_OK == status )
    {
        singleImageSize = this->size / this->batchSize;
        imageDesc.batchSize = batchSize;
        imageDesc.size = batchSize * singleImageSize;
        imageDesc.offset = this->offset + batchOffset * singleImageSize;
        imageDesc.pBuf =
                static_cast<void *>( static_cast<uint8_t *>( this->pBufBase ) + imageDesc.offset );
    }

    return status;
}

ImageDescriptor &ImageDescriptor::operator=( const BufferDescriptor &other )
{
    if ( this != &other )
    {
        this->name = other.name;
        this->pBuf = other.pBuf;
        this->size = other.size;
        this->type = QC_BUFFER_TYPE_IMAGE;
        this->pBufBase = other.pBufBase;
        this->dmaHandle = other.dmaHandle;
        this->dmaSize = other.dmaSize;
        this->offset = other.offset;
        this->id = other.id;
        this->pid = other.pid;
        this->usage = other.usage;
        this->cache = other.cache;
        const ImageDescriptor_t *pImage = dynamic_cast<const ImageDescriptor_t *>( &other );
        if ( pImage )
        {
            this->format = pImage->format;
            this->batchSize = pImage->batchSize;
            this->width = pImage->width;
            this->height = pImage->height;
            uint32_t numPlanes = std::min( pImage->numPlanes, (uint32_t) QC_NUM_IMAGE_PLANES );
            std::copy( pImage->stride, pImage->stride + numPlanes, this->stride );
            std::copy( pImage->actualHeight, pImage->actualHeight + numPlanes, this->actualHeight );
            std::copy( pImage->planeBufSize, pImage->planeBufSize + numPlanes, this->planeBufSize );
            this->numPlanes = pImage->numPlanes;
        }
    }
    return *this;
}

ImageDescriptor &ImageDescriptor::operator=( const QCSharedBuffer_t &other )
{
    this->pBuf = other.data();
    this->size = other.size;
    this->type = QC_BUFFER_TYPE_IMAGE;
    this->pBufBase = other.buffer.pData;
    this->dmaHandle = other.buffer.dmaHandle;
    this->dmaSize = other.buffer.size;
    this->offset = other.offset;
    this->id = other.buffer.id;
    this->pid = other.buffer.pid;
    this->usage = other.buffer.usage;
    this->cache = QC_CACHEABLE;
    this->format = other.imgProps.format;
    this->batchSize = other.imgProps.batchSize;
    this->width = other.imgProps.width;
    this->height = other.imgProps.height;
    std::copy( other.imgProps.stride, other.imgProps.stride + other.imgProps.numPlanes,
               this->stride );
    std::copy( other.imgProps.actualHeight, other.imgProps.actualHeight + other.imgProps.numPlanes,
               this->actualHeight );
    std::copy( other.imgProps.planeBufSize, other.imgProps.planeBufSize + other.imgProps.numPlanes,
               this->planeBufSize );
    this->numPlanes = other.imgProps.numPlanes;
    return *this;
}

}   // namespace Memory

}   // namespace QC
