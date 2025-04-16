// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_VIDEO_DECODER_HPP_
#define _QC_SAMPLE_VIDEO_DECODER_HPP_

#include "QC/component/VideoDecoder.hpp"
#include "QC/sample/SampleIF.hpp"
#include <map>
#include <mutex>

using namespace QC;
using namespace QC::component;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleVideoDecoder
///
/// SampleVideoDecoder that to demonstate how to use the QC component VideoDecoder
class SampleVideoDecoder : public SampleIF
{
public:
    SampleVideoDecoder();
    ~SampleVideoDecoder();

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

private:
    void InFrameCallback( const VideoDecoder_InputFrame_t *pInputFrame );
    void OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame );
    void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload );

    static void InFrameCallback( const VideoDecoder_InputFrame_t *pInputFrame, void *pPrivData );
    static void OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame, void *pPrivData );
    static void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload,
                               void *pPrivData );

private:
    struct FrameInfo
    {
        uint64_t frameId;
        uint64_t timestamp;
    };

private:
    VideoDecoder m_decoder;
    VideoDecoder_Config_t m_config;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    std::mutex m_lock;
    std::map<uint64_t, DataFrame_t> m_inFrameMap;
    std::queue<FrameInfo> m_frameInfoQueue;
};   // class SampleVideoDecoder

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_VIDEO_DECODER_HPP_
