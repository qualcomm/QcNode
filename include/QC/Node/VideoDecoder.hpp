// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_VIDEO_DECODER_HPP
#define QC_NODE_VIDEO_DECODER_HPP

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

typedef struct VideoDecoderConfig : public QCNodeConfigBase_t
{
    VideoDecoder_Config_t params;
} VideoDecoder_Config_t;

class VideoDecoderConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief VideoDecoderConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by VideoDecoderConfigIfs.
     * @param[in] vide A reference to the RideHal Video Decoder component to be used by VideoDecoderConfigIfs.
     * VideoDecoderConfigIfs.
     * @return None
     */
    VideoDecoderConfigIfs( Logger &logger, VideoDecoder &vide )
        : NodeConfigIfs( logger ),
          m_vide( vide )
    {}

    /**
     * @brief VideoDecoderConfigIfs Destructor
     * @return None
     */
    virtual ~VideoDecoderConfigIfs() {}

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
     *        "frameRate": "The encoder frames per second, type: uint32_t, default: 0",
     *        "inputDynamicMode": "If true encoder allocates the buffer for input frames,
     *                             otherwise caller should provide the buffers for input,
     *                             type: bool, default: true",
     *        "outputDynamicMode": "If true encoder allocates the buffer for output frames,
     *                             otherwise caller should provide the buffers for output,
     *                             type: bool, default: true",
     *        "numInputBufferReq": "The number of input buffers, type: uint32_t, default: 0",
     *        "numOutputBufferReq": "The number of output buffers, type: uint32_t, default: 0",
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


    VideoDecoder &m_vide;
    VideoDecoder_Config_t m_config;
    std::string m_options;
};

typedef struct VideoDecoderMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} VideoDecoderMonitorConfig_t;

class VideoDecoderMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    VideoDecoderMonitoringIfs( Logger &logger, VideoDecoder &vide )
        : m_logger( logger ),
          m_vide( vide )
    {}
    virtual ~VideoDecoderMonitoringIfs() {}

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
    VideoDecoder &m_vide;
    Logger &m_logger;
    std::string m_options;
    VideoDecoderMonitorConfig_t m_config;
};

typedef enum
{
    QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID = 0,
    QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID = 1,
    QC_NODE_VIDEO_DECODER_EVENT_BUFF_ID = 2
} VideoDecoderBufferId_e;

/** @brief Top level control for interfacing with vidc based driver */
class VideoDecoder : public NodeBase
{

public:
    /**
     * @brief VideoDecoder Constructor
     * @return None
     */
    VideoDecoder() : m_configIfs( m_logger, m_vide ), m_monitorIfs( m_logger, m_vide ) {}

    /**
     * @brief VideoDecoder Destructor
     * @return None
     */
    virtual ~VideoDecoder() {}

    /**
     * @brief Initialize Node Video Decoder
     * @param[in] config The Node Video Decoder configuration
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node Video Decoder configuration interface.
     * @return A reference to the Node Video Decoder configuration interface.
     */
    virtual inline QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node Video Decoder monitoring interface.
     * @return A reference to the Node Video Decoder monitoring interface.
     */
    virtual inline QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node Video Decoder
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Stop the Node Video Decoder
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node Video Decoder
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
     * o) QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID for input
     * o) QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID for output
     * o) QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID for status and error events
     * The callback of this function use GetBuffer() passing the required buffer ID
     * to specify the required channel, so to get the input and output buffers by the callback
     * the following pattern can be used:
     *     auto &bufIn = GetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID )
     *     auto &bufOut = GetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID );
     * @note This API is not thread-safe. Avoid calling the ProcessFrameDescriptor API
     * on the same instance from multiple threads simultaneously.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Get the current state of the Node VideoDecoder
     * @return The current state of the Node VideoDecoder
     */
    virtual QCObjectState_e GetState();

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

    void InFrameCallback( const VideoDecoder_InputFrame_t *pInputFrame );
    void OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame );
    void EventCallback( const VideoDecoder_EventType_e eventId, const void *pEvent );

private:
    QC::component::VideoDecoder m_vide;
    VideoDecoderConfigIfs m_configIfs;
    VideoDecoderMonitoringIfs m_monitorIfs;
    QCNodeEventCallBack_t m_callback;

    static void NVD_InFrameCallback( const VideoDecoder_InputFrame_t *pInputFrame,
                                     void *pPrivData );

    static void NVD_OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame,
                                      void *pPrivData );

    static void NVD_EventCallback( const VideoDecoder_EventType_e eventId,
                                   const void *pEvent, void *pPrivData );
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_VIDEO_DECODER_HPP
