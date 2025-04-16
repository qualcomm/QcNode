// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_GL2DFLEX_HPP
#define QC_SAMPLE_GL2DFLEX_HPP

#include "QC/component/GL2DFlex.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC::common;
using namespace QC::component;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleGL2DFlex
///
/// SampleGL2DFlex that to demonstate how to use the QC component GL2DFlex
class SampleGL2DFlex : public SampleIF
{
public:
    SampleGL2DFlex();
    ~SampleGL2DFlex();

    /// @brief Initialize the GL2DFlex
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the GL2DFlex
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the GL2DFlex
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the GL2DFlex
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    GL2DFlex_Config_t m_config;
    QCImageFormat_e m_outputFormat;
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    uint32_t m_poolSize = 4;
    QCBufferFlags_t m_bufferFlags = QC_BUFFER_FLAGS_CACHE_WB_WA;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_imagePool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    GL2DFlex m_GL2DFlex;
};   // class SampleGL2DFlex

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_GL2DFLEX_HPP
