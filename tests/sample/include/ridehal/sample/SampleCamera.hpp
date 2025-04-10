// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_CAMERA_HPP_
#define _RIDEHAL_SAMPLE_CAMERA_HPP_

#include "ridehal/component/Camera.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleCamera
///
/// SampleCamera that to demonstate how to use the RideHal component Camera
class SampleCamera : public SampleIF
{
public:
    SampleCamera();
    ~SampleCamera();

    /// @brief Initialize the camera
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the camera
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the camera
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the camera
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    void FrameCallBack( CameraFrame_t *pFrame );
    void EventCallBack( const uint32_t eventId, const void *pPayload );
    static void FrameCallBack( CameraFrame_t *pFrame, void *pPrivData );
    static void EventCallBack( const uint32_t eventId, const void *pPayload, void *pPrivData );

private:
    Camera m_camera;
    Camera_Config_t m_camConfig = { 0 };

    std::map<uint32_t, std::string> m_topicNameMap;

    std::map<uint32_t, std::shared_ptr<DataPublisher<DataFrames_t>>> m_pubMap;
    uint64_t m_frameId[MAX_CAMERA_STREAM] = { 0 };

    bool m_bImmediateRelease = false;

    /* if true, ignore any camera Init or Start error */
    bool m_bIgnoreError = false;
};   // class SampleCamera

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_CAMERA_HPP_
