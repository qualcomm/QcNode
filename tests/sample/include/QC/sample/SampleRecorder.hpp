// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef _QC_SAMPLE_RECORDER_HPP_
#define _QC_SAMPLE_RECORDER_HPP_

#include "QC/sample/SampleIF.hpp"
#include <stdio.h>

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleRecorder
///
/// SampleRecorder that to saving HEVC bit stream to a file
class SampleRecorder : public SampleIF
{
public:
    SampleRecorder();
    ~SampleRecorder();

    /// @brief Initialize the HEVC recorder
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the HEVC recorder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the HEVC recorder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the HEVC recorder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    std::string m_topicName;

    std::thread m_thread;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;

    uint32_t m_maxImages = 1000;
    FILE *m_meta = nullptr;
    FILE *m_file = nullptr;
};   // class SampleRecorder

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_RECORDER_HPP_
