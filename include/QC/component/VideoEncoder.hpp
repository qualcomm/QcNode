// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_VIDEO_ENCODER_HPP
#define QC_VIDEO_ENCODER_HPP

#include "QC/component/ComponentIF.hpp"
#include "VidcCompBase.hpp"
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>

namespace QC
{
namespace component
{

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

/** @brief The VideoEncoder Init Config */
typedef struct
{
    uint32_t width;     /**< in pixels */
    uint32_t height;    /**< in pixels */
    uint32_t bitRate;   /**< bps */
    uint32_t gop;       /**< number of p frames in a period */
    uint32_t frameRate; /**< fps */
    uint32_t numInputBufferReq;
    uint32_t numOutputBufferReq;
    bool bInputDynamicMode;
    bool bOutputDynamicMode;
    bool bSyncFrameSeqHdr = false;                 /**< enable sync frame sequence header */
    QCSharedBuffer_t *pInputBufferList = nullptr;  /**< set input buffer in non-dynamic mode */
    QCSharedBuffer_t *pOutputBufferList = nullptr; /**< set output buffer in non-dynamic mode */
    VideoEncoder_RateControlMode_e rateControlMode;
    VideoEncoder_Profile_e profile;
    QCImageFormat_e inFormat;  /**< uncompressed type */
    QCImageFormat_e outFormat; /**< compressed type */
} VideoEncoder_Config_t;

/** @brief The VideoEncoder on-the-fly command */
typedef struct
{
    VideoEncoder_Prop_e propID;
    uint32_t value;
} VideoEncoder_OnTheFlyCmd_t;

/** @brief The VideoEncoder Input Frame */
typedef struct
{
    QCSharedBuffer_t sharedBuffer;
    uint64_t timestampNs; /**< frame data's timestamp. */
    uint64_t appMarkData; /**< frame data's mark data, this data will be copied to corresponding
                        output   compressed frame's VideoEncoder_OutputFrame_t. API won't touch this
                        data,   only copy it. */
    VideoEncoder_OnTheFlyCmd_t *pOnTheFlyCmd =
            nullptr; /**< use to send on-the-fly commands to encoder, like intra refresh, bps reset
                     and so on. */
    uint32_t numCmd = 0; /**< number of on-the-fly commands. */
} VideoEncoder_InputFrame_t;

/** @brief The VideoEncoder Output Frame */
typedef struct
{
    QCSharedBuffer_t sharedBuffer;
    uint64_t timestampNs;
    uint64_t appMarkData;
    uint32_t frameFlag; /**< indicate whether some error occurred during encoding this frame. */
    VideoEncoder_FrameType_e frameType; /**< indicate it's I, P, B, or IDR frame. */
} VideoEncoder_OutputFrame_t;

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

/** @brief callback for input frame done */
typedef void ( *VideoEncoder_InFrameCallback_t )( const VideoEncoder_InputFrame_t *pInputFrame,
                                                  void *pPrivData );
/** @brief callback for output frame done */
typedef void ( *VideoEncoder_OutFrameCallback_t )( const VideoEncoder_OutputFrame_t *pOutputFrame,
                                                   void *pPrivData );
/** @brief callback for event */
typedef void ( *VideoEncoder_EventCallback_t )( const VideoEncoder_EventType_e eventId,
                                                const void *pEvent, void *pPrivData );


/** @brief Top level control for interfacing with vidc based driver */
class VideoEncoder final : public VidcCompBase
{
public:
    /** @brief Default constructor */
    VideoEncoder() = default;
    /** @brief Default destructor */
    ~VideoEncoder() = default;

    /**
     * @brief Init the video encoder
     * @param pName the video encoder unique instance name
     * @param pConfig pointer to the video config information
     * @param level the log level used by the video encoder, default is error
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const VideoEncoder_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Start the video encoder
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief Stop the video encoder
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief deinitialize the video encoder
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

    /**
     * @brief submit a video frame to VIDC driver for encoding
     * @param pInput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitInputFrame( const VideoEncoder_InputFrame_t *pInput );

    /**
     * @brief submit a video frame back to VIDC driver
     * @param pOutput pointer to hold the video frame information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SubmitOutputFrame( const VideoEncoder_OutputFrame_t *pOutput );

    /**
     * @brief set config dynamically to VIDC driver
     * @param pCmd pointer to the video config information
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Configure( const VideoEncoder_OnTheFlyCmd_t *pCmd );

    /**
     * @brief register callback
     * @param inputDoneCb input frame callback function
     * @param outputDoneCb output frame callback function
     * @param eventCb event callback function
     * @param pAppPriv app private data
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterCallback( VideoEncoder_InFrameCallback_t inputDoneCb,
                                 VideoEncoder_OutFrameCallback_t outputDoneCb,
                                 VideoEncoder_EventCallback_t eventCb, void *pAppPriv );

private:
    QCStatus_e InitFromConfig( const VideoEncoder_Config_t *cfg );
    QCStatus_e ValidateConfig( const char *name, const VideoEncoder_Config_t *pConfig );
    QCStatus_e SetVidcProfileLevel( VideoEncoder_Profile_e profile );
    QCStatus_e InitDrvProperty();
    QCStatus_e GetInputInformation( void );
    QCStatus_e CheckBuffer( const QCSharedBuffer_t *pBuffer, VideoCodec_BufType_e bufferType );

    QCStatus_e FreeOutputBuffer( void );
    QCStatus_e FreeInputBuffer( void );
    void PrintEncoderConfig( void );

    VideoEncoder_InFrameCallback_t m_inputDoneCb = nullptr;
    VideoEncoder_OutFrameCallback_t m_outputDoneCb = nullptr;
    VideoEncoder_EventCallback_t m_eventCb = nullptr;
    void *m_pAppPriv = nullptr;

    uint32_t m_bitRate = 0;
    uint32_t m_gop;
    VideoEncoder_RateControlMode_e m_rateControl;
    VidcEncoderData_t m_vidcEncoderData;

    bool m_bEnableSyncFrameSeq = false;

    void InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame );
    void OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame );
    void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload );

    static void InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame, void *pPrivData );
    static void OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame, void *pPrivData );
    static void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload,
                               void *pPrivData );
};

}   // namespace component
}   // namespace QC

#endif   // QC_VIDEO_ENCODER_HPP
