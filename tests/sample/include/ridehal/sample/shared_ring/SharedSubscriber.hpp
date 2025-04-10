// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_SHARED_SUBSCRIBER_HPP
#define RIDEHAL_SAMPLE_SHARED_SUBSCRIBER_HPP

#include "ridehal/sample/shared_ring/SharedMemory.hpp"
#include "ridehal/sample/shared_ring/SharedSemaphore.hpp"
#include "ridehal/sample/shared_ring/SpinLock.hpp"

#include "ridehal/sample/DataTypes.hpp"

#include <map>
#include <thread>
#include <vector>

namespace ridehal
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

    RideHalError_e Init( std::string name, std::string topicName, uint32_t queueDepth = 2 );

    RideHalError_e Start();

    RideHalError_e Receive( DataFrames_t &data, uint32_t timeoutMs = 1000 );

    RideHalError_e Stop();

    RideHalError_e Deinit();


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
    RIDEHAL_DECLARE_LOGGER();
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

    std::map<uint64_t, RideHal_SharedBuffer> m_buffers;
    std::map<uint16_t, DataFrmes> m_dataFrames;

    uint32_t m_queueDepth = 2;
};

}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal
#endif   // RIDEHAL_SAMPLE_SHARED_SUBSCRIBER_HPP
