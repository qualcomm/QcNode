// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_CL2DFLEX_HPP
#define QC_SAMPLE_CL2DFLEX_HPP

#include "QC/component/CL2DFlex.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC::component;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleCL2DFlex
///
/// SampleCL2DFlex that to demonstate how to use the QC component CL2DFlex
class SampleCL2DFlex : public SampleIF
{
public:
    CL2DFlex_Work_Mode_e GetMode( SampleConfig_t &config, std::string key,
                                  CL2DFlex_Work_Mode_e defaultV );

    SampleCL2DFlex();
    ~SampleCL2DFlex();

    /// @brief Initialize the CL2DFlex
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the CL2DFlex
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the CL2DFlex
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the CL2DFlex
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    CL2DFlex_Config_t m_config;
    uint32_t m_poolSize = 4;
    QCBufferFlags_t m_bufferFlags = QC_BUFFER_FLAGS_CACHE_WB_WA;
    bool m_bNoPadding = false;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_imagePool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    CL2DFlex m_CL2DFlex;
    bool m_executeWithROIs = false;
    uint32_t m_roiNumber = 1;
    CL2DFlex_ROIConfig_t m_ROIs[QC_MAX_INPUTS];

    QCSharedBuffer_t m_mapXBuffer[QC_MAX_INPUTS];
    QCSharedBuffer_t m_mapYBuffer[QC_MAX_INPUTS];
};   // class SampleCL2DFlex

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_CL2DFLEX_HPP
