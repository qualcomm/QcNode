// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


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

    /**
     * @brief Retrieves the version identifier of the Node VideoDecoder.
     */
    const uint32_t GetVersion() const;

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    void ThreadProcMain();

    struct FrameInfo
    {
        uint64_t frameId;
        uint64_t timestamp;
    };

private:
    VideoDecoder m_decoder;
    DataTree m_config;
    DataTree m_dataTree;
    QCNodeInit_t m_nodeCfg;

    std::thread m_thread;
    std::thread m_threadProc;
    bool m_stop = false;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    std::string m_inputTopicName;
    std::string m_outputTopicName;
    std::string m_modelInOutInfoTopicName;

    SharedBufferPool m_imagePool;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_poolSize;
    QCImageFormat_e m_inFormat;  /**< uncompressed type */
    QCImageFormat_e m_outFormat; /**< compressed type */
    uint32_t m_numInputBufferReq;
    uint32_t m_numOutputBufferReq;

    std::mutex m_lock;
    std::map<uint64_t, DataFrame_t> m_camFrameMap;
    std::queue<FrameInfo> m_frameInfoQueue;
    std::queue<uint64_t> m_frameReleaseQueue;
    std::queue<VideoFrameDescriptor_t> m_frameOutQueue;
    std::condition_variable m_condVar;

    void OnDoneCb( const QCNodeEventInfo_t &eventInfo );

    void InFrameCallback( VideoFrameDescriptor &inFrame,
                          const QCNodeEventInfo_t &eventInfo );
    void OutFrameCallback( VideoFrameDescriptor &outFrame );

};   // class SampleVideoDecoder

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_NODE_VIDEO_DECODER_HPP_
