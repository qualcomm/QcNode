// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_GL2DFLEX_HPP
#define RIDEHAL_SAMPLE_GL2DFLEX_HPP

#include "ridehal/component/GL2DFlex.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleGL2DFlex
///
/// SampleGL2DFlex that to demonstate how to use the RideHal component GL2DFlex
class SampleGL2DFlex : public SampleIF
{
public:
    SampleGL2DFlex();
    ~SampleGL2DFlex();

    /// @brief Initialize the GL2DFlex
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the GL2DFlex
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the GL2DFlex
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the GL2DFlex
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    GL2DFlex_Config_t m_config;
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

    GL2DFlex m_GL2DFlex;
};   // class SampleGL2DFlex

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_GL2DFLEX_HPP

