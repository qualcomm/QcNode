// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_VIDEO_DECODER_HPP
#define QC_NODE_VIDEO_DECODER_HPP

#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>

#include <vidc_ioctl.h>
#include <vidc_types.h>

#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <cstring>
#include <vidc_client.h>
#endif
#include "VidcDrvClient.hpp"
#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/VideoFrameDescriptor.hpp"
#include "QC/Node/NodeBase.hpp"
#include "VidcNodeBase.hpp"

namespace QC::Node
{

/** @brief This data type list the different VideoDecoder Callback Event Type */
typedef VideoCodec_EventType_e VideoDecoder_EventType_e;

/**
 * @brief The VideoDecoder Init Config
 * Buffer allocate has three modes:
 * dynamic mode: app allocate buffers, told component when SubmitInputFrame() & SubmitOutputFrame().
 * We call it mode 1).
 * none dynamic mode: different with dynamic mode, none dynamic mode will set buffer address to
 * driver before decode. For none dynamic mode, app can allocate buffers, we call it mode 2); if app
 * don't allocate, qcnode component will allocate, we call it mode 3). mode 2): app allocate
 * buffers, transfer buffer list by init() with bufferList ptr and number. In this mode, bufferList
 * ptr must not be nullptr, and app must make sure bufferList number <= bufferList.len(). mode 3):
 * app not allocate buffers, and bufferList ptr must be nullptr; and app will tell the buffer number
 * requested to component with Init(). When app call init(), qcnode will allocate buffers in
 * component.
 */
using VideoDecoder_Config_t = VidcNodeBase_Config_t;

class VideoDecoderConfigIfs : public VidcNodeBaseConfigIfs
{
public:
    /**
     * @brief VideoDecoderConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by VideoDecoderConfigIfs.
     * @param[in] vide A reference to the RideHal Video Decoder component to be used by VideoDecoderConfigIfs.
     * VideoDecoderConfigIfs.
     * @return None
     */
    VideoDecoderConfigIfs( Logger &logger )
        : VidcNodeBaseConfigIfs( logger )
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
    const virtual std::string& GetOptions( );

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    const virtual QCNodeConfigBase_t& Get( )
    {
        return m_config;
    }

private:
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ApplyDynamicConfig( DataTree &dt, std::string &errors );

    VideoDecoder_Config_t m_config;
    std::string m_options = "{}";
};

typedef struct VideoDecoderMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} VideoDecoderMonitorConfig_t;

class VideoDecoderMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    VideoDecoderMonitoringIfs( Logger &logger )
        : m_logger( logger )
    {}
    virtual ~VideoDecoderMonitoringIfs() {}

    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    const virtual std::string& GetOptions( )
    {
        return m_options;
    }

    const virtual QCNodeMonitoringBase_t& Get( )
    {
        return m_config;
    }
    ;

    virtual inline uint32_t GetMaximalSize() { return UINT32_MAX; }
    virtual inline uint32_t GetCurrentSize() { return UINT32_MAX; }

    virtual QCStatus_e Place( void *ptr, uint32_t &size ) { return QC_STATUS_UNSUPPORTED; }

private:
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
class VideoDecoder : public VidcNodeBase
{

public:
    /**
     * @brief VideoDecoder Constructor
     * @return None
     */
    VideoDecoder()
        : m_configIfs( m_logger ), m_monitorIfs( m_logger )
    {
        m_state = QC_OBJECT_STATE_INITIAL;
    }

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
    virtual QCObjectState_e GetState() { return m_state; }

    virtual QCStatus_e Start();
    virtual QCStatus_e Stop();

protected:
    /**
     * @brief submit a video frame to VIDC driver for encoding
     * @param pInput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitInputFrame( VideoFrameDescriptor_t &inFrameDesc );

    /**
     * @brief submit a video frame back to VIDC driver
     * @param pOutput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitOutputFrame( VideoFrameDescriptor_t &outFrameDesc );

    QCStatus_e ValidateConfig( );
    QCStatus_e HandleOutputReconfig();
    QCStatus_e FinishOutputReconfig();
    QCStatus_e InitDrvProperty();
    QCStatus_e CheckBuffer( const VideoFrameDescriptor_t &frameDesc,
                            VideoCodec_BufType_e bufferType );

    void InFrameCallback( VideoFrameDescriptor_t &inFrameDesc );
    void OutFrameCallback( VideoFrameDescriptor_t &outFrameDesc );
    void EventCallback( VideoDecoder_EventType_e eventId, const void *pEvent );

private:
    VideoDecoderConfigIfs m_configIfs;
    VideoDecoderMonitoringIfs m_monitorIfs;
    QCNodeEventCallBack_t m_callback;

    const VideoDecoder_Config_t *m_pConfig = nullptr;

    bool m_OutputStarted = false;
    bool m_OutputReconfigInprogress = false;
    std::mutex m_reconfigMutex;  // Mutex for reconfig state

    static void InFrameCallback( VideoFrameDescriptor_t &inFrameDesc, void *pPrivData );
    static void OutFrameCallback( VideoFrameDescriptor_t &outFrameDesc, void *pPrivData );
    static void EventCallback( VideoCodec_EventType_e eventId, const void *pEvent, void *pPrivData );
};

}   // namespace QC::Node

#endif   // QC_NODE_VIDEO_DECODER_HPP
