// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef QC_SAMPLE_SHARED_RING_MEMORY_HPP
#define QC_SAMPLE_SHARED_RING_MEMORY_HPP

#include "QC/sample/shared_ring/SharedRing.hpp"

namespace QC
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
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Create( std::string name, size_t size );

    /**
     * @brief Destroy the shared memory created
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Destroy();

    /**
     * @brief Open an existed shared memory
     * @param[in] name the shared memory name
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Open( std::string name );


    /**
     * @brief Close the opened shared memory
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Close();

    void *Data() { return m_ptr; }

    size_t Size() { return m_size; }

private:
    QC_DECLARE_LOGGER();

    int m_fd = -1;
    std::string m_name;
    void *m_ptr = nullptr;
    size_t m_size;
};

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_SHARED_RING_MEMORY_HPP
