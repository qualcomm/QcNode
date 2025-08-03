// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/UtilsBase.hpp"

namespace QC
{
namespace Memory
{

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
    (void) (void) QC_LOGGER_DEINIT();
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

QCStatus_e UtilsBase::SetTensorBuffSizeFromTensorProp( TensorProps_t &prop )
{
    QCStatus_e status = QC_STATUS_OK;

    size_t size = 1;
    uint32_t i = 0;
    prop.size = 0;

    if ( ( prop.numDims > QC_NUM_TENSOR_DIMS ) || ( prop.tensorType >= QC_TENSOR_TYPE_MAX ) ||
         ( prop.tensorType < QC_TENSOR_TYPE_INT_8 ) )
    {
        QC_ERROR( "pTensorProps has invalid numDims or type" );
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        /* check each dimension is reasonable */
        for ( i = 0; i < prop.numDims; i++ )
        {
            if ( 0 == prop.dims[i] )
            {
                QC_ERROR( "prop dims[%u] is 0", i );
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
        prop.size = size;
    }

    return status;
}


QCStatus_e UtilsBase::SetImageBuffSizeFromImageProp( ImageProps_t &prop )
{
    QCStatus_e status = QC_STATUS_OK;

    size_t size = 0;
    uint32_t i = 0;
    uint32_t bpp = 0;
    uint32_t divider = 0;

    // Check if the image properties are valid
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
            size = prop.planeBufSize[0];
        }
    }

    prop.size = size;

    return status;
}

}   // namespace Memory
}   // namespace QC
