// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_SAMPLE_SHARED_RING_EXAMPLE_HPP
#define QC_SAMPLE_SHARED_RING_EXAMPLE_HPP

#include "QC/sample/SampleIF.hpp"
#include "QC/sample/shared_ring/SharedPublisher.hpp"
#include "QC/sample/shared_ring/SharedSubscriber.hpp"

using namespace QC;
using namespace QC::sample::shared_ring;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleSharedRing
///
/// SampleSharedRing that to demonstate how to do memory sharing between process
class SampleSharedRing : public SampleIF
{
public:
    SampleSharedRing();
    ~SampleSharedRing();

    /// @brief Initialize the SharedRing
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the SharedRing
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the SharedRing
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the SharedRing
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadPubMain();
    void ThreadSubMain();

private:
    std::string m_topicName;

    std::thread m_thread;
    bool m_stop;

    bool m_bIsPub = true;
    uint32_t m_queueDepth = 2;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    SharedPublisher m_sharedPub;
    SharedSubscriber m_sharedSub;
};   // class SampleSharedRing

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_SHARED_RING_EXAMPLE_HPP
