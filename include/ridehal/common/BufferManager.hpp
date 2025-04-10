// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_BUFFER_MANAGER_HPP
#define RIDEHAL_BUFFER_MANAGER_HPP

#include <map>
#include <mutex>

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"

namespace ridehal
{
namespace common
{

/**
 * @brief Buffer Manager
 *
 * Manage the allocated DMA buffer
 */
class BufferManager
{
public:
    BufferManager();
    ~BufferManager();
    /**
     * @brief Initialize the buffer manager
     * @param[in] pName the buffer manager unique instance name
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register the allocated shared buffer to the buffer manager
     * @param[in] pSharedBuffer the allocated shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Register( RideHal_SharedBuffer_t *pSharedBuffer );

    /**
     * @brief unregister the buffer from the buffer manager
     * @param[in] id the unique ID of the buffer assigned by the buffer manager
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deregister( uint64_t id );

    /**
     * @brief Get the shared buffer information
     * @param[in] id the unique ID of the buffer assigned by the buffer manager
     * @param[out] pSharedBuffer pointer to hold the shared buffer information
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetSharedBuffer( uint64_t id, RideHal_SharedBuffer_t *pSharedBuffer );

public:
    /**
     * @brief Get the default buffer manager
     * @return the buffer manager pointer, nullptr on failure
     */
    static BufferManager *GetDefaultBufferManager();

private:
    std::mutex m_lock;
    std::map<uint64_t, RideHal_SharedBuffer_t> m_bufferMap;
    uint64_t m_IDAllocator = 0;

private:
    RIDEHAL_DECLARE_LOGGER();
    static std::mutex s_lock;
    static BufferManager *s_pDefaultBufferManager;
};

}   // namespace common
}   // namespace ridehal

#endif   // RIDEHAL_BUFFER_MANAGER_HPP
