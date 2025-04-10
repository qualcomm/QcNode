// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_SHARED_RING_SHARED_SEMAPHORE_HPP
#define RIDEHAL_SAMPLE_SHARED_RING_SHARED_SEMAPHORE_HPP

#include "ridehal/sample/shared_ring/SharedRing.hpp"
#include <semaphore.h>

namespace ridehal
{
namespace sample
{
namespace shared_ring
{

#define SEMAPHORE_WAIT_INFINITE ( (uint32_t) 0xFFFFFFFF )

class SharedSemaphore
{
public:
    SharedSemaphore();
    ~SharedSemaphore();

    /**
     * @brief Create a shared semaphore
     * @param[in] name the shared semaphore name
     * @param[in] value the initial value of the semaphore
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Create( std::string name, int value );

    /**
     * @brief Destroy the shared semaphore created
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Destroy();

    /**
     * @brief Open an existed shared semaphore
     * @param[in] name the shared semaphore name
     * @param[in] value the initial value of the semaphore
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Open( std::string name );

    /**
     * @brief Close the opened shared semaphore
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Close();

    /**
     * @brief Wait the semaphore
     * @param[in] timeoutMs the timeout time in ms
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Wait( uint32_t timeoutMs = SEMAPHORE_WAIT_INFINITE );


    /**
     * @brief Release the semaphore
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Post();

    /**
     * @brief Reset the semaphore
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Reset();

private:
    RIDEHAL_DECLARE_LOGGER();

    std::string m_name;
    sem_t *m_pSem = nullptr;
};

}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_SHARED_RING_SHARED_SEMAPHORE_HPP
