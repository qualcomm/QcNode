// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_C2D_HPP
#define QC_SAMPLE_C2D_HPP

#include "C2D.hpp"
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleC2D
///
/// SampleC2D that to demonstate how to use the QC component C2D
class SampleC2D : public SampleIF
{
public:
    SampleC2D();
    ~SampleC2D();

    /// @brief Initialize the C2D
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the C2D
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the C2D
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the C2D
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    C2D_Config_t m_config;
    QCImageFormat_e m_outputFormat;
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    uint32_t m_poolSize = 4;
    QCAllocationCache_e m_bufferCache = QC_CACHEABLE;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_imagePool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    C2D m_c2d;
};   // class SampleC2D

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_C2D_HPP
