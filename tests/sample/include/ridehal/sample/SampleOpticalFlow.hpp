// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_OPTICALFLOW_HPP
#define RIDEHAL_SAMPLE_OPTICALFLOW_HPP

#include "ridehal/component/OpticalFlow.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleOpticalFlow
///
/// SampleOpticalFlow that to demonstate how to use the RideHal component OpticalFlow
class SampleOpticalFlow : public SampleIF
{
public:
    SampleOpticalFlow();
    ~SampleOpticalFlow();

    /// @brief Initialize the OpticalFlow
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the OpticalFlow
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the OpticalFlow
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the OpticalFlow
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    OpticalFlow_Config_t m_config;
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_mvPool;
    SharedBufferPool m_mvConfPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    DataFrames_t m_LastFrames;

    OpticalFlow m_ofl;
};   // class SampleOpticalFlow

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_OPTICALFLOW_HPP
