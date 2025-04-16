// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_VIDEO_CODEC_COMPONENT_BASE_HPP
#define QC_VIDEO_CODEC_COMPONENT_BASE_HPP

#include "QC/component/ComponentIF.hpp"
#include "VidcDrvClient.hpp"
#include <mutex>
#include <queue>
#include <sys/uio.h>
#include <unordered_map>

using namespace QC;

namespace QC
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
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetInputBuffers( QCSharedBuffer_t *pInputList, uint32_t num );

    /**
     * @brief get video output buffers to submit output in non-dynamic mode
     * @param pOutputList pointer to hold the video output buffer list
     * @param num size of pOutputList
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetOutputBuffers( QCSharedBuffer_t *pOutputList, uint32_t num );

protected:
    /**
     * @brief Init the video codec component
     * @param pName the video codec unique instance name
     * @param level the log level used , default is error level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, VideoEncDecType_e type,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    QCStatus_e PostInit( QCSharedBuffer_t *pBufListIn, QCSharedBuffer_t *pBufListOut );

    QCStatus_e Start();

    QCStatus_e Stop();

    /**
     * @brief deinitialize the video codec
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

    void EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload );

    QCStatus_e SetBuffer( VideoCodec_BufType_e bufferType );

    QCStatus_e FreeOutputBuffer( void );
    QCStatus_e FreeInputBuffer( void );

    QCStatus_e ValidateBuffer( const QCSharedBuffer_t *pBuffer, VideoCodec_BufType_e bufferType );

    QCStatus_e NegotiateBufferReq( VideoCodec_BufType_e bufType );

    QCStatus_e WaitForState( QCObjectState_e expectedState );

    vidc_color_format_type GetVidcFormat( QCImageFormat_e format );

    VidcDrvClient m_drvClient;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_frameRate = 0;
    uint32_t m_bufNum[VIDEO_CODEC_BUF_TYPE_NUM] = { 0, 0 };
    uint32_t m_bufSize[VIDEO_CODEC_BUF_TYPE_NUM];
    bool m_bDynamicMode[VIDEO_CODEC_BUF_TYPE_NUM] = { true, true };
    bool m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_TYPE_NUM] = { false, false };

    QCImageFormat_e m_inFormat;
    QCImageFormat_e m_outFormat;

    QCSharedBuffer_t *m_pInputList = nullptr;  /**< store input buffer list for non-dynamic mode */
    QCSharedBuffer_t *m_pOutputList = nullptr; /**< store output buffer list for non-dynamic mode */

private:
    typedef enum
    {
        COMPRESSED_DATA_MODE = 0,
        YUV_OR_RGB_MODE
    } VideoCodec_BufAllocMode_e;

    QCStatus_e AllocateBuffer( VideoCodec_BufType_e bufferType );
    QCStatus_e InitBufferForNonDynamicMode( QCSharedBuffer_t *pBufList,
                                            VideoCodec_BufType_e bufferType );
    QCStatus_e AllocateSharedBuf( QCSharedBuffer_t &sharedBuffer, VideoCodec_BufAllocMode_e mode,
                                  int32_t bufSize, QCImageFormat_e format );
    QCStatus_e FreeSharedBuf( VideoCodec_BufType_e bufType );
    void PrintConfig();

    VideoEncDecType_e m_encDecType;
};

}   // namespace component
}   // namespace QC

#endif   // QC_VIDEO_CODEC_COMPONENT_BASE_HPP
