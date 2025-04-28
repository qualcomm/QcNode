// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_CL2DFLEX_HPP
#define QC_SAMPLE_CL2DFLEX_HPP

#include "QC/node/CL2DFlex.hpp"
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

using namespace QC::node;

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

    uint32_t m_numOfInputs;
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    QCImageFormat_e m_outputFormat;
    DataTree m_dataTree;
    QC::node::CL2DFlex m_cl2d;
    bool m_bEnableUndistortion = false;
    bool m_bExecuteWithROIs = false;
    uint32_t m_roiNumber = 1;
    CL2DFlex_ROIConfig_t m_ROIs[QC_MAX_INPUTS];

    QCSharedBufferDescriptor_t m_mapXBufferDesc[QC_MAX_INPUTS];
    QCSharedBufferDescriptor_t m_mapYBufferDesc[QC_MAX_INPUTS];
};   // class SampleCL2DFlex

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_CL2DFLEX_HPP
