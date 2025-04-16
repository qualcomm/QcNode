// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_PLRPRE_HPP_
#define _QC_SAMPLE_PLRPRE_HPP_

#include "QC/component/Voxelization.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC;
using namespace QC::component;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SamplePlrPre
///
/// SamplePlrPre that to demonstate how to use the QC component Voxelization
class SamplePlrPre : public SampleIF
{
public:
    SamplePlrPre();
    ~SamplePlrPre();

    /// @brief Initialize the Voxelization
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the Voxelization
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the Voxelization
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the Voxelization
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    Voxelization_Config_t m_config = { QC_PROCESSOR_HTP0, 0 };
    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_coordsPool;
    SharedBufferPool m_featuresPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    Voxelization m_plrPre;
};   // class SamplePlrPre

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_PLRPRE_HPP_
