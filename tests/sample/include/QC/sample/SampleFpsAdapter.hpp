// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_SAMPLE_FPS_ADAPTER_HPP
#define QC_SAMPLE_FPS_ADAPTER_HPP

#include "QC/Node/Camera.hpp"
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

using namespace QC::Node;

/// @brief qcnode::sample::SampleFpsAdapter
///
/// SampleFpsAdapter that to frame drop according to the drop pattern configured
class SampleFpsAdapter : public SampleIF
{
public:
    SampleFpsAdapter();
    ~SampleFpsAdapter();

    /// @brief Initialize the FPS adapter
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the FPS adapter
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the FPS adapter
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the FPS adapter
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    bool m_stop;

    /**< number of bits in pattern (min value of 1. max value of 32). */
    uint32_t m_frameDropPeriod;

    /**< active bit pattern. value of 1 to keep frame, value of 0 to drop frame. */
    uint32_t m_frameDropPattern;

    uint32_t m_curPeriod = 0; /* current peroid to apply the pattern */

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;
};   // class SampleFpsAdapter

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_FPS_ADAPTER_HPP
