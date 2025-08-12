// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SHARED_BUFFER_POOL_HPP
#define QC_SHARED_BUFFER_POOL_HPP

#include <cinttypes>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/BufferDescriptor.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/sample/BufferManager.hpp"

namespace QC
{
namespace sample
{

using namespace QC::Memory;

/** @brief The Shared Buffer information structure */
typedef struct SharedBuffer
{
public:
    SharedBuffer() : buffer( dummy )
    {
        dummy.name = "dummy";
        dummy.pBuf = nullptr;
        dummy.size = 0;
        dummy.type = QC_BUFFER_TYPE_MAX;
    }
    std::reference_wrapper<QCBufferDescriptorBase_t> buffer;
    QCSharedBuffer_t sharedBuffer; /**< The QC shared buffer */
    uint64_t pubHandle; /**< The publish handle associated with shared buffer that to be used to
                           release the shared buffer */

    QCBufferDescriptorBase_t dummy;
    ImageDescriptor_t imgDesc;
    TensorDescriptor_t tensorDesc;

    /* hold 2 more tensor descriptor that as for QNN, maybe need to convert the image to 2 tensor
     * descriptors */
    TensorDescriptor_t luma;
    TensorDescriptor_t chroma;

    QCBufferDescriptorBase_t &GetBuffer() { return buffer; }

    void *GetDataPtr()
    {
        QCBufferDescriptorBase_t &bufDesc = buffer;
        return bufDesc.pBuf;
    }

    size_t GetDataSize()
    {
        QCBufferDescriptorBase_t &bufDesc = buffer;
        return bufDesc.size;
    }

    QCBufferType_e GetBufferType()
    {
        QCBufferDescriptorBase_t &bufDesc = buffer;
        return bufDesc.type;
    }

    QCImageProps_t GetImageProps()
    {
        QCImageProps_t imgProps = {
                QC_IMAGE_FORMAT_MAX,
                0,
        };
        QCBufferDescriptorBase_t &bufDesc = buffer;
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            imgProps.format = pImage->format;
            imgProps.width = pImage->width;
            imgProps.height = pImage->height;
            imgProps.batchSize = pImage->batchSize;
            std::copy( pImage->stride, pImage->stride + pImage->numPlanes, imgProps.stride );
            std::copy( pImage->actualHeight, pImage->actualHeight + pImage->numPlanes,
                       imgProps.actualHeight );
            std::copy( pImage->planeBufSize, pImage->planeBufSize + pImage->numPlanes,
                       imgProps.planeBufSize );
            imgProps.numPlanes = pImage->numPlanes;
        }
        return imgProps;
    };
    QCTensorProps_t GetTensorProps()
    {
        QCTensorProps_t tsProps = {
                QC_TENSOR_TYPE_MAX,
                { 0 },
        };
        QCBufferDescriptorBase_t &bufDesc = buffer;
        TensorDescriptor_t *pTensor = dynamic_cast<TensorDescriptor_t *>( &bufDesc );
        if ( nullptr != pTensor )
        {
            tsProps.type = pTensor->tensorType;
            tsProps.numDims = pTensor->numDims;
            std::copy( pTensor->dims, pTensor->dims + pTensor->numDims, tsProps.dims );
        }
        return tsProps;
    };
} SharedBuffer_t;

/** @brief The QC shared buffer ping-pong pool */
class SharedBufferPool
{
public:
    SharedBufferPool();
    ~SharedBufferPool();

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] nodeId the nodeId to be used to create the shared memory pool
     * @param[in] level the logger level
     * @param[in] number the number of the ping-pong shared buffers
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] allocatorType The allocaor type used for allocation the buffer.
     * @param[in] cache The cache type of the buffer.
     * @detdesc
     * It was by using the image width/height/format to allocate image buffers with best the best
     * strides/paddings that can be shared among CPU/GPU/VPU/HTP, etc
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level, uint32_t number,
                     uint32_t width, uint32_t height, QCImageFormat_e format,
                     QCMemoryAllocator_e allocatorType = QC_MEMORY_ALLOCATOR_DMA,
                     QCAllocationCache_e cache = QC_CACHEABLE );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] nodeId the nodeId to be used to create the shared memory pool
     * @param[in] level the logger level
     * @param[in] batchSize the image batch size
     * @param[in] number the number of the ping-pong shared buffers
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] allocatorType The allocaor type used for allocation the buffer.
     * @param[in] cache The cache type of the buffer.
     * @detdesc
     * It was by using the image batchSize/width/height/format to allocate batched image buffers
     * with best the best strides/paddings that can be shared among CPU/GPU/VPU/HTP, etc
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level, uint32_t number,
                     uint32_t batchSize, uint32_t width, uint32_t height, QCImageFormat_e format,
                     QCMemoryAllocator_e allocatorType = QC_MEMORY_ALLOCATOR_DMA,
                     QCAllocationCache_e cache = QC_CACHEABLE );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] nodeId the nodeId to be used to create the shared memory pool
     * @param[in] level the logger level
     * @param[in] imageProps the specified image properties
     * @param[in] allocatorType The allocaor type used for allocation the buffer.
     * @param[in] cache The cache type of the buffer.
     * @detdesc
     * It was by using the specified image properties to allocate image buffers.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level, uint32_t number,
                     const QCImageProps_t &imageProps,
                     QCMemoryAllocator_e allocatorType = QC_MEMORY_ALLOCATOR_DMA,
                     QCAllocationCache_e cache = QC_CACHEABLE );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] nodeId the nodeId to be used to create the shared memory pool
     * @param[in] level the logger level
     * @param[in] tensorProps the specified tensor properties
     * @param[in] allocatorType The allocaor type used for allocation the buffer.
     * @param[in] cache The cache type of the buffer.
     * @detdesc
     * It was by using the specified tensor properties to allocate tensor buffers.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level, uint32_t number,
                     const QCTensorProps_t &tensorProps,
                     QCMemoryAllocator_e allocatorType = QC_MEMORY_ALLOCATOR_DMA,
                     QCAllocationCache_e cache = QC_CACHEABLE );

    /**
     * @brief Retrieves a list of shared buffer descriptors.
     *
     * @param[out] buffers A reference to a vector that will be populated with references to shared
     * buffer descriptors.
     * @return QC_STATUS_OK on success; otherwise, returns an appropriate error code indicating the
     * failure reason.
     */
    QCStatus_e GetBuffers( std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers );

    /**
     * @brief Get a free shared buffer
     * @return The shared buffer on success, nullptr on failure
     */
    std::shared_ptr<SharedBuffer_t> Get();

    /**
     * @brief deinitialize the shared memory ping-pong pool
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

private:
    QCStatus_e Init( std::string name, QCNodeID_t nodeId, Logger_Level_e level, uint32_t number );
    void Deleter( SharedBuffer_t *ptrToDelete );

    QCStatus_e Register( void );

    struct SharedBufferInfo
    {
        SharedBuffer_t sharedBuffer;
        int dirty; /**< A flag to indicate the buffer is in use or free */
    };

    QC_DECLARE_LOGGER();
    std::string m_name;
    std::vector<SharedBufferInfo> m_queue;
    bool m_bIsInited = false;

    BufferManager *m_pBufMgr = nullptr;
};

}   // namespace sample
}   // namespace QC

#endif   // #ifndef QC_SHARED_BUFFER_POOL_HPP
