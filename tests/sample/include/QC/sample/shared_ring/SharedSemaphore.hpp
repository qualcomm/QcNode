// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef QC_SAMPLE_SHARED_RING_SHARED_SEMAPHORE_HPP
#define QC_SAMPLE_SHARED_RING_SHARED_SEMAPHORE_HPP

#include "QC/sample/shared_ring/SharedRing.hpp"
#include <semaphore.h>

namespace QC
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
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Create( std::string name, int value );

    /**
     * @brief Destroy the shared semaphore created
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Destroy();

    /**
     * @brief Open an existed shared semaphore
     * @param[in] name the shared semaphore name
     * @param[in] value the initial value of the semaphore
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Open( std::string name );

    /**
     * @brief Close the opened shared semaphore
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Close();

    /**
     * @brief Wait the semaphore
     * @param[in] timeoutMs the timeout time in ms
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Wait( uint32_t timeoutMs = SEMAPHORE_WAIT_INFINITE );


    /**
     * @brief Release the semaphore
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Post();

    /**
     * @brief Reset the semaphore
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Reset();

private:
    QC_DECLARE_LOGGER();

    std::string m_name;
    sem_t *m_pSem = nullptr;
};

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_SHARED_RING_SHARED_SEMAPHORE_HPP
