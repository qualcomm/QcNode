// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_VIDEO_DECODER_HPP
#define RIDEHAL_VIDEO_DECODER_HPP

#include "VidcCompBase.hpp"
#include "ridehal/component/ComponentIF.hpp"
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>

using namespace ridehal::common;

namespace ridehal
{
namespace component
{

/**
 * @brief The VideoDecoder Init Config
 * Buffer allocate has three modes:
 * dynamic mode: app allocate buffers, told component when SubmitInputFrame() & SubmitOutputFrame().
 * We call it mode 1).
 * none dynamic mode: different with dynamic mode, none dynamic mode will set buffer address to
 * driver before decode. For none dynamic mode, app can allocate buffers, we call it mode 2); if app
 * don't allocate, ridehal component will allocate, we call it mode 3). mode 2): app allocate
 * buffers, transfer buffer list by init() with bufferList ptr and number. In this mode, bufferList
 * ptr must not be nullptr, and app must make sure bufferList number <= bufferList.len(). mode 3):
 * app not allocate buffers, and bufferList ptr must be nullptr; and app will tell the buffer number
 * requested to component with Init(). When app call init(), ridehal will allocate buffers in
 * component.
 */
typedef struct
{
    uint32_t width;           /**< in pixels */
    uint32_t height;          /**< in pixels */
    uint32_t frameRate;       /**< fps */
    uint32_t numInputBuffer;  /**< bufferList number for input buffer */
    uint32_t numOutputBuffer; /**< bufferList number for output buffer */
    bool bInputDynamicMode;   /**< is dynamic mode or not, for input buffer */
    bool bOutputDynamicMode;  /**< is dynamic mode or not, for output buffer */

    /* in non-dynamic mode, app could alloc and config buf list */
    RideHal_SharedBuffer_t *pInputBufferList = nullptr;  /**< bufferList ptr for input buffer */
    RideHal_SharedBuffer_t *pOutputBufferList = nullptr; /**< bufferList ptr for output buffer */

    RideHal_ImageFormat_e inFormat;  /**< compress mode, now support h264 and h265 */
    RideHal_ImageFormat_e outFormat; /**< support NV12, NV12_UBWC, P010 */
} VideoDecoder_Config_t;

/** @brief The VideoDecoder Input Frame */
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer; /**< contains input buffer */
    uint64_t timestampNs;                /**< frame data's timestamp. */
    uint64_t appMarkData; /**< frame data's mark data, this data will be copied to corresponding
                               output buf. API won't touch this data, only copy it. */
} VideoDecoder_InputFrame_t;

/** @brief The VideoDecoder Output Frame */
typedef struct
{
    RideHal_SharedBuffer_t sharedBuffer; /**< contains output buffer */
    uint64_t timestampNs;                /**< frame data's timestamp. */
    uint64_t appMarkData; /**< frame data's mark data, this data copied from corresponding
                               input buf. API won't touch this data, only copy it. */
    uint32_t frameFlag;   /**< indicate whether some error occurred during deccoding this frame. */
} VideoDecoder_OutputFrame_t;

/** @brief This data type list the different VideoDecoder Callback Event Type */
typedef VideoCodec_EventType_e VideoDecoder_EventType_e;

/** @brief callback for input buffer done */
typedef void ( *VideoDecoder_InFrameCallback_t )( const VideoDecoder_InputFrame_t *pInputBuf,
                                                  void *pPrivData );
/** @brief callback for output buffer done */
typedef void ( *VideoDecoder_OutFrameCallback_t )( const VideoDecoder_OutputFrame_t *pOutputBuf,
                                                   void *pPrivData );
/** @brief callback for event */
typedef void ( *VideoDecoder_EventCallback_t )( const VideoDecoder_EventType_e eventId,
                                                const void *pEvent, void *pPrivData );

/** @brief Top level control for video decoder */
class VideoDecoder final : public VidcCompBase
{
public:
    /** @brief Default constructor */
    VideoDecoder() = default;
    /** @brief Default destructor */
    ~VideoDecoder() = default;

    /**
     * @brief Init the video decoder
     * @param pName the video decoder unique instance name
     * @param pConfig pointer to the video config information
     * @param level the log level used by the video decoder, default is error
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const VideoDecoder_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Start the video decoder
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief Stop the video decoder
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief deinitialize the video decoder
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @brief submit a video input buffer to VIDC driver for decoder
     * @param pInput pointer to hold the video buffer information
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e SubmitInputFrame( const VideoDecoder_InputFrame_t *pInput );

    /**
     * @brief submit a video output buffer back to VIDC driver
     * @param pOutput pointer to hold the video buffer information
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e SubmitOutputFrame( const VideoDecoder_OutputFrame_t *pOutput );

    /**
     * @brief register callback
     * @param inputDoneCb input frame callback function
     * @param outputDoneCb output frame callback function
     * @param pAppPriv app private data
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterCallback( VideoDecoder_InFrameCallback_t inputDoneCb,
                                     VideoDecoder_OutFrameCallback_t outputDoneCb,
                                     VideoDecoder_EventCallback_t eventCb, void *pAppPriv );

private:
    RideHalError_e HandleOutputReconfig();
    RideHalError_e FinishOutputReconfig();
    RideHalError_e ValidateConfig( const char *name, const VideoDecoder_Config_t *cfg );
    RideHalError_e InitFromConfig( const VideoDecoder_Config_t *cfg );
    RideHalError_e InitDrvProperty();
    RideHalError_e CheckBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                VideoCodec_BufType_e bufferType );

    VideoDecoder_InFrameCallback_t m_inputDoneCb = nullptr;
    VideoDecoder_OutFrameCallback_t m_outputDoneCb = nullptr;
    VideoDecoder_EventCallback_t m_eventCb = nullptr;
    void *m_pAppPriv = nullptr;

    bool m_OutputStarted = false;
    bool m_OutputReconfigInprogress = false;

    void InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame );
    void OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame );
    void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload );

    static void InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame, void *pPrivData );
    static void OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame, void *pPrivData );
    static void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload,
                               void *pPrivData );
};

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_VIDEO_DECODER_HPP


