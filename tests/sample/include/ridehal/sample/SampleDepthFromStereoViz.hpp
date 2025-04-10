// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP
#define RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP

#include "OpenclIface.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::OpenclIface;

namespace ridehal
{
namespace sample
{

#define RIDEHAL_DFS_VIZ_COLORS                                                                     \
    { { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 0, 1 },                                          \
      { 0, 1, 0 }, { 0, 1, 1 }, { 1, 1, 0 }, { 1, 1, 1 } };

#define RIDEHAL_DFS_VIZ_RELATIVE_WEIGHTS                                                           \
    { 8.77193, 5.40541, 8.77193, 5.74713, 8.77193, 5.40541, 8.77193, 0.00000 };

#define RIDEHAL_DFS_VIZ_CUMULATIVE_WEIGHTS                                                         \
    { 0.00000, 0.11400, 0.29900, 0.41300, 0.58700, 0.70100, 0.88600, 1.00000 };

/// @brief ridehal::sample::SampleDepthFromStereoViz
///
/// SampleDepthFromStereoViz that to demonstate how to decoding the output of the RideHal component
/// DepthFromStereo
class SampleDepthFromStereoViz : public SampleIF
{
public:
    SampleDepthFromStereoViz();
    ~SampleDepthFromStereoViz();

    /// @brief Initialize the DepthFromStereo Viz
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the DepthFromStereo Viz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the DepthFromStereo Viz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the DepthFromStereo Viz
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

    RideHalError_e ConvertToRgbCPU( RideHal_SharedBuffer_t *pDisparity,
                                    RideHal_SharedBuffer_t *pConf, RideHal_SharedBuffer_t *pRGB );
    RideHalError_e ConvertToRgbGPU( RideHal_SharedBuffer_t *pDisparity,
                                    RideHal_SharedBuffer_t *pConf, RideHal_SharedBuffer_t *pRGB );

private:
    uint32_t m_width;
    uint32_t m_height;

    float m_colors[8][3] = RIDEHAL_DFS_VIZ_COLORS;
    float m_relativeWeights[8] = RIDEHAL_DFS_VIZ_RELATIVE_WEIGHTS;
    float m_cumulativeWeights[8] = RIDEHAL_DFS_VIZ_CUMULATIVE_WEIGHTS;

    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_rgbPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    uint16_t m_disparityMax = 1008;

    uint8_t m_nConfMapThreshold = 0;
    uint8_t m_nTransparency = 0xFF;

    RideHal_ProcessorType_e m_processor;
    OpenclSrv m_openclSrvObj;
    cl_kernel m_kernel;
};   // class SampleDepthFromStereoViz

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP
