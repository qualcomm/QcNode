// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_SHARED_SUBSCRIBER_HPP
#define QC_SAMPLE_SHARED_SUBSCRIBER_HPP

#include "QC/sample/SharedBufferPool.hpp"
#include "QC/sample/shared_ring/SharedMemory.hpp"
#include "QC/sample/shared_ring/SharedSemaphore.hpp"
#include "QC/sample/shared_ring/SpinLock.hpp"

#include "QC/sample/DataTypes.hpp"

#include <map>
#include <thread>
#include <vector>

namespace QC
{
namespace sample
{
namespace shared_ring
{

class SharedSubscriber
{
public:
    SharedSubscriber();
    ~SharedSubscriber();

    QCStatus_e Init( std::string name, std::string topicName, uint32_t queueDepth = 2 );

    QCStatus_e Start();

    QCStatus_e Receive( DataFrames_t &data, uint32_t timeoutMs = 1000 );

    QCStatus_e Stop();

    QCStatus_e Deinit();


private:
    void ThreadMain();
    void ReleaseDesc( uint16_t idx );
    void ReleaseSharedBuffer( SharedBuffer_t *pSharedBuffer );

private:
    struct DataFrmes
    {
        int32_t ref;
        std::vector<SharedBuffer_t> bufs;
    };

private:
    QC_DECLARE_LOGGER();
    std::string m_name;
    std::string m_shName;
    SharedMemory m_shmem;
    SharedRing_Memory_t *m_pRingMem = nullptr;
    SharedRing_Ring_t *m_pUsedRing = nullptr;
    int32_t m_slotId = -1;
    SharedSemaphore m_semFree;
    SharedSemaphore m_semUsed;

    std::thread m_thread;
    bool m_bStarted = false;

    std::mutex m_lock;

    std::map<uint64_t, SharedBuffer_t> m_buffers;
    std::map<uint16_t, DataFrmes> m_dataFrames;

    uint32_t m_queueDepth = 2;
};

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC
#endif   // QC_SAMPLE_SHARED_SUBSCRIBER_HPP
