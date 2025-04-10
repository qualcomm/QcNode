// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_VIDEO_DECODER_HPP_
#define _RIDEHAL_SAMPLE_VIDEO_DECODER_HPP_

#include "ridehal/component/VideoDecoder.hpp"
#include "ridehal/sample/SampleIF.hpp"
#include <map>
#include <mutex>

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleVideoDecoder
///
/// SampleVideoDecoder that to demonstate how to use the RideHal component VideoDecoder
class SampleVideoDecoder : public SampleIF
{
public:
    SampleVideoDecoder();
    ~SampleVideoDecoder();

    /// @brief Initialize the VideoDecoder
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the VideoDecoder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the VideoDecoder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the VideoDecoder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
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
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_VIDEO_DECODER_HPP_


