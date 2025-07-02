// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/Utils/ImageAllocator.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/Utils/BasicImageAllocator.hpp"
#include <algorithm>
#include <apdf.h>
#include <unistd.h>

namespace QC
{
namespace Memory
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
        4  /* QC_IMAGE_FORMAT_TP10_UBWC */
};

static uint32_t s_qcFormatToHeightDividerPerPlanes[QC_IMAGE_FORMAT_MAX][QC_NUM_IMAGE_PLANES] = {
        { 1, 0, 0, 0 }, /* QC_IMAGE_FORMAT_RGB888 */
        { 1, 0, 0, 0 }, /* QC_IMAGE_FORMAT_BGR888 */
        { 1, 0, 0, 0 }, /* QC_IMAGE_FORMAT_UYVY */
        { 1, 2, 0, 0 }, /* QC_IMAGE_FORMAT_NV12 */
        { 1, 2, 0, 0 }, /* QC_IMAGE_FORMAT_P010 */
        { 1, 1, 2, 2 }, /* QC_IMAGE_FORMAT_NV12_UBWC */
        { 1, 1, 2, 2 }  /* QC_IMAGE_FORMAT_TP10_UBWC */
};

static const char *s_qcFormatToString[QC_IMAGE_FORMAT_MAX] = {
        "RGB888",    /* QC_IMAGE_FORMAT_RGB888 */
        "BGR888",    /* QC_IMAGE_FORMAT_BGR888 */
        "UYVY",      /* QC_IMAGE_FORMAT_UYVY */
        "NV12",      /* QC_IMAGE_FORMAT_NV12 */
        "P010",      /* QC_IMAGE_FORMAT_P010 */
        "NV12_UBWC", /* QC_IMAGE_FORMAT_NV12_UBWC */
        "TP_UBWC"    /* QC_IMAGE_FORMAT_TP10_UBWC */
};

BasicImageAllocator::BasicImageAllocator( const std::string &name ) : BinaryAllocator( name )
{
    (void) QC_LOGGER_INIT( m_name.c_str(), LOGGER_LEVEL_ERROR );
};

BasicImageAllocator::~BasicImageAllocator()
{
    (void) QC_LOGGER_DEINIT();
};

QCStatus_e BasicImageAllocator::Allocate( const QCBufferPropBase_t &request,
                                          QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    const ImageBasicProps_t *pImageBasicProps = dynamic_cast<const ImageBasicProps_t *>( &request );
    ImageDescriptor_t *pImageDescriptor = dynamic_cast<ImageDescriptor_t *>( &response );

    if ( ( nullptr != pImageBasicProps ) && ( nullptr != pImageDescriptor ) )
    {
        uint32_t batchSize = pImageBasicProps->batchSize;
        uint32_t width = pImageBasicProps->width;
        uint32_t height = pImageBasicProps->height;
        QCImageFormat_e format = pImageBasicProps->format;
        BufferProps_t bufProps = *pImageBasicProps;
        size_t size = 0;
        uint32_t nUsage = PLANEDEF_HW_USAGE_FLAGS;
        FrameRes_t frameRes;
        PlaneDef_t planeDef;
        uint32_t numPlanes = 0;
        PDColorFormat_e eColorFormat = PD_FORMAT_MAX;
        PDStatus_e pdStatus;
        uint32_t i = 0;
        if ( ( 0 == batchSize ) || ( 0 == width ) || ( 0 == height ) ||
             ( format >= QC_IMAGE_FORMAT_MAX ) || ( format < QC_IMAGE_FORMAT_RGB888 ) )
        {
            QC_ERROR( "invalid args: batchSize=%u, width=%u, height=%u, format=%d", batchSize,
                      width, height, format );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            eColorFormat = s_qcFormatToApdfFormat[format];
        }

        if ( ( QC_IMAGE_FORMAT_NV12_UBWC == format ) || ( QC_IMAGE_FORMAT_TP10_UBWC == format ) )
        {
            nUsage = nUsage | WFD_USAGE_COMPRESSION;
        }

        if ( QC_STATUS_OK == status )
        {
            pdStatus = PDQueryNumPlanes( eColorFormat, nUsage, &numPlanes, 0 );
            if ( ( PD_OK != pdStatus ) || ( 0 == numPlanes ) ||
                 ( numPlanes > QC_NUM_IMAGE_PLANES ) )
            {
                QC_ERROR( "failed to query plane numbers for image width=%u, height=%u, "
                          "format=%d: %d",
                          width, height, format, pdStatus );
                status = QC_STATUS_FAIL;
            }
            else
            {
                frameRes.nWidthInPixels = width;
                frameRes.nHeightInPixels = height;
                pImageDescriptor->format = format;
                pImageDescriptor->batchSize = batchSize;
                pImageDescriptor->width = width;
                pImageDescriptor->height = height;
                pImageDescriptor->numPlanes = numPlanes;

                QC_VERBOSE( "PlaneDef for image %ux%u format %s\n", width, height,
                            s_qcFormatToString[format] );
            }
        }

        for ( i = 0; i < numPlanes; i++ )
        {
            if ( QC_STATUS_OK != status )
            {
                break;
            }
            planeDef.nPlaneIndex = i + 1;
            pdStatus = PDQueryPlaneDef( eColorFormat, nUsage, &frameRes, &planeDef, 0 );
            if ( PD_OK == pdStatus )
            {
                QC_VERBOSE( " plane %u: nMinStride = %u, nMaxstride = %u nStrideMultiples = %u", i,
                            planeDef.nMinStride, planeDef.nMaxstride, planeDef.nStrideMultiples );
                QC_VERBOSE( "   nActualStride = %u nMinPlaneBufHeight = %u, nHeightMultiples = %u",
                            planeDef.nActualStride, planeDef.nMinPlaneBufHeight,
                            planeDef.nHeightMultiples );
                QC_VERBOSE( "   nActualPlaneBufHeight = %u nActualBufSizeAlignment =%u,",
                            planeDef.nActualPlaneBufHeight, planeDef.nActualBufSizeAlignment );
                QC_VERBOSE( "   nBufAddrAlignment = %u nPlaneBufSize = %u, nPlanePaddingSize = %u",
                            planeDef.nBufAddrAlignment, planeDef.nPlaneBufSize,
                            planeDef.nPlanePaddingSize );
                pImageDescriptor->stride[i] = planeDef.nActualStride;
                pImageDescriptor->actualHeight[i] = planeDef.nActualPlaneBufHeight;
                pImageDescriptor->planeBufSize[i] =
                        planeDef.nPlaneBufSize + planeDef.nPlanePaddingSize;
                size += (size_t) pImageDescriptor->planeBufSize[i];
            }
            else
            {
                QC_ERROR( "failed to query plane def for image width=%u, height=%u, format=%d: %d",
                          width, height, format, pdStatus );
                status = QC_STATUS_FAIL;
            }
        }

        if ( QC_STATUS_OK == status )
        {
            size = size * batchSize;
            bufProps.size = size;
            status = BinaryAllocator::Allocate( bufProps, *pImageDescriptor );
            if ( QC_STATUS_OK == status )
            {
                pImageDescriptor->type = QC_BUFFER_TYPE_IMAGE;
            }
        }
    }
    else
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

ImageAllocator::ImageAllocator( const std::string &name ) : BinaryAllocator( name )
{
    (void) QC_LOGGER_INIT( m_name.c_str(), LOGGER_LEVEL_ERROR );
};

ImageAllocator::~ImageAllocator()
{
    (void) QC_LOGGER_DEINIT();
};

QCStatus_e ImageAllocator::Allocate( const QCBufferPropBase_t &request,
                                     QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    const ImageProps_t *pImageProps = dynamic_cast<const ImageProps_t *>( &request );
    ImageDescriptor_t *pImageDescriptor = dynamic_cast<ImageDescriptor_t *>( &response );

    if ( ( nullptr != pImageProps ) && ( nullptr != pImageDescriptor ) )
    {
        BufferProps_t bufProps = *pImageProps;
        size_t size = 0;
        uint32_t i = 0;
        uint32_t bpp = 0;
        uint32_t divider = 0;

        if ( ( 0 == pImageProps->batchSize ) || ( 0 == pImageProps->width ) ||
             ( 0 == pImageProps->height ) )
        {
            QC_ERROR( "invalid args: batchSize=%u, width=%u, height=%u", pImageProps->batchSize,
                      pImageProps->width, pImageProps->height );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( ( pImageProps->format >= QC_IMAGE_FORMAT_MAX ) &&
                  ( pImageProps->format < QC_IMAGE_FORMAT_COMPRESSED_MIN ) )
        {
            QC_ERROR( "invalid args: format=%d", pImageProps->format );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( pImageProps->format >= QC_IMAGE_FORMAT_COMPRESSED_MAX )
        {
            QC_ERROR( "invalid args: format=%d", pImageProps->format );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( pImageProps->format < QC_IMAGE_FORMAT_RGB888 )
        {
            QC_ERROR( "invalid args: format=%d", pImageProps->format );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            if ( pImageProps->format < QC_IMAGE_FORMAT_MAX )
            { /* check properties for uncompressed image */
                if ( s_qcFormatToNumPlanes[pImageProps->format] != pImageProps->numPlanes )
                {
                    QC_ERROR( "plane number %u != expected %u for format %d",
                              pImageProps->numPlanes, s_qcFormatToNumPlanes[pImageProps->format],
                              pImageProps->format );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
            }
            else
            { /* check properties for compressed image */
                if ( 1 != pImageProps->numPlanes )
                {
                    QC_ERROR( "invalid numPlanes for compressed image" );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( 0 == pImageProps->planeBufSize[0] )
                {
                    QC_ERROR( "invalid planeBufSize[0] for compressed image" );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
                else
                {
                    /* OK */
                }
            }
        }

        if ( QC_STATUS_OK == status )
        {
            if ( pImageProps->format < QC_IMAGE_FORMAT_MAX )
            {
                bpp = s_qcFormatToBytesPerPixel[pImageProps->format];
                /* check each plane def is reasonable */
                for ( i = 0; i < pImageProps->numPlanes; i++ )
                {
                    divider = s_qcFormatToHeightDividerPerPlanes[pImageProps->format][i];
                    if ( ( pImageProps->width * bpp ) > pImageProps->stride[i] )
                    {
                        QC_ERROR( "given stride %u(<%u) too small for plane %u for format %d",
                                  pImageProps->stride[i], pImageProps->width * bpp, i,
                                  pImageProps->format );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    else if ( ( pImageProps->height / divider ) > pImageProps->actualHeight[i] )
                    {
                        QC_ERROR(
                                "given actual height %u(<%u) too small for plane %u for format %d",
                                pImageProps->actualHeight[i], pImageProps->height / divider, i,
                                pImageProps->format );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    else if ( ( 0 != pImageProps->planeBufSize[i] ) &&
                              ( pImageProps->planeBufSize[i] <
                                ( pImageProps->stride[i] * pImageProps->actualHeight[i] ) ) )
                    {
                        QC_ERROR( "given plane buffer size %u(<%u) too small for plane %u for "
                                  "format %d",
                                  pImageProps->planeBufSize[i],
                                  pImageProps->stride[i] * pImageProps->actualHeight[i], i,
                                  pImageProps->format );
                        status = QC_STATUS_BAD_ARGUMENTS;
                    }
                    else
                    {
                        /* OK */
                    }

                    if ( QC_STATUS_OK != status )
                    {
                        break;
                    }
                }
                if ( QC_STATUS_OK == status )
                {
                    for ( i = 0; i < pImageProps->numPlanes; i++ )
                    {
                        if ( 0 != pImageProps->planeBufSize[i] )
                        {
                            size += (size_t) pImageProps->planeBufSize[i];
                        }
                        else
                        {
                            size += (size_t) pImageProps->stride[i] * pImageProps->actualHeight[i];
                        }
                    }
                    size = size * pImageProps->batchSize;
                }
            }
            else
            {
                size = pImageProps->planeBufSize[0];
            }
        }

        if ( QC_STATUS_OK == status )
        {
            bufProps.size = size;
            status = BinaryAllocator::Allocate( bufProps, *pImageDescriptor );
            if ( QC_STATUS_OK == status )
            {
                pImageDescriptor->type = QC_BUFFER_TYPE_IMAGE;
                pImageDescriptor->format = pImageProps->format;
                pImageDescriptor->batchSize = pImageProps->batchSize;
                pImageDescriptor->width = pImageProps->width;
                pImageDescriptor->height = pImageProps->height;
                pImageDescriptor->numPlanes = pImageProps->numPlanes;
                std::copy( pImageProps->stride, pImageProps->stride + pImageProps->numPlanes,
                           pImageDescriptor->stride );
                std::copy( pImageProps->actualHeight,
                           pImageProps->actualHeight + pImageProps->numPlanes,
                           pImageDescriptor->actualHeight );
                std::copy( pImageProps->planeBufSize,
                           pImageProps->planeBufSize + pImageProps->numPlanes,
                           pImageDescriptor->planeBufSize );
                for ( i = 0; i < pImageDescriptor->numPlanes; i++ )
                {
                    if ( 0 == pImageDescriptor->planeBufSize[i] )
                    {
                        pImageDescriptor->planeBufSize[i] =
                                pImageDescriptor->stride[i] * pImageDescriptor->actualHeight[i];
                    }
                }
            }
        }
    }
    else
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

}   // namespace Memory

}   // namespace QC
