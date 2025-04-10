// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_HPP
#define RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_HPP

#include "ridehal/component/DepthFromStereo.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleDepthFromStereo
///
/// SampleDepthFromStereo that to demonstate how to use the RideHal component DepthFromStereo
class SampleDepthFromStereo : public SampleIF
{
public:
    SampleDepthFromStereo();
    ~SampleDepthFromStereo();

    /// @brief Initialize the DepthFromStereo
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the DepthFromStereo
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the DepthFromStereo
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the DepthFromStereo
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    DepthFromStereo_Config_t m_config;
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    RideHal_BufferFlags_t m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;

    std::thread m_thread;
    SharedBufferPool m_dispPool;
    SharedBufferPool m_confPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    DepthFromStereo m_dfs;
};   // class SampleDepthFromStereo

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_HPP
