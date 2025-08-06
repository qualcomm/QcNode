// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/UtilsBase.hpp"


#include "gtest/gtest.h"

using namespace QC;
using namespace QC::Memory;

class Test_QCMemoryUtilsBase : public testing::Test
{

protected:
    void SetUp() override {}

    void TearDown() override {}
};

TEST_F( Test_QCMemoryUtilsBase, SANITY )
{

    QCStatus_e status;
    UtilsBase Ifs;
    // QCMemoryUtilsIfs

    ImageDescriptor_t imgDesc;
    TensorDescriptor_t tensorDesc;

    ImageProps_t imageProp;
    ImageBasicProps_t imageBaseProp;
    TensorProps_t tensProp;

    /* testing for UYVY */
    imageBaseProp = ImageBasicProps_t( 3840, 2160, QC_IMAGE_FORMAT_UYVY );
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( imgDesc.GetDataPtr(), imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.size, imgDesc.GetDataSize() );
    ASSERT_LE( 3840 * 2160 * 2, imageBaseProp.size );
    ASSERT_LE( 0, imgDesc.size );
    ASSERT_LE( 3840 * 2160 * 2, imgDesc.planeBufSize[0] );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_EQ( 1, imgDesc.batchSize );
    ASSERT_EQ( QC_IMAGE_FORMAT_UYVY, imgDesc.format );
    ASSERT_LE( 3840 * 2, imgDesc.stride[0] );
    ASSERT_LE( 2160, imgDesc.actualHeight[0] );

    /* testing allocate image for NV12 */
    imageBaseProp = ImageBasicProps_t( 3840, 2160, QC_IMAGE_FORMAT_NV12 );
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.size, imgDesc.GetDataSize() );
    ASSERT_LE( 3840 * 2160 * 3 / 20, imageBaseProp.size );
    ASSERT_LE( 0, imgDesc.size );
    ASSERT_EQ( 2, imgDesc.numPlanes );
    ASSERT_LE( 3840, imgDesc.stride[0] );
    ASSERT_LE( 2160, imgDesc.actualHeight[0] );
    ASSERT_LE( 3840, imgDesc.stride[1] );
    ASSERT_LE( 2160 / 2, imgDesc.actualHeight[1] );

    /* testing for RGB */
    imageBaseProp = ImageBasicProps_t( 1024, 768, QC_IMAGE_FORMAT_RGB888 );
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    ASSERT_EQ( imgDesc.GetDataSize(), imgDesc.size );
    ASSERT_LE( 1024 * 768 * 3, imageBaseProp.size );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_LE( 1024, imgDesc.stride[0] );
    ASSERT_LE( 768, imgDesc.actualHeight[0] );
    ASSERT_EQ( QC_STATUS_OK, status );

    /* testing batched for RGB */
    imageBaseProp = ImageBasicProps_t( 7, 1024, 768, QC_IMAGE_FORMAT_RGB888 );
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.GetDataSize(), imgDesc.size );
    ASSERT_LE( 1024 * 768 * 3 * 7, imageBaseProp.size );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_LE( 1024, imgDesc.stride[0] );
    ASSERT_LE( 768, imgDesc.actualHeight[0] );

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

    imageBaseProp.format = QC_IMAGE_FORMAT_NV12_UBWC;
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );

    imageBaseProp.format = QC_IMAGE_FORMAT_TP10_UBWC;
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );

    imageBaseProp.format = QC_IMAGE_FORMAT_MAX;
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageBaseProp.format = (QCImageFormat_e) ( -1 );
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageBaseProp.format = QC_IMAGE_FORMAT_RGB888;
    imageBaseProp.height = 0;
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageBaseProp.width = 0;
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageBaseProp.batchSize = 0;
    status = Ifs.SetImageDescFromImageBasicProp( imageBaseProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageProp.format = QC_IMAGE_FORMAT_UYVY;
    imageProp.batchSize = 1;
    imageProp.width = 3840;
    imageProp.height = 2160;
    imageProp.stride[0] = 3840 * 2;
    imageProp.actualHeight[0] = 2160;
    imageProp.numPlanes = 1;
    imageProp.planeBufSize[0] = 0;

    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.GetDataSize(), imgDesc.size );
    ASSERT_EQ( 3840 * 2160 * 2, imageProp.size );
    ASSERT_EQ( 3840 * 2160 * 2, imgDesc.planeBufSize[0] );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_EQ( 3840 * 2, imgDesc.stride[0] );
    ASSERT_EQ( 2160, imgDesc.actualHeight[0] );

    imageProp.format = QC_IMAGE_FORMAT_NV12;
    imageProp.batchSize = 1;
    imageProp.width = 1920;
    imageProp.height = 1024;
    imageProp.stride[0] = 1920;
    imageProp.actualHeight[0] = 1024;
    imageProp.stride[1] = 1920;
    imageProp.actualHeight[1] = 512;
    imageProp.numPlanes = 2;
    imageProp.planeBufSize[0] = 0;
    imageProp.planeBufSize[1] = 0;

    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.GetDataSize(), imgDesc.size );
    ASSERT_EQ( 1920 * 1024 * 3 / 2, imageProp.size );
    ASSERT_EQ( 2, imgDesc.numPlanes );
    ASSERT_EQ( 1920 * 1, imgDesc.stride[0] );
    ASSERT_EQ( 1024, imgDesc.actualHeight[0] );

    imageProp.format = QC_IMAGE_FORMAT_RGB888;
    imageProp.batchSize = 3;
    imageProp.width = 1024;
    imageProp.height = 768;
    imageProp.stride[0] = 1024 * 3;
    imageProp.actualHeight[0] = 768;
    imageProp.numPlanes = 1;
    imageProp.planeBufSize[0] = 0;

    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.GetDataSize(), imgDesc.size );
    ASSERT_EQ( 3 * 1024 * 768 * 3, imageProp.size );
    ASSERT_EQ( 1, imgDesc.numPlanes );
    ASSERT_EQ( 3, imgDesc.batchSize );
    ASSERT_EQ( 1024 * 3, imgDesc.stride[0] );
    ASSERT_EQ( 768, imgDesc.actualHeight[0] );

    imageProp.format = QC_IMAGE_FORMAT_COMPRESSED_H265;
    imageProp.batchSize = 1;
    imageProp.width = 3840;
    imageProp.height = 2160;
    imageProp.numPlanes = 1;
    imageProp.planeBufSize[0] = 1024 * 64;

    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, imgDesc.pBuf );
    ASSERT_EQ( 0, imgDesc.offset );
    std::generate( (uint8_t *) imgDesc.pBuf, (uint8_t *) imgDesc.pBuf + imgDesc.size, std::rand );
    ASSERT_EQ( imgDesc.GetDataSize(), imgDesc.size );
    ASSERT_EQ( 0, imgDesc.size );
    ASSERT_EQ( 1, imgDesc.numPlanes );

    imageProp.format = (QCImageFormat_e) ( -1 );
    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageProp.width = 0;
    imageProp.height = 2160;
    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageProp.batchSize = 0;
    imageProp.width = 2160;
    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );
    imageProp.batchSize = 1;

    imageProp.format = QC_IMAGE_FORMAT_MAX;
    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    imageProp.format = QC_IMAGE_FORMAT_COMPRESSED_MAX;
    status = Ifs.SetImageDescFromImageProp( imageProp, imgDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    // Tensor
    tensProp = TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 128, 128, 10 } );
    status = Ifs.SetTensorDescFromTensorProp( tensProp, tensorDesc );
    ASSERT_EQ( QC_STATUS_OK, status );
    ASSERT_EQ( nullptr, tensorDesc.pBuf );
    ASSERT_EQ( 0, tensorDesc.offset );
    std::generate( (uint8_t *) tensorDesc.pBuf, (uint8_t *) tensorDesc.pBuf + tensorDesc.size,
                   std::rand );
    ASSERT_EQ( tensorDesc.GetDataSize(), tensorDesc.size );
    ASSERT_EQ( 1 * 128 * 128 * 10, tensProp.size );
    ASSERT_EQ( 4, tensorDesc.numDims );

    tensProp.dims[0] = 0;
    status = Ifs.SetTensorDescFromTensorProp( tensProp, tensorDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    tensProp.tensorType = QC_TENSOR_TYPE_MAX;
    status = Ifs.SetTensorDescFromTensorProp( tensProp, tensorDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    tensProp.tensorType = (QCTensorType_e) 0;
    status = Ifs.SetTensorDescFromTensorProp( tensProp, tensorDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );

    tensProp.numDims = 10;
    status = Ifs.SetTensorDescFromTensorProp( tensProp, tensorDesc );
    ASSERT_EQ( QC_STATUS_BAD_ARGUMENTS, status );


    QCBufferDescriptorBase_t buff;
    QCBufferDescriptorBase_t mapped;
    status = Ifs.MemoryMap( buff, mapped );
    ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );

    status = Ifs.MemoryUnMap( buff );
    ASSERT_EQ( QC_STATUS_UNSUPPORTED, status );
}
