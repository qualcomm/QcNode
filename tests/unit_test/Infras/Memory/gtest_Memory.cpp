// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/Utils/BasicImageAllocator.hpp"
#include "QC/Infras/Memory/Utils/BinaryAllocator.hpp"
#include "QC/Infras/Memory/Utils/ImageAllocator.hpp"
#include "gtest/gtest.h"
#include <algorithm>
#include <chrono>
#include <functional>
#include <queue>
#include <stdio.h>
#include <thread>

using namespace QC;
using namespace QC::Memory;
using namespace std::chrono_literals;

typedef struct SharedBuffer
{
public:
    SharedBuffer() : buffer( imgDesc ) {}
    std::reference_wrapper<QCBufferDescriptorBase_t> buffer;

    ImageDescriptor_t imgDesc;
    TensorDescriptor_t tensorDesc;
} SharedBuffer_t;

typedef struct
{
    std::shared_ptr<SharedBuffer_t> buffer;

public:
    QCBufferDescriptorBase_t &GetBuffer() { return buffer->buffer; }
} DataFrame_t;

TEST( Memory, SANITY_BufferTypeCast )
{
    {
        SharedBuffer_t sbuf;
        sbuf.buffer = sbuf.imgDesc;
        QCBufferDescriptorBase_t &buffer = sbuf.buffer;
        const ImageDescriptor_t *pImage = dynamic_cast<const ImageDescriptor_t *>( &buffer );
        ASSERT_EQ( &sbuf.imgDesc, pImage );
    }

    {
        SharedBuffer_t sbuf;
        sbuf.buffer = sbuf.tensorDesc;
        QCBufferDescriptorBase_t &buffer = sbuf.buffer;
        const TensorDescriptor_t *pTensor = dynamic_cast<const TensorDescriptor_t *>( &buffer );
        ASSERT_EQ( &sbuf.tensorDesc, pTensor );
    }

    {
        std::shared_ptr<SharedBuffer_t> sbuf = std::make_shared<SharedBuffer_t>();

        sbuf->buffer = sbuf->imgDesc;
        QCBufferDescriptorBase_t &buffer = sbuf->buffer;
        const ImageDescriptor_t *pImage = dynamic_cast<const ImageDescriptor_t *>( &buffer );
        ASSERT_EQ( &sbuf->imgDesc, pImage );

        const TensorDescriptor_t *pTensor = dynamic_cast<const TensorDescriptor_t *>( &buffer );
        ASSERT_EQ( nullptr, pTensor );
    }

    {
        std::shared_ptr<SharedBuffer_t> sbuf = std::make_shared<SharedBuffer_t>();
        sbuf->buffer = sbuf->imgDesc;
        DataFrame_t frame;
        frame.buffer = sbuf;
        QCBufferDescriptorBase_t &buffer = frame.GetBuffer();
        const ImageDescriptor_t *pImage = dynamic_cast<const ImageDescriptor_t *>( &buffer );
        ASSERT_EQ( &sbuf->imgDesc, pImage );

        const TensorDescriptor_t *pTensor = dynamic_cast<const TensorDescriptor_t *>( &buffer );
        ASSERT_EQ( nullptr, pTensor );
        std::queue<DataFrame_t> que;
        que.push( frame );

        DataFrame_t frame2;
        frame2 = que.front();
        que.pop();

        QCBufferDescriptorBase_t &buffer2 = frame2.GetBuffer();
        pImage = dynamic_cast<const ImageDescriptor_t *>( &buffer2 );
        ASSERT_EQ( &sbuf->imgDesc, pImage );
    }
}

TEST( Memory, SANITY_BinaryAllocate )
{
    QCStatus_e status;
    BinaryAllocator ma( "BINARY" );
    BufferDescriptor_t bufDesc;

    status = ma.Allocate( BufferProps_t( 1024 * 1024 ), bufDesc );
    ASSERT_EQ( QC_STATUS_OK, status );

    status = ma.Free( bufDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST( Memory, SANITY_ImageAllocateByWHF )
{
    QCStatus_e status;
    BasicImageAllocator ma( "IMAGE" );
    ImageDescriptor_t imgDesc;
    ImageDescriptor_t imgDescM;
    TensorDescriptor_t imgDescTs;

    /* testing allocate image for UYVY */
    status = ma.Allocate( ImageBasicProps_t( 3840, 2160, QC_IMAGE_FORMAT_UYVY ), imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.validSize,
                   std::rand );
    ASSERT_EQ( imgDesc.validSize, imgDesc.size );
    ASSERT_LE( 3840 * 2160 * 2, imgDesc.validSize );
    ASSERT_LE( 3840 * 2160 * 2, imgDesc.planeBufSize[0] );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_EQ( 1, imgDesc.batchSize );
    ASSERT_EQ( QC_IMAGE_FORMAT_UYVY, imgDesc.format );
    ASSERT_LE( 3840 * 2, imgDesc.stride[0] );
    ASSERT_LE( 2160, imgDesc.actualHeight[0] );

    status = imgDesc.ImageToTensor( imgDescTs );
    ASSERT_EQ( QC_STATUS_OK, status ); /* supported as padding is 0 */
    ASSERT_EQ( 0, imgDescTs.offset );
    ASSERT_LE( 3840 * 2160 * 2, imgDescTs.validSize );
    ASSERT_EQ( QC_BUFFER_TYPE_TENSOR, imgDescTs.type );
    ASSERT_EQ( 4, imgDescTs.numDims );
    ASSERT_EQ( 1, imgDescTs.dims[0] );
    ASSERT_EQ( 2160, imgDescTs.dims[1] );
    ASSERT_EQ( 3840, imgDescTs.dims[2] );
    ASSERT_EQ( 2, imgDescTs.dims[3] );

    status = ma.Free( imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );

    /* testing allocate image for NV12 */
    status = ma.Allocate( ImageBasicProps_t( 3840, 2160, QC_IMAGE_FORMAT_NV12 ), imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.validSize,
                   std::rand );
    ASSERT_EQ( imgDesc.validSize, imgDesc.size );
    ASSERT_LE( 3840 * 2160 * 3 / 2, imgDesc.validSize );
    ASSERT_EQ( 2, imgDesc.numPlanes );
    ASSERT_LE( 3840, imgDesc.stride[0] );
    ASSERT_LE( 2160, imgDesc.actualHeight[0] );
    ASSERT_LE( 3840, imgDesc.stride[1] );
    ASSERT_LE( 2160 / 2, imgDesc.actualHeight[1] );

    status = imgDesc.ImageToTensor( imgDescTs );
    ASSERT_EQ( QC_STATUS_UNSUPPORTED, status ); /* not supported as format */

    status = ma.Free( imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );

    /* testing allocate image for RGB */
    status = ma.Allocate( ImageBasicProps_t( 1024, 768, QC_IMAGE_FORMAT_RGB888 ), imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    ASSERT_EQ( imgDesc.size, imgDesc.validSize );
    ASSERT_LE( 1024 * 768 * 3, imgDesc.validSize );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_LE( 1024, imgDesc.stride[0] );
    ASSERT_LE( 768, imgDesc.actualHeight[0] );
    status = ma.Free( imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );

    /* testing allocate batched image for RGB */
    status = ma.Allocate( ImageBasicProps_t( 7, 1024, 768, QC_IMAGE_FORMAT_RGB888 ), imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.validSize,
                   std::rand );
    ASSERT_EQ( imgDesc.size, imgDesc.validSize );
    ASSERT_LE( 1024 * 768 * 3 * 7, imgDesc.validSize );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_LE( 1024, imgDesc.stride[0] );
    ASSERT_LE( 768, imgDesc.actualHeight[0] );

    /* testing get the middle batch of the shared image */
    status = imgDesc.GetImageDesc( imgDescM, 3 );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( (void *) ( (uintptr_t) imgDesc.pBuf + imgDescM.validSize * 3 ),
               imgDescM.GetDataPtr() );
    ASSERT_EQ( imgDescM.validSize * 3, imgDescM.offset );
    ASSERT_EQ( imgDescM.size / 7, imgDescM.validSize );
    ASSERT_LE( 1024 * 768 * 3, imgDescM.validSize );
    ASSERT_EQ( 1, imgDescM.numPlanes );
    ASSERT_LE( 1024 * 3, imgDescM.stride[0] );
    ASSERT_LE( 768, imgDescM.actualHeight[0] );


    status = imgDesc.ImageToTensor( imgDescTs );
    if ( 1024 * 3 == imgDesc.stride[0] )
    {
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( 0, imgDescTs.offset );
        ASSERT_LE( 1024 * 768 * 3 * 7, imgDescTs.validSize );
        ASSERT_EQ( QC_BUFFER_TYPE_TENSOR, imgDescTs.type );
        ASSERT_EQ( 4, imgDescTs.numDims );
        ASSERT_EQ( 7, imgDescTs.dims[0] );
        ASSERT_EQ( 768, imgDescTs.dims[1] );
        ASSERT_EQ( 1024, imgDescTs.dims[2] );
        ASSERT_EQ( 3, imgDescTs.dims[3] );
    }
    else
    { /* if has padding along width, image to tensor is not supported */
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
    }

    status = ma.Free( imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );

    BufferProps_t bufProps;
    ImageProps_t imgProps;
    imgProps.size = 1024;
    imgProps.cache = static_cast<QCAllocationCache_e>( 1234 );
    imgProps.alignment = 8096;
    imgProps.allocatorType = static_cast<QCMemoryAllocator_e>( 5678 );
    bufProps = imgProps;
    ASSERT_EQ( 1024, bufProps.size );
    ASSERT_EQ( 1234, bufProps.cache );
    ASSERT_EQ( 8096, bufProps.alignment );
    ASSERT_EQ( 5678, bufProps.allocatorType );
}

TEST( Memory, SANITY_ImageAllocateByProps )
{
    QCStatus_e status;
    ImageAllocator ma( "IMAGE" );
    ImageDescriptor_t imgDesc;
    ImageProps_t imgProp;

    imgProp.format = QC_IMAGE_FORMAT_UYVY;
    imgProp.batchSize = 1;
    imgProp.width = 3840;
    imgProp.height = 2160;
    imgProp.stride[0] = 3840 * 2;
    imgProp.actualHeight[0] = 2160;
    imgProp.numPlanes = 1;
    imgProp.planeBufSize[0] = 0; /* auto calculated the required size by stride*actualHeight */
    status = ma.Allocate( imgProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.validSize,
                   std::rand );
    ASSERT_EQ( imgDesc.size, imgDesc.validSize );
    ASSERT_EQ( 3840 * 2160 * 2, imgDesc.validSize );
    ASSERT_EQ( 3840 * 2160 * 2, imgDesc.planeBufSize[0] );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_EQ( 3840 * 2, imgDesc.stride[0] );
    ASSERT_EQ( 2160, imgDesc.actualHeight[0] );
    status = ma.Free( imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );


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
    status = ma.Allocate( imgProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.validSize,
                   std::rand );
    ASSERT_EQ( imgDesc.size, imgDesc.validSize );
    ASSERT_EQ( 1920 * 1024 * 3 / 2, imgDesc.validSize );
    ASSERT_EQ( 2, imgDesc.numPlanes );
    ASSERT_EQ( 1920 * 1, imgDesc.stride[0] );
    ASSERT_EQ( 1024, imgDesc.actualHeight[0] );
    status = ma.Free( imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST( Memory, SANITY_ImageAllocateRGBByProps )
{
    QCStatus_e status;
    ImageAllocator ma( "IMAGE" );
    ImageDescriptor_t imgDescAll;
    ImageDescriptor_t imgDescMiddle;
    ImageProps_t imgProp;

    imgProp.format = QC_IMAGE_FORMAT_RGB888;
    imgProp.batchSize = 3;
    imgProp.width = 1024;
    imgProp.height = 768;
    imgProp.stride[0] = 1024 * 3;
    imgProp.actualHeight[0] = 768;
    imgProp.numPlanes = 1;
    imgProp.planeBufSize[0] = 0;

    // testing allocated a batched RGB image
    status = ma.Allocate( imgProp, imgDescAll );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDescAll.pBuf );
    ASSERT_EQ( 0, imgDescAll.offset );
    std::generate( (uint8_t *) imgDescAll.pBuf, (uint8_t *) imgDescAll.pBuf + imgDescAll.validSize,
                   std::rand );
    ASSERT_EQ( imgDescAll.size, imgDescAll.validSize );
    ASSERT_EQ( 3 * 1024 * 768 * 3, imgDescAll.validSize );
    ASSERT_EQ( 1, imgDescAll.numPlanes );
    ASSERT_EQ( 3, imgDescAll.batchSize );
    ASSERT_EQ( 1024 * 3, imgDescAll.stride[0] );
    ASSERT_EQ( 768, imgDescAll.actualHeight[0] );

    /* get the RGB image in the middle of imgDescAll */
    status = imgDescAll.GetImageDesc( imgDescMiddle, 1, 1 );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( ( (uint8_t *) imgDescAll.pBuf ) + 1024 * 768 * 3, imgDescMiddle.GetDataPtr() );
    ASSERT_EQ( 1024 * 768 * 3, imgDescMiddle.offset );
    ASSERT_EQ( imgDescAll.size / 3, imgDescMiddle.validSize );
    ASSERT_EQ( 1024 * 768 * 3, imgDescMiddle.validSize );
    ASSERT_EQ( 1, imgDescMiddle.numPlanes );
    ASSERT_EQ( 1, imgDescMiddle.batchSize );
    ASSERT_EQ( 1024 * 3, imgDescMiddle.stride[0] );
    ASSERT_EQ( 768, imgDescMiddle.actualHeight[0] );

    status = ma.Free( imgDescAll );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST( Memory, SANITY_CompressedImageAllocateByProps )
{
    QCStatus_e status;
    ImageAllocator ma( "IMAGE" );
    ImageDescriptor_t imgDesc;
    ImageProps_t imgProp;

    imgProp.format = QC_IMAGE_FORMAT_COMPRESSED_H265;
    imgProp.batchSize = 1;
    imgProp.width = 3840;
    imgProp.height = 2160;
    imgProp.numPlanes = 1;
    imgProp.planeBufSize[0] = 1024 * 64;
    status = ma.Allocate( imgProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.size, imgDesc.validSize );
    ASSERT_EQ( 1024 * 64, imgDesc.validSize );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    status = ma.Free( imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
}

TEST( Memory, SANITY_TensorAllocate )
{
    TensorAllocator ta( "TENSOR" );
    TensorDescriptor_t tensorDesc;
    TensorProps_t tensorProp( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 128, 128, 10 } );

    auto status = ta.Allocate( tensorProp, tensorDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_NE( nullptr, tensorDesc.pBuf );
    ASSERT_EQ( 0, tensorDesc.offset );
    std::generate( (uint8_t *) tensorDesc.pBuf, (uint8_t *) tensorDesc.pBuf + tensorDesc.size,
                   std::rand );
    ASSERT_EQ( tensorDesc.validSize, tensorDesc.size );
    ASSERT_EQ( 1 * 128 * 128 * 10, tensorDesc.size );
    ASSERT_EQ( 4, tensorDesc.numDims );
    status = ta.Free( tensorDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
}


static std::string GetBufferTextInfo( const BufferDescriptor_t &bufDesc )
{
    std::string str = "";
    std::stringstream ss;

    if ( QC_BUFFER_TYPE_RAW == bufDesc.type )
    {
        str = "Raw";
    }
    else if ( QC_BUFFER_TYPE_IMAGE == bufDesc.type )
    {
        const ImageDescriptor_t *pImgDesc = dynamic_cast<const ImageDescriptor_t *>( &bufDesc );
        ss << "Image format=" << pImgDesc->format << " batch=" << pImgDesc->batchSize
           << " resolution=" << pImgDesc->width << "x" << pImgDesc->height;
        if ( pImgDesc->format < QC_IMAGE_FORMAT_MAX )
        {
            ss << " stride=[";
            for ( uint32_t i = 0; i < pImgDesc->numPlanes; i++ )
            {
                ss << pImgDesc->stride[i] << ", ";
            }
            ss << "] actual height=[";
            for ( uint32_t i = 0; i < pImgDesc->numPlanes; i++ )
            {
                ss << pImgDesc->actualHeight[i] << ", ";
            }
            ss << "] plane size=[";
            for ( uint32_t i = 0; i < pImgDesc->numPlanes; i++ )
            {
                ss << pImgDesc->planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        else
        {
            ss << " plane size=[";
            for ( uint32_t i = 0; i < pImgDesc->numPlanes; i++ )
            {
                ss << pImgDesc->planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        str = ss.str();
    }
    else if ( QC_BUFFER_TYPE_TENSOR == bufDesc.type )
    {
        const TensorDescriptor_t *pTsDesc = dynamic_cast<const TensorDescriptor_t *>( &bufDesc );
        ss << "Tensor type=" << pTsDesc->type << " dims=[";
        for ( uint32_t i = 0; i < pTsDesc->numDims; i++ )
        {
            ss << pTsDesc->dims[i] << ", ";
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

static bool IsTheSameSharedBuffer( BufferDescriptor_t &bufferA, BufferDescriptor_t &bufferB )
{
    bool bEqual = true;
    if ( ( bufferA.pBuf != bufferB.pBuf ) || ( bufferA.pBuf != bufferB.pBuf ) ||
         ( bufferA.dmaHandle != bufferB.dmaHandle ) || ( bufferA.size != bufferB.size ) ||
         ( bufferA.id != bufferB.id ) || ( bufferA.pid != bufferB.pid ) ||
         ( bufferA.allocatorType != bufferB.allocatorType ) || ( bufferA.cache != bufferB.cache ) )
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
        {
            ImageDescriptor_t *pImgDescA = dynamic_cast<ImageDescriptor_t *>( &bufferA );
            ImageDescriptor_t *pImgDescB = dynamic_cast<ImageDescriptor_t *>( &bufferB );
            if ( ( pImgDescA->format != pImgDescB->format ) ||
                 ( pImgDescA->batchSize != pImgDescB->batchSize ) ||
                 ( pImgDescA->width != pImgDescB->width ) ||
                 ( pImgDescA->height != pImgDescB->height ) ||
                 ( pImgDescA->numPlanes != pImgDescB->numPlanes ) ||
                 ( 0 != memcmp( pImgDescA->stride, pImgDescB->stride,
                                sizeof( uint32_t ) * pImgDescA->numPlanes ) ) ||
                 ( 0 != memcmp( pImgDescA->actualHeight, pImgDescB->actualHeight,
                                sizeof( uint32_t ) * pImgDescA->numPlanes ) ) ||
                 ( 0 != memcmp( pImgDescA->planeBufSize, pImgDescB->planeBufSize,
                                sizeof( uint32_t ) * pImgDescA->numPlanes ) ) )
            {
                printf( "buffer imgProps not equal\n" );
                bEqual = false;
            }
            break;
        }
        case QC_BUFFER_TYPE_TENSOR:
        {
            TensorDescriptor_t *pTsDescA = dynamic_cast<TensorDescriptor_t *>( &bufferA );
            TensorDescriptor_t *pTsDescB = dynamic_cast<TensorDescriptor_t *>( &bufferB );
            if ( ( pTsDescA->tensorType != pTsDescB->tensorType ) ||
                 ( pTsDescA->numDims != pTsDescB->numDims ) ||
                 ( 0 != memcmp( pTsDescA->dims, pTsDescB->dims,
                                sizeof( uint32_t ) * pTsDescA->numDims ) ) )
            {
                printf( "buffer tensorProps not equal\n" );
                bEqual = false;
            }
            break;
        }
        default: /* do nothing for RAW type */
            break;
    }

    if ( false == bEqual )
    {
        printf( "bufferA: %s\n", GetBufferTextInfo( bufferA ).c_str() );
        printf( "bufferB: %s\n", GetBufferTextInfo( bufferA ).c_str() );
    }

    return bEqual;
}

TEST( Memory, L2_Buffer )
{
    QCStatus_e status;
    BinaryAllocator ma( "BINARY" );
    for ( int i = (int) QC_MEMORY_ALLOCATOR_DMA; i < (int) QC_MEMORY_ALLOCATOR_LAST; i++ )
    {

        QCMemoryAllocator_e allocatorType = (QCMemoryAllocator_e) i;
        QCAllocationCache_e cache = QC_CACHEABLE_NON;
        BufferDescriptor_t bufDesc;
        status = ma.Allocate(
                BufferProps_t( (size_t) 1024 * 1024 * 1024 * 256, allocatorType, cache ), bufDesc );
#if !defined( __QNXNTO__ )
        if ( QC_STATUS_UNSUPPORTED == status )
        {
            printf( "Linux has no /dev/dma_heap/system-uncached, skip uncached buffer test\n" );
            break;
        }
#endif
        ASSERT_EQ( QC_STATUS_NOMEM, status );

        status = ma.Allocate( BufferProps_t( (size_t) 1024 * 1024 * 32, allocatorType, cache ),
                              bufDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = ma.Free( bufDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    for ( int i = (int) QC_MEMORY_ALLOCATOR_DMA; i < (int) QC_MEMORY_ALLOCATOR_LAST; i++ )
    {
        QCMemoryAllocator_e allocatorType = (QCMemoryAllocator_e) i;
        QCAllocationCache_e cache = QC_CACHEABLE;
        BufferDescriptor_t bufDesc;
#if defined( __QNXNTO__ )
        /* not do this for Linux, see device crashed */
        status = ma.Allocate(
                BufferProps_t( (size_t) 1024 * 1024 * 1024 * 256, allocatorType, cache ), bufDesc );
        ASSERT_EQ( QC_STATUS_NOMEM, status );
#endif

        status = ma.Allocate( BufferProps_t( (size_t) 1024 * 1024 * 32, allocatorType, cache ),
                              bufDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        BufferDescriptor_t bufDesc2 = bufDesc;
        ASSERT_EQ( IsTheSameSharedBuffer( bufDesc, bufDesc2 ), true );

        status = ma.Free( bufDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        BufferDescriptor_t bufDesc;
        status = ma.Allocate( BufferProps_t( (size_t) 1024 * 1024 * 64, QC_MEMORY_ALLOCATOR_LAST ),
                              bufDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = ma.Allocate( BufferProps_t( (size_t) 1024 * 1024 * 64, (QCMemoryAllocator_e) -2 ),
                              bufDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    }

    {
        void *pData;
        uint64_t dmaHandle;

        status = QCDmaAllocate( &pData, nullptr, 1000000, QC_BUFFER_FLAGS_CACHE_WB_WA,
                                QC_BUFFER_USAGE_DEFAULT );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = QCDmaAllocate( nullptr, &dmaHandle, 1000000, QC_BUFFER_FLAGS_CACHE_WB_WA,
                                QC_BUFFER_USAGE_DEFAULT );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = QCDmaAllocate( &pData, &dmaHandle, 1000000, QC_BUFFER_FLAGS_CACHE_WB_WA,
                                QC_BUFFER_USAGE_DEFAULT );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = QCDmaFree( nullptr, 0, 1000000 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = QCDmaFree( (void *) 0x1, 0, 1000000 );
        ASSERT_EQ( QC_STATUS_FAIL, status );
#if !defined( __QNXNTO__ ) /* dmaHandle was only used by Linux when do free */
        status = QCDmaFree( pData, (uint64_t) -1, 1000000 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = QCDmaFree( pData, (uint64_t) 0x7FFFFFFF, 1000000 );
        ASSERT_EQ( QC_STATUS_FAIL, status );
#endif

        status = QCDmaFree( pData, dmaHandle, 1000000 );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}


static void InitImageProps( ImageProps_t &imgProp )
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

TEST( Memory, L2_Image )
{
    ImageAllocator ima( "IMA" );
    BasicImageAllocator bma( "BMA" );
    {
        ImageDescriptor_t imgDesc;
        auto status =
                bma.Allocate( ImageBasicProps( 0, 1920, 2160, QC_IMAGE_FORMAT_UYVY ), imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* invalid batch size */

        status = bma.Allocate( ImageBasicProps( 1, 0, 2160, QC_IMAGE_FORMAT_UYVY ), imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* invalid width */

        status = bma.Allocate( ImageBasicProps( 1, 1920, 0, QC_IMAGE_FORMAT_UYVY ), imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* invalid height */

        status = bma.Allocate( ImageBasicProps( 1, 1920, 1024, QC_IMAGE_FORMAT_MAX ), imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* invalid format */

        status = bma.Allocate( ImageBasicProps( 1, 1920, 1024, (QCImageFormat_e) -5 ), imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* invalid format */

        status = bma.Allocate( ImageBasicProps( 1, 4096 * 4, 4096 * 4, QC_IMAGE_FORMAT_UYVY ),
                               imgDesc );
        ASSERT_EQ( QC_STATUS_FAIL, status ); /* with large image as not supported by apdf */

        ImageProps_t imgProp;

        InitImageProps( imgProp );
        imgProp.batchSize = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid batch size */

        InitImageProps( imgProp );
        imgProp.width = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid width */

        InitImageProps( imgProp );
        imgProp.height = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid height */

        InitImageProps( imgProp );
        imgProp.stride[0] = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid stride */

        InitImageProps( imgProp );
        imgProp.actualHeight[0] = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid actualHeight */

        InitImageProps( imgProp );
        imgProp.numPlanes = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid numPlanes */

        InitImageProps( imgProp );
        imgProp.format = QC_IMAGE_FORMAT_MAX;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = (QCImageFormat_e) -5;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = (QCImageFormat_e) ( (int) QC_IMAGE_FORMAT_COMPRESSED_MIN - 1 );
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = QC_IMAGE_FORMAT_COMPRESSED_MAX;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid foramt */

        InitImageProps( imgProp );
        imgProp.format = QC_IMAGE_FORMAT_COMPRESSED_H265;
        imgProp.numPlanes = 1;
        imgProp.planeBufSize[0] = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status ); /* pImgProps with invalid size */
    }

    for ( int i = 0; i < (int) QC_IMAGE_FORMAT_MAX; i++ )
    {
        ImageDescriptor_t imgDesc;
        ImageProps_t imgProp;

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

        auto status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    for ( int i = 0; i < (int) QC_IMAGE_FORMAT_MAX; i++ )
    {
        ImageDescriptor_t imgDesc;
        QCImageFormat_e format = (QCImageFormat_e) i;

        auto status = bma.Allocate( ImageBasicProps( 1, 1920, 2160, format ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ImageDescriptor_t imgDesc;
        ImageDescriptor_t imgDesc3;
        TensorDescriptor_t imgDescTs;

        auto status = imgDesc.GetImageDesc( imgDesc3, 0 );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, status );

        status = imgDesc.ImageToTensor( imgDescTs );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, status );

        status = bma.Allocate( ImageBasicProps( 3840, 2160, QC_IMAGE_FORMAT_UYVY ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        ImageDescriptor_t imgDesc2 = imgDesc;
        ASSERT_EQ( IsTheSameSharedBuffer( imgDesc, imgDesc2 ), true );

        status = imgDesc.GetImageDesc( imgDesc3, 3 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = imgDesc.GetImageDesc( imgDesc3, 0, 3 );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ImageDescriptor_t imgDesc;
        TensorDescriptor_t imgDescTs;
        ImageProps_t imgProp;

        imgProp.batchSize = 1;
        imgProp.format = QC_IMAGE_FORMAT_BGR888;
        imgProp.width = 1013;
        imgProp.height = 753;
        imgProp.stride[0] = 1024 * 3;
        imgProp.actualHeight[0] = 768;
        imgProp.numPlanes = 1;
        auto status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = imgDesc.ImageToTensor( imgDescTs );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );

        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ImageDescriptor_t imgDesc;
        TensorDescriptor_t imgDescTs;
        ImageProps_t imgProp;

        imgProp.batchSize = 2;
        imgProp.format = QC_IMAGE_FORMAT_BGR888;
        imgProp.width = 1024;
        imgProp.height = 763;
        imgProp.stride[0] = 1024 * 3;
        imgProp.actualHeight[0] = 768;
        imgProp.numPlanes = 1;
        auto status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = imgDesc.ImageToTensor( imgDescTs );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );

        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ImageDescriptor_t imgDesc;
        auto status = bma.Allocate( ImageBasicProps( 1920, 2160, QC_IMAGE_FORMAT_UYVY ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        pid_t pid = imgDesc.pid;
        imgDesc.pid = static_cast<pid_t>( pid + 100 );
        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OUT_OF_BOUND, status );
        imgDesc.pid = pid;
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

TEST( Memory, L2_Tensor )
{
    QCStatus_e status;
    TensorAllocator tsa( "TSA" );
    for ( uint32_t i = 0; i < (uint32_t) QC_TENSOR_TYPE_MAX; i++ )
    {
        TensorDescriptor_t tsDesc;
        TensorProps_t tensorProp( (QCTensorType_e) i,
                                  { i + 1, 1024 * ( i + 1 ) / QC_TENSOR_TYPE_MAX,
                                    1024 * ( QC_TENSOR_TYPE_MAX - i ) / QC_TENSOR_TYPE_MAX, 10 } );

        status = tsa.Allocate( tensorProp, tsDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( QC_BUFFER_TYPE_TENSOR, tsDesc.type );

        TensorDescriptor_t tsDesc2 = tsDesc;
        ASSERT_EQ( IsTheSameSharedBuffer( tsDesc, tsDesc2 ), true );

        status = tsa.Free( tsDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        TensorDescriptor_t tsDesc;
        TensorProps_t tensorProp( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 1024, 768, 3 } );
        tensorProp.numDims = QC_NUM_TENSOR_DIMS + 3;

        status = tsa.Allocate( tensorProp, tsDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        tensorProp.numDims = 4;
        tensorProp.dims[2] = 0;

        status = tsa.Allocate( tensorProp, tsDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        tensorProp.numDims = 4;
        tensorProp.dims[2] = 768;
        tensorProp.tensorType = QC_TENSOR_TYPE_MAX;

        status = tsa.Allocate( tensorProp, tsDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

        tensorProp.tensorType = (QCTensorType_e) -3;

        status = tsa.Allocate( tensorProp, tsDesc );
        ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    }
}

TEST( Memory, L2_Image2Tensor )
{
    QCStatus_e status;
    ImageAllocator ima( "IMA" );
    BasicImageAllocator bma( "BMA" );
    {
        ImageDescriptor_t imgDesc;
        TensorDescriptor_t tensor;
        ImageProps_t imgProp;
        imgProp.format = QC_IMAGE_FORMAT_RGB888;
        imgProp.batchSize = 3;
        imgProp.width = 1920;
        imgProp.height = 1024;
        imgProp.stride[0] = 1920 * 3;
        imgProp.actualHeight[0] = 1028;
        imgProp.numPlanes = 1;
        imgProp.planeBufSize[0] = 0;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( tensor );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        imgProp.actualHeight[0] = 1024;
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( tensor );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ImageDescriptor_t imgDesc;
        TensorDescriptor_t luma;
        TensorDescriptor_t chroma;
        TensorProps_t tensorProp( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 1024, 768, 3 } );

        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, status );

        status = bma.Allocate( ImageBasicProps_t( 1920, 1024, QC_IMAGE_FORMAT_NV12 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( luma.offset, 0 );
        ASSERT_EQ( chroma.offset, 1920 * 1024 );
        ASSERT_EQ( luma.validSize, 1920 * 1024 );
        ASSERT_EQ( chroma.validSize, 1920 * 1024 / 2 );

        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Allocate( ImageBasicProps_t( 1921, 1024, QC_IMAGE_FORMAT_NV12 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );


        ImageProps_t imgProp;
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
        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Allocate( ImageBasicProps_t( 1920, 1025, QC_IMAGE_FORMAT_NV12 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Allocate( ImageBasicProps_t( 2, 1920, 1024, QC_IMAGE_FORMAT_NV12 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Allocate( ImageBasicProps_t( 1920, 1024, QC_IMAGE_FORMAT_RGB888 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }

    {
        ImageDescriptor_t imgDesc;
        TensorDescriptor_t luma;
        TensorDescriptor_t chroma;

        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_INVALID_BUF, status );

        status = bma.Allocate( ImageBasicProps_t( 1920, 1024, QC_IMAGE_FORMAT_P010 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_OK, status );
        ASSERT_EQ( luma.offset, 0 );
        ASSERT_EQ( chroma.offset, 1920 * 1024 * 2 );
        ASSERT_EQ( luma.validSize, 1920 * 1024 * 2 );
        ASSERT_EQ( chroma.validSize, 1920 * 1024 );

        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Allocate( ImageBasicProps_t( 1921, 1024, QC_IMAGE_FORMAT_P010 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        ImageProps_t imgProp;
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

        status = ima.Allocate( imgProp, imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = ima.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Allocate( ImageBasicProps_t( 1920, 1025, QC_IMAGE_FORMAT_P010 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );

        status = bma.Allocate( ImageBasicProps_t( 2, 1920, 1024, QC_IMAGE_FORMAT_P010 ), imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
        status = imgDesc.ImageToTensor( luma, chroma );
        ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
        status = bma.Free( imgDesc );
        ASSERT_EQ( QC_STATUS_OK, status );
    }
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
