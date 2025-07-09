// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_IMAGE_DESCRIPTOR_HPP
#define QC_IMAGE_DESCRIPTOR_HPP

#include "QC/Infras/Memory/BufferDescriptor.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"

#include <string>
#include <vector>

namespace QC
{
namespace Memory
{
/**
 * @brief QCNode Image Basic Buffer Properties.
 *
 * Inherited Members from BufferProps:
 * @param size The total required buffer size.
 * @param alignment The alignment requirement of the buffer.
 * @param attr The attributes of the buffer.
 * @param usage The intended usage of the buffer.
 *
 * New Members:
 * @param format The image format.
 * @param batchSize The image batch size.
 * @param width The image width in pixels.
 * @param height The image height in pixels.
 *
 * @note When using this struct to request image buffer allocation, the member "size" will not be
 * used. The size is determined by the other new members.
 */
typedef struct ImageBasicProps : public BufferProps
{
public:
    /**
     * @brief Constructor for ImageBasicProps.
     * @return None.
     */
    ImageBasicProps() : BufferProps() {}


    /**
     * @brief Constructor for ImageBasicProps.
     * @param width The image width in pixels.
     * @param height The image height in pixels.
     * @param format The image format.
     * @param usage The intended usage of the buffer, default is QC_BUFFER_USAGE_CAMERA.
     * @param attr The attributes of the buffer, default is QC_CACHEABLE.
     * @return None.
     * @note This constructor sets up the properties of the buffer with the given parameters, while
     * most other properties are set to their default suggested values.
     */
    ImageBasicProps( uint32_t width, uint32_t height, QCImageFormat_e format,
                     QCBufferUsage_e usage = QC_BUFFER_USAGE_CAMERA,
                     QCAllocationAttribute_e attr = QC_CACHEABLE )
        : BufferProps( 0, usage, attr ),
          format( format ),
          batchSize( 1 ),
          width( width ),
          height( height )
    {}


    /**
     * @brief Constructor for ImageBasicProps.
     * @param batchSize The image batch size.
     * @param width The image width in pixels.
     * @param height The image height in pixels.
     * @param format The image format.
     * @param usage The intended usage of the buffer, default is QC_BUFFER_USAGE_CAMERA.
     * @param attr The attributes of the buffer, default is QC_CACHEABLE.
     * @return None.
     * @note This constructor sets up the properties of the image with the given parameters, while
     * most other properties are set to their default suggested values.
     */
    ImageBasicProps( uint32_t batchSize, uint32_t width, uint32_t height, QCImageFormat_e format,
                     QCBufferUsage_e usage = QC_BUFFER_USAGE_CAMERA,
                     QCAllocationAttribute_e attr = QC_CACHEABLE )
        : BufferProps( 0, usage, attr ),
          format( format ),
          batchSize( batchSize ),
          width( width ),
          height( height )
    {}
    QCImageFormat_e format;
    uint32_t batchSize;
    uint32_t width;
    uint32_t height;
} ImageBasicProps_t;


/**
 * @brief QCNode Detailed Image Buffer Properties.
 *
 * Inherited Members from ImageBasicProps:
 * @param size The total required buffer size.
 * @param alignment The alignment requirement of the buffer.
 * @param attr The attributes of the buffer.
 * @param usage The intended usage of the buffer.
 * @param format The image format.
 * @param batchSize The image batch size.
 * @param width The image width in pixels.
 * @param height The image height in pixels.
 *
 * New Members:
 * @param stride The image stride along the width in bytes for each plane.
 * @param actualHeight The actual height of the image in scanlines for each plane.
 * @param planeBufSize The actual buffer size of the image for each plane, calculated as (stride *
 * actualHeight + padding size).
 * @param numPlanes The number of image planes.
 *
 * @note When using this struct to request buffer allocation, the member "size" will not be used.
 * The size is determined by the other new members.
 */
typedef struct ImageProps : public ImageBasicProps
{
public:
    /**
     * @brief Constructor for BufferProps.
     * @return None.
     */
    ImageProps() : ImageBasicProps() {}

    /**
     * @brief Constructor for ImageProps.
     * @param batchSize The image batch size.
     * @param width The image width in pixels.
     * @param height The image height in pixels.
     * @param format The image format.
     * @param strides The image stride along the width in bytes for each plane.
     * @param actualHeights The actual height of the image in scanlines for each plane.
     * @param planeBufSizes The actual buffer size of the image for each plane, calculated as
     * (stride * actualHeight + padding size).
     * @param usage The intended usage of the buffer, default is QC_BUFFER_USAGE_CAMERA.
     * @param attr The attributes of the buffer, default is QC_CACHEABLE.
     * @return None.
     * @note This constructor sets up the properties of the image with the given parameters, while
     * most other properties are set to their default suggested values.
     * @note The user must ensure the sizes of the vector strides, actualHeights, and numPlanes are
     * equal to the number of planes in the image format, and must ensure the size is less than or
     * equal to QC_NUM_IMAGE_PLANES.
     */
    ImageProps( uint32_t batchSize, uint32_t width, uint32_t height, QCImageFormat_e format,
                std::vector<uint32_t> strides, std::vector<uint32_t> actualHeights,
                std::vector<uint32_t> planeBufSizes, QCBufferUsage_e usage = QC_BUFFER_USAGE_CAMERA,
                QCAllocationAttribute_e attr = QC_CACHEABLE )
        : ImageBasicProps( batchSize, width, height, format, usage, attr )
    {
        numPlanes = static_cast<uint32_t>( strides.size() );
        std::copy( strides.begin(), strides.end(), stride );
        std::copy( actualHeights.begin(), actualHeights.end(), actualHeight );
        std::copy( planeBufSizes.begin(), planeBufSizes.end(), planeBufSize );
    }
    uint32_t stride[QC_NUM_IMAGE_PLANES];
    uint32_t actualHeight[QC_NUM_IMAGE_PLANES];
    uint32_t planeBufSize[QC_NUM_IMAGE_PLANES];
    uint32_t numPlanes;
} ImageProps_t;

/**
 * @brief Descriptor for QCNode Shared Image Buffer.
 *
 * This structure represents the shared image buffer descriptor for QCNode. It extends the
 * BufferDescriptor and includes additional members specific to image properties.
 *
 * Inherited Members from BufferDescriptor:
 * @param name The name of the buffer.
 * @param pBuf The virtual address of the actual buffer.
 * @param size The actual size of the buffer.
 * @param type The type of the buffer.
 * @param pBufBase The base virtual address of the DMA buffer.
 * @param dmaHandle The DMA handle of the buffer.
 * @param dmaSize The size of the DMA buffer.
 * @param offset The offset of the valid buffer within the shared buffer.
 * @param id The unique ID assigned by the buffer manager.
 * @param pid The process ID that allocated this buffer.
 * @param usage The intended usage of the buffer.
 * @param attr The attributes associated with the buffer.
 *
 * New Members:
 * @param format The image format.
 * @param batchSize The image batch size.
 * @param width The image width in pixels.
 * @param height The image height in pixels.
 * @param stride The image stride along the width in bytes for each plane.
 * @param actualHeight The actual height of the image in scanlines for each plane.
 * @param planeBufSize The actual buffer size of the image for each plane, calculated as (stride *
 * actualHeight + padding size).
 * @param numPlanes The number of image planes.
 */
typedef struct ImageDescriptor : public BufferDescriptor
{
public:
    ImageDescriptor() : BufferDescriptor() {}

    /**
     * @brief Sets up the image descriptor from another buffer descriptor object.
     * @param[in] other The buffer descriptor object from which buffer members are copied.
     * @return The updated image descriptor object.
     */
    ImageDescriptor &operator=( const BufferDescriptor &other );


    /**
     * @brief Initializes the image descriptor using another shared buffer object.
     * @param[in] other The shared buffer object from which buffer data is copied.
     * @return The updated image descriptor.
     * @note This is a temporary workaround API introduced to facilitate development during phase 2.
     *       It will be removed once phase 2 development is complete.
     */
    ImageDescriptor &operator=( const QCSharedBuffer_t &other );

    /**
     * @brief Converts the image buffer descriptor to a tensor buffer descriptor.
     * @param[out] tensorDesc The tensor buffer descriptor.
     * @return QC_STATUS_OK on success, other status codes on failure.
     * @note The image must have 1 plane and no padding.
     */
    QCStatus_e ImageToTensor( TensorDescriptor_t &tensorDesc ) const;

    /**
     * @brief Converts the image buffer descriptor to the tensor luma and chroma buffer descriptors.
     * @param[out] luma The tensor buffer descriptor representing the luma (luminance) plane Y.
     * @param[out] chroma The tensor buffer descriptor representing the chroma plane.
     * @return QC_STATUS_OK on success, other status codes on failure.
     * @note The image must be in NV12 or P010 format and have no padding.
     */
    QCStatus_e ImageToTensor( TensorDescriptor_t &luma, TensorDescriptor_t &chroma ) const;

    /**
     * @brief Gets a new buffer descriptor that represents the image batches specified by
     * batchOffset and batchSize.
     * @param[out] imageDesc The image buffer descriptor representing the image batches specified by
     * batchOffset and batchSize.
     * @param[in] batchOffset The image batch offset.
     * @param[in] batchSize The image batch size.
     * @return QC_STATUS_OK on success, other status codes on failure.
     */
    QCStatus_e GetImageDesc( ImageDescriptor &imageDesc, uint32_t batchOffset,
                             uint32_t batchSize = 1 ) const;

    QCImageFormat_e format;
    uint32_t batchSize;
    uint32_t width;
    uint32_t height;
    uint32_t stride[QC_NUM_IMAGE_PLANES];
    uint32_t actualHeight[QC_NUM_IMAGE_PLANES];
    uint32_t planeBufSize[QC_NUM_IMAGE_PLANES];
    uint32_t numPlanes;
} ImageDescriptor_t;

}   // namespace Memory
}   // namespace QC

#endif   // QC_IMAGE_DESCRIPTOR_HPP
