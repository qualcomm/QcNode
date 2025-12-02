// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/UtilsBase.hpp"

namespace QC
{
namespace Memory
{

#define PLANEDEF_HW_USAGE_FLAGS                                                                    \
    ( (uint32_t) WFD_USAGE_OPENGL_ES2 | (uint32_t) WFD_USAGE_OPENGL_ES3 |                          \
      (uint32_t) WFD_USAGE_CAPTURE | (uint32_t) WFD_USAGE_VIDEO | (uint32_t) WFD_USAGE_DISPLAY |   \
      (uint32_t) WFD_USAGE_NATIVE )

const char *UtilsBase::s_qcFormatToString[QC_IMAGE_FORMAT_MAX] = {
        "RGB888",    /* QC_IMAGE_FORMAT_RGB888 */
        "BGR888",    /* QC_IMAGE_FORMAT_BGR888 */
        "UYVY",      /* QC_IMAGE_FORMAT_UYVY */
        "NV12",      /* QC_IMAGE_FORMAT_NV12 */
        "P010",      /* QC_IMAGE_FORMAT_P010 */
        "NV12_UBWC", /* QC_IMAGE_FORMAT_NV12_UBWC */
        "TP_UBWC"    /* QC_IMAGE_FORMAT_TP10_UBWC */
};

const PDColorFormat_e UtilsBase::s_qcFormatToApdfFormat[QC_IMAGE_FORMAT_MAX] = {
        PD_FORMAT_RGB888, /* QC_IMAGE_FORMAT_RGB888 */
        PD_FORMAT_RGB888, /* QC_IMAGE_FORMAT_BGR888 */
        PD_FORMAT_UYVY,   /* QC_IMAGE_FORMAT_UYVY */
        PD_FORMAT_NV12,   /* QC_IMAGE_FORMAT_NV12 */
        PD_FORMAT_P010,   /* QC_IMAGE_FORMAT_P010 */
        PD_FORMAT_NV12,   /* QC_IMAGE_FORMAT_NV12_UBWC */
        PD_FORMAT_TP10    /* QC_IMAGE_FORMAT_TP10_UBWC */
};

const uint32_t UtilsBase::s_qcTensorTypeToDataSize[QC_TENSOR_TYPE_MAX] = {
        1, /* QC_TENSOR_TYPE_INT_8 */
        2, /* QC_TENSOR_TYPE_INT_16 */
        4, /* QC_TENSOR_TYPE_INT_32 */
        8, /* QC_TENSOR_TYPE_INT_64 */

        1, /* QC_TENSOR_TYPE_UINT_8 */
        2, /* QC_TENSOR_TYPE_UINT_16 */
        4, /* QC_TENSOR_TYPE_UINT_32 */
        8, /* QC_TENSOR_TYPE_UINT_64 */

        2, /* QC_TENSOR_TYPE_FLOAT_16 */
        4, /* QC_TENSOR_TYPE_FLOAT_32 */
        8, /* QC_TENSOR_TYPE_FLOAT_64 */

        1, /* QC_TENSOR_TYPE_SFIXED_POINT_8 */
        2, /* QC_TENSOR_TYPE_SFIXED_POINT_16 */
        4, /* QC_TENSOR_TYPE_SFIXED_POINT_32 */

        1, /* QC_TENSOR_TYPE_UFIXED_POINT_8 */
        2, /* QC_TENSOR_TYPE_UFIXED_POINT_16 */
        4  /* QC_TENSOR_TYPE_UFIXED_POINT_32 */
};

const uint32_t UtilsBase::s_qcFormatToBytesPerPixel[QC_IMAGE_FORMAT_MAX] = {
        3, /* QC_IMAGE_FORMAT_RGB888 */
        3, /* QC_IMAGE_FORMAT_BGR888 */
        2, /* QC_IMAGE_FORMAT_UYVY */
        1, /* QC_IMAGE_FORMAT_NV12 */
        2, /* QC_IMAGE_FORMAT_P010 */
        1, /* QC_IMAGE_FORMAT_NV12_UBWC */
        2  /* QC_IMAGE_FORMAT_TP10_UBWC */
};

const uint32_t UtilsBase::s_qcFormatToNumPlanes[QC_IMAGE_FORMAT_MAX] = {
        1, /* QC_IMAGE_FORMAT_RGB888 */
        1, /* QC_IMAGE_FORMAT_BGR888 */
        1, /* QC_IMAGE_FORMAT_UYVY */
        2, /* QC_IMAGE_FORMAT_NV12 */
        2, /* QC_IMAGE_FORMAT_P010 */
        4, /* QC_IMAGE_FORMAT_NV12_UBWC */
        4  /* QC_IMAGE_FORMAT_TP10_UBWC */
};

const uint32_t
        UtilsBase::s_qcFormatToHeightDividerPerPlanes[QC_IMAGE_FORMAT_MAX][QC_NUM_IMAGE_PLANES] = {
                { 1, 1, 1, 1 }, /* QC_IMAGE_FORMAT_RGB888 */
                { 1, 1, 1, 1 }, /* QC_IMAGE_FORMAT_BGR888 */
                { 1, 1, 1, 1 }, /* QC_IMAGE_FORMAT_UYVY */
                { 1, 2, 1, 1 }, /* QC_IMAGE_FORMAT_NV12 */
                { 1, 2, 1, 1 }, /* QC_IMAGE_FORMAT_P010 */
                { 1, 1, 2, 2 }, /* QC_IMAGE_FORMAT_NV12_UBWC */
                { 1, 1, 2, 2 }  /* QC_IMAGE_FORMAT_TP10_UBWC */
};


UtilsBase::UtilsBase()
{
    (void) QC_LOGGER_INIT( "UtilsBase", LOGGER_LEVEL_VERBOSE );
}

UtilsBase::~UtilsBase()
{
    (void) QC_LOGGER_DEINIT();
}

QCStatus_e UtilsBase::MemoryMap( const QCBufferDescriptorBase_t &orig,
                                 QCBufferDescriptorBase_t &mapped )
{
    return QC_STATUS_UNSUPPORTED;
}

QCStatus_e UtilsBase::MemoryUnMap( const QCBufferDescriptorBase_t &buff )
{
    return QC_STATUS_UNSUPPORTED;
}


QCStatus_e UtilsBase::SetTensorDescFromTensorProp( TensorProps_t &prop, TensorDescriptor_t &desc )
{
    QCStatus_e status = QC_STATUS_OK;

    size_t size = 1;
    uint32_t i = 0;

    if ( ( prop.numDims > QC_NUM_TENSOR_DIMS ) || ( prop.tensorType >= QC_TENSOR_TYPE_MAX ) ||
         ( prop.tensorType < QC_TENSOR_TYPE_INT_8 ) )
    {
        QC_ERROR( "Props has invalid numDims or type" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        /* check each dimension is reasonable */
        for ( i = 0; i < prop.numDims; i++ )
        {
            if ( 0 == prop.dims[i] )
            {
                QC_ERROR( "Props dims[%u] is 0", i );
                status = QC_STATUS_BAD_ARGUMENTS;
                break;
            }
        }
    }

    if ( QC_STATUS_OK == status )
    {
        for ( i = 0; i < prop.numDims; i++ )
        {
            size *= prop.dims[i];
        }
        size *= s_qcTensorTypeToDataSize[prop.tensorType];
    }

    if ( QC_STATUS_OK == status )
    {
        desc.type = QC_BUFFER_TYPE_TENSOR;
        desc.numDims = prop.numDims;
        desc.tensorType = prop.tensorType;
        prop.size = size;
        std::copy( prop.dims, prop.dims + prop.numDims, desc.dims );
    }

    return status;
}

QCStatus_e UtilsBase::SetImageDescFromImageBasicProp( ImageBasicProps_t &prop,
                                                      ImageDescriptor_t &desc )
{
    QCStatus_e status = QC_STATUS_OK;

    uint32_t batchSize = prop.batchSize;
    uint32_t width = prop.width;
    uint32_t height = prop.height;
    QCImageFormat_e format = prop.format;
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
        QC_ERROR( "invalid args: batchSize=%u, width=%u, height=%u, format=%d", batchSize, width,
                  height, format );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        eColorFormat = s_qcFormatToApdfFormat[format];
    }

    if ( QC_STATUS_OK == status )
    {
        if ( ( QC_IMAGE_FORMAT_NV12_UBWC == format ) || ( QC_IMAGE_FORMAT_TP10_UBWC == format ) )
        {
            nUsage = nUsage | (uint32_t) WFD_USAGE_COMPRESSION;
        }

        pdStatus = PDQueryNumPlanes( eColorFormat, nUsage, &numPlanes, 0 );
        if ( ( PD_OK != pdStatus ) || ( 0 == numPlanes ) || ( numPlanes > QC_NUM_IMAGE_PLANES ) )
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
            desc.format = format;
            desc.batchSize = batchSize;
            desc.width = width;
            desc.height = height;
            desc.numPlanes = numPlanes;

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
            desc.stride[i] = planeDef.nActualStride;
            desc.actualHeight[i] = planeDef.nActualPlaneBufHeight;
            desc.planeBufSize[i] = planeDef.nPlaneBufSize + planeDef.nPlanePaddingSize;
            size += (size_t) desc.planeBufSize[i];
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
        prop.size = size;
        desc.type = QC_BUFFER_TYPE_IMAGE;
    }

    return status;
}

QCStatus_e UtilsBase::SetImageDescFromImageProp( ImageProps_t &prop, ImageDescriptor_t &desc )
{
    QCStatus_e status = QC_STATUS_OK;

    size_t size = 0;
    uint32_t i = 0;
    uint32_t bpp = 0;
    uint32_t divider = 0;

    if ( ( 0 == prop.batchSize ) || ( 0 == prop.width ) || ( 0 == prop.height ) )
    {
        QC_ERROR( "invalid args: batchSize=%u, width=%u, height=%u", prop.batchSize, prop.width,
                  prop.height );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( prop.format >= QC_IMAGE_FORMAT_MAX ) &&
              ( prop.format < QC_IMAGE_FORMAT_COMPRESSED_MIN ) )
    {
        QC_ERROR( "invalid args: format=%d", prop.format );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( prop.format >= QC_IMAGE_FORMAT_COMPRESSED_MAX )
    {
        QC_ERROR( "invalid args: format=%d", prop.format );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( prop.format < QC_IMAGE_FORMAT_RGB888 )
    {
        QC_ERROR( "invalid args: format=%d", prop.format );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        if ( prop.format < QC_IMAGE_FORMAT_MAX )
        { /* check properties for uncompressed image */
            if ( s_qcFormatToNumPlanes[prop.format] != prop.numPlanes )
            {
                QC_ERROR( "plane number %u != expected %u for format %d", prop.numPlanes,
                          s_qcFormatToNumPlanes[prop.format], prop.format );
                status = QC_STATUS_BAD_ARGUMENTS;
            }
        }
        else
        { /* check properties for compressed image */
            if ( 1 != prop.numPlanes )
            {
                QC_ERROR( "invalid numPlanes for compressed image" );
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( 0 == prop.planeBufSize[0] )
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
        if ( prop.format < QC_IMAGE_FORMAT_MAX )
        {
            bpp = s_qcFormatToBytesPerPixel[prop.format];
            /* check each plane def is reasonable */
            for ( i = 0; i < prop.numPlanes; i++ )
            {
                divider = s_qcFormatToHeightDividerPerPlanes[prop.format][i];
                if ( ( prop.width * bpp ) > prop.stride[i] )
                {
                    QC_ERROR( "given stride %u(<%u) too small for plane %u for format %d",
                              prop.stride[i], prop.width * bpp, i, prop.format );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( ( prop.height / divider ) > prop.actualHeight[i] )
                {
                    QC_ERROR( "given actual height %u(<%u) too small for plane %u for format %d",
                              prop.actualHeight[i], prop.height / divider, i, prop.format );
                    status = QC_STATUS_BAD_ARGUMENTS;
                }
                else if ( ( 0 != prop.planeBufSize[i] ) &&
                          ( prop.planeBufSize[i] < ( prop.stride[i] * prop.actualHeight[i] ) ) )
                {
                    QC_ERROR( "given plane buffer size %u(<%u) too small for plane %u for "
                              "format %d",
                              prop.planeBufSize[i], prop.stride[i] * prop.actualHeight[i], i,
                              prop.format );
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
                for ( i = 0; i < prop.numPlanes; i++ )
                {
                    if ( 0 != prop.planeBufSize[i] )
                    {
                        size += (size_t) prop.planeBufSize[i];
                    }
                    else
                    {
                        size += (size_t) prop.stride[i] * prop.actualHeight[i];
                    }
                }
                size = size * prop.batchSize;
            }
        }
        else
        {
            size = static_cast<size_t>( prop.planeBufSize[0] );
        }
    }

    if ( QC_STATUS_OK == status )
    {
        prop.size = size;

        desc.type = QC_BUFFER_TYPE_IMAGE;
        desc.format = prop.format;
        desc.batchSize = prop.batchSize;
        desc.width = prop.width;
        desc.height = prop.height;
        desc.numPlanes = prop.numPlanes;
        std::copy( prop.stride, prop.stride + prop.numPlanes, desc.stride );
        std::copy( prop.actualHeight, prop.actualHeight + prop.numPlanes, desc.actualHeight );
        std::copy( prop.planeBufSize, prop.planeBufSize + prop.numPlanes, desc.planeBufSize );
        for ( i = 0; i < desc.numPlanes; i++ )
        {
            if ( 0 == desc.planeBufSize[i] )
            {
                desc.planeBufSize[i] = desc.stride[i] * desc.actualHeight[i];
            }
        }
    }

    return status;
}


}   // namespace Memory
}   // namespace QC
