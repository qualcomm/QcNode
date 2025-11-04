// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_OPTICALFLOW_EVA_AUTO_HPP
#define QC_SAMPLE_OPTICALFLOW_EVA_AUTO_HPP

#include "OpticalFlow.hpp"
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleOpticalFlowEvaAuto
///
/// SampleOpticalFlowEvaAuto that to demonstate how to use the QC component OpticalFlow
class SampleOpticalFlowEvaAuto : public SampleIF
{
public:
    SampleOpticalFlowEvaAuto();
    ~SampleOpticalFlowEvaAuto();

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

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    OpticalFlow_Config_t m_config;
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_mvPool;
    SharedBufferPool m_mvConfPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    DataFrames_t m_LastFrames;

    OpticalFlow m_ofl;
};   // class SampleOpticalFlowEvaAuto

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_OPTICALFLOW_EVA_AUTO_HPP
