// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_SAMPLE_NODE_TEMPORAL_HPP
#define QC_SAMPLE_NODE_TEMPORAL_HPP

#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleTemporal
///
/// SampleTemporal that to demonstate how to handle temporal kind of AI model
class SampleTemporal : public SampleIF
{
public:
    SampleTemporal();
    ~SampleTemporal();

    /// @brief Initialize the Temporal
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the Temporal
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the Temporal
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the Temporal
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    void ThreadMain();
    QCStatus_e ParseConfig( SampleConfig_t &config );

    QCStatus_e FillTensor( TensorDescriptor_t &tensorDesc, float scale, int32_t offset,
                           float value );

private:
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    std::shared_ptr<SharedBuffer_t> m_temporal = nullptr;
    std::shared_ptr<SharedBuffer_t> m_useFlag = nullptr;

    TensorProps_t m_temporalTsProps;
    TensorProps_t m_useFlagTsProps;

    float m_temporalQuantScale;
    int32_t m_temporalQuantOffset;

    uint32_t m_temporalIndex = 0;

    float m_useFlagQuantScale;
    int32_t m_useFlagQuantOffset;

    TensorDescriptor_t m_initTempTs;
    TensorDescriptor_t m_useFlagTs;

    uint32_t m_windowMs;

    bool m_bHasUseFlag = false; /* use temporal flag can be optional */

    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    BufferManager *m_pBufMgr = nullptr;
};   // class SampleTemporal

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_NODE_TEMPORAL_HPP
