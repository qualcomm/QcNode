// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_TINYVIZ_HPP_
#define _RIDEHAL_SAMPLE_TINYVIZ_HPP_
#include "TinyViz.hpp"
#include "ridehal/sample/SampleIF.hpp"

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleTinyViz
///
/// SampleTinyViz that to demonstate how to use the RideHal component TinyViz
class SampleTinyViz : public SampleIF
{
public:
    SampleTinyViz();
    ~SampleTinyViz();

    /// @brief Initialize the tinyviz
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the tinyviz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the tinyviz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the tinyviz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void CamThreadMain( uint32_t idx );
    void ObjThreadMain( uint32_t idx );

private:
    uint32_t m_winW;
    uint32_t m_winH;

    std::vector<std::string> m_camNames;
    std::vector<std::string> m_camTopicNames;
    std::vector<std::string> m_objTopicNames;
    std::vector<uint32_t> m_batchIndexs;

    std::vector<std::thread *> m_threads;

    bool m_stop;

    std::vector<DataSubscriber<DataFrames_t>> m_camSubs;
    std::vector<DataSubscriber<Road2DObjects_t>> m_objSubs;

    TinyViz m_tinyViz;
};   // class SampleTinyViz

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_TINYVIZ_HPP_
