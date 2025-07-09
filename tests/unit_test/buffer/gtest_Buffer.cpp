// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
#include "QC/Infras/Memory/BufferManager.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "gtest/gtest.h"
#include <algorithm>
#include <chrono>
#include <stdio.h>
#include <thread>

using namespace QC;
using namespace std::chrono_literals;

TEST( Buffer, SANITY_ImageAllocateByWHF )
{
    QCSharedBuffer_t sharedBuffer;
    QCSharedBuffer_t sharedBufferM;
    QCSharedBuffer_t sharedBufferTs;

    /* testing allocate image for UYVY */
    auto ret = sharedBuffer.Allocate( 3840, 2160, QC_IMAGE_FORMAT_UYVY );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 3840 * 2160 * 2, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 3840 * 2, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 2160, sharedBuffer.imgProps.actualHeight[0] );

    ret = sharedBuffer.ImageToTensor( &sharedBufferTs );
    ASSERT_EQ( QC_STATUS_OK, ret ); /* supported as padding is 0 */
    ASSERT_EQ( 0, sharedBufferTs.offset );
    ASSERT_LE( 3840 * 2160 * 2, sharedBufferTs.size );
    ASSERT_EQ( QC_BUFFER_TYPE_TENSOR, sharedBufferTs.type );
    ASSERT_EQ( 4, sharedBufferTs.tensorProps.numDims );
    ASSERT_EQ( 1, sharedBufferTs.tensorProps.dims[0] );
    ASSERT_EQ( 2160, sharedBufferTs.tensorProps.dims[1] );
    ASSERT_EQ( 3840, sharedBufferTs.tensorProps.dims[2] );
    ASSERT_EQ( 2, sharedBufferTs.tensorProps.dims[3] );

    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    /* testing allocate image for NV12 */
    ret = sharedBuffer.Allocate( 3840, 2160, QC_IMAGE_FORMAT_NV12 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 3840 * 2160 * 3 / 2, sharedBuffer.size );
    ASSERT_EQ( 2, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 3840, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 2160, sharedBuffer.imgProps.actualHeight[0] );
    ASSERT_LE( 3840, sharedBuffer.imgProps.stride[1] );
    ASSERT_LE( 2160 / 2, sharedBuffer.imgProps.actualHeight[1] );
    ret = sharedBuffer.ImageToTensor( &sharedBufferTs );
    ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret ); /* not supported as format */
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    /* testing allocate image for RGB */
    ret = sharedBuffer.Allocate( 1024, 768, QC_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 1024 * 768 * 3, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 1024, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBuffer.imgProps.actualHeight[0] );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    /* testing allocate batched image for RGB */
    ret = sharedBuffer.Allocate( 7, 1024, 768, QC_IMAGE_FORMAT_RGB888 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_LE( 1024 * 768 * 3 * 7, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_LE( 1024, sharedBuffer.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBuffer.imgProps.actualHeight[0] );
    /* testing get the middle batch of the shared image */
    ret = sharedBuffer.GetSharedBuffer( &sharedBufferM, 3 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( (uint8_t *) sharedBuffer.buffer.pData + sharedBufferM.size * 3,
               sharedBufferM.data() );
    ASSERT_EQ( sharedBufferM.size * 3, sharedBufferM.offset );
    ASSERT_EQ( sharedBufferM.buffer.size / 7, sharedBufferM.size );
    ASSERT_LE( 1024 * 768 * 3, sharedBufferM.size );
    ASSERT_EQ( 1, sharedBufferM.imgProps.numPlanes );
    ASSERT_LE( 1024 * 3, sharedBufferM.imgProps.stride[0] );
    ASSERT_LE( 768, sharedBufferM.imgProps.actualHeight[0] );

    ret = sharedBuffer.ImageToTensor( &sharedBufferTs );
    if ( 1024 * 3 == sharedBuffer.imgProps.stride[0] )
    {
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( 0, sharedBufferTs.offset );
        ASSERT_LE( 1024 * 768 * 3 * 7, sharedBufferTs.size );
        ASSERT_EQ( QC_BUFFER_TYPE_TENSOR, sharedBufferTs.type );
        ASSERT_EQ( 4, sharedBufferTs.tensorProps.numDims );
        ASSERT_EQ( 7, sharedBufferTs.tensorProps.dims[0] );
        ASSERT_EQ( 768, sharedBufferTs.tensorProps.dims[1] );
        ASSERT_EQ( 1024, sharedBufferTs.tensorProps.dims[2] );
        ASSERT_EQ( 3, sharedBufferTs.tensorProps.dims[3] );
    }
    else
    { /* if has padding along width, image to tensor is not supported */
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
    }


    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( Buffer, SANITY_ImageAllocateByProps )
{
    QCSharedBuffer_t sharedBuffer;
    QCImageProps_t imgProp;

    imgProp.format = QC_IMAGE_FORMAT_UYVY;
    imgProp.batchSize = 1;
    imgProp.width = 3840;
    imgProp.height = 2160;
    imgProp.stride[0] = 3840 * 2;
    imgProp.actualHeight[0] = 2160;
    imgProp.numPlanes = 1;
    imgProp.planeBufSize[0] = 0; /* auto calculated the required size by stride*actualHeight */
    auto ret = sharedBuffer.Allocate( &imgProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 3840 * 2160 * 2, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ASSERT_EQ( 3840 * 2, sharedBuffer.imgProps.stride[0] );
    ASSERT_EQ( 2160, sharedBuffer.imgProps.actualHeight[0] );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );


    imgProp.format = QC_IMAGE_FORMAT_NV12;
    imgProp.batchSize = 1;
    imgProp.width = 1920;
    imgProp.height = 1024;
    imgProp.stride[0] = 1920;
    imgProp.actualHeight[0] = 1024;
    imgProp.stride[1] = 1920;
    imgProp.actualHeight[1] = 512;
    imgProp.numPlanes = 2;
    imgProp.planeBufSize[0] = 0;
    imgProp.planeBufSize[1] = 0;
    ret = sharedBuffer.Allocate( &imgProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 1920 * 1024 * 3 / 2, sharedBuffer.size );
    ASSERT_EQ( 2, sharedBuffer.imgProps.numPlanes );
    ASSERT_EQ( 1920 * 1, sharedBuffer.imgProps.stride[0] );
    ASSERT_EQ( 1024, sharedBuffer.imgProps.actualHeight[0] );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( Buffer, SANITY_ImageAllocateRGBByProps )
{
    QCSharedBuffer_t sharedBufferAll;
    QCSharedBuffer_t sharedBufferMiddle;
    QCImageProps_t imgProp;

    imgProp.format = QC_IMAGE_FORMAT_RGB888;
    imgProp.batchSize = 3;
    imgProp.width = 1024;
    imgProp.height = 768;
    imgProp.stride[0] = 1024 * 3;
    imgProp.actualHeight[0] = 768;
    imgProp.numPlanes = 1;
    imgProp.planeBufSize[0] = 0;

    // testing allocated a batched RGB image
    auto ret = sharedBufferAll.Allocate( &imgProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBufferAll.data() );
    ASSERT_EQ( 0, sharedBufferAll.offset );
    std::generate( (uint8_t *) sharedBufferAll.data(),
                   (uint8_t *) sharedBufferAll.data() + sharedBufferAll.size, std::rand );
    ASSERT_EQ( sharedBufferAll.buffer.size, sharedBufferAll.size );
    ASSERT_EQ( 3 * 1024 * 768 * 3, sharedBufferAll.size );
    ASSERT_EQ( 1, sharedBufferAll.imgProps.numPlanes );
    ASSERT_EQ( 3, sharedBufferAll.imgProps.batchSize );
    ASSERT_EQ( 1024 * 3, sharedBufferAll.imgProps.stride[0] );
    ASSERT_EQ( 768, sharedBufferAll.imgProps.actualHeight[0] );

    /* get the RGB image in the middle of sharedBufferAll */
    ret = sharedBufferAll.GetSharedBuffer( &sharedBufferMiddle, 1, 1 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( ( (uint8_t *) sharedBufferAll.data() ) + 1024 * 768 * 3, sharedBufferMiddle.data() );
    ASSERT_EQ( 1024 * 768 * 3, sharedBufferMiddle.offset );
    ASSERT_EQ( sharedBufferAll.buffer.size / 3, sharedBufferMiddle.size );
    ASSERT_EQ( 1024 * 768 * 3, sharedBufferMiddle.size );
    ASSERT_EQ( 1, sharedBufferMiddle.imgProps.numPlanes );
    ASSERT_EQ( 1, sharedBufferMiddle.imgProps.batchSize );
    ASSERT_EQ( 1024 * 3, sharedBufferMiddle.imgProps.stride[0] );
    ASSERT_EQ( 768, sharedBufferMiddle.imgProps.actualHeight[0] );

    ret = sharedBufferAll.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( Buffer, SANITY_CompressedImageAllocateByProps )
{
    QCSharedBuffer_t sharedBuffer;
    QCImageProps_t imgProp;

    imgProp.format = QC_IMAGE_FORMAT_COMPRESSED_H265;
    imgProp.batchSize = 1;
    imgProp.width = 3840;
    imgProp.height = 2160;
    imgProp.numPlanes = 1;
    imgProp.planeBufSize[0] = 1024 * 64;
    auto ret = sharedBuffer.Allocate( &imgProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 1024 * 64, sharedBuffer.size );
    ASSERT_EQ( 1, sharedBuffer.imgProps.numPlanes );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

TEST( Buffer, SANITY_TensorAllocate )
{
    QCSharedBuffer_t sharedBuffer;
    QCTensorProps_t tensorProp = { QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 128, 128, 10 }, 4 };

    auto ret = sharedBuffer.Allocate( &tensorProp );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_NE( nullptr, sharedBuffer.data() );
    ASSERT_EQ( 0, sharedBuffer.offset );
    std::generate( (uint8_t *) sharedBuffer.data(),
                   (uint8_t *) sharedBuffer.data() + sharedBuffer.size, std::rand );
    ASSERT_EQ( sharedBuffer.buffer.size, sharedBuffer.size );
    ASSERT_EQ( 1 * 128 * 128 * 10, sharedBuffer.size );
    ASSERT_EQ( 4, sharedBuffer.tensorProps.numDims );
    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );
}

static std::string GetBufferTextInfo( const QCSharedBuffer_t *pSharedBuffer )
{
    std::string str = "";
    std::stringstream ss;

    if ( QC_BUFFER_TYPE_RAW == pSharedBuffer->type )
    {
        str = "Raw";
    }
    else if ( QC_BUFFER_TYPE_IMAGE == pSharedBuffer->type )
    {
        ss << "Image format=" << pSharedBuffer->imgProps.format
           << " batch=" << pSharedBuffer->imgProps.batchSize
           << " resolution=" << pSharedBuffer->imgProps.width << "x"
           << pSharedBuffer->imgProps.height;
        if ( pSharedBuffer->imgProps.format < QC_IMAGE_FORMAT_MAX )
        {
            ss << " stride=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.stride[i] << ", ";
            }
            ss << "] actual height=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.actualHeight[i] << ", ";
            }
            ss << "] plane size=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        else
        {
            ss << " plane size=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        str = ss.str();
    }
    else if ( QC_BUFFER_TYPE_TENSOR == pSharedBuffer->type )
    {
        ss << "Tensor type=" << pSharedBuffer->tensorProps.type << " dims=[";
        for ( uint32_t i = 0; i < pSharedBuffer->tensorProps.numDims; i++ )
        {
            ss << pSharedBuffer->tensorProps.dims[i] << ", ";
        }
        ss << "]";
        str = ss.str();
    }
    else
    {
        /* Invalid type, impossible case */
    }

    return str;
}

static bool IsTheSameSharedBuffer( QCSharedBuffer_t &bufferA, QCSharedBuffer_t &bufferB )
{
    bool bEqual = true;
    int ret = memcmp( &bufferA.buffer, &bufferB.buffer, sizeof( bufferA.buffer ) );
    if ( 0 != ret )
    {
        printf( "buffer buffer not equal\n" );
        bEqual = false;
    }

    if ( ( bufferA.size != bufferB.size ) || ( bufferA.offset != bufferB.offset ) ||
         ( bufferA.type != bufferB.type ) )
    {
        printf( "buffer size/offset/type not equal\n" );
        bEqual = false;
    }

    switch ( bufferA.type )
    {
        case QC_BUFFER_TYPE_IMAGE:
            ret = memcmp( &bufferA.imgProps, &bufferB.imgProps, sizeof( bufferA.imgProps ) );
            if ( 0 != ret )
            {
                printf( "buffer imgProps not equal\n" );
                bEqual = false;
            }
            break;
        case QC_BUFFER_TYPE_TENSOR:
            ret = memcmp( &bufferA.tensorProps, &bufferB.tensorProps,
                          sizeof( bufferA.tensorProps ) );
            if ( 0 != ret )
            {
                printf( "buffer tensorProps not equal\n" );
                bEqual = false;
            }
            break;
        default: /* do nothing for RAW type */
            break;
    }

    if ( false == bEqual )
    {
        printf( "bufferA: %s\n", GetBufferTextInfo( &bufferA ).c_str() );
        printf( "bufferB: %s\n", GetBufferTextInfo( &bufferA ).c_str() );
    }

    return bEqual;
}

TEST( Buffer, L2_Buffer )
{
    QCStatus_e ret;
    for ( int i = 0; i < (int) QC_BUFFER_USAGE_MAX; i++ )
    {
        QCBufferUsage_e usage = (QCBufferUsage_e) i;
        QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_NONE;
        QCSharedBuffer_t sharedBuffer;
        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 1024 * 256, usage, flags );
#if !defined( __QNXNTO__ )
        if ( QC_STATUS_UNSUPPORTED == ret )
        {
            printf( "Linux has no /dev/dma_heap/system-uncached, skip uncached buffer test" );
            break;
        }
#endif
        ASSERT_EQ( QC_STATUS_NOMEM, ret );

        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 32, usage, flags );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 32, usage, flags );
        ASSERT_EQ( QC_STATUS_ALREADY, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    }

    for ( int i = 0; i < (int) QC_BUFFER_USAGE_MAX; i++ )
    {
        QCBufferUsage_e usage = (QCBufferUsage_e) i;
        QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA;
        QCSharedBuffer_t sharedBuffer;
#if defined( __QNXNTO__ )
        /* not do this for Linux, see device crashed */
        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 1024 * 256, usage, flags );
        ASSERT_EQ( QC_STATUS_NOMEM, ret );
#endif

        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 32, usage, flags );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 32, usage, flags );
        ASSERT_EQ( QC_STATUS_ALREADY, ret );

        QCSharedBuffer_t sharedBuffer2( sharedBuffer );
        ASSERT_EQ( IsTheSameSharedBuffer( sharedBuffer, sharedBuffer2 ), true );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 64, QC_BUFFER_USAGE_MAX );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.Allocate( (size_t) 1024 * 1024 * 64, (QCBufferUsage_e) -2 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    }

    {
        void *pData;
        uint64_t dmaHandle;

        ret = QCDmaAllocate( &pData, nullptr, 1000000, QC_BUFFER_FLAGS_CACHE_WB_WA,
                             QC_BUFFER_USAGE_DEFAULT );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = QCDmaAllocate( nullptr, &dmaHandle, 1000000, QC_BUFFER_FLAGS_CACHE_WB_WA,
                             QC_BUFFER_USAGE_DEFAULT );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = QCDmaAllocate( &pData, &dmaHandle, 1000000, QC_BUFFER_FLAGS_CACHE_WB_WA,
                             QC_BUFFER_USAGE_DEFAULT );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = QCDmaFree( nullptr, 0, 1000000 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = QCDmaFree( (void *) 0x1, 0, 1000000 );
        ASSERT_EQ( QC_STATUS_FAIL, ret );
#if !defined( __QNXNTO__ ) /* dmaHandle was only used by Linux when do free */
        ret = QCDmaFree( pData, (uint64_t) -1, 1000000 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = QCDmaFree( pData, (uint64_t) 0x7FFFFFFF, 1000000 );
        ASSERT_EQ( QC_STATUS_FAIL, ret );
#endif

        ret = QCDmaFree( pData, dmaHandle, 1000000 );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

static void InitImageProps( QCImageProps_t &imgProp )
{
    imgProp.batchSize = 1;
    imgProp.format = QC_IMAGE_FORMAT_UYVY;
    imgProp.width = 3840;
    imgProp.height = 2160;
    imgProp.stride[0] = 3840 * 2;
    imgProp.actualHeight[0] = 2160;
    imgProp.numPlanes = 1;
    imgProp.planeBufSize[0] = 0;
}

TEST( Buffer, L2_Image )
{
    {
        QCSharedBuffer_t sharedBuffer;
        auto ret = sharedBuffer.Allocate( 0, 1920, 2160, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* invalid batch size */

        ret = sharedBuffer.Allocate( 1, 0, 2160, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* invalid width */

        ret = sharedBuffer.Allocate( 1, 1920, 0, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* invalid height */

        ret = sharedBuffer.Allocate( 1, 1920, 1024, QC_IMAGE_FORMAT_MAX );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* invalid format */

        ret = sharedBuffer.Allocate( 1, 1920, 1024, (QCImageFormat_e) -5 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* invalid format */

        ret = sharedBuffer.Allocate( (QCImageProps_t *) nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps is null */

        ret = sharedBuffer.Allocate( 1, 4096 * 4, 4096 * 4, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_FAIL, ret ); /* with large image as not supported by apdf */

        QCImageProps_t imgProp;

        InitImageProps( imgProp );
        imgProp.batchSize = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid batch size */

        InitImageProps( imgProp );
        imgProp.width = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid width */

        InitImageProps( imgProp );
        imgProp.height = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid height */

        InitImageProps( imgProp );
        imgProp.stride[0] = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid stride */

        InitImageProps( imgProp );
        imgProp.actualHeight[0] = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid actualHeight */

        InitImageProps( imgProp );
        imgProp.numPlanes = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid numPlanes */

        InitImageProps( imgProp );
        imgProp.format = QC_IMAGE_FORMAT_MAX;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = (QCImageFormat_e) -5;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = ( QCImageFormat_e )( (int) QC_IMAGE_FORMAT_COMPRESSED_MIN - 1 );
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = QC_IMAGE_FORMAT_COMPRESSED_MAX;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = QC_IMAGE_FORMAT_COMPRESSED_H265;
        imgProp.numPlanes = 1;
        imgProp.planeBufSize[0] = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret ); /* pImgProps with invalid size */
    }

    for ( int i = 0; i < (int) QC_IMAGE_FORMAT_MAX; i++ )
    {
        QCSharedBuffer_t sharedBuffer;
        QCImageProps_t imgProp;

        if ( ( QC_IMAGE_FORMAT_NV12_UBWC == (QCImageFormat_e) i ) ||
             ( QC_IMAGE_FORMAT_TP10_UBWC == (QCImageFormat_e) i ) )
        {
            continue;
        }

        imgProp.batchSize = i + 1;
        imgProp.format = (QCImageFormat_e) i;
        imgProp.width = 3840 / QC_IMAGE_FORMAT_MAX * ( i + 1 );
        imgProp.height = 2160 / QC_IMAGE_FORMAT_MAX * ( i + 1 );
        imgProp.stride[0] = imgProp.width * 3;
        imgProp.actualHeight[0] = imgProp.height;
        imgProp.numPlanes = 1;
        if ( ( QC_IMAGE_FORMAT_NV12 == imgProp.format ) ||
             ( QC_IMAGE_FORMAT_P010 == imgProp.format ) )
        {
            imgProp.numPlanes = 2;
            imgProp.stride[1] = imgProp.width * 3;
            imgProp.actualHeight[1] = ( imgProp.height + 1 ) / 2;
        }
        imgProp.planeBufSize[0] = 0;
        imgProp.planeBufSize[1] = 0;

        auto ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_ALREADY, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( int i = 0; i < (int) QC_IMAGE_FORMAT_MAX; i++ )
    {
        QCSharedBuffer_t sharedBuffer;
        QCImageFormat_e format = (QCImageFormat_e) i;

        auto ret = sharedBuffer.Allocate( 1, 1920, 2160, format );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        QCSharedBuffer_t sharedBuffer3;
        QCSharedBuffer_t sharedBufferTs;

        auto ret = sharedBuffer.GetSharedBuffer( &sharedBuffer3, 0 );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = sharedBuffer.ImageToTensor( &sharedBufferTs );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = sharedBuffer.Allocate( 3840, 2160, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 3840, 2160, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_ALREADY, ret );

        QCSharedBuffer_t sharedBuffer2( sharedBuffer );
        ASSERT_EQ( IsTheSameSharedBuffer( sharedBuffer, sharedBuffer2 ), true );

        ret = sharedBuffer.GetSharedBuffer( (QCSharedBuffer_t *) nullptr, 0 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.GetSharedBuffer( &sharedBuffer3, 3 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.GetSharedBuffer( &sharedBuffer3, 0, 3 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.ImageToTensor( (QCSharedBuffer_t *) nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        QCSharedBuffer_t sharedBufferTs;
        QCImageProps_t imgProp;

        imgProp.batchSize = 1;
        imgProp.format = QC_IMAGE_FORMAT_BGR888;
        imgProp.width = 1013;
        imgProp.height = 753;
        imgProp.stride[0] = 1024 * 3;
        imgProp.actualHeight[0] = 768;
        imgProp.numPlanes = 1;
        auto ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.ImageToTensor( &sharedBufferTs );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        QCSharedBuffer_t sharedBufferTs;
        QCImageProps_t imgProp;

        imgProp.batchSize = 2;
        imgProp.format = QC_IMAGE_FORMAT_BGR888;
        imgProp.width = 1024;
        imgProp.height = 763;
        imgProp.stride[0] = 1024 * 3;
        imgProp.actualHeight[0] = 768;
        imgProp.numPlanes = 1;
        auto ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.ImageToTensor( &sharedBufferTs );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        auto ret = sharedBuffer.Allocate( 1920, 2160, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_OK, ret );
        auto pid = sharedBuffer.buffer.pid;
        sharedBuffer.buffer.pid = pid + 100;
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );
        sharedBuffer.buffer.pid = pid;
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( Buffer, L2_Tensor )
{
    for ( uint32_t i = 0; i < (uint32_t) QC_TENSOR_TYPE_MAX; i++ )
    {
        QCSharedBuffer_t sharedBuffer;
        QCTensorProps_t tensorProp = { (QCTensorType_e) i,
                                       { i + 1, 1024 * ( i + 1 ) / QC_TENSOR_TYPE_MAX,
                                         1024 * ( QC_TENSOR_TYPE_MAX - i ) / QC_TENSOR_TYPE_MAX,
                                         10 },
                                       4 };

        auto ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_ALREADY, ret );

        QCSharedBuffer_t sharedBuffer2( sharedBuffer );
        ASSERT_EQ( IsTheSameSharedBuffer( sharedBuffer, sharedBuffer2 ), true );

        QCSharedBuffer_t sharedBuffer3;
        ret = sharedBuffer.GetSharedBuffer( &sharedBuffer3, 0 );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );

        ret = sharedBuffer.ImageToTensor( &sharedBuffer3 );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        QCTensorProps_t tensorProp = { QC_TENSOR_TYPE_UFIXED_POINT_8,
                                       { 1, 1024, 768, 3 },
                                       QC_NUM_TENSOR_DIMS + 3 };

        auto ret = sharedBuffer.Allocate( (QCTensorProps_t *) nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );


        ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        tensorProp.numDims = 4;
        tensorProp.dims[2] = 0;

        ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        tensorProp.numDims = 4;
        tensorProp.dims[2] = 768;
        tensorProp.type = QC_TENSOR_TYPE_MAX;

        ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        tensorProp.type = (QCTensorType_e) -3;

        ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    }
}

TEST( Buffer, L2_BufferManager )
{
    BufferManager *pBufferManager = BufferManager::GetDefaultBufferManager();
    ASSERT_NE( nullptr, pBufferManager );

    {
        QCSharedBuffer_t sharedBuffer;
        auto ret = sharedBuffer.Allocate( 1920, 1024, QC_IMAGE_FORMAT_UYVY );
        ASSERT_EQ( QC_STATUS_OK, ret );

        QCSharedBuffer_t sharedBuffer2;

        ret = pBufferManager->GetSharedBuffer( sharedBuffer.buffer.id, nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = pBufferManager->GetSharedBuffer( sharedBuffer.buffer.id, &sharedBuffer2 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( IsTheSameSharedBuffer( sharedBuffer, sharedBuffer2 ), true );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = pBufferManager->GetSharedBuffer( sharedBuffer.buffer.id, &sharedBuffer2 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = pBufferManager->GetSharedBuffer( 0xdeadbeef, &sharedBuffer2 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        QCTensorProps_t tensorProp = { QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 512, 512, 10 }, 4 };

        auto ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = pBufferManager->Deregister( sharedBuffer.buffer.id );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = pBufferManager->Register( nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    }

    {
        BufferManager bufMgr;

        auto ret = bufMgr.Init( nullptr, LOGGER_LEVEL_ERROR );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( Buffer, L2_Image2Tensor )
{
    QCStatus_e ret;
    {
        QCSharedBuffer_t sharedBuffer;
        QCSharedBuffer_t tensor;
        QCImageProps_t imgProp;
        imgProp.format = QC_IMAGE_FORMAT_RGB888;
        imgProp.batchSize = 3;
        imgProp.width = 1920;
        imgProp.height = 1024;
        imgProp.stride[0] = 1920 * 3;
        imgProp.actualHeight[0] = 1028;
        imgProp.numPlanes = 1;
        imgProp.planeBufSize[0] = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &tensor );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        imgProp.actualHeight[0] = 1024;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &tensor );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        QCSharedBuffer_t luma;
        QCSharedBuffer_t chroma;
        QCTensorProps_t tensorProp = { QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 1024, 768, 3 }, 4 };

        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = sharedBuffer.Allocate( 1920, 1024, QC_IMAGE_FORMAT_NV12 );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( luma.offset, 0 );
        ASSERT_EQ( chroma.offset, 1920 * 1024 );
        ASSERT_EQ( luma.size, 1920 * 1024 );
        ASSERT_EQ( chroma.size, 1920 * 1024 / 2 );

        ret = sharedBuffer.ImageToTensor( nullptr, &chroma );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.ImageToTensor( &luma, nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 1921, 1024, QC_IMAGE_FORMAT_NV12 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );


        QCImageProps_t imgProp;
        imgProp.format = QC_IMAGE_FORMAT_NV12;
        imgProp.batchSize = 1;
        imgProp.width = 1921;
        imgProp.height = 1024;
        imgProp.stride[0] = 1921;
        imgProp.actualHeight[0] = 1024;
        imgProp.stride[1] = 1921;
        imgProp.actualHeight[1] = 1024 / 2;
        imgProp.numPlanes = 2;
        imgProp.planeBufSize[0] = 0;
        imgProp.planeBufSize[1] = 0;
        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 1920, 1025, QC_IMAGE_FORMAT_NV12 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 2, 1920, 1024, QC_IMAGE_FORMAT_NV12 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 1920, 1024, QC_IMAGE_FORMAT_RGB888 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( &tensorProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    {
        QCSharedBuffer_t sharedBuffer;
        QCSharedBuffer_t luma;
        QCSharedBuffer_t chroma;

        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

        ret = sharedBuffer.Allocate( 1920, 1024, QC_IMAGE_FORMAT_P010 );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ASSERT_EQ( luma.offset, 0 );
        ASSERT_EQ( chroma.offset, 1920 * 1024 * 2 );
        ASSERT_EQ( luma.size, 1920 * 1024 * 2 );
        ASSERT_EQ( chroma.size, 1920 * 1024 );

        ret = sharedBuffer.ImageToTensor( nullptr, &chroma );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.ImageToTensor( &luma, nullptr );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 1921, 1024, QC_IMAGE_FORMAT_P010 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        QCImageProps_t imgProp;
        imgProp.format = QC_IMAGE_FORMAT_P010;
        imgProp.batchSize = 1;
        imgProp.width = 1921;
        imgProp.height = 1024;
        imgProp.stride[0] = 1921 * 2;
        imgProp.actualHeight[0] = 1024;
        imgProp.stride[1] = 1921 * 2;
        imgProp.actualHeight[1] = 1024 / 2;
        imgProp.numPlanes = 2;
        imgProp.planeBufSize[0] = 0;
        imgProp.planeBufSize[1] = 0;

        ret = sharedBuffer.Allocate( &imgProp );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 1920, 1025, QC_IMAGE_FORMAT_P010 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = sharedBuffer.Allocate( 2, 1920, 1024, QC_IMAGE_FORMAT_P010 );
        ASSERT_EQ( QC_STATUS_OK, ret );
        ret = sharedBuffer.ImageToTensor( &luma, &chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, ret );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( Buffer, SanityImport )
{
    QCSharedBuffer_t sharedBuffer;
    uint32_t *pData;
    QCStatus_e ret;
    pid_t pid;

    ret = sharedBuffer.Allocate( 1920, 1024, QC_IMAGE_FORMAT_P010 );
    ASSERT_EQ( QC_STATUS_OK, ret );
    pData = (uint32_t *) sharedBuffer.data();
    pData[0] = 1234;

    pid = fork();
    if ( pid == 0 )
    { /* child process */
        QCSharedBuffer_t importedBuffer;
        uint32_t *pImportedData;
        ret = importedBuffer.Import( &sharedBuffer );
        ASSERT_EQ( QC_STATUS_OK, ret );
        pImportedData = (uint32_t *) importedBuffer.data();
        ASSERT_EQ( 1234, pImportedData[0] );
        pImportedData[0] = 5678;
        ret = importedBuffer.UnImport();
        ASSERT_EQ( QC_STATUS_OK, ret );
        printf( "This is child process %" PRIi32 "\n", getpid() );
    }
    else
    {
        std::this_thread::sleep_for( 1000ms );
        printf( "This is parent process %" PRIi32 "\n", getpid() );
        ASSERT_EQ( 5678, pData[0] );
        ret = sharedBuffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

TEST( Buffer, L2_Import )
{
    QCSharedBuffer_t sharedBuffer;
    QCSharedBuffer_t importedBuffer;
    QCStatus_e ret;

    void *pData = nullptr;
    uint64_t dmaHandle = 0;

    ret = importedBuffer.Import( (QCSharedBuffer_t *) nullptr );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = sharedBuffer.Allocate( 1920, 1024, QC_IMAGE_FORMAT_UYVY );
    ASSERT_EQ( QC_STATUS_OK, ret );

    dmaHandle = sharedBuffer.buffer.dmaHandle;
    sharedBuffer.buffer.dmaHandle = 0;
    ret = importedBuffer.Import( &sharedBuffer );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    sharedBuffer.buffer.dmaHandle = dmaHandle;

    auto size = sharedBuffer.buffer.size;
    sharedBuffer.buffer.size = 0;
    ret = importedBuffer.Import( &sharedBuffer );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
    sharedBuffer.buffer.size = size;

    ret = importedBuffer.Import( &sharedBuffer );
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );

    ret = importedBuffer.UnImport();
    ASSERT_EQ( QC_STATUS_INVALID_BUF, ret );

    ret = sharedBuffer.UnImport();
    ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, ret );

    ret = QCDmaImport( nullptr, &dmaHandle, sharedBuffer.buffer.pid, sharedBuffer.buffer.dmaHandle,
                       sharedBuffer.buffer.size, sharedBuffer.buffer.flags,
                       sharedBuffer.buffer.usage );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = QCDmaImport( &pData, nullptr, sharedBuffer.buffer.pid, sharedBuffer.buffer.dmaHandle,
                       sharedBuffer.buffer.size, sharedBuffer.buffer.flags,
                       sharedBuffer.buffer.usage );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

#if defined( __QNXNTO__ )
    ret = QCDmaImport( &pData, &dmaHandle, sharedBuffer.buffer.pid, 0, sharedBuffer.buffer.size,
                       sharedBuffer.buffer.flags, sharedBuffer.buffer.usage );
#else
    ret = QCDmaImport( &pData, &dmaHandle, sharedBuffer.buffer.pid, (uint64_t) -1,
                       sharedBuffer.buffer.size, sharedBuffer.buffer.flags,
                       sharedBuffer.buffer.usage );
#endif
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = QCDmaImport( &pData, &dmaHandle, sharedBuffer.buffer.pid, sharedBuffer.buffer.dmaHandle,
                       sharedBuffer.buffer.size, sharedBuffer.buffer.flags, QC_BUFFER_USAGE_MAX );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = QCDmaImport( &pData, &dmaHandle, sharedBuffer.buffer.pid, sharedBuffer.buffer.dmaHandle,
                       sharedBuffer.buffer.size, sharedBuffer.buffer.flags, (QCBufferUsage_e) -1 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = QCDmaImport( &pData, &dmaHandle, sharedBuffer.buffer.pid, sharedBuffer.buffer.dmaHandle,
                       sharedBuffer.buffer.size + 4096, sharedBuffer.buffer.flags,
                       sharedBuffer.buffer.usage );
    ASSERT_EQ( QC_STATUS_FAIL, ret );

    ret = QCDmaImport( &pData, &dmaHandle, sharedBuffer.buffer.pid, 0x1eadbeef,
                       sharedBuffer.buffer.size, sharedBuffer.buffer.flags,
                       sharedBuffer.buffer.usage );
    ASSERT_EQ( QC_STATUS_FAIL, ret );

#if !defined( __QNXNTO__ )
    QCSharedBuffer_t sharedBufferPid0 = sharedBuffer;
    sharedBufferPid0.buffer.pid = 0;
    ret = importedBuffer.Import( &sharedBufferPid0 );
    ASSERT_EQ( QC_STATUS_OK, ret );
#endif

    ret = sharedBuffer.Free();
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = QCDmaUnImport( nullptr, 0, 0 );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    ret = QCDmaUnImport( (void *) 0x1234, 0x1eadbeef, 1024 );
    ASSERT_EQ( QC_STATUS_FAIL, ret );
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
