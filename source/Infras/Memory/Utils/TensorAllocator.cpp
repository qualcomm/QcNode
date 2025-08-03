// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/Utils/TensorAllocator.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include <algorithm>
#include <unistd.h>

namespace QC
{
namespace Memory
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

TensorAllocator::TensorAllocator( const std::string &name ) : BinaryAllocator( name )
{
    (void) QC_LOGGER_INIT( GetConfiguration().name.c_str(), LOGGER_LEVEL_ERROR );
};

TensorAllocator::~TensorAllocator()
{
    (void) QC_LOGGER_DEINIT();
};

QCStatus_e TensorAllocator::Allocate( const QCBufferPropBase_t &request,
                                      QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;

    const TensorProps_t *pTensorProps = dynamic_cast<const TensorProps_t *>( &request );
    TensorDescriptor_t *pTensorDescriptor = dynamic_cast<TensorDescriptor_t *>( &response );

    if ( ( nullptr != pTensorProps ) && ( nullptr != pTensorDescriptor ) )
    {
        BufferProps_t bufProps = *pTensorProps;
        size_t size = 1;
        uint32_t i = 0;

        if ( ( pTensorProps->numDims > QC_NUM_TENSOR_DIMS ) ||
             ( pTensorProps->tensorType >= QC_TENSOR_TYPE_MAX ) ||
             ( pTensorProps->tensorType < QC_TENSOR_TYPE_INT_8 ) )
        {
            QC_ERROR( "pTensorProps has invalid numDims or type" );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            /* check each dimension is reasonable */
            for ( i = 0; i < pTensorProps->numDims; i++ )
            {
                if ( 0 == pTensorProps->dims[i] )
                {
                    QC_ERROR( "pTensorProps dims[%u] is 0", i );
                    status = QC_STATUS_BAD_ARGUMENTS;
                    break;
                }
            }
        }

        if ( QC_STATUS_OK == status )
        {
            for ( i = 0; i < pTensorProps->numDims; i++ )
            {
                size *= pTensorProps->dims[i];
            }
            size *= s_qcTensorTypeToDataSize[pTensorProps->tensorType];
        }

        if ( QC_STATUS_OK == status )
        {
            bufProps.size = size;
            status = BinaryAllocator::Allocate( bufProps, response );
            if ( QC_STATUS_OK == status )
            {
                pTensorDescriptor->type = QC_BUFFER_TYPE_TENSOR;
                pTensorDescriptor->numDims = pTensorProps->numDims;
                pTensorDescriptor->tensorType = pTensorProps->tensorType;
                std::copy( pTensorProps->dims, pTensorProps->dims + pTensorProps->numDims,
                           pTensorDescriptor->dims );
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
