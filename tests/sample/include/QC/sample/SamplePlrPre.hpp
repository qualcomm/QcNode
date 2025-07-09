// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef _QC_SAMPLE_PLRPRE_HPP_
#define _QC_SAMPLE_PLRPRE_HPP_

#include "QC/Node/Voxelization.hpp"
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{

using namespace QC::Node;

/// @brief qcnode::sample::SamplePlrPre
///
/// SamplePlrPre that to demonstate how to use the QCNodeVoxelization
class SamplePlrPre : public SampleIF
{
public:
    SamplePlrPre();
    ~SamplePlrPre();

    /// @brief Initialize the voxelization
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the voxelization
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the voxelization
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the voxelization
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    void ThreadMain();
    QCStatus_e ParseConfig( SampleConfig_t &config );

private:
    QC::Node::Voxelization m_voxel;
    DataTree m_config;
    DataTree m_dataTree;
    QCProcessorType_e m_processor;

    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_coordsPool;
    SharedBufferPool m_featuresPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

};   // class SamplePlrPre

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_NODE_PLRPRE_HPP_

