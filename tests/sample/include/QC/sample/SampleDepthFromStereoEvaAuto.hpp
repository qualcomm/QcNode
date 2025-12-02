// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_SAMPLE_DEPTH_FROM_STEREO_EVA_AUTO_HPP
#define QC_SAMPLE_DEPTH_FROM_STEREO_EVA_AUTO_HPP

#include "QC/sample/SampleIF.hpp"

#include "DepthFromStereo.hpp"

namespace QC
{
namespace sample
{
/// @brief qcnode::sample::SampleDepthFromStereoEvaAuto
///
/// SampleDepthFromStereoEvaAuto that to demonstate how to use the QC component DepthFromStereo
class SampleDepthFromStereoEvaAuto : public SampleIF
{
public:
    SampleDepthFromStereoEvaAuto();

    ~SampleDepthFromStereoEvaAuto();

    /// @brief Initialize the DepthFromStereo
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the DepthFromStereo
    /// @return QC_STATUS_OK on success, others on failure

    QCStatus_e Start();
    /// @brief Stop the DepthFromStereo
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the DepthFromStereo
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    DepthFromStereo_Config_t m_config;
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    QCAllocationCache_e m_bufferCache = QC_CACHEABLE;

    std::thread m_thread;
    SharedBufferPool m_dispPool;
    SharedBufferPool m_confPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    DepthFromStereo m_dfs;
};   // class SampleDepthFromStereoEvaAuto
}   // namespace sample
}   // namespace QC
#endif   // QC_SAMPLE_DEPTH_FROM_STEREO_EVA_AUTO_HPP