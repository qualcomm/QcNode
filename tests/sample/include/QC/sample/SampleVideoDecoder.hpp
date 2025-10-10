// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef _QC_SAMPLE_NODE_VIDEO_DECODER_HPP_
#define _QC_SAMPLE_NODE_VIDEO_DECODER_HPP_

#include "QC/Node/VideoDecoder.hpp"
#include "QC/sample/SampleIF.hpp"
#include <map>
#include <mutex>

namespace QC
{
namespace sample
{

using namespace QC;
using namespace QC::Node;

/// @brief qcnode::sample::SampleVideoDecoder
///
/// SampleVideoDecoder that to demonstrate how to use the QC component VideoDecoder
class SampleVideoDecoder : public SampleIF
{
public:
    SampleVideoDecoder() = default;
    virtual ~SampleVideoDecoder() = default;

    /// @brief Initialize the VideoDecoder
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the VideoDecoder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the VideoDecoder
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the VideoDecoder
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
    QC::Node::VideoDecoder m_encoder;
    DataTree m_config;
    DataTree m_dataTree;

    std::thread m_thread;
    std::thread m_threadRelease;
    bool m_stop = false;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    std::string m_inputTopicName;
    std::string m_outputTopicName;
    std::string m_modelInOutInfoTopicName;

    std::mutex m_lock;
    std::map<uint64_t, DataFrame_t> m_camFrameMap;
    std::queue<FrameInfo> m_frameInfoQueue;
    std::queue<uint64_t> m_frameReleaseQueue;
    std::condition_variable m_condVar;

    void OnDoneCb( const QCNodeEventInfo_t &eventInfo );

    void InFrameCallback( QCSharedVideoFrameDescriptor_t &inFrame,
                          const QCNodeEventInfo_t &eventInfo );
    void OutFrameCallback( QCSharedVideoFrameDescriptor_t &outFrame,
                           const QCNodeEventInfo_t &eventInfo );

};   // class SampleVideoDecoder

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_NODE_VIDEO_DECODER_HPP_
