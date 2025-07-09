// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_DEPTH_FROM_STEREO_HPP
#define QC_SAMPLE_DEPTH_FROM_STEREO_HPP

#include "QC/Node/DepthFromStereo.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC;
using namespace QC::Node;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleDepthFromStereo
///
/// SampleDepthFromStereo that to demonstate how to use the QC component DepthFromStereo
class SampleDepthFromStereo : public SampleIF
{
public:
    SampleDepthFromStereo();
    ~SampleDepthFromStereo();

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
    DataTree m_config;
    DataTree m_dataTree;
    uint32_t m_poolSize = 4;

    uint32_t m_width;
    uint32_t m_height;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    QCBufferFlags_t m_bufferFlags = QC_BUFFER_FLAGS_CACHE_WB_WA;

    std::thread m_thread;
    SharedBufferPool m_dispPool;
    SharedBufferPool m_confPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    void OnDoneCb( const QCNodeEventInfo_t &eventInfo );

    QC::Node::DepthFromStereo m_dfs;
};   // class SampleDepthFromStereo

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_DEPTH_FROM_STEREO_HPP
