// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_VIDEO_ENCODER_HPP_
#define _RIDEHAL_SAMPLE_VIDEO_ENCODER_HPP_

#include "ridehal/component/VideoEncoder.hpp"
#include "ridehal/sample/SampleIF.hpp"
#include <map>
#include <mutex>

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleVideoEncoder
///
/// SampleVideoEncoder that to demonstate how to use the RideHal component VideoEncoder
class SampleVideoEncoder : public SampleIF
{
public:
    SampleVideoEncoder();
    ~SampleVideoEncoder();

    /// @brief Initialize the VideoEncoder
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the VideoEncoder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the VideoEncoder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the VideoEncoder
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    void ThreadReleaseMain();

private:
    void InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame );
    void OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame );
    void EventCallback( const VideoEncoder_EventType_e eventId, const void *pPayload );

    static void InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame, void *pPrivData );
    static void OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame, void *pPrivData );
    static void EventCallback( const VideoEncoder_EventType_e eventId, const void *pPayload,
                               void *pPrivData );

private:
    struct FrameInfo
    {
        uint64_t frameId;
        uint64_t timestamp;
    };

private:
    VideoEncoder m_encoder;
    VideoEncoder_Config_t m_config;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    std::thread m_threadRelease;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    std::mutex m_lock;
    std::map<uint64_t, DataFrame_t> m_camFrameMap;
    std::queue<FrameInfo> m_frameInfoQueue;
    std::queue<uint64_t> m_frameReleaseQueue;
    std::condition_variable m_condVar;
};   // class SampleVideoEncoder

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_VIDEO_ENCODER_HPP_


