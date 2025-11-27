// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_VIDEO_ENCODER_HPP
#define QC_NODE_VIDEO_ENCODER_HPP

#include <mutex>
#include <queue>
#include <string>
#include <sys/uio.h>
#include <unordered_map>

#include "VidcDrvClient.hpp"
#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/VideoFrameDescriptor.hpp"
#include "QC/Node/NodeBase.hpp"
#include "VidcNodeBase.hpp"

namespace QC::Node
{

/** @brief The QCNode VideoEncoder Version */
#define QCNODE_VIDEOENCODER_VERSION_MAJOR 2U
#define QCNODE_VIDEOENCODER_VERSION_MINOR 0U
#define QCNODE_VIDEOENCODER_VERSION_PATCH 0U

#define QCNODE_VIDEOENCODER_VERSION                                                                \
    ( ( QCNODE_VIDEOENCODER_VERSION_MAJOR << 16U ) | ( QCNODE_VIDEOENCODER_VERSION_MINOR << 8U ) | \
      QCNODE_VIDEOENCODER_VERSION_PATCH )

/** @brief This data type list the different rate control mode */
typedef enum
{
    VIDEO_ENCODER_RCM_CBR_CFR = VIDC_RATE_CONTROL_CBR_CFR,
    VIDEO_ENCODER_RCM_CBR_VFR = VIDC_RATE_CONTROL_CBR_VFR,
    VIDEO_ENCODER_RCM_VBR_CFR = VIDC_RATE_CONTROL_VBR_CFR,
    VIDEO_ENCODER_RCM_VBR_VFR = VIDC_RATE_CONTROL_VBR_VFR,
    VIDEO_ENCODER_RCM_UNUSED = VIDC_RATE_CONTROL_UNUSED
} VideoEncoder_RateControlMode_e;

/** @brief This data type list the different profile */
typedef enum
{
    VIDEO_ENCODER_PROFILE_H264_BASELINE = 0,
    VIDEO_ENCODER_PROFILE_H264_HIGH,
    VIDEO_ENCODER_PROFILE_H264_MAIN,
    VIDEO_ENCODER_PROFILE_HEVC_MAIN,
    VIDEO_ENCODER_PROFILE_HEVC_MAIN10,
    VIDEO_ENCODER_PROFILE_MAX
} VideoEncoder_Profile_e;

/** @brief This data type list the different frame types */
typedef enum
{
    VIDEO_ENCODER_FRAME_I = VIDC_FRAME_I,
    VIDEO_ENCODER_FRAME_P = VIDC_FRAME_P,
    VIDEO_ENCODER_FRAME_B = VIDC_FRAME_B,
    VIDEO_ENCODER_FRAME_IDR = VIDC_FRAME_IDR,
    VIDEO_ENCODER_FRAME_NOTCODED = VIDC_FRAME_NOTCODED,
    VIDEO_ENCODER_FRAME_YUV = VIDC_FRAME_YUV,
    VIDEO_ENCODER_FRAME_UNUSED = VIDC_FRAME_UNUSED
} VideoEncoder_FrameType_e;

/** @brief This data type list the different VideoEncoder Callback Event Type */
typedef VideoCodec_EventType_e VideoEncoder_EventType_e;

/** @brief This data type list the different VideoEncoder Property that can set dynamically */
typedef enum
{
    VIDEO_ENCODER_PROP_BITRATE = 0,
    VIDEO_ENCODER_PROP_FRAME_RATE,
    VIDEO_ENCODER_PROP_MAX
} VideoEncoder_Prop_e;

/** @brief Store the data interacted with video core */
typedef struct
{
    vidc_profile_type profile;      /**< The codec profile payload */
    vidc_level_type level;          /**< The codec level payload */
    vidc_frame_rate_type frameRate; /**< Frame rate for Encoder */
    vidc_iperiod_type iPeriod;      /**< The data type to set I frame period pattern for encoder */
    vidc_idr_period_type idrPeriod; /**< The IDR frame periodicity within Intra coded frames */
    vidc_target_bitrate_type bitrate;    /**< The encoder target bitrate */
    vidc_enable_type enableSyncFrameSeq; /**< Enable sync frame sequence header */
    vidc_plane_def_type planeDefY;       /**< Specifies layout of raw data for planeY */
    vidc_plane_def_type planeDefUV;      /**< Specifies layout of raw data for planeUV */
} VidcEncoderData_t;

/**
 * @brief Video Encoder Node Configuration Data Structure
 * @param params The QC component Video Encoder configuration data structure.
 */
typedef struct VideoEncoder_Config : public VidcNodeBase_Config_t
{
    uint32_t bitRate;   /**< bps */
    uint32_t gop;       /**< number of p frames in a period */
    bool bSyncFrameSeqHdr = false;                 /**< enable sync frame sequence header */
    VideoEncoder_RateControlMode_e rateControlMode;
    VideoEncoder_Profile_e profile;
} VideoEncoder_Config_t;

class VideoEncoderConfigIfs : public VidcNodeBaseConfigIfs
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
    VideoEncoderConfigIfs( Logger &logger )
        : VidcNodeBaseConfigIfs( logger )
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
    const virtual std::string& GetOptions( )
    {
        return m_options;
    }

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

    VideoEncoder_Config_t m_config;
    std::string m_options = "{}";
};

typedef struct VideoEncoderMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} VideoEncoderMonitorConfig_t;

class VideoEncoderMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    VideoEncoderMonitoringIfs( Logger &logger ) : m_logger( logger ) {}
    virtual ~VideoEncoderMonitoringIfs() {}

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

    virtual inline uint32_t GetMaximalSize( ) { return UINT32_MAX; }
    virtual inline uint32_t GetCurrentSize( ) { return UINT32_MAX; }

    virtual QCStatus_e Place( void *ptr, uint32_t &size )
    {
        return QC_STATUS_UNSUPPORTED;
    }

private:
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
class VideoEncoder : public VidcNodeBase
{

public:
    /**
     * @brief VideoEncoder Constructor
     * @return None
     */
    VideoEncoder() : m_configIfs( m_logger ), m_monitorIfs( m_logger )
    {
        m_state = QC_OBJECT_STATE_INITIAL;
    }

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
    virtual QCObjectState_e GetState() { return m_state; }

    virtual QCStatus_e Start();
    virtual QCStatus_e Stop();

protected:
    /**
     * @brief submit a video frame to VIDC driver for encoding
     * @param pInput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitInputFrame( VideoFrameDescriptor &inFrameDesc );

    /**
     * @brief submit a video frame back to VIDC driver
     * @param pOutput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitOutputFrame( VideoFrameDescriptor &outFrameDesc );

    QCStatus_e ValidateConfig( );
    QCStatus_e SetVidcProfileLevel( VideoEncoder_Profile_e profile );
    QCStatus_e InitDrvProperty( );
    QCStatus_e GetInputInformation( );
//    QCStatus_e Configure( const VideoEncoder_OnTheFlyCmd_t *pCmd );

    void PrintEncoderConfig();

    QCStatus_e CheckBuffer( const VideoFrameDescriptor &vidFrmDesc, VideoCodec_BufType_e bufferType );
    void InFrameCallback( VideoFrameDescriptor &inFrameDesc );
    void OutFrameCallback( VideoFrameDescriptor &outFrameDesc );
    void EventCallback( VideoEncoder_EventType_e eventId, const void *pEvent );

private:
    VideoEncoderConfigIfs m_configIfs;
    VideoEncoderMonitoringIfs m_monitorIfs;
    QCNodeEventCallBack_t m_callback;

    const VideoEncoder_Config_t *m_pConfig = nullptr;

    VidcEncoderData_t m_vidcEncoderData;

    static void InFrameCallback( VideoFrameDescriptor &inFrameDesc, void *pPrivData );
    static void OutFrameCallback( VideoFrameDescriptor &outFrameDesc, void *pPrivData );
    static void EventCallback( VideoCodec_EventType_e eventId, const void *pPayload,
                               void *pPrivData );
};

} // namespace QC::Node

#endif   // QC_NODE_VIDEO_ENCODER_HPP
