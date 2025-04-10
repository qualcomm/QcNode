// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_SHARED_PUBLISHER_HPP
#define RIDEHAL_SAMPLE_SHARED_PUBLISHER_HPP

#include "ridehal/sample/shared_ring/SharedMemory.hpp"
#include "ridehal/sample/shared_ring/SharedSemaphore.hpp"
#include "ridehal/sample/shared_ring/SpinLock.hpp"

#include "ridehal/common/Logger.hpp"
#include "ridehal/sample/DataTypes.hpp"

#include <mutex>
#include <thread>

using namespace ridehal::common;

namespace ridehal
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

    RideHalError_e Init( std::string name, std::string topicName );

    RideHalError_e Start();

    RideHalError_e Publish( DataFrames_t &data );

    RideHalError_e Stop();

    RideHalError_e Deinit();

    size_t GetNumberOfSubscribers();

private:
    void ThreadMain();
    void ReclaimResource( int32_t slotId );
    void ReleaseDesc( uint16_t idx );

private:
    RIDEHAL_DECLARE_LOGGER();
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
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_SHARED_PUBLISHER_HPP
