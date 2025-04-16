// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_REMAP_HPP_
#define _QC_SAMPLE_REMAP_HPP_

#include "QC/component/Remap.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC::common;
using namespace QC::component;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleRemap
///
/// SampleRemap that to demonstate how to use the QC component Remap
class SampleRemap : public SampleIF
{
public:
    SampleRemap();
    ~SampleRemap();

    /// @brief Initialize the remap
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the remap
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the remap
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the remap
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    Remap_Config_t m_config;
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_imagePool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    Remap m_remap;

    QCSharedBuffer_t m_mapXBuffer[QC_MAX_INPUTS];
    QCSharedBuffer_t m_mapYBuffer[QC_MAX_INPUTS];
};   // class SampleRemap

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_REMAP_HPP_
