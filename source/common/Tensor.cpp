// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/infras/memory/BufferManager.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"

namespace QC
{
#define SIZE_OF_FLOAT16 2
static uint32_t s_qcTensorTypeToDataSize[QC_TENSOR_TYPE_MAX] = {
        sizeof( int8_t ),  /* QC_TENSOR_TYPE_INT_8 */
        sizeof( int16_t ), /* QC_TENSOR_TYPE_INT_16 */
        sizeof( int32_t ), /* QC_TENSOR_TYPE_INT_32 */
        sizeof( int64_t ), /* QC_TENSOR_TYPE_INT_64 */

        sizeof( uint8_t ),  /* QC_TENSOR_TYPE_UINT_8 */
        sizeof( uint16_t ), /* QC_TENSOR_TYPE_UINT_16 */
        sizeof( uint32_t ), /* QC_TENSOR_TYPE_UINT_32 */
        sizeof( uint64_t ), /* QC_TENSOR_TYPE_UINT_64 */

        SIZE_OF_FLOAT16,  /* QC_TENSOR_TYPE_FLOAT_16 */
        sizeof( float ),  /* QC_TENSOR_TYPE_FLOAT_32 */
        sizeof( double ), /* QC_TENSOR_TYPE_FLOAT_64 */

        sizeof( int8_t ),  /* QC_TENSOR_TYPE_SFIXED_POINT_8 */
        sizeof( int16_t ), /* QC_TENSOR_TYPE_SFIXED_POINT_16 */
        sizeof( int32_t ), /* QC_TENSOR_TYPE_SFIXED_POINT_32 */

        sizeof( uint8_t ),  /* QC_TENSOR_TYPE_UFIXED_POINT_8 */
        sizeof( uint16_t ), /* QC_TENSOR_TYPE_UFIXED_POINT_16 */
        sizeof( uint32_t )  /* QC_TENSOR_TYPE_UFIXED_POINT_32 */
};

QCStatus_e QCSharedBuffer::Allocate( const QCTensorProps_t *pTensorProps, QCBufferUsage_e usage,
                                     QCBufferFlags_t flags )
{
    QCStatus_e ret = QC_STATUS_OK;
    size_t size = 1;
    uint32_t i = 0;

    if ( nullptr == pTensorProps )
    {
        QC_LOG_ERROR( "pTensorProps is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( pTensorProps->numDims > QC_NUM_TENSOR_DIMS ) ||
              ( pTensorProps->type >= QC_TENSOR_TYPE_MAX ) ||
              ( pTensorProps->type < QC_TENSOR_TYPE_INT_8 ) )
    {
        QC_LOG_ERROR( "pTensorProps has invalid numDims or type" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        QC_LOG_ERROR( "tensor is already allocated" );
        ret = QC_STATUS_ALREADY;
    }
    else
    {
        /* check each dimension is reasonable */
        for ( i = 0; i < pTensorProps->numDims; i++ )
        {
            if ( 0 == pTensorProps->dims[i] )
            {
                QC_LOG_ERROR( "pTensorProps dims[%u] is 0", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        this->tensorProps = *pTensorProps;
        for ( i = 0; i < pTensorProps->numDims; i++ )
        {
            size *= pTensorProps->dims[i];
        }
        size *= s_qcTensorTypeToDataSize[pTensorProps->type];
    }

    if ( QC_STATUS_OK == ret )
    {
        this->type = QC_BUFFER_TYPE_TENSOR;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}
}   // namespace QC
