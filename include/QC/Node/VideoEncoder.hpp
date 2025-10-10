// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_NODE_VIDEO_ENCODER_HPP
#define QC_NODE_VIDEO_ENCODER_HPP

#include "QC/component/VideoDecoder.hpp"
#include "QC/Node/NodeBase.hpp"
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>

namespace QC
{
namespace Node
{
using namespace QC::component;

/**
 * @brief Video Encoder Node Configuration Data Structure
 * @param params The QC component Video Encoder configuration data structure.
 */
typedef struct VideoEncoderConfig : public QCNodeConfigBase_t
{
    VideoEncoder_Config_t params;
} VideoEncoder_Config_t;

class VideoEncoderConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief VideoEncoderConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by VideoEncoderConfigIfs.
     * @param[in] vide A reference to the RideHal Video Encoder component to be used by VideoEncoderConfigIfs.
     * @param[in] vide A reference to the QC Video Encoder component to be used by
     * VideoEncoderConfigIfs.
     * @return None
     */
    VideoEncoderConfigIfs( Logger &logger, VideoEncoder &vide )
        : NodeConfigIfs( logger ),
          m_Vide( vide )
    {}

    /**
     * @brief VideoEncoderConfigIfs Destructor
     * @return None
     */
    virtual ~VideoEncoderConfigIfs() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * @param[in] config The configuration string.
     * @param[out] errors The error string returned if there is an error.
     * @note The config is a JSON string according to the templates below.
     * 1. Static configuration used for initialization:
     *   {
     *     "static": {
     *        "name": "The Node unique name, type: string, default: 'unnamed'",
     *        "id": "The Node unique ID, type: uint32_t, default: 0",
     *        "width": "The width in pixels of the I/O frame, type: uint32_t, default: 0",
     *        "height": "The height in pixels of the I/O frame, type: uint32_t, default: 0",
     *        "bitRate": "The encoding bits per second, type: uint32_t, default: 0",
     *        "frameRate": "The encoder frames per second, type: uint32_t, default: 0",
     *        "gop": "The number of p frames in a period, type: uint32_t, default: 0"
     *        "inputDynamicMode": "If true encoder allocates the buffer for input frames,
     *                             otherwise caller should provide the buffers for input,
     *                             type: bool, default: true",
     *        "outputDynamicMode": "If true encoder allocates the buffer for output frames,
     *                             otherwise caller should provide the buffers for output,
     *                             type: bool, default: true",
     *        "numInputBufferReq": "The number of input buffers, type: uint32_t, default: 0",
     *        "numOutputBufferReq": "The number of output buffers, type: uint32_t, default: 0",
     *        "rateControlMode": "The algorithm used for rate control, type: string,
     *                           options: [CBR_CFR, CBR_VFR, VBR_CFR, UNUSED],
     *                           default: NUSED",
     *        "profile": "The video encoder profile, type: string,
     *                   options: [H264_BASELINE, H264_HIGH, H264_MAIN, HEVC_MAIN, HEVC_MAIN10],
     *                   default: MAIN10",
     *        "inFormat": "input compress mode, now support h264 and h265, type: uint32_t,
     *                     default: h265",
     *        "outFormat": "output format support NV12, NV12_UBWC, P010, type: uint32_t,
     *                      default: nv12"
     *     }
     *   }
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get Configuration Options
     * @return A reference string to the JSON configuration options.
     * @note
     * TODO: Provide a more detailed introduction about the JSON configuration options.
     */
    virtual const std::string &GetOptions();

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    virtual const QCNodeConfigBase_t &Get() { return m_config; }

private:
    QCStatus_e VerifyStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ApplyDynamicConfig( DataTree &dt, std::string &errors );

    VideoEncoder &m_Vide;
    VideoEncoder_Config_t m_config;
    std::string m_options;
};

typedef struct VideoEncoderMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} VideoEncoderMonitorConfig_t;

class VideoEncoderMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    VideoEncoderMonitoringIfs( Logger &logger, VideoEncoder &vide )
        : m_logger( logger ),
          m_Vide( vide )
    {}
    virtual ~VideoEncoderMonitoringIfs() {}

    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    virtual const std::string &GetOptions() { return m_options; }

    virtual const QCNodeMonitoringBase_t &Get() { return m_config; };

    virtual inline uint32_t GetMaximalSize() { return UINT32_MAX; }
    virtual inline uint32_t GetCurrentSize() { return UINT32_MAX; }

    virtual QCStatus_e Place( void *ptr, uint32_t &size ) { return QC_STATUS_UNSUPPORTED; }

private:
    VideoEncoder &m_Vide;
    Logger &m_logger;
    std::string m_options;
    VideoEncoderMonitorConfig_t m_config;
};

typedef enum
{
    QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID = 0,
    QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID = 1,
    QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID = 2
} VideoEncoderBufferId_e;

/** @brief Top level control for interfacing with vidc based driver */
class VideoEncoder : public NodeBase
{
public:
    /**
     * @brief VideoEncoder Constructor
     * @return None
     */
    VideoEncoder() : m_configIfs( m_logger, m_vide ), m_monitorIfs( m_logger, m_vide ) {}

    /**
     * @brief VideoEncoder Destructor
     * @return None
     */
    virtual ~VideoEncoder() {}

    /**
     * @brief Initialize Node Video Encoder
     * @param[in] config The Node Video Encoder configuration
     * @return QC_STATUS_OK on success, others on failure
     * @note QCNodeInit::config - Refer to the comments of the API VideoEncoderConfigIfs::VerifyAndSet.
     * @note QCNodeInit::callback - The user application callback to notify the status of
     * the API ProcessFrameDescriptor. This is mandatory for video. The ProcessFrameDescriptor
     * is asynchronous.
     * @brief Initializes Node VideoEncoder.
     * @param[in] config The Node VideoEncoder configuration.
     * @note QCNodeInit::config - Refer to the comments of the API VideoEncoderConfigIfs::VerifyAndSet.
     * @note QCNodeInit::buffers - Two variants:
     *   if @inputDynamicMode is false, input buffers are provided by the user application.
     *   if @inputDynamicMode is true, input buffers are allocated by the framework itself.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node Video Encoder configuration interface.
     * @return A reference to the Node Video Encoder configuration interface.
     */
    virtual inline QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node Video Encoder monitoring interface.
     * @return A reference to the Node Video Encoder monitoring interface.
     */
    virtual inline QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node Video Encoder
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Stop the Node Video Encoder
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node Video Encoder
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note ProcessFrameDescriptor call will return immediately after queuing the job into the
     * backend video encoder engine. Once the inference job is completed, the callback will be
     * invoked, making this an asynchronous API.
     * @note There are three channels (buffers IDs):
     * o) QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID for input
     * o) QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID for output
     * o) QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID for status and error events
     * The callback of this function use GetBuffer() passing the required buffer ID
     * to specify the required channel, so to get the input and output buffers by the callback
     * the following pattern can be used:
     *     auto &bufIn = GetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID )
     *     auto &bufOut = GetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID );
     * @note This API is not thread-safe. Avoid calling the ProcessFrameDescriptor API
     * on the same instance from multiple threads simultaneously.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Get the current state of the Node VideoEncoder
     * @return The current state of the Node VideoEncoder
     */
    virtual QCObjectState_e GetState() { return m_vide.GetState(); }

protected:
    /**
     * @brief submit a video frame to VIDC driver for encoding
     * @param pInput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitInputFrame( const QCSharedVideoFrameDescriptor *pInput );

    /**
     * @brief submit a video frame back to VIDC driver
     * @param pOutput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitOutputFrame( const QCSharedVideoFrameDescriptor *pOutput );

    void InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame );
    void OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame );
    void EventCallback( const VideoEncoder_EventType_e eventId, const void *pEvent );

private:
    QC::component::VideoEncoder m_vide;
    VideoEncoderConfigIfs m_configIfs;
    VideoEncoderMonitoringIfs m_monitorIfs;
    QCNodeEventCallBack_t m_callback;

    static void NVE_InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame,
                                     void *pPrivData );

    static void NVE_OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame,
                                      void *pPrivData );

    static void NVE_EventCallback( const VideoEncoder_EventType_e eventId, const void *pEvent,
                                   void *pPrivData );
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_VIDEO_ENCODER_HPP
