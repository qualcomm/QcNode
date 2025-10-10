// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef QC_SAMPLE_SHARED_PUBLISHER_HPP
#define QC_SAMPLE_SHARED_PUBLISHER_HPP

#include "QC/sample/shared_ring/SharedMemory.hpp"
#include "QC/sample/shared_ring/SharedSemaphore.hpp"
#include "QC/sample/shared_ring/SpinLock.hpp"

#include "QC/Infras/Log/Logger.hpp"
#include "QC/sample/DataTypes.hpp"

#include <mutex>
#include <thread>

using namespace QC;

namespace QC
{
namespace sample
{
namespace shared_ring
{

class SharedPublisher
{
public:
    SharedPublisher();
    ~SharedPublisher();

    QCStatus_e Init( std::string name, std::string topicName );

    QCStatus_e Start();

    QCStatus_e Publish( DataFrames_t &data );

    QCStatus_e Stop();

    QCStatus_e Deinit();

    size_t GetNumberOfSubscribers();

private:
    void ThreadMain();
    void ReclaimResource( int32_t slotId );
    void ReleaseDesc( uint16_t idx );

private:
    QC_DECLARE_LOGGER();
    std::string m_shName;
    SharedMemory m_shmem;
    SharedRing_Memory_t *m_pRingMem = nullptr;
    SharedSemaphore m_semFree;
    SharedSemaphore m_semUsed[SHARED_RING_NUM_SUBSCRIBERS];

    std::thread m_thread;
    bool m_bStarted = false;
    std::mutex m_lock;

    std::map<int32_t, SharedRing_Ring_t *> m_subscribers;
    std::map<uint16_t, DataFrames_t> m_dataFrames;
};

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_SHARED_PUBLISHER_HPP
