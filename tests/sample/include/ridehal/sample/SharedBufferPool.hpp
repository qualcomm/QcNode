// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SHARED_BUFFER_POOL_HPP_
#define _RIDEHAL_SHARED_BUFFER_POOL_HPP_

#include <cinttypes>
#include <cstring>
#include <memory>
#include <vector>

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"


using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

/** @brief The Shared Buffer information structure */
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer; /**< The RideHal shared buffer */
    uint64_t pubHandle; /**< The publish handle associated with shared buffer that to be used to
                           release the shared buffer */
} SharedBuffer_t;

/** @brief The RideHal shared buffer ping-pong pool */
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
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( std::string name, Logger_Level_e level, uint32_t number, uint32_t width,
                         uint32_t height, RideHal_ImageFormat_e format,
                         RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_DEFAULT,
                         RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

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
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( std::string name, Logger_Level_e level, uint32_t number,
                         uint32_t batchSize, uint32_t width, uint32_t height,
                         RideHal_ImageFormat_e format,
                         RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_DEFAULT,
                         RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] level the logger level
     * @param[in] imageProps the specified image properties
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @detdesc
     * It was by using the specified image properties to allocate image buffers.
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( std::string name, Logger_Level_e level, uint32_t number,
                         const RideHal_ImageProps_t &imageProps,
                         RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_DEFAULT,
                         RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief Do initialization of the shared memory ping-pong pool
     * @param[in] name the shared memory pool name
     * @param[in] level the logger level
     * @param[in] tensorProps the specified tensor properties
     * @param[in] usage the DMA buffer usage
     * @param[in] flags the DMA buffer flags
     * @detdesc
     * It was by using the specified tensor properties to allocate tensor buffers.
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( std::string name, Logger_Level_e level, uint32_t number,
                         const RideHal_TensorProps_t &tensorProps,
                         RideHal_BufferUsage_e usage = RIDEHAL_BUFFER_USAGE_DEFAULT,
                         RideHal_BufferFlags_t flags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA );

    /**
     * @brief get shared buffers
     * @param pBuffers pointer to hold the shared buffers
     * @param numBuffers number of shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetBuffers( RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Get a free shared buffer
     * @return The shared buffer on success, nullptr on failure
     */
    std::shared_ptr<SharedBuffer_t> Get();

    /**
     * @brief deinitialize the shared memory ping-pong pool
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

private:
    RideHalError_e Init( std::string name, Logger_Level_e level, uint32_t number );
    void Deleter( SharedBuffer_t *ptrToDelete );

    RideHalError_e Register( void );

    struct SharedBufferInfo
    {
        SharedBuffer_t sharedBuffer;
        int dirty; /**< A flag to indicate the buffer is in use or free */
    };

    RIDEHAL_DECLARE_LOGGER();
    std::string m_name;
    std::vector<SharedBufferInfo> m_queue;
    bool m_bIsInited = false;
};

}   // namespace sample
}   // namespace ridehal

#endif   // #ifndef _RIDEHAL_SHARED_BUFFER_POOL_HPP_
