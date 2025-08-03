// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
#ifndef QC_SHARED_BUFFER_HPP
#define QC_SHARED_BUFFER_HPP

#include "QC/Common/Types.hpp"
#include "QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp"

namespace QC
{
namespace Memory
{

/** @brief QC Shared Buffer between Components for zero copy purpose */
typedef struct QCSharedBuffer
{
    QCBuffer_t buffer;   /**< The shared buffer */
    size_t size;         /**< The size of the valid buffer in the shared buffer.
                          * Note: For the compressed image in the H264 or H265 format, this size represents
                          * the encoded bit stream data lenth, which can be changed during runtime. */
    size_t offset;       /**< The offset of the valid buffer in the shared buffer */
    QCBufferType_e type; /**< The buffer type */
    union
    {
        /**< RAW has no properties. */
        QCImageProps_t imgProps;     /**< The image properties if type is IMAGE */
        QCTensorProps_t tensorProps; /**< The tensor properties if type is TENSOR */
    };

public:
    QCSharedBuffer();
    ~QCSharedBuffer();

    /**
     * @brief Construct the shared buffer descriptor from another shared buffer object.
     * @param[in] other The shared buffer object from which buffer members are copied.
     * @return The updated tensor descriptor.
     * @note This is a temporary workaround API to support smoother development during phase 2.
     *       It will be removed once phase 2 is complete.
     */
    QCSharedBuffer( const QCBufferDescriptorBase_t &other );

    /**
     * @brief Construct an shared buffer from another shared buffer
     * @param[in] rhs the shared buffer
     * @return void
     */
    QCSharedBuffer( const QCSharedBuffer &rhs );
    QCSharedBuffer &operator=( const QCSharedBuffer &rhs );

    /**
     * @brief Allocate the DMA memory
     * @param[in] size the wanted DMA memory size
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Allocate( size_t size, QCBufferUsage_e usage = QC_BUFFER_USAGE_DEFAULT,
                         QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for the image with the best strides/paddings
     * that can be shared among CPU/GPU/VPU/HTP, etc
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Allocate( uint32_t width, uint32_t height, QCImageFormat_e format,
                         QCBufferUsage_e usage = QC_BUFFER_USAGE_CAMERA,
                         QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for batched image with best strides/paddings
     * that can be shared among CPU/GPU/VPU/HTP, etc
     * @param[in] batchSize the image batch size
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Allocate( uint32_t batchSize, uint32_t width, uint32_t height,
                         QCImageFormat_e format, QCBufferUsage_e usage = QC_BUFFER_USAGE_CAMERA,
                         QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for image with specified image properties
     * @param[in] pImgProps the specified image properties
     * @param[in] flags the DMA buffer flags
     * @param[in] usage the DMA buffer usage
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Allocate( const QCImageProps_t *pImgProps,
                         QCBufferUsage_e usage = QC_BUFFER_USAGE_CAMERA,
                         QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Allocate the DMA memory for tensor with specified tensor properties
     * @param[in] pTensorProps the specified tensor properties
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Allocate( const QCTensorProps_t *pTensorProps,
                         QCBufferUsage_e usage = QC_BUFFER_USAGE_HTP,
                         QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Free the DMA memory
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Free();

    /**
     * @brief Get the shared buffer information
     * @param[out] pSharedBuffer pointer to hold the shared buffer information for
     * the image batches specified by batchOffset and batchSize
     * @param[in] batchOffset the image batch offset
     * @param[in] batchSize the image batch size
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetSharedBuffer( QCSharedBuffer *pSharedBuffer, uint32_t batchOffset,
                                uint32_t batchSize = 1 );

    /**
     * @brief get the valid buffer virtual address
     * @return the valid buffer virtual address
     */
    void *data() const { return (void *) ( &( (uint8_t *) buffer.pData )[offset] ); }

    /**
     * @brief Convert the shared buffer type from image to tensor
     * @param[out] pSharedBuffer pointer to hold the shared buffer information for
     * the tensor that converted from the image
     * @return QC_STATUS_OK on success, others on failure
     * @note: The image must has 1 plane and has no paddings .
     */
    QCStatus_e ImageToTensor( QCSharedBuffer *pSharedBuffer );

    /**
     * @brief Convert the shared buffer type from image to tensor luma and chroma
     * @param[out] pLuma pointer to hold the shared buffer information for
     * the tensor that represent the luma "luminance" plane Y.
     * @param[out] pChroma pointer to hold the shared buffer information for
     * the tensor that represent the chroma plane.
     * @return QC_STATUS_OK on success, others on failure
     * @note: The image must be in format NV12 or P010 and has no paddings.
     */
    QCStatus_e ImageToTensor( QCSharedBuffer *pLuma, QCSharedBuffer *pChroma );

    /**
     * @brief Import a shared DMA memory allocated by the other process
     * @param[in] pSharedBuffer pointer to hold the shared buffer information
     * @note: Except the pSharedBuffer->buffer.pData will be ignored, all the
     * other information must be provided and will be used to import the shared
     * DMA memory allocated by the other process in this process, and create a
     * new QCSharedBuffer descriptor.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Import( const QCSharedBuffer *pSharedBuffer );

    /**
     * @brief Un-Import a shared memory allocated by the other process
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e UnImport();

private:
    /**
     * @brief Initialize the shared buffer variables
     */
    void Init();
} QCSharedBuffer_t;

/**
 * @brief Allocate the DMA memory
 * @param[out] pData the allocated DMA data address
 * @param[out] pDmaHandle the allocated DMA handle
 * @param[in] size the wanted DMA memory size
 * @param[in] flags the DMA buffer flags
 * @param[in] usage the DMA buffer usage
 * @return QC_STATUS_OK on success, others on failure
 */
QCStatus_e QCDmaAllocate( void **pData, uint64_t *pDmaHandle, size_t size, QCBufferFlags_t flags,
                          QCBufferUsage_e usage );

/**
 * @brief Free the DMA memory
 * @param[in] pData the allocated DMA data address
 * @param[in] dmaHandle the allocated DMA handle
 * @param[in] size the DMA memory size
 * @return QC_STATUS_OK on success, others on failure
 */
QCStatus_e QCDmaFree( void *pData, uint64_t dmaHandle, size_t size );

/**
 * @brief Import the DMA memory
 * @param[out] pData the imported DMA data address
 * @param[out] pDmaHandle the imported DMA handle
 * @param[in] pid the process id that allocated this DMA memory
 * @param[in] dmaHandle the DMA handle
 * @param[in] size the DMA memory size
 * @param[in] flags the DMA buffer flags
 * @param[in] usage the DMA buffer usage
 * @return QC_STATUS_OK on success, others on failure
 */
QCStatus_e QCDmaImport( void **pData, uint64_t *pDmaHandle, pid_t pid, uint64_t dmaHandle,
                        size_t size, QCBufferFlags_t flags, QCBufferUsage_e usage );

/**
 * @brief Un-Import the DMA memory
 * @param[in] pData the imported DMA data address
 * @param[in] dmaHandle the imported DMA handle
 * @param[in] size the DMA memory size
 * @return QC_STATUS_OK on success, others on failure
 */
QCStatus_e QCDmaUnImport( void *pData, uint64_t dmaHandle, size_t size );
}   // namespace Memory
}   // namespace QC

#endif   // QC_SHARED_BUFFER_HPP
