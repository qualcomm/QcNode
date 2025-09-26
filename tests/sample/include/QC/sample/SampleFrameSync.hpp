// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_SAMPLE_FRAME_SYNC_HPP
#define QC_SAMPLE_FRAME_SYNC_HPP
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleFrameSync
///
/// SampleFrameSync that to synchronize multiple inputs
class SampleFrameSync : public SampleIF
{
public:
    SampleFrameSync();
    ~SampleFrameSync();

    /// @brief Initialize the FrameSync
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the FrameSync
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the FrameSync
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the FrameSync
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void threadWindowMain();

private:
    typedef enum
    {
        FRAME_SYNC_MODE_WINDOW,
    } FrameSyncMode_e;

private:
    std::thread m_thread;
    bool m_stop;

    FrameSyncMode_e m_syncMode;

    uint32_t m_number;
    uint32_t m_windowMs;

    std::vector<std::string> m_inputTopicNames;
    std::string m_outputTopicName;

    std::vector<uint32_t> m_perms; /* the permutation */

    std::vector<DataSubscriber<DataFrames_t>> m_subs;
    DataPublisher<DataFrames_t> m_pub;
};   // class SampleFrameSync

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_FRAME_SYNC_HPP
