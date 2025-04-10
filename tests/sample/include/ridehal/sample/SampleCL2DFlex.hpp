// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_CL2DFLEX_HPP
#define RIDEHAL_SAMPLE_CL2DFLEX_HPP

#include "ridehal/component/CL2DFlex.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleCL2DFlex
///
/// SampleCL2DFlex that to demonstate how to use the RideHal component CL2DFlex
class SampleCL2DFlex : public SampleIF
{
public:
    CL2DFlex_Work_Mode_e GetMode( SampleConfig_t &config, std::string key,
                                  CL2DFlex_Work_Mode_e defaultV );

    SampleCL2DFlex();
    ~SampleCL2DFlex();

    /// @brief Initialize the CL2DFlex
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the CL2DFlex
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the CL2DFlex
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the CL2DFlex
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    CL2DFlex_Config_t m_config;
    uint32_t m_poolSize = 4;
    RideHal_BufferFlags_t m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;
    bool m_bNoPadding = false;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_imagePool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    CL2DFlex m_CL2DFlex;
    bool m_executeWithROIs = false;
    uint32_t m_roiNumber = 1;
    CL2DFlex_ROIConfig_t m_ROIs[RIDEHAL_MAX_INPUTS];

    RideHal_SharedBuffer_t m_mapXBuffer[RIDEHAL_MAX_INPUTS];
    RideHal_SharedBuffer_t m_mapYBuffer[RIDEHAL_MAX_INPUTS];
};   // class SampleCL2DFlex

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_CL2DFLEX_HPP
