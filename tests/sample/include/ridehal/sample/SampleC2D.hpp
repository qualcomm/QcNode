// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_C2D_HPP_
#define _RIDEHAL_SAMPLE_C2D_HPP_

#include "ridehal/component/C2D.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleC2D
///
/// SampleC2D that to demonstate how to use the RideHal component C2D
class SampleC2D : public SampleIF
{
public:
    SampleC2D();
    ~SampleC2D();

    /// @brief Initialize the C2D
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the C2D
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the C2D
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the C2D
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    C2D_Config_t m_config;
    RideHal_ImageFormat_e m_outputFormat;
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    uint32_t m_poolSize = 4;
    RideHal_BufferFlags_t m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_imagePool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    C2D m_c2d;
};   // class SampleC2D

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_C2D_HPP_
