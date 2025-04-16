// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/infras/memory/BufferManager.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"
#include <apdf.h>

namespace QC
{
#define PLANEDEF_HW_USAGE_FLAGS                                                                    \
    ( WFD_USAGE_OPENGL_ES2 | WFD_USAGE_OPENGL_ES3 | WFD_USAGE_CAPTURE | WFD_USAGE_VIDEO |          \
      WFD_USAGE_DISPLAY | WFD_USAGE_NATIVE )

static PDColorFormat_e s_qcFormatToApdfFormat[QC_IMAGE_FORMAT_MAX] = {
        PD_FORMAT_RGB888, /* QC_IMAGE_FORMAT_RGB888 */
        PD_FORMAT_RGB888, /* QC_IMAGE_FORMAT_BGR888 */
        PD_FORMAT_UYVY,   /* QC_IMAGE_FORMAT_UYVY */
        PD_FORMAT_NV12,   /* QC_IMAGE_FORMAT_NV12 */
        PD_FORMAT_P010,   /* QC_IMAGE_FORMAT_P010 */
        PD_FORMAT_NV12,   /* QC_IMAGE_FORMAT_NV12_UBWC */
        PD_FORMAT_TP10    /* QC_IMAGE_FORMAT_TP10_UBWC */
};

static uint32_t s_qcFormatToBytesPerPixel[QC_IMAGE_FORMAT_MAX] = {
        3, /* QC_IMAGE_FORMAT_RGB888 */
        3, /* QC_IMAGE_FORMAT_BGR888 */
        2, /* QC_IMAGE_FORMAT_UYVY */
        1, /* QC_IMAGE_FORMAT_NV12 */
        2, /* QC_IMAGE_FORMAT_P010 */
        1, /* QC_IMAGE_FORMAT_NV12_UBWC */
        2  /* QC_IMAGE_FORMAT_TP10_UBWC */
};

static uint32_t s_qcFormatToNumPlanes[QC_IMAGE_FORMAT_MAX] = {
        1, /* QC_IMAGE_FORMAT_RGB888 */
        1, /* QC_IMAGE_FORMAT_BGR888 */
        1, /* QC_IMAGE_FORMAT_UYVY */
        2, /* QC_IMAGE_FORMAT_NV12 */
        2, /* QC_IMAGE_FORMAT_P010 */
        4, /* QC_IMAGE_FORMAT_NV12_UBWC */
        4  /* QC_IMAGE_FORMAT_P010_UBWC */
};

static uint32_t s_qcFormatToHeightDividerPerPlanes[QC_IMAGE_FORMAT_MAX][QC_NUM_IMAGE_PLANES] = {
        { 1, 0, 0, 0 }, /* QC_IMAGE_FORMAT_RGB888 */
        { 1, 0, 0, 0 }, /* QC_IMAGE_FORMAT_BGR888 */
        { 1, 0, 0, 0 }, /* QC_IMAGE_FORMAT_UYVY */
        { 1, 2, 0, 0 }, /* QC_IMAGE_FORMAT_NV12 */
        { 1, 2, 0, 0 }, /* QC_IMAGE_FORMAT_P010 */
        { 1, 1, 2, 2 }  /* QC_IMAGE_FORMAT_NV12_UBWC */
};

static const char *s_qcFormatToString[QC_IMAGE_FORMAT_MAX] = {
        "RGB888",   /* QC_IMAGE_FORMAT_RGB888 */
        "BGR888",   /* QC_IMAGE_FORMAT_BGR888 */
        "UYVY",     /* QC_IMAGE_FORMAT_UYVY */
        "NV12",     /* QC_IMAGE_FORMAT_NV12 */
        "P010",     /* QC_IMAGE_FORMAT_P010 */
        "NV12_UBWC" /* QC_IMAGE_FORMAT_NV12_UBWC */
};

QCStatus_e QCSharedBuffer::Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                                     QCImageFormat_e format, QCBufferUsage_e usage,
                                     QCBufferFlags_t flags )
{
    QCStatus_e ret = QC_STATUS_OK;
    size_t size = 0;
    uint32_t nUsage = PLANEDEF_HW_USAGE_FLAGS;
    FrameRes_t frameRes;
    PlaneDef_t planeDef;
    uint32_t numPlanes = 0;
    PDColorFormat_e eColorFormat = PD_FORMAT_MAX;
    PDStatus_e status;
    uint32_t i = 0;

    if ( ( 0 == batchSize ) || ( 0 == width ) || ( 0 == height ) ||
         ( format >= QC_IMAGE_FORMAT_MAX ) || ( format < QC_IMAGE_FORMAT_RGB888 ) )
    {
        QC_LOG_ERROR( "invalid args: batchSize=%u, width=%u, height=%u, format=%d", batchSize,
                      width, height, format );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        QC_LOG_ERROR( "buffer is already allocated" );
        ret = QC_STATUS_ALREADY;
    }
    else
    {
        eColorFormat = s_qcFormatToApdfFormat[format];
    }

    if ( ( QC_IMAGE_FORMAT_NV12_UBWC == format ) || ( QC_IMAGE_FORMAT_TP10_UBWC == format ) )
    {
        nUsage = nUsage | WFD_USAGE_COMPRESSION;
    }

    if ( QC_STATUS_OK == ret )
    {
        status = PDQueryNumPlanes( eColorFormat, nUsage, &numPlanes, 0 );
        if ( ( PD_OK != status ) || ( 0 == numPlanes ) || ( numPlanes > QC_NUM_IMAGE_PLANES ) )
        {
            QC_LOG_ERROR(
                    "failed to query plane numbers for image width=%u, height=%u, format=%d: %d",
                    width, height, format, status );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            frameRes.nWidthInPixels = width;
            frameRes.nHeightInPixels = height;
            this->imgProps.format = format;
            this->imgProps.batchSize = batchSize;
            this->imgProps.width = width;
            this->imgProps.height = height;
            this->imgProps.numPlanes = numPlanes;
            this->type = QC_BUFFER_TYPE_IMAGE;
            QC_LOG_VERBOSE( "PlaneDef for image %ux%u format %s\n", width, height,
                            s_qcFormatToString[format] );
        }
    }

    for ( i = 0; i < numPlanes; i++ )
    {
        if ( QC_STATUS_OK != ret )
        {
            break;
        }
        planeDef.nPlaneIndex = i + 1;
        status = PDQueryPlaneDef( eColorFormat, nUsage, &frameRes, &planeDef, 0 );
        if ( PD_OK == status )
        {
            QC_LOG_VERBOSE( " plane %u: nMinStride = %u, nMaxstride = %u nStrideMultiples = %u", i,
                            planeDef.nMinStride, planeDef.nMaxstride, planeDef.nStrideMultiples );
            QC_LOG_VERBOSE( "   nActualStride = %u nMinPlaneBufHeight = %u, nHeightMultiples = %u",
                            planeDef.nActualStride, planeDef.nMinPlaneBufHeight,
                            planeDef.nHeightMultiples );
            QC_LOG_VERBOSE( "   nActualPlaneBufHeight = %u nActualBufSizeAlignment =%u,",
                            planeDef.nActualPlaneBufHeight, planeDef.nActualBufSizeAlignment );
            QC_LOG_VERBOSE( "   nBufAddrAlignment = %u nPlaneBufSize = %u, nPlanePaddingSize = %u",
                            planeDef.nBufAddrAlignment, planeDef.nPlaneBufSize,
                            planeDef.nPlanePaddingSize );
            this->imgProps.stride[i] = planeDef.nActualStride;
            this->imgProps.actualHeight[i] = planeDef.nActualPlaneBufHeight;
            this->imgProps.planeBufSize[i] = planeDef.nPlaneBufSize + planeDef.nPlanePaddingSize;
            size += (size_t) this->imgProps.planeBufSize[i];
        }
        else
        {
            QC_LOG_ERROR( "failed to query plane def for image width=%u, height=%u, format=%d: %d",
                          width, height, format, status );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        size = size * batchSize;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}

QCStatus_e QCSharedBuffer::Allocate( uint32_t width, uint32_t height, QCImageFormat_e format,
                                     QCBufferUsage_e usage, QCBufferFlags_t flags )
{
    return Allocate( 1, width, height, format, usage, flags );
}

QCStatus_e QCSharedBuffer::Allocate( const QCImageProps_t *pImgProps, QCBufferUsage_e usage,
                                     QCBufferFlags_t flags )
{
    QCStatus_e ret = QC_STATUS_OK;
    size_t size = 0;
    uint32_t i = 0;
    uint32_t bpp = 0;
    uint32_t divider = 0;

    if ( nullptr == pImgProps )
    {
        QC_LOG_ERROR( "pImgProps is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( 0 == pImgProps->batchSize ) || ( 0 == pImgProps->width ) ||
              ( 0 == pImgProps->height ) )
    {
        QC_LOG_ERROR( "invalid args: batchSize=%u, width=%u, height=%u", pImgProps->batchSize,
                      pImgProps->width, pImgProps->height );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( pImgProps->format >= QC_IMAGE_FORMAT_MAX ) &&
              ( pImgProps->format < QC_IMAGE_FORMAT_COMPRESSED_MIN ) )
    {
        QC_LOG_ERROR( "invalid args: format=%d", pImgProps->format );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( pImgProps->format >= QC_IMAGE_FORMAT_COMPRESSED_MAX )
    {
        QC_LOG_ERROR( "invalid args: format=%d", pImgProps->format );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( pImgProps->format < QC_IMAGE_FORMAT_RGB888 )
    {
        QC_LOG_ERROR( "invalid args: format=%d", pImgProps->format );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        QC_LOG_ERROR( "buffer is already allocated" );
        ret = QC_STATUS_ALREADY;
    }
    else
    {
        if ( pImgProps->format < QC_IMAGE_FORMAT_MAX )
        { /* check properties for uncompressed image */
            if ( s_qcFormatToNumPlanes[pImgProps->format] != pImgProps->numPlanes )
            {
                QC_LOG_ERROR( "plane number %u != expected %u for format %d", pImgProps->numPlanes,
                              s_qcFormatToNumPlanes[pImgProps->format], pImgProps->format );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
        }
        else
        { /* check properties for compressed image */
            if ( 1 != pImgProps->numPlanes )
            {
                QC_LOG_ERROR( "invalid numPlanes for compressed image" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( 0 == pImgProps->planeBufSize[0] )
            {
                QC_LOG_ERROR( "invalid planeBufSize[0] for compressed image" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                /* OK */
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( pImgProps->format < QC_IMAGE_FORMAT_MAX )
        {
            bpp = s_qcFormatToBytesPerPixel[pImgProps->format];
            /* check each plane def is reasonable */
            for ( i = 0; i < pImgProps->numPlanes; i++ )
            {
                divider = s_qcFormatToHeightDividerPerPlanes[pImgProps->format][i];
                if ( ( pImgProps->width * bpp ) > pImgProps->stride[i] )
                {
                    QC_LOG_ERROR( "given stride %u(<%u) too small for plane %u for format %d",
                                  pImgProps->stride[i], pImgProps->width * bpp, i,
                                  pImgProps->format );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( ( pImgProps->height / divider ) > pImgProps->actualHeight[i] )
                {
                    QC_LOG_ERROR(
                            "given actual height %u(<%u) too small for plane %u for format %d",
                            pImgProps->actualHeight[i], pImgProps->height / divider, i,
                            pImgProps->format );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( ( 0 != pImgProps->planeBufSize[i] ) &&
                          ( pImgProps->planeBufSize[i] <
                            ( pImgProps->stride[i] * pImgProps->actualHeight[i] ) ) )
                {
                    QC_LOG_ERROR(
                            "given plane buffer size %u(<%u) too small for plane %u for format %d",
                            pImgProps->planeBufSize[i],
                            pImgProps->stride[i] * pImgProps->actualHeight[i], i,
                            pImgProps->format );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else
                {
                    /* OK */
                }

                if ( QC_STATUS_OK != ret )
                {
                    break;
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                for ( i = 0; i < pImgProps->numPlanes; i++ )
                {
                    if ( 0 != pImgProps->planeBufSize[i] )
                    {
                        size += (size_t) pImgProps->planeBufSize[i];
                    }
                    else
                    {
                        size += (size_t) pImgProps->stride[i] * pImgProps->actualHeight[i];
                    }
                }
                size = size * pImgProps->batchSize;
            }
        }
        else
        {
            size = pImgProps->planeBufSize[0];
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        this->imgProps = *pImgProps;
        for ( i = 0; i < pImgProps->numPlanes; i++ )
        {
            if ( 0 == pImgProps->planeBufSize[i] )
            {
                this->imgProps.planeBufSize[i] = pImgProps->stride[i] * pImgProps->actualHeight[i];
            }
        }
        this->type = QC_BUFFER_TYPE_IMAGE;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}

QCStatus_e QCSharedBuffer::GetSharedBuffer( QCSharedBuffer_t *pSharedBuffer, uint32_t batchOffset,
                                            uint32_t batchSize )
{
    QCStatus_e ret = QC_STATUS_OK;
    size_t singleImageSize = 0;

    if ( nullptr == this->buffer.pData )
    {
        QC_LOG_ERROR( "image not allocated" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != this->type )
    {
        QC_LOG_ERROR( "buffer type %d is not image", this->type );
        ret = QC_STATUS_UNSUPPORTED;
    }
    else if ( nullptr == pSharedBuffer )
    {
        QC_LOG_ERROR( "pSharedBuffer is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( batchOffset >= this->imgProps.batchSize )
    {
        QC_LOG_ERROR( "buffer batch offset %u(>=%u) out of range", batchOffset,
                      this->imgProps.batchSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( batchOffset + batchSize ) >= this->imgProps.batchSize )
    {
        QC_LOG_ERROR( "buffer batch size %u out of range", batchSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        *pSharedBuffer = *this;
    }

    if ( QC_STATUS_OK == ret )
    {
        singleImageSize = pSharedBuffer->buffer.size / pSharedBuffer->imgProps.batchSize;
        pSharedBuffer->imgProps.batchSize = batchSize;
        pSharedBuffer->size = batchSize * singleImageSize;
        pSharedBuffer->offset = batchOffset * singleImageSize;
    }

    return ret;
}

QCStatus_e QCSharedBuffer::ImageToTensor( QCSharedBuffer *pSharedBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSharedBuffer )
    {
        QC_LOG_ERROR( "pSharedBuffer is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == this->buffer.pData )
    {
        QC_LOG_ERROR( "image not allocated" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != this->type )
    {
        QC_LOG_ERROR( "buffer type %d is not image", this->type );
        ret = QC_STATUS_UNSUPPORTED;
    }
    else if ( ( QC_IMAGE_FORMAT_RGB888 == this->imgProps.format ) ||
              ( QC_IMAGE_FORMAT_BGR888 == this->imgProps.format ) ||
              ( QC_IMAGE_FORMAT_UYVY == this->imgProps.format ) )
    { /* for image format with just 1 plane */
        if ( this->imgProps.stride[0] !=
             ( this->imgProps.width * s_qcFormatToBytesPerPixel[this->imgProps.format] ) )
        {
            QC_LOG_ERROR(
                    "image with format %d with extra padding along width is not supported to be "
                    "converted to tensor: stride=%u width=%u",
                    this->imgProps.format, this->imgProps.stride[0], this->imgProps.width );
            ret = QC_STATUS_UNSUPPORTED;
        }
        else if ( ( this->imgProps.batchSize > 1 ) &&
                  ( this->imgProps.actualHeight[0] != this->imgProps.height ) )
        {
            QC_LOG_ERROR( "image with format %d with extra padding along height is not "
                          "supported to be "
                          "converted to tensor: actualHeight=%u height=%u",
                          this->imgProps.format, this->imgProps.actualHeight[0],
                          this->imgProps.height );
            ret = QC_STATUS_UNSUPPORTED;
        }
        else
        {
            /* OK */
        }
    }
    else
    {
        QC_LOG_ERROR( "image with format %d is not supported to be converted to tensor",
                      this->imgProps.format );
        ret = QC_STATUS_UNSUPPORTED;
    }

    if ( QC_STATUS_OK == ret )
    {
        pSharedBuffer->buffer = this->buffer;
        pSharedBuffer->offset = this->offset;
        pSharedBuffer->type = QC_BUFFER_TYPE_TENSOR;
        pSharedBuffer->tensorProps.type = QC_TENSOR_TYPE_UFIXED_POINT_8;
        pSharedBuffer->tensorProps.numDims = 4;
        pSharedBuffer->tensorProps.dims[0] = this->imgProps.batchSize;
        pSharedBuffer->tensorProps.dims[1] = this->imgProps.height;
        pSharedBuffer->tensorProps.dims[2] = this->imgProps.width;
        pSharedBuffer->tensorProps.dims[3] = s_qcFormatToBytesPerPixel[this->imgProps.format];
        pSharedBuffer->size = (size_t) this->imgProps.batchSize * this->imgProps.height *
                              this->imgProps.width *
                              s_qcFormatToBytesPerPixel[this->imgProps.format];
    }

    return ret;
}

QCStatus_e QCSharedBuffer::ImageToTensor( QCSharedBuffer *pLuma, QCSharedBuffer *pChroma )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pLuma )
    {
        QC_LOG_ERROR( "pLuma is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == pChroma )
    {
        QC_LOG_ERROR( "pChroma is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == this->buffer.pData )
    {
        QC_LOG_ERROR( "image not allocated" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( QC_BUFFER_TYPE_IMAGE != this->type )
    {
        QC_LOG_ERROR( "buffer type %d is not image", this->type );
        ret = QC_STATUS_UNSUPPORTED;
    }
    else if ( ( QC_IMAGE_FORMAT_NV12 == this->imgProps.format ) ||
              ( QC_IMAGE_FORMAT_P010 == this->imgProps.format ) )
    { /* for image format with 2 plane */
        if ( this->imgProps.stride[0] !=
             ( this->imgProps.width * s_qcFormatToBytesPerPixel[this->imgProps.format] ) )
        {
            QC_LOG_ERROR(
                    "image with format %d with extra padding along width is not supported to be "
                    "converted to tensor: stride=%u width=%u",
                    this->imgProps.format, this->imgProps.stride[0], this->imgProps.width );
            ret = QC_STATUS_UNSUPPORTED;
        }
        else if ( this->imgProps.batchSize > 1 )
        {
            QC_LOG_ERROR( "not supported for batched image" );
            ret = QC_STATUS_UNSUPPORTED;
        }
        else if ( 0 != ( this->imgProps.height & 0x1 ) )
        {
            QC_LOG_ERROR( "height is not n times of 2" );
            ret = QC_STATUS_UNSUPPORTED;
        }
        else if ( 0 != ( this->imgProps.width & 0x1 ) )
        {
            QC_LOG_ERROR( "width is not n times of 2" );
            ret = QC_STATUS_UNSUPPORTED;
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
                this->imgProps.format );
        ret = QC_STATUS_UNSUPPORTED;
    }

    if ( QC_STATUS_OK == ret )
    {
        pLuma->buffer = this->buffer;
        pLuma->offset = this->offset;
        pLuma->type = QC_BUFFER_TYPE_TENSOR;
        if ( QC_IMAGE_FORMAT_NV12 == this->imgProps.format )
        {
            pLuma->tensorProps.type = QC_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else
        {
            pLuma->tensorProps.type = QC_TENSOR_TYPE_UFIXED_POINT_16;
        }
        pLuma->tensorProps.numDims = 4;
        pLuma->tensorProps.dims[0] = 1;
        pLuma->tensorProps.dims[1] = this->imgProps.height;
        pLuma->tensorProps.dims[2] = this->imgProps.width;
        pLuma->tensorProps.dims[3] = 1;
        pLuma->size = (size_t) this->imgProps.height * this->imgProps.width *
                      s_qcFormatToBytesPerPixel[this->imgProps.format];

        pChroma->buffer = this->buffer;
        pChroma->offset = this->offset + this->imgProps.planeBufSize[0];
        pChroma->type = QC_BUFFER_TYPE_TENSOR;
        if ( QC_IMAGE_FORMAT_NV12 == this->imgProps.format )
        {
            pChroma->tensorProps.type = QC_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else
        {
            pChroma->tensorProps.type = QC_TENSOR_TYPE_UFIXED_POINT_16;
        }
        pChroma->tensorProps.numDims = 4;
        pChroma->tensorProps.dims[0] = 1;
        pChroma->tensorProps.dims[1] = this->imgProps.height / 2;
        pChroma->tensorProps.dims[2] = this->imgProps.width / 2;
        pChroma->tensorProps.dims[3] = 2;
        pChroma->size = (size_t) this->imgProps.height * this->imgProps.width *
                        s_qcFormatToBytesPerPixel[this->imgProps.format] / 2;
    }

    return ret;
}
}   // namespace QC
