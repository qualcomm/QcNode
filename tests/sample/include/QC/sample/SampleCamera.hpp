// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_CAMERA_HPP_
#define _QC_SAMPLE_CAMERA_HPP_

#include "QC/component/Camera.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC::common;
using namespace QC::component;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleCamera
///
/// SampleCamera that to demonstate how to use the QC component Camera
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
}   // namespace QC

#endif   // _QC_SAMPLE_CAMERA_HPP_
