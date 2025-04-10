// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_SHARED_RING_MEMORY_HPP
#define RIDEHAL_SAMPLE_SHARED_RING_MEMORY_HPP

#include "ridehal/sample/shared_ring/SharedRing.hpp"

namespace ridehal
{
namespace sample
{
namespace shared_ring
{

class SharedMemory
{
public:
    SharedMemory();
    ~SharedMemory();

    /**
     * @brief Create a shared memory
     * @param[in] name the shared memory name
     * @param[in] size the wanted shared memory size
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Create( std::string name, size_t size );

    /**
     * @brief Destroy the shared memory created
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Destroy();

    /**
     * @brief Open an existed shared memory
     * @param[in] name the shared memory name
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Open( std::string name );


    /**
     * @brief Close the opened shared memory
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Close();

    void *Data() { return m_ptr; }

    size_t Size() { return m_size; }

private:
    RIDEHAL_DECLARE_LOGGER();

    int m_fd = -1;
    std::string m_name;
    void *m_ptr = nullptr;
    size_t m_size;
};

}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_SHARED_RING_MEMORY_HPP
