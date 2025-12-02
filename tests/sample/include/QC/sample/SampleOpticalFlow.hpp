// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_NODE_OPTICALFLOW_HPP
#define QC_SAMPLE_NODE_OPTICALFLOW_HPP

#include "QC/Node/OpticalFlow.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC;
using namespace QC::Node;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleOpticalFlow
///
/// SampleOpticalFlow that to demonstate how to use the QC component OpticalFlow
class SampleOpticalFlow : public SampleIF
{
public:
    SampleOpticalFlow();
    ~SampleOpticalFlow();

    /// @brief Initialize the OpticalFlow
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the OpticalFlow
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the OpticalFlow
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the OpticalFlow
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

    /**
     * @brief Retrieves the version identifier of the Node OpticalFlow.
     */
    const uint32_t GetVersion() const;

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    DataTree m_config;
    DataTree m_dataTree;

    uint32_t m_poolSize = 4;

    uint32_t m_nStepSize;
    uint32_t m_width;
    uint32_t m_height;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_mvPool;
    SharedBufferPool m_mvConfPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    DataFrames_t m_LastFrames;

    void OnDoneCb( const QCNodeEventInfo_t &eventInfo );

    QC::Node::OpticalFlow m_of;
};   // class SampleOpticalFlow

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_NODE_OPTICALFLOW_HPP
