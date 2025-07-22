// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_NODE_VIDEO_ENCODER_HPP
#define QC_SAMPLE_NODE_VIDEO_ENCODER_HPP

#include "QC/Node/VideoEncoder.hpp"
#include "QC/sample/SampleIF.hpp"
#include <map>
#include <mutex>

namespace QC
{
namespace sample
{

using namespace QC::Node;

/// @brief qcnode::sample::SampleVideoEncoder
///
/// SampleVideoEncoder that to demonstrate how to use the QC component VideoEncoder
class SampleVideoEncoder : public SampleIF
{
public:
    SampleVideoEncoder() = default;
    virtual ~SampleVideoEncoder() = default;

    /// @brief Initialize the VideoEncoder
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the VideoEncoder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the VideoEncoder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the VideoEncoder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    void ThreadReleaseMain();

    struct FrameInfo
    {
        uint64_t frameId;
        uint64_t timestamp;
    };

private:
    QC::Node::VideoEncoder m_encoder;
    DataTree m_config;
    DataTree m_dataTree;
    QCNodeInit_t m_nodeCfg;

    std::thread m_thread;
    std::thread m_threadRelease;
    bool m_stop = false;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    SharedBufferPool m_frameBufferPools;

    std::mutex m_lock;
    std::map<uint64_t, DataFrame_t> m_camFrameMap;
    std::queue<FrameInfo> m_frameInfoQueue;
    std::queue<uint64_t> m_frameReleaseQueue;
    std::condition_variable m_condVar;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_poolSize;
    QCImageFormat_e m_inFormat;  /**< uncompressed type */
    QCImageFormat_e m_outFormat; /**< compressed type */
    uint32_t m_numInputBufferReq;
    uint32_t m_numOutputBufferReq;
    uint32_t m_bitRate;
    uint32_t m_frameRate;
    bool m_bSyncFrameSeqHdr;

    void OnDoneCb( const QCNodeEventInfo_t &eventInfo );

    void InFrameCallback( VideoFrameDescriptor &inFrame,
                          const QCNodeEventInfo_t &eventInfo );
    void OutFrameCallback( VideoFrameDescriptor &outFrame,
                           const QCNodeEventInfo_t &eventInfo );

};   // class SampleVideoEncoder

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_NODE_VIDEO_ENCODER_HPP
