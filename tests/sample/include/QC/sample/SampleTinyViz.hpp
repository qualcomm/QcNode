// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef _QC_SAMPLE_TINYVIZ_HPP_
#define _QC_SAMPLE_TINYVIZ_HPP_
#include "QC/sample/SampleIF.hpp"
#include "TinyViz.hpp"

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleTinyViz
///
/// SampleTinyViz that to demonstate how to use the QC component TinyViz
class SampleTinyViz : public SampleIF
{
public:
    SampleTinyViz();
    ~SampleTinyViz();

    /// @brief Initialize the tinyviz
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the tinyviz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the tinyviz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the tinyviz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void CamThreadMain( uint32_t idx );
    void ObjThreadMain( uint32_t idx );

private:
    uint32_t m_winW;
    uint32_t m_winH;

    std::vector<std::string> m_camNames;
    std::vector<std::string> m_camTopicNames;
    std::vector<std::string> m_objTopicNames;
    std::vector<uint32_t> m_batchIndexs;

    std::vector<std::thread *> m_threads;

    bool m_stop;

    std::vector<DataSubscriber<DataFrames_t>> m_camSubs;
    std::vector<DataSubscriber<Road2DObjects_t>> m_objSubs;

    TinyViz m_tinyViz;
};   // class SampleTinyViz

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_TINYVIZ_HPP_
