// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_BUFFER_MANAGER_HPP
#define QC_BUFFER_MANAGER_HPP

#include <map>
#include <mutex>

#include "QC/common/Types.hpp"
#include "QC/infras/logger/Logger.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"

namespace QC
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
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register the allocated shared buffer to the buffer manager
     * @param[in] pSharedBuffer the allocated shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Register( QCSharedBuffer_t *pSharedBuffer );

    /**
     * @brief unregister the buffer from the buffer manager
     * @param[in] id the unique ID of the buffer assigned by the buffer manager
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deregister( uint64_t id );

    /**
     * @brief Get the shared buffer information
     * @param[in] id the unique ID of the buffer assigned by the buffer manager
     * @param[out] pSharedBuffer pointer to hold the shared buffer information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetSharedBuffer( uint64_t id, QCSharedBuffer_t *pSharedBuffer );

public:
    /**
     * @brief Get the default buffer manager
     * @return the buffer manager pointer, nullptr on failure
     */
    static BufferManager *GetDefaultBufferManager();

private:
    std::mutex m_lock;
    std::map<uint64_t, QCSharedBuffer_t> m_bufferMap;
    uint64_t m_IDAllocator = 0;

private:
    QC_DECLARE_LOGGER();
    static std::mutex s_lock;
    static BufferManager *s_pDefaultBufferManager;
};

}   // namespace common
}   // namespace QC

#endif   // QC_BUFFER_MANAGER_HPP
