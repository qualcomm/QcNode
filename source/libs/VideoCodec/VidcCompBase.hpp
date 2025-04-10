// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_VIDEO_CODEC_COMPONENT_BASE_HPP
#define RIDEHAL_VIDEO_CODEC_COMPONENT_BASE_HPP

#include "VidcDrvClient.hpp"
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

/** @brief base class for video codec component */
class VidcCompBase : public ComponentIF
{
public:
    /** @brief Default constructor */
    VidcCompBase() = default;
    /** @brief Default destructor */
    ~VidcCompBase() = default;

    /**
     * @brief get video input buffers to submit input in non-dynamic mode
     * @param pInputList pointer to hold the video input buffer list
     * @param num size of pInputList
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetInputBuffers( RideHal_SharedBuffer_t *pInputList, uint32_t num );

    /**
     * @brief get video output buffers to submit output in non-dynamic mode
     * @param pOutputList pointer to hold the video output buffer list
     * @param num size of pOutputList
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e GetOutputBuffers( RideHal_SharedBuffer_t *pOutputList, uint32_t num );

protected:
    /**
     * @brief Init the video codec component
     * @param pName the video codec unique instance name
     * @param level the log level used , default is error level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, VideoEncDecType_e type,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    RideHalError_e PostInit( RideHal_SharedBuffer_t *pBufListIn,
                             RideHal_SharedBuffer_t *pBufListOut );

    RideHalError_e Start();

    RideHalError_e Stop();

    /**
     * @brief deinitialize the video codec
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload );

    RideHalError_e SetBuffer( VideoCodec_BufType_e bufferType );

    RideHalError_e FreeOutputBuffer( void );
    RideHalError_e FreeInputBuffer( void );

    RideHalError_e ValidateBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                   VideoCodec_BufType_e bufferType );

    RideHalError_e NegotiateBufferReq( VideoCodec_BufType_e bufType );

    RideHalError_e WaitForState( RideHal_ComponentState_t expectedState );

    vidc_color_format_type GetVidcFormat( RideHal_ImageFormat_e format );

    VidcDrvClient m_drvClient;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_frameRate = 0;
    uint32_t m_bufNum[VIDEO_CODEC_BUF_TYPE_NUM] = { 0, 0 };
    uint32_t m_bufSize[VIDEO_CODEC_BUF_TYPE_NUM];
    bool m_bDynamicMode[VIDEO_CODEC_BUF_TYPE_NUM] = { true, true };
    bool m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_TYPE_NUM] = { false, false };

    RideHal_ImageFormat_e m_inFormat;
    RideHal_ImageFormat_e m_outFormat;

    RideHal_SharedBuffer_t *m_pInputList =
            nullptr; /**< store input buffer list for non-dynamic mode */
    RideHal_SharedBuffer_t *m_pOutputList =
            nullptr; /**< store output buffer list for non-dynamic mode */

private:
    typedef enum
    {
        COMPRESSED_DATA_MODE = 0,
        YUV_OR_RGB_MODE
    } VideoCodec_BufAllocMode_e;

    RideHalError_e AllocateBuffer( VideoCodec_BufType_e bufferType );
    RideHalError_e InitBufferForNonDynamicMode( RideHal_SharedBuffer_t *pBufList,
                                                VideoCodec_BufType_e bufferType );
    RideHalError_e AllocateSharedBuf( RideHal_SharedBuffer_t &sharedBuffer,
                                      VideoCodec_BufAllocMode_e mode, int32_t bufSize,
                                      RideHal_ImageFormat_e format );
    RideHalError_e FreeSharedBuf( VideoCodec_BufType_e bufType );
    void PrintConfig();

    VideoEncDecType_e m_encDecType;
};

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_VIDEO_CODEC_COMPONENT_BASE_HPP

