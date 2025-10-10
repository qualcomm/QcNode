// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_CAMERA_HPP
#define QC_SAMPLE_CAMERA_HPP

#include "QC/Node/Camera.hpp"
#include "QC/sample/SampleIF.hpp"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

namespace QC
{
namespace sample
{

using namespace QC::Node;

/// @brief qcnode::sample::SampleCamera
///
/// SampleCamera that to demonstate how to use the QCNodeCamera
class SampleCamera : public SampleIF
{
public:
    SampleCamera();
    ~SampleCamera();

    /// @brief Initialize the camera
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the camera
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the camera
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the camera
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ProcessFrame( CameraFrameDescriptor_t *pFrameDesc );
    void ThreadMain();

    void ProcessDoneCb( const QCNodeEventInfo_t &eventInfo );

private:
    QC::Node::Camera m_camera;
    DataTree m_config;
    DataTree m_dataTree;
    QCNodeInit_t m_nodeCfg;

    std::map<uint32_t, std::string> m_topicNameMap;
    std::map<uint32_t, std::shared_ptr<DataPublisher<DataFrames_t>>> m_pubMap;

    uint64_t m_frameId[MAX_CAMERA_STREAM] = { 0 };
    std::vector<DataTree> m_streamConfigs;

    bool m_bImmediateRelease = false;
    bool m_bIgnoreError = false;

    bool m_stop;
    std::mutex m_mutex;
    std::thread m_thread;
    std::condition_variable m_condVar;
    std::queue<CameraFrameDescriptor_t> m_camFrameQueue;

    std::vector<SharedBufferPool> m_frameBufferPools;

};   // class SampleCamera

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_CAMERA_HPP

