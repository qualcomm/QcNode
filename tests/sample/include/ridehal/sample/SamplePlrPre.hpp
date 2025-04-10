// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_PLRPRE_HPP_
#define _RIDEHAL_SAMPLE_PLRPRE_HPP_

#include "ridehal/component/Voxelization.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SamplePlrPre
///
/// SamplePlrPre that to demonstate how to use the RideHal component Voxelization
class SamplePlrPre : public SampleIF
{
public:
    SamplePlrPre();
    ~SamplePlrPre();

    /// @brief Initialize the Voxelization
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the Voxelization
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the Voxelization
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the Voxelization
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    Voxelization_Config_t m_config = { RIDEHAL_PROCESSOR_HTP0, 0 };
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_coordsPool;
    SharedBufferPool m_featuresPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    Voxelization m_plrPre;
};   // class SamplePlrPre

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_PLRPRE_HPP_
