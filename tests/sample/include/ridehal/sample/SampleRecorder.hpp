// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_RECORDER_HPP_
#define _RIDEHAL_SAMPLE_RECORDER_HPP_

#include "ridehal/sample/SampleIF.hpp"
#include <stdio.h>

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleRecorder
///
/// SampleRecorder that to saving HEVC bit stream to a file
class SampleRecorder : public SampleIF
{
public:
    SampleRecorder();
    ~SampleRecorder();

    /// @brief Initialize the HEVC recorder
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the HEVC recorder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the HEVC recorder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the HEVC recorder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    std::string m_topicName;

    std::thread m_thread;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;

    uint32_t m_maxImages = 1000;
    FILE *m_meta = nullptr;
    FILE *m_file = nullptr;
};   // class SampleRecorder

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_RECORDER_HPP_
