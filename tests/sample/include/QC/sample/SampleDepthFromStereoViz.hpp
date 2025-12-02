// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP
#define QC_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP

#include "OpenclIface.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC;
using namespace QC::libs::OpenclIface;

namespace QC
{
namespace sample
{

#define QC_DFS_VIZ_COLORS                                                                          \
    { { 0, 0, 0 }, { 0, 0, 1 }, { 1, 0, 0 }, { 1, 0, 1 },                                          \
      { 0, 1, 0 }, { 0, 1, 1 }, { 1, 1, 0 }, { 1, 1, 1 } };

#define QC_DFS_VIZ_RELATIVE_WEIGHTS                                                                \
    { 8.77193, 5.40541, 8.77193, 5.74713, 8.77193, 5.40541, 8.77193, 0.00000 };

#define QC_DFS_VIZ_CUMULATIVE_WEIGHTS                                                              \
    { 0.00000, 0.11400, 0.29900, 0.41300, 0.58700, 0.70100, 0.88600, 1.00000 };

/// @brief qcnode::sample::SampleDepthFromStereoViz
///
/// SampleDepthFromStereoViz that to demonstate how to decoding the output of the QC component
/// DepthFromStereo
class SampleDepthFromStereoViz : public SampleIF
{
public:
    SampleDepthFromStereoViz();
    ~SampleDepthFromStereoViz();

    /// @brief Initialize the DepthFromStereo Viz
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the DepthFromStereo Viz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the DepthFromStereo Viz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the DepthFromStereo Viz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

    QCStatus_e ConvertToRgbCPU( QCBufferDescriptorBase_t &disparity, QCBufferDescriptorBase_t &conf,
                                QCBufferDescriptorBase_t &RGB );
    QCStatus_e ConvertToRgbGPU( QCBufferDescriptorBase_t &disparity, QCBufferDescriptorBase_t &conf,
                                QCBufferDescriptorBase_t &RGB );

private:
    uint32_t m_width;
    uint32_t m_height;

    float m_colors[8][3] = QC_DFS_VIZ_COLORS;
    float m_relativeWeights[8] = QC_DFS_VIZ_RELATIVE_WEIGHTS;
    float m_cumulativeWeights[8] = QC_DFS_VIZ_CUMULATIVE_WEIGHTS;

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

    QCProcessorType_e m_processor;
    OpenclSrv m_openclSrvObj;
    cl_kernel m_kernel;
};   // class SampleDepthFromStereoViz

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_DEPTH_FROM_STEREO_VIZ_HPP
