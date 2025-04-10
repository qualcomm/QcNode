// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/common/BufferManager.hpp"
#include "ridehal/common/SharedBuffer.hpp"

namespace ridehal
{
namespace common
{
#define SIZE_OF_FLOAT16 2
static uint32_t s_rideHalTensorTypeToDataSize[RIDEHAL_TENSOR_TYPE_MAX] = {
        sizeof( int8_t ),  /* RIDEHAL_TENSOR_TYPE_INT_8 */
        sizeof( int16_t ), /* RIDEHAL_TENSOR_TYPE_INT_16 */
        sizeof( int32_t ), /* RIDEHAL_TENSOR_TYPE_INT_32 */
        sizeof( int64_t ), /* RIDEHAL_TENSOR_TYPE_INT_64 */

        sizeof( uint8_t ),  /* RIDEHAL_TENSOR_TYPE_UINT_8 */
        sizeof( uint16_t ), /* RIDEHAL_TENSOR_TYPE_UINT_16 */
        sizeof( uint32_t ), /* RIDEHAL_TENSOR_TYPE_UINT_32 */
        sizeof( uint64_t ), /* RIDEHAL_TENSOR_TYPE_UINT_64 */

        SIZE_OF_FLOAT16,  /* RIDEHAL_TENSOR_TYPE_FLOAT_16 */
        sizeof( float ),  /* RIDEHAL_TENSOR_TYPE_FLOAT_32 */
        sizeof( double ), /* RIDEHAL_TENSOR_TYPE_FLOAT_64 */

        sizeof( int8_t ),  /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8 */
        sizeof( int16_t ), /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16 */
        sizeof( int32_t ), /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32 */

        sizeof( uint8_t ),  /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8 */
        sizeof( uint16_t ), /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16 */
        sizeof( uint32_t )  /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32 */
};

RideHalError_e RideHal_SharedBuffer::Allocate( const RideHal_TensorProps_t *pTensorProps,
                                               RideHal_BufferUsage_e usage,
                                               RideHal_BufferFlags_t flags )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    size_t size = 1;
    uint32_t i = 0;

    if ( nullptr == pTensorProps )
    {
        RIDEHAL_LOG_ERROR( "pTensorProps is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( pTensorProps->numDims > RIDEHAL_NUM_TENSOR_DIMS ) ||
              ( pTensorProps->type >= RIDEHAL_TENSOR_TYPE_MAX ) ||
              ( pTensorProps->type < RIDEHAL_TENSOR_TYPE_INT_8 ) )
    {
        RIDEHAL_LOG_ERROR( "pTensorProps has invalid numDims or type" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr != this->buffer.pData )
    {
        RIDEHAL_LOG_ERROR( "tensor is already allocated" );
        ret = RIDEHAL_ERROR_ALREADY;
    }
    else
    {
        /* check each dimension is reasonable */
        for ( i = 0; i < pTensorProps->numDims; i++ )
        {
            if ( 0 == pTensorProps->dims[i] )
            {
                RIDEHAL_LOG_ERROR( "pTensorProps dims[%u] is 0", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        this->tensorProps = *pTensorProps;
        for ( i = 0; i < pTensorProps->numDims; i++ )
        {
            size *= pTensorProps->dims[i];
        }
        size *= s_rideHalTensorTypeToDataSize[pTensorProps->type];
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        this->type = RIDEHAL_BUFFER_TYPE_TENSOR;
        ret = Allocate( size, usage, flags );
    }

    return ret;
}
}   // namespace common
}   // namespace ridehal
