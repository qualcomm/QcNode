// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SHARED_BUFFER_POOL_HPP_
#define _QC_SHARED_BUFFER_POOL_HPP_

#include <cinttypes>
#include <cstring>
#include <memory>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"


using namespace QC;

namespace QC
{
namespace sample
{

/** @brief The Shared Buffer information structure */
typedef struct
{
    QCSharedBuffer_t sharedBuffer; /**< The QC shared buffer */
    uint64_t pubHandle; /**< The publish handle associated with shared buffer that to be used to
                           release the shared buffer */
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
     * @param[in] level the logger level
     * @param[in] number the number of the ping-pong shared buffers
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @detdesc
     * It was by using the image width/height/format to allocate image buffers with best the best
     * strides/paddings that can be shared among CPU/GPU/VPU/HTP, etc
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, Logger_Level_e level, uint32_t number, uint32_t width,
                     uint32_t height, QCImageFormat_e format,
                     QCBufferUsage_e usage = QC_BUFFER_USAGE_DEFAULT,
                     QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] level the logger level
     * @param[in] batchSize the image batch size
     * @param[in] number the number of the ping-pong shared buffers
     * @param[in] width the image width
     * @param[in] height the image height
     * @param[in] format the image format
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @detdesc
     * It was by using the image batchSize/width/height/format to allocate batched image buffers
     * with best the best strides/paddings that can be shared among CPU/GPU/VPU/HTP, etc
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, Logger_Level_e level, uint32_t number, uint32_t batchSize,
                     uint32_t width, uint32_t height, QCImageFormat_e format,
                     QCBufferUsage_e usage = QC_BUFFER_USAGE_DEFAULT,
                     QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] level the logger level
     * @param[in] imageProps the specified image properties
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @detdesc
     * It was by using the specified image properties to allocate image buffers.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, Logger_Level_e level, uint32_t number,
                     const QCImageProps_t &imageProps,
                     QCBufferUsage_e usage = QC_BUFFER_USAGE_DEFAULT,
                     QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] level the logger level
     * @param[in] tensorProps the specified tensor properties
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @detdesc
     * It was by using the specified tensor properties to allocate tensor buffers.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( std::string name, Logger_Level_e level, uint32_t number,
                     const QCTensorProps_t &tensorProps,
                     QCBufferUsage_e usage = QC_BUFFER_USAGE_DEFAULT,
                     QCBufferFlags_t flags = QC_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief get shared buffers
     * @param pBuffers pointer to hold the shared buffers
     * @param numBuffers number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetBuffers( QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

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
    QCStatus_e Init( std::string name, Logger_Level_e level, uint32_t number );
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
};

}   // namespace sample
}   // namespace QC

#endif   // #ifndef _QC_SHARED_BUFFER_POOL_HPP_
