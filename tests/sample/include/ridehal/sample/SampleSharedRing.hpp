// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_SHARED_RING_EXAMPLE_HPP
#define RIDEHAL_SAMPLE_SHARED_RING_EXAMPLE_HPP

#include "ridehal/sample/SampleIF.hpp"
#include "ridehal/sample/shared_ring/SharedPublisher.hpp"
#include "ridehal/sample/shared_ring/SharedSubscriber.hpp"

using namespace ridehal::common;
using namespace ridehal::sample::shared_ring;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleSharedRing
///
/// SampleSharedRing that to demonstate how to do memory sharing between process
class SampleSharedRing : public SampleIF
{
public:
    SampleSharedRing();
    ~SampleSharedRing();

    /// @brief Initialize the SharedRing
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the SharedRing
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the SharedRing
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the SharedRing
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadPubMain();
    void ThreadSubMain();

private:
    std::string m_topicName;

    std::thread m_thread;
    bool m_stop;

    bool m_bIsPub = true;
    uint32_t m_queueDepth = 2;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    SharedPublisher m_sharedPub;
    SharedSubscriber m_sharedSub;
};   // class SampleSharedRing

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_SHARED_RING_EXAMPLE_HPP
