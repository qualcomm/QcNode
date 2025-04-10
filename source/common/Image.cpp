// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/common/BufferManager.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include <apdf.h>

namespace ridehal
{
namespace common
{
#define PLANEDEF_HW_USAGE_FLAGS                                                                    \
    ( WFD_USAGE_OPENGL_ES2 | WFD_USAGE_OPENGL_ES3 | WFD_USAGE_CAPTURE | WFD_USAGE_VIDEO |          \
      WFD_USAGE_DISPLAY | WFD_USAGE_NATIVE )

static PDColorFormat_e s_rideHalFormatToApdfFormat[RIDEHAL_IMAGE_FORMAT_MAX] = {
        PD_FORMAT_RGB888, /* RIDEHAL_IMAGE_FORMAT_RGB888 */
        PD_FORMAT_RGB888, /* RIDEHAL_IMAGE_FORMAT_BGR888 */
        PD_FORMAT_UYVY,   /* RIDEHAL_IMAGE_FORMAT_UYVY */
        PD_FORMAT_NV12,   /* RIDEHAL_IMAGE_FORMAT_NV12 */
        PD_FORMAT_P010,   /* RIDEHAL_IMAGE_FORMAT_P010 */
        PD_FORMAT_NV12,   /* RIDEHAL_IMAGE_FORMAT_NV12_UBWC */
        PD_FORMAT_TP10    /* RIDEHAL_IMAGE_FORMAT_TP10_UBWC */
};

static uint32_t s_rideHalFormatToBytesPerPixel[RIDEHAL_IMAGE_FORMAT_MAX] = {
        3, /* RIDEHAL_IMAGE_FORMAT_RGB888 */
        3, /* RIDEHAL_IMAGE_FORMAT_BGR888 */
        2, /* RIDEHAL_IMAGE_FORMAT_UYVY */
        1, /* RIDEHAL_IMAGE_FORMAT_NV12 */
        2, /* RIDEHAL_IMAGE_FORMAT_P010 */
        1, /* RIDEHAL_IMAGE_FORMAT_NV12_UBWC */
        2  /* RIDEHAL_IMAGE_FORMAT_TP10_UBWC */
};

static uint32_t s_rideHalFormatToNumPlanes[RIDEHAL_IMAGE_FORMAT_MAX] = {
        1, /* RIDEHAL_IMAGE_FORMAT_RGB888 */
        1, /* RIDEHAL_IMAGE_FORMAT_BGR888 */
        1, /* RIDEHAL_IMAGE_FORMAT_UYVY */
        2, /* RIDEHAL_IMAGE_FORMAT_NV12 */
        2, /* RIDEHAL_IMAGE_FORMAT_P010 */
        4, /* RIDEHAL_IMAGE_FORMAT_NV12_UBWC */
        4  /* RIDEHAL_IMAGE_FORMAT_P010_UBWC */
};

static uint32_t s_rideHalFormatToHeightDividerPerPlanes
        [RIDEHAL_IMAGE_FORMAT_MAX][RIDEHAL_NUM_IMAGE_PLANES] = {
                { 1, 0, 0, 0 }, /* RIDEHAL_IMAGE_FORMAT_RGB888 */
                { 1, 0, 0, 0 }, /* RIDEHAL_IMAGE_FORMAT_BGR888 */
                { 1, 0, 0, 0 }, /* RIDEHAL_IMAGE_FORMAT_UYVY */
                { 1, 2, 0, 0 }, /* RIDEHAL_IMAGE_FORMAT_NV12 */
                { 1, 2, 0, 0 }, /* RIDEHAL_IMAGE_FORMAT_P010 */
                { 1, 1, 2, 2 }  /* RIDEHAL_IMAGE_FORMAT_NV12_UBWC */
};

static const char *s_rideHalFormatToString[RIDEHAL_IMAGE_FORMAT_MAX] = {
        "RGB888",   /* RIDEHAL_IMAGE_FORMAT_RGB888 */
        "BGR888",   /* RIDEHAL_IMAGE_FORMAT_BGR888 */
        "UYVY",     /* RIDEHAL_IMAGE_FORMAT_UYVY */
        "NV12",     /* RIDEHAL_IMAGE_FORMAT_NV12 */
        "P010",     /* RIDEHAL_IMAGE_FORMAT_P010 */
        "NV12_UBWC" /* RIDEHAL_IMAGE_FORMAT_NV12_UBWC */
};

RideHalError_e RideHal_SharedBuffer::Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                                               RideHal_ImageFormat_e format,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    size_t size = 0;
    uint32_t nUsage = PLANEDEF_HW_USAGE_FLAGS;
    FrameRes_t frameRes;
    PlaneDef_t planeDef;
    uint32_t numPlanes = 0;
    PDColorFormat_e eColorFormat = PD_FORMAT_MAX;
    PDStatus_e status;
    uint32_t i = 0;

    if ( ( 0 == batchSize ) || ( 0 == width ) || ( 0 == height ) ||
         ( format >= RIDEHAL_IMAGE_FORMAT_MAX ) || ( format < RIDEHAL_IMAGE_FORMAT_RGB888 ) )
    {
        RIDEHAL_LOG_ERROR( "invalid args: batchSize=%u, width=%u, height=%u, format=%d", batchSize,
                           width, height, format );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "buffer is already allocated" );
        ret = RIDEHAL_ERROR_ALREADY;
    }
    else
    {
        eColorFormat = s_rideHalFormatToApdfFormat[format];
    }

    if ( ( RIDEHAL_IMAGE_FORMAT_NV12_UBWC == format ) ||
         ( RIDEHAL_IMAGE_FORMAT_TP10_UBWC == format ) )
    {
        nUsage = nUsage | WFD_USAGE_COMPRESSION;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        status = PDQueryNumPlanes( eColorFormat, nUsage, &numPlanes, 0 );
        if ( ( PD_OK != status ) || ( 0 == numPlanes ) || ( numPlanes > RIDEHAL_NUM_IMAGE_PLANES ) )
        {
            RIDEHAL_LOG_ERROR(
                    "failed to query plane numbers for image width=%u, height=%u, format=%d: %d",
                    width, height, format, status );
            ret = RIDEHAL_ERROR_FAIL;
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
            this->type = RIDEHAL_BUFFER_TYPE_IMAGE;
            RIDEHAL_LOG_VERBOSE( "PlaneDef for image %ux%u format %s\n", width, height,
                                 s_rideHalFormatToString[format] );
        }
    }

    for ( i = 0; i < numPlanes; i++ )
    {
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            break;
        }
        planeDef.nPlaneIndex = i + 1;
        status = PDQueryPlaneDef( eColorFormat, nUsage, &frameRes, &planeDef, 0 );
        if ( PD_OK == status )
        {
            RIDEHAL_LOG_VERBOSE(
                    " plane %u: nMinStride = %u, nMaxstride = %u nStrideMultiples = %u", i,
                    planeDef.nMinStride, planeDef.nMaxstride, planeDef.nStrideMultiples );
            RIDEHAL_LOG_VERBOSE(
                    "   nActualStride = %u nMinPlaneBufHeight = %u, nHeightMultiples = %u",
                    planeDef.nActualStride, planeDef.nMinPlaneBufHeight,
                    planeDef.nHeightMultiples );
            RIDEHAL_LOG_VERBOSE( "   nActualPlaneBufHeight = %u nActualBufSizeAlignment =%u,",
                                 planeDef.nActualPlaneBufHeight, planeDef.nActualBufSizeAlignment );
            RIDEHAL_LOG_VERBOSE(
                    "   nBufAddrAlignment = %u nPlaneBufSize = %u, nPlanePaddingSize = %u",
                    planeDef.nBufAddrAlignment, planeDef.nPlaneBufSize,
                    planeDef.nPlanePaddingSize );
            this->imgProps.stride[i] = planeDef.nActualStride;
            this->imgProps.actualHeight[i] = planeDef.nActualPlaneBufHeight;
            this->imgProps.planeBufSize[i] = planeDef.nPlaneBufSize + planeDef.nPlanePaddingSize;
            size += (size_t) this->imgProps.planeBufSize[i];
        }
        else
        {
            RIDEHAL_LOG_ERROR(
                    "failed to query plane def for image width=%u, height=%u, format=%d: %d", width,
                    height, format, status );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        size = size * batchSize;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::Allocate( uint32_t width, uint32_t height,
                                               RideHal_ImageFormat_e format,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    return Allocate( 1, width, height, format, usage, flags );
}

RideHalError_e RideHal_SharedBuffer::Allocate( const RideHal_ImageProps_t *pImgProps,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    size_t size = 0;
    uint32_t i = 0;
    uint32_t bpp = 0;
    uint32_t divider = 0;

    if ( nullptr == pImgProps )
    {
        RIDEHAL_LOG_ERROR( "pImgProps is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( 0 == pImgProps->batchSize ) || ( 0 == pImgProps->width ) ||
              ( 0 == pImgProps->height ) )
    {
        RIDEHAL_LOG_ERROR( "invalid args: batchSize=%u, width=%u, height=%u", pImgProps->batchSize,
                           pImgProps->width, pImgProps->height );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( pImgProps->format >= RIDEHAL_IMAGE_FORMAT_MAX ) &&
              ( pImgProps->format < RIDEHAL_IMAGE_FORMAT_COMPRESSED_MIN ) )
    {
        RIDEHAL_LOG_ERROR( "invalid args: format=%d", pImgProps->format );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( pImgProps->format >= RIDEHAL_IMAGE_FORMAT_COMPRESSED_MAX )
    {
        RIDEHAL_LOG_ERROR( "invalid args: format=%d", pImgProps->format );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( pImgProps->format < RIDEHAL_IMAGE_FORMAT_RGB888 )
    {
        RIDEHAL_LOG_ERROR( "invalid args: format=%d", pImgProps->format );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "buffer is already allocated" );
        ret = RIDEHAL_ERROR_ALREADY;
    }
    else
    {
        if ( pImgProps->format < RIDEHAL_IMAGE_FORMAT_MAX )
        { /* check properties for uncompressed image */
            if ( s_rideHalFormatToNumPlanes[pImgProps->format] != pImgProps->numPlanes )
            {
                RIDEHAL_LOG_ERROR(
                        "plane number %u != expected %u for format %d", pImgProps->numPlanes,
                        s_rideHalFormatToNumPlanes[pImgProps->format], pImgProps->format );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
        else
        { /* check properties for compressed image */
            if ( 1 != pImgProps->numPlanes )
            {
                RIDEHAL_LOG_ERROR( "invalid numPlanes for compressed image" );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( 0 == pImgProps->planeBufSize[0] )
            {
                RIDEHAL_LOG_ERROR( "invalid planeBufSize[0] for compressed image" );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                /* OK */
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( pImgProps->format < RIDEHAL_IMAGE_FORMAT_MAX )
        {
            bpp = s_rideHalFormatToBytesPerPixel[pImgProps->format];
            /* check each plane def is reasonable */
            for ( i = 0; i < pImgProps->numPlanes; i++ )
            {
                divider = s_rideHalFormatToHeightDividerPerPlanes[pImgProps->format][i];
                if ( ( pImgProps->width * bpp ) > pImgProps->stride[i] )
                {
                    RIDEHAL_LOG_ERROR( "given stride %u(<%u) too small for plane %u for format %d",
                                       pImgProps->stride[i], pImgProps->width * bpp, i,
                                       pImgProps->format );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( ( pImgProps->height / divider ) > pImgProps->actualHeight[i] )
                {
                    RIDEHAL_LOG_ERROR(
                            "given actual height %u(<%u) too small for plane %u for format %d",
                            pImgProps->actualHeight[i], pImgProps->height / divider, i,
                            pImgProps->format );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( ( 0 != pImgProps->planeBufSize[i] ) &&
                          ( pImgProps->planeBufSize[i] <
                            ( pImgProps->stride[i] * pImgProps->actualHeight[i] ) ) )
                {
                    RIDEHAL_LOG_ERROR(
                            "given plane buffer size %u(<%u) too small for plane %u for format %d",
                            pImgProps->planeBufSize[i],
                            pImgProps->stride[i] * pImgProps->actualHeight[i], i,
                            pImgProps->format );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else
                {
                    /* OK */
                }

                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    break;
                }
            }
            if ( RIDEHAL_ERROR_NONE == ret )
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

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        this->imgProps = *pImgProps;
        for ( i = 0; i < pImgProps->numPlanes; i++ )
        {
            if ( 0 == pImgProps->planeBufSize[i] )
            {
                this->imgProps.planeBufSize[i] = pImgProps->stride[i] * pImgProps->actualHeight[i];
            }
        }
        this->type = RIDEHAL_BUFFER_TYPE_IMAGE;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::GetSharedBuffer( RideHal_SharedBuffer_t *pSharedBuffer,
                                                      uint32_t batchOffset, uint32_t batchSize )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    size_t singleImageSize = 0;

    if ( nullptr == this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "image not allocated" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != this->type )
    {
        RIDEHAL_LOG_ERROR( "buffer type %d is not image", this->type );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }
    else if ( nullptr == pSharedBuffer )
    {
        RIDEHAL_LOG_ERROR( "pSharedBuffer is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( batchOffset >= this->imgProps.batchSize )
    {
        RIDEHAL_LOG_ERROR( "buffer batch offset %u(>=%u) out of range", batchOffset,
                           this->imgProps.batchSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( batchOffset + batchSize ) >= this->imgProps.batchSize )
    {
        RIDEHAL_LOG_ERROR( "buffer batch size %u out of range", batchSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        *pSharedBuffer = *this;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        singleImageSize = pSharedBuffer->buffer.size / pSharedBuffer->imgProps.batchSize;
        pSharedBuffer->imgProps.batchSize = batchSize;
        pSharedBuffer->size = batchSize * singleImageSize;
        pSharedBuffer->offset = batchOffset * singleImageSize;
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::ImageToTensor( RideHal_SharedBuffer *pSharedBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSharedBuffer )
    {
        RIDEHAL_LOG_ERROR( "pSharedBuffer is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "image not allocated" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != this->type )
    {
        RIDEHAL_LOG_ERROR( "buffer type %d is not image", this->type );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }
    else if ( ( RIDEHAL_IMAGE_FORMAT_RGB888 == this->imgProps.format ) ||
              ( RIDEHAL_IMAGE_FORMAT_BGR888 == this->imgProps.format ) ||
              ( RIDEHAL_IMAGE_FORMAT_UYVY == this->imgProps.format ) )
    { /* for image format with just 1 plane */
        if ( this->imgProps.stride[0] !=
             ( this->imgProps.width * s_rideHalFormatToBytesPerPixel[this->imgProps.format] ) )
        {
            RIDEHAL_LOG_ERROR(
                    "image with format %d with extra padding along width is not supported to be "
                    "converted to tensor: stride=%u width=%u",
                    this->imgProps.format, this->imgProps.stride[0], this->imgProps.width );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
        else if ( ( this->imgProps.batchSize > 1 ) &&
                  ( this->imgProps.actualHeight[0] != this->imgProps.height ) )
        {
            RIDEHAL_LOG_ERROR( "image with format %d with extra padding along height is not "
                               "supported to be "
                               "converted to tensor: actualHeight=%u height=%u",
                               this->imgProps.format, this->imgProps.actualHeight[0],
                               this->imgProps.height );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
        else
        {
            /* OK */
        }
    }
    else
    {
        RIDEHAL_LOG_ERROR( "image with format %d is not supported to be converted to tensor",
                           this->imgProps.format );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        pSharedBuffer->buffer = this->buffer;
        pSharedBuffer->offset = this->offset;
        pSharedBuffer->type = RIDEHAL_BUFFER_TYPE_TENSOR;
        pSharedBuffer->tensorProps.type = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8;
        pSharedBuffer->tensorProps.numDims = 4;
        pSharedBuffer->tensorProps.dims[0] = this->imgProps.batchSize;
        pSharedBuffer->tensorProps.dims[1] = this->imgProps.height;
        pSharedBuffer->tensorProps.dims[2] = this->imgProps.width;
        pSharedBuffer->tensorProps.dims[3] = s_rideHalFormatToBytesPerPixel[this->imgProps.format];
        pSharedBuffer->size = (size_t) this->imgProps.batchSize * this->imgProps.height *
                              this->imgProps.width *
                              s_rideHalFormatToBytesPerPixel[this->imgProps.format];
    }

    return ret;
}

RideHalError_e RideHal_SharedBuffer::ImageToTensor( RideHal_SharedBuffer *pLuma,
                                                    RideHal_SharedBuffer *pChroma )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pLuma )
    {
        RIDEHAL_LOG_ERROR( "pLuma is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pChroma )
    {
        RIDEHAL_LOG_ERROR( "pChroma is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "image not allocated" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != this->type )
    {
        RIDEHAL_LOG_ERROR( "buffer type %d is not image", this->type );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }
    else if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == this->imgProps.format ) ||
              ( RIDEHAL_IMAGE_FORMAT_P010 == this->imgProps.format ) )
    { /* for image format with 2 plane */
        if ( this->imgProps.stride[0] !=
             ( this->imgProps.width * s_rideHalFormatToBytesPerPixel[this->imgProps.format] ) )
        {
            RIDEHAL_LOG_ERROR(
                    "image with format %d with extra padding along width is not supported to be "
                    "converted to tensor: stride=%u width=%u",
                    this->imgProps.format, this->imgProps.stride[0], this->imgProps.width );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
        else if ( this->imgProps.batchSize > 1 )
        {
            RIDEHAL_LOG_ERROR( "not supported for batched image" );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
        else if ( 0 != ( this->imgProps.height & 0x1 ) )
        {
            RIDEHAL_LOG_ERROR( "height is not n times of 2" );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
        else if ( 0 != ( this->imgProps.width & 0x1 ) )
        {
            RIDEHAL_LOG_ERROR( "width is not n times of 2" );
            ret = RIDEHAL_ERROR_UNSUPPORTED;
        }
        else
        {
            /* OK */
        }
    }
    else
    {
        RIDEHAL_LOG_ERROR(
                "image with format %d is not supported to be converted to luma and chroma tensor",
                this->imgProps.format );
        ret = RIDEHAL_ERROR_UNSUPPORTED;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        pLuma->buffer = this->buffer;
        pLuma->offset = this->offset;
        pLuma->type = RIDEHAL_BUFFER_TYPE_TENSOR;
        if ( RIDEHAL_IMAGE_FORMAT_NV12 == this->imgProps.format )
        {
            pLuma->tensorProps.type = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else
        {
            pLuma->tensorProps.type = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16;
        }
        pLuma->tensorProps.numDims = 4;
        pLuma->tensorProps.dims[0] = 1;
        pLuma->tensorProps.dims[1] = this->imgProps.height;
        pLuma->tensorProps.dims[2] = this->imgProps.width;
        pLuma->tensorProps.dims[3] = 1;
        pLuma->size = (size_t) this->imgProps.height * this->imgProps.width *
                      s_rideHalFormatToBytesPerPixel[this->imgProps.format];

        pChroma->buffer = this->buffer;
        pChroma->offset = this->offset + this->imgProps.planeBufSize[0];
        pChroma->type = RIDEHAL_BUFFER_TYPE_TENSOR;
        if ( RIDEHAL_IMAGE_FORMAT_NV12 == this->imgProps.format )
        {
            pChroma->tensorProps.type = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else
        {
            pChroma->tensorProps.type = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16;
        }
        pChroma->tensorProps.numDims = 4;
        pChroma->tensorProps.dims[0] = 1;
        pChroma->tensorProps.dims[1] = this->imgProps.height / 2;
        pChroma->tensorProps.dims[2] = this->imgProps.width / 2;
        pChroma->tensorProps.dims[3] = 2;
        pChroma->size = (size_t) this->imgProps.height * this->imgProps.width *
                        s_rideHalFormatToBytesPerPixel[this->imgProps.format] / 2;
    }

    return ret;
}
}   // namespace common
}   // namespace ridehal
