// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_VIDC_DRV_CLIENT_HPP
#define QC_VIDC_DRV_CLIENT_HPP

#include <vidc_ioctl.h>
#include <vidc_types.h>

#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <cstring>
#include <vidc_client.h>
#endif

#include "QC/Infras/Memory/VideoFrameDescriptor.hpp"
#include "QC/Node/NodeBase.hpp"
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>

using std::string;

namespace QC
{
namespace Node
{

typedef enum
{
    VIDEO_CODEC_BUF_INPUT = 0,
    VIDEO_CODEC_BUF_OUTPUT
} VideoCodec_BufType_e;
#define VIDEO_CODEC_BUF_TYPE_NUM 2

typedef enum
{
    VIDEO_ENC = 0,
    VIDEO_DEC
} VideoEncDecType_e;

typedef enum
{
    VIDEO_CODEC_H265 = 0,
    VIDEO_CODEC_H264
} VideoCodecType_e;

typedef enum
{
    VIDEO_CODEC_START_OUTPUT = 0,
    VIDEO_CODEC_START_INPUT,
    VIDEO_CODEC_START_ALL,
} VideoCodec_StartType_e;

/** @brief This data type list the different VideoCodec Callback Event Type */
typedef enum
{
    VIDEO_CODEC_EVT_FLUSH_INPUT_DONE = 0, /**< flush input buffer done */
    VIDEO_CODEC_EVT_FLUSH_OUTPUT_DONE,    /**< flush output buffer done */
    VIDEO_CODEC_EVT_INPUT_RECONFIG,       /**< received input buffer reconfig event */
    VIDEO_CODEC_EVT_OUTPUT_RECONFIG,      /**< received output buffer reconfig event */
    VIDEO_CODEC_EVT_RESP_START,
    VIDEO_CODEC_EVT_RESP_START_INPUT_DONE,
    VIDEO_CODEC_EVT_RESP_START_OUTPUT_DONE,
    VIDEO_CODEC_EVT_RESP_PAUSE,
    VIDEO_CODEC_EVT_RESP_RESUME,
    VIDEO_CODEC_EVT_RESP_LOAD_RESOURCES,
    VIDEO_CODEC_EVT_RESP_RELEASE_RESOURCES,
    VIDEO_CODEC_EVT_RESP_STOP,
    VIDEO_CODEC_EVT_ERROR = 0xf0000000, /**< normal error */
    VIDEO_CODEC_EVT_FATAL               /**< fatal error */
} VideoCodec_EventType_e;

typedef struct
{
    uint32_t width;
    uint32_t height;
    uint32_t frameRate;
    VideoCodecType_e codecType;
} VidcCodecMeta_t;

/** @brief callback for input buffer done */
typedef void ( *VideoCodec_InFrameCallback_t )( VideoFrameDescriptor &inFrameDesc,
                                                void *pPrivData );
/** @brief callback for output buffer done */
typedef void ( *VideoCodec_OutFrameCallback_t )( VideoFrameDescriptor &outFrameDesc,
                                                 void *pPrivData );
/** @brief callback for event */
typedef void ( *VideoCodec_EventCallback_t )( VideoCodec_EventType_e eventId,
                                              const void *pEvent, void *pPrivData );

typedef struct VidcNodeBase_Config VidcNodeBase_Config_t;

/** @brief Top level control for interfacing with vidc based driver */
class VidcDrvClient
{
public:
    /** @brief Default constructor */
    VidcDrvClient() = default;
    /** @brief Default destructor */
    ~VidcDrvClient() = default;

    void Init( const string &name, Logger_Level_e level, VideoEncDecType_e type,
               const VidcNodeBase_Config_t &config);

    /**
     * @brief open driver and register callback
     * @param inputDoneCb input frame callback function
     * @param outputDoneCb output frame callback function
     * @param eventCb event callback function
     * @param pAppPriv app private data
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e OpenDriver( VideoCodec_InFrameCallback_t inputDoneCb,
                           VideoCodec_OutFrameCallback_t outputDoneCb,
                           VideoCodec_EventCallback_t eventCb, void *pAppPriv );

    QCStatus_e InitDriver( const VidcCodecMeta_t &meta );

    QCStatus_e GetDrvProperty( uint32_t id, uint32_t nPktSize, const uint8_t &pkt );
    QCStatus_e SetDrvProperty( uint32_t id, uint32_t nPktSize, const uint8_t &pkt );

    QCStatus_e LoadResources();
    QCStatus_e ReleaseResources();

    QCStatus_e StartDriver( VideoCodec_StartType_e type );

    /**
     * @brief Inform the video driver of an externally allocated buffer address.
     *        i.e register a buffer address with driver.
     *        to be used only if an external allocator allocated this buffer.
     * @note NegotiateBufferReq() should be called before to get the buffer requirements
     *       from the driver.
     * @note API type: Synchronous
     */
    QCStatus_e SetBuffer( VideoCodec_BufType_e bufferType,
                          const std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> &buffers );

    /**
     * @brief: Passing an input video buffer (frame) for processing.
     * @note: API type: Asynchronous.
     */
    QCStatus_e EmptyBuffer( VideoFrameDescriptor &frameDesc );

    /**
     * @brief: Provides the video driver with an unfilled output buffer to
     *         "fill in" the processed video output frame.
     *         This buffer would be added to driver's unfilled output buffers
     *         queue and will later get used to place the processed output frame contents.
     * @note API type: Asynchronous.
     */
    QCStatus_e FillBuffer( VideoFrameDescriptor &frameDesc );

    /**
    * @brief: Request freeing/de-registering of a single buffer.
    *         Driver would de-register buffer address from driver buffer pool.
    * @note:  API type: Synchronous.
    */
    QCStatus_e FreeBuffers( VideoCodec_BufType_e bufferType,
                            const std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> &buffers );

    QCStatus_e StopDecoder();
    QCStatus_e StopEncoder();

    void CloseDriver();

    QCStatus_e SetDynamicMode( VideoCodec_BufType_e type, bool mode );
    QCStatus_e NegotiateBufferReq( VideoCodec_BufType_e bufType, uint32_t &bufNum, uint32_t &bufSize );
    void PrintCodecConfig();

    VideoEncDecType_e GetType() const { return m_encDecType; }

private:
    ioctl_session_t *m_pIoHandle = nullptr; /**< The IOSession */
    vidc_codec_type m_codecType;
    VideoEncDecType_e m_encDecType;

    uint32_t m_width = 0;
    uint32_t m_height = 0;

    bool m_bInputDynamicMode = true;
    bool m_bOutputDynamicMode = true;

    typedef int ( *VideoCodec_Callback_Handler_t )( uint8_t *pMsg, uint32_t length, void *cd );

    typedef struct
    {
        VideoCodec_Callback_Handler_t handler;
        void *data;
    } VideoCodec_Ioctl_Callback_t;

    /** The Vidc Driver Callback Message */
    typedef enum
    {
        VIDEO_CODEC_COMMAND_NONE,          /**< none command received */
        VIDEO_CODEC_COMMAND_LOAD_RESOURCE, /**< load resources */
        VIDEO_CODEC_COMMAND_START_INPUT,
        VIDEO_CODEC_COMMAND_START_OUTPUT,
        VIDEO_CODEC_COMMAND_START_ALL,
        VIDEO_CODEC_COMMAND_DRAIN,       /**< drain input buffers */
        VIDEO_CODEC_COMMAND_INPUT_STOP,  /**< stop input executing */
        VIDEO_CODEC_COMMAND_OUTPUT_STOP, /**< stop output executing */
        VIDEO_CODEC_COMMAND_STOP,
        VIDEO_CODEC_COMMAND_LAST_FLAG,        /**< last flag event received */
        VIDEO_CODEC_COMMAND_RELEASE_RESOURCE, /**< release resources */
    } VideoCodec_CommandType_e;

    VideoCodec_CommandType_e m_cmdCompleted = VIDEO_CODEC_COMMAND_NONE;
    bool m_bCmdDrainReceived = false;

    QCStatus_e WaitForCmdCompleted( VideoCodec_CommandType_e expectedCmd, int32_t timeoutMs );
    inline void InitCmdCompleted( void ) { m_cmdCompleted = VIDEO_CODEC_COMMAND_NONE; };

    static int DeviceCallback( uint8_t *pMsg, uint32_t length, void *pCdata );
    int DeviceCbHandler( uint8_t *pMsg, uint32_t length );

    typedef struct
    {
        VideoFrameDescriptor_t inFrameDesc;
        bool bUsedFlag = false; /**< indicate whether sharedBuffer is using by driver or available */
    } VideoCodec_InputInfo_t;

    typedef struct
    {
        VideoFrameDescriptor_t outFrameDesc;
        bool bUsedFlag = false; /**< indicate whether sharedBuffer is using by driver or available */
    } VideoCodec_OutputInfo_t;

    uint32_t m_bufNum[VIDEO_CODEC_BUF_TYPE_NUM] = { 0, 0 };

    std::mutex m_inLock;
    std::unordered_map<uint64_t, VideoCodec_InputInfo_t> m_inputMap; /**< store input info */
    std::mutex m_outLock;
    std::unordered_map<uint64_t, VideoCodec_OutputInfo_t> m_outputMap; /**< store output info */

    VideoCodec_InFrameCallback_t m_inputDoneCb = nullptr;
    VideoCodec_OutFrameCallback_t m_outputDoneCb = nullptr;
    VideoCodec_EventCallback_t m_eventCb = nullptr;
    void *m_pAppPriv = nullptr;

private:
    QC_DECLARE_LOGGER();
};

}   // namespace Node
}   // namespace QC

#endif   // QC_VIDC_DRV_CLIENT_HPP
