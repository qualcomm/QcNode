// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include <MMTimer.h>
#include <cmath>
#include <malloc.h>

#include <vidc_ioctl.h>
#include <vidc_types.h>

#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <cstring>
#include <vidc_client.h>
#endif

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "VidcDrvClient.hpp"
#include "VidcNodeBase.hpp"

namespace QC {
namespace Node {

static constexpr uint16_t VIDEO_MAX_DEV_CMD_BUFFER_SIZE = 256;
static constexpr int WAIT_TIMEOUT_10_MSEC = 10;

static const char* VidcErrToStr( vidc_status_type err )
{
    const char *ret;
    switch ( err )
    {
        case VIDC_ERR_NONE:
            ret = "No error.";
            break;
        case VIDC_ERR_INDEX_NOMORE:
            ret = "No More indices can be enumerated";
            break;
        case VIDC_ERR_FAIL:
            ret = "General failure";
            break;
        case VIDC_ERR_ALLOC_FAIL:
            ret = "Failed while allocating memory";
            break;
        case VIDC_ERR_ILLEGAL_OP:
            ret = "Illegal operation was requested";
            break;
        case VIDC_ERR_BAD_PARAM:
            ret = "Bad paramter(s) or memory pointer(s) provided";
            break;
        case VIDC_ERR_BAD_HANDLE:
            ret = "Bad driver or client handle provided";
            break;
        case VIDC_ERR_NOT_SUPPORTED:
            ret = "API is currently not supported";
            break;
        case VIDC_ERR_BAD_STATE:
            ret = "Requested API is not supported in current device or client state";
            break;
        case VIDC_ERR_MAX_CLIENT:
            ret = "Allowed maximum number of clients are already opened";
            break;
        case VIDC_ERR_IFRAME_EXPECTED:
            ret = " Decoding: VIDC was expecting I-frame which was not provided";
            break;
        case VIDC_ERR_HW_FATAL:
            ret = "Fatal irrecoverable hardware error detected";
            break;
        case VIDC_ERR_BITSTREAM_ERR:
            ret = "Decoding: Error in input frame detected and frame processing "
                  "skipped";
            break;
        case VIDC_ERR_SEQHDR_PARSE_FAIL:
            ret = "Sequence header parsing failed in decoder";
            break;
        case VIDC_ERR_INSUFFICIENT_BUFFER:
            ret = "Error to indicate that the buffer supplied was insufficient in "
                  "size";
            break;
        case VIDC_ERR_BAD_POWER_STATE:
            ret = "Error indicating that the device is in bad power state and can not "
                  "service the request API.";
            break;
        case VIDC_ERR_NO_VALID_SESSION:
            ret = "The error is returned in case a session related API call be made "
                  "wihtout"
                  "initializing a session. (For e.g. if ABORT is called on a hanlde "
                  "for"
                  "which vidc_initialize() has not been called";
            break;
        case VIDC_ERR_TIMEOUT:
            ret = "API was not completed and returned with a timeout";
            break;
        case VIDC_ERR_CMDQFULL:
            ret = "API request was not accepted as command queue is full ";
            break;
        case VIDC_ERR_START_CODE_NOT_FOUND:
            ret = "Valid start code was not found within provided compressed video "
                  "frame";
            break;
        case VIDC_ERR_UNSUPPORTED_STREAM:
            ret = "Error indicates stream is unsupported by video core";
            break;
        case VIDC_ERR_SESSION_PICTURE_DROPPED:
            ret = "Error indicates frame was dropped as per VIDC_I_DEC_PICTYPE "
                  "property";
            break;
        case VIDC_ERR_CLIENTFATAL:
            ret = "To indicate irrecoverable fatal client error was detected."
                  "Client should initiate session clean up.";
            break;
        case VIDC_ERR_NONCOMPLIANT_STREAM:
            ret = "Error indicates stream is non-compliant";
            break;
        case VIDC_ERR_SPURIOUS_INTERRUPT:
            ret = "Error indicates spurious interrupt handled and no further"
                  "processing required";
            break;
        case VIDC_ERR_UNUSED:
        default:
            ret = "Unknown error code";
            break;
    }

    return ret;
}

void VidcDrvClient::Init( const string &name, Logger_Level_e level, VideoEncDecType_e type,
                          const VidcNodeBase_Config_t &nodeConfig )
{
    QC_LOGGER_INIT( name.c_str(), level );
    m_bufNum[VIDEO_CODEC_BUF_INPUT] = nodeConfig.numInputBufferReq;
    m_bufNum[VIDEO_CODEC_BUF_OUTPUT] = nodeConfig.numOutputBufferReq;
    m_encDecType = type;
}

QCStatus_e VidcDrvClient::InitDriver( const VidcCodecMeta_t &meta )
{
    QCStatus_e ret = QC_STATUS_OK;
    vidc_frame_rate_type vidcFrameRate;
    vidc_frame_size_type vidcFrameSize;

    /* now only support h264 and h265 */
    if ( VIDEO_CODEC_H265 == meta.codecType )
    {
        m_codecType = VIDC_CODEC_HEVC;
    }
    else
    {
        m_codecType = VIDC_CODEC_H264;
    }

    m_width = meta.width;
    m_height = meta.height;

    QC_INFO( "init: w:%u, h:%u, fps:%u, codecType:%d", m_width, m_height, meta.frameRate,
             m_codecType );

    if ( QC_STATUS_OK == ret )
    {
        vidc_session_codec_type sessionCodec; /**< Session & Codec type setting */
        if ( VIDEO_DEC == m_encDecType )
        {
            sessionCodec.session = VIDC_SESSION_DECODE;
        }
        else
        {
            sessionCodec.session = VIDC_SESSION_ENCODE;
        }

        sessionCodec.codec = m_codecType;

        QC_DEBUG( "Setting VIDC_I_SESSION_CODEC %0x.0x%x", sessionCodec.session, sessionCodec.codec );
        ret = SetDrvProperty( VIDC_I_SESSION_CODEC, sizeof( vidc_session_codec_type ),
                              (uint8_t &) ( sessionCodec ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        vidcFrameRate.buf_type = VIDC_BUFFER_OUTPUT;
        vidcFrameRate.fps_numerator = meta.frameRate;
        vidcFrameRate.fps_denominator = 1;
        QC_DEBUG( "Setting FRAME_RATE for output %u/%u", vidcFrameRate.fps_numerator,
                  vidcFrameRate.fps_denominator);
        ret = SetDrvProperty( VIDC_I_FRAME_RATE, sizeof( vidc_frame_rate_type ),
                              (uint8_t &) ( vidcFrameRate ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        vidcFrameRate.buf_type = VIDC_BUFFER_INPUT;
        vidcFrameRate.fps_numerator = meta.frameRate;
        vidcFrameRate.fps_denominator = 1;
        QC_DEBUG( "Setting FRAME_RATE for input %u/%u", vidcFrameRate.fps_numerator,
                  vidcFrameRate.fps_denominator );
        ret = SetDrvProperty( VIDC_I_FRAME_RATE, sizeof( vidc_frame_rate_type ),
                              (uint8_t &) ( vidcFrameRate ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        vidcFrameSize.buf_type = VIDC_BUFFER_INPUT;
        vidcFrameSize.width = m_width;
        vidcFrameSize.height = m_height;
        QC_DEBUG( "Setting input FRAME_SIZE %ux%u", vidcFrameSize.width, vidcFrameSize.height );
        ret = SetDrvProperty( VIDC_I_FRAME_SIZE, sizeof( vidc_frame_size_type ),
                              (uint8_t &) ( vidcFrameSize ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        vidcFrameSize.buf_type = VIDC_BUFFER_OUTPUT;
        vidcFrameSize.width = m_width;
        vidcFrameSize.height = m_height;
        QC_DEBUG( "Setting output FRAME_SIZE %ux%u", vidcFrameSize.width, vidcFrameSize.height );
        ret = SetDrvProperty( VIDC_I_FRAME_SIZE, sizeof( vidc_frame_size_type ),
                              (uint8_t &) vidcFrameSize );
    }

    return ret;
}

QCStatus_e VidcDrvClient::OpenDriver( VideoCodec_InFrameCallback_t inputDoneCb,
                                      VideoCodec_OutFrameCallback_t outputDoneCb,
                                      VideoCodec_EventCallback_t eventCb, void *pAppPriv )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( nullptr == inputDoneCb ) || ( nullptr == outputDoneCb ) || ( nullptr == eventCb )
         || nullptr == pAppPriv )
    {
        QC_ERROR( "callback is NULL pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_inputDoneCb = inputDoneCb;
        m_outputDoneCb = outputDoneCb;
        m_eventCb = eventCb;
        m_pAppPriv = pAppPriv;

        VideoCodec_Ioctl_Callback_t ioctlCb;
        ioctlCb.handler = VidcDrvClient::DeviceCallback;
        ioctlCb.data = (void *) this;

        QC_DEBUG( "Opening vidc device" );
        m_pIoHandle = device_open( (char *) "VideoCore/vidc_drv", (ioctl_callback_t *) &ioctlCb );
        if ( nullptr == m_pIoHandle )
        {
            QC_ERROR( "Failed to open vidc device!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            QC_INFO( "Open vidc device succeed" );
        }
    }

    return ret;
}

void VidcDrvClient::CloseDriver()
{
    if ( m_pIoHandle != nullptr )
    {
        (void) device_close( m_pIoHandle );
        m_pIoHandle = nullptr;
    }
}

QCStatus_e VidcDrvClient::LoadResources()
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t rc = 0;

    QC_DEBUG( "Loading vidc resources" );
    InitCmdCompleted();
    rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_LOAD_RESOURCES, nullptr, 0, nullptr, 0 );
    if ( VIDC_ERR_NONE != rc )
    {
        QC_ERROR( "Loading vidc resources failed! rc=0x%x, %s", rc,
                  VidcErrToStr( vidc_status_type( rc ) ) );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        ret = WaitForCmdCompleted( VIDEO_CODEC_COMMAND_LOAD_RESOURCE, WAIT_TIMEOUT_10_MSEC );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "VIDC_IOCTL_LOAD_RESOURCES timeout" );
        }
        else
        {
            QC_INFO( "Successfully load vidc resources" );
        }
    }
    return ret;
}

QCStatus_e VidcDrvClient::ReleaseResources()
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t rc = 0;

    QC_DEBUG( "release vidc resources" );
    InitCmdCompleted();
    rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_RELEASE_RESOURCES, nullptr, 0, nullptr, 0 );
    if ( VIDC_ERR_NONE != rc )
    {
        QC_ERROR( "release vidc resources failed! rc=0x%x, %s", rc,
                  VidcErrToStr( vidc_status_type( rc ) ) );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        ret = WaitForCmdCompleted( VIDEO_CODEC_COMMAND_RELEASE_RESOURCE, WAIT_TIMEOUT_10_MSEC );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "VIDC_IOCTL_RELEASE_RESOURCES timeout" );
        }
        else
        {
            QC_INFO( "Successfully release vidc resources" );
        }
    }
    return ret;
}

QCStatus_e VidcDrvClient::SetDynamicMode( VideoCodec_BufType_e type, bool mode )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( type == VIDEO_CODEC_BUF_INPUT )
    {
        m_bInputDynamicMode = mode;
    }
    else
    {
        m_bOutputDynamicMode = mode;
    }

    vidc_buffer_alloc_mode_type buffer_alloc_mode = {};

    if ( type == VIDEO_CODEC_BUF_INPUT )
    {
        buffer_alloc_mode.buf_type = VIDC_BUFFER_INPUT;
        if ( mode )
            QC_DEBUG( "Enable input dynamic mode" );
    }
    else
    {
        buffer_alloc_mode.buf_type = VIDC_BUFFER_OUTPUT;
        if ( mode )
            QC_DEBUG( "Enable output dynamic mode" );
    }

    if ( mode )
    {
        buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_DYNAMIC;
    }
    else {
        buffer_alloc_mode.buf_mode = VIDC_BUFFER_MODE_STATIC;
    }

    ret = SetDrvProperty( VIDC_I_BUFFER_ALLOC_MODE, sizeof( buffer_alloc_mode ),
                          (uint8_t &) ( buffer_alloc_mode ) );


    return ret;
}

QCStatus_e VidcDrvClient::NegotiateBufferReq( VideoCodec_BufType_e bufType,
                                              uint32_t &bufNum, uint32_t &bufSize )
{
    QCStatus_e ret = QC_STATUS_OK;
    vidc_buffer_reqmnts_type bufferReq = {};

    if ( VIDEO_CODEC_BUF_INPUT == bufType )
    {
        bufferReq.buf_type = VIDC_BUFFER_INPUT;
    }
    else
    {
        bufferReq.buf_type = VIDC_BUFFER_OUTPUT;
    }

    ret = GetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                          (uint8_t &) ( bufferReq ) );

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "buf-type:%d, driver req num:%" PRIu32 " size:%" PRIu32, bufType,
                 bufferReq.actual_count, bufferReq.size );

        bufSize = bufferReq.size;

        if ( bufferReq.actual_count > bufNum )
        {
            bufNum = bufferReq.actual_count;
        }
        else if ( bufferReq.actual_count < bufNum )
        {
            bufferReq.actual_count = bufNum;
            ret = SetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS, sizeof( vidc_buffer_reqmnts_type ),
                                  (uint8_t &) ( bufferReq ) );
            if ( QC_STATUS_OK == ret )
            {
                ret = GetDrvProperty( VIDC_I_BUFFER_REQUIREMENTS,
                                      sizeof( vidc_buffer_reqmnts_type ),
                                      (uint8_t &) ( bufferReq ) );
                QC_INFO( "buf-type:%d: re-config done, req:%" PRIu32 ", actual_count:%" PRIu32,
                         bufType, bufNum, bufferReq.actual_count );

                if ( ( QC_STATUS_OK == ret ) && ( bufNum != bufferReq.actual_count ) )
                {
                    QC_ERROR( "buf-type:%d: req:%" PRIu32 " and actual_count:%" PRIu32
                              " must be same!",
                              bufType, bufNum, bufferReq.actual_count );
                    ret = QC_STATUS_FAIL;
                }
            }
        }
        m_bufNum[bufType] = bufNum;
    }

    return ret;
}

void VidcDrvClient::PrintCodecConfig()
{
    if ( VIDC_CODEC_HEVC == m_codecType )
    {
        QC_DEBUG( "CodecCfg: Codec = H265" );
    }
    else if ( VIDC_CODEC_H264 == m_codecType )
    {
        QC_DEBUG( "CodecCfg: Codec = H264" );
    }
    else
    {
        QC_DEBUG( "CodecCfg: Codec type = 0x%x", m_codecType );
    }
}

QCStatus_e VidcDrvClient::StartDriver( VideoCodec_StartType_e type )
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t rc = VIDC_ERR_NONE;
    VideoCodec_CommandType_e cmd;

    if ( VIDEO_CODEC_START_OUTPUT == type || VIDEO_CODEC_START_INPUT == type )
    {
        QC_INFO( "START %s begin", type == VIDEO_CODEC_START_OUTPUT ? "OUTPUT" : "INPUT" );

        vidc_start_mode_type start_mode = VIDC_START_INPUT;
        if ( VIDEO_CODEC_START_INPUT == type )
        {
            cmd = VIDEO_CODEC_COMMAND_START_INPUT;
        }
        else
        {
            start_mode = VIDC_START_OUTPUT;
            cmd = VIDEO_CODEC_COMMAND_START_OUTPUT;
        }

        InitCmdCompleted();
        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_START, (uint8 *) &start_mode,
                           sizeof( vidc_start_mode_type ), nullptr, 0 );
    }
    else
    {
        QC_INFO( "START all begin" );

        cmd = VIDEO_CODEC_COMMAND_START_ALL;
        InitCmdCompleted();
        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_START, nullptr, 0, nullptr, 0 );
    }

    if ( VIDC_ERR_NONE != rc )
    {
        QC_ERROR( "Starting vidc failed! rc=0x%x %s", rc, VidcErrToStr( vidc_status_type( rc ) ) );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        /* For VIDEO_CODEC_START_OUTPUT started in the same thread of driver callback,
         * so, for VIDEO_CODEC_START_OUTPUT, will not call WaitForCmdCompleted here.
         */
        if ( VIDEO_CODEC_START_INPUT == type || VIDEO_CODEC_START_ALL == type )
        {
            ret = WaitForCmdCompleted( cmd, WAIT_TIMEOUT_10_MSEC );

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "VIDC_IOCTL_START timeout" );
            }
            else
            {
                QC_INFO( "START succeed %d", type );
            }
        }
    }

    return ret;
}

QCStatus_e VidcDrvClient::SetBuffer( VideoCodec_BufType_e bufferType,
                                     const std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> &buffers )
{
    int32_t i = 0, rc = 0;
    QCStatus_e ret = QC_STATUS_OK;
    vidc_buffer_type vidcBufType;

    if ( VIDEO_CODEC_BUF_INPUT == bufferType )
    {
        vidcBufType = VIDC_BUFFER_INPUT;
    }
    else
    {
        vidcBufType = VIDC_BUFFER_OUTPUT;
    }

    for ( const VideoFrameDescriptor_t &buf : buffers )
    {
        vidc_buffer_info_type buf_info = { VIDC_BUFFER_UNUSED, 0 };

        buf_info.buf_addr = static_cast<uint8_t *>( buf.pBuf );
#if defined( __QNXNTO__ )
        buf_info.buf_handle = (pmem_handle_t) buf.dmaHandle;
#else
        buf_info.buf_handle = (int)( buf.dmaHandle );
#endif
        buf_info.buf_type = vidcBufType;
        buf_info.contiguous = true;
        buf_info.buf_size = buf.size;
        // register it before lookup in future for local allocation
        QC_DEBUG( "set-buffer to driver [%" PRId32 "]: buf_addr = 0x%x, buf_handle = 0x%x, "
                 "type:%d, buf_size:%d",
                 i, buf_info.buf_addr, buf_info.buf_handle, buf_info.buf_type, buf_info.buf_size );
        // for non dynamic mode, need to set buffer to driver
        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_SET_BUFFER, (uint8_t*) (&buf_info),
                           sizeof(vidc_buffer_info_type),
                           nullptr, 0 );
        if (VIDC_ERR_NONE != rc)
        {
            QC_ERROR( "set-buffer VIDC_IOCTL_SET_BUFFER failed. Index=%" PRId32 " rc=0x%x", i,
                            rc );
            ret = QC_STATUS_FAIL;
        }

        if (QC_STATUS_OK != ret)
        {
            break;
        }

        if (VIDEO_CODEC_BUF_INPUT == bufferType)
        {
            VideoCodec_InputInfo_t inputInfo;
            inputInfo.inFrameDesc = buf;
            inputInfo.bUsedFlag = false;
            m_inputMap[buf.dmaHandle] = inputInfo;
        }
        else /* VIDEO_CODEC_BUF_OUTPUT == bufferType */
        {
            VideoCodec_OutputInfo_t outputInfo;
            outputInfo.outFrameDesc = buf;
            outputInfo.bUsedFlag = false;
            m_outputMap[buf.dmaHandle] = outputInfo;
        }

        i++;
    }

    return ret;
}

QCStatus_e VidcDrvClient::FreeBuffers( VideoCodec_BufType_e bufferType,
                                       const std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> &buffers )
{
    int32_t i, rc = 0;
    QCStatus_e ret = QC_STATUS_OK;
    vidc_buffer_type vidcBufType;

    if ( VIDEO_CODEC_BUF_INPUT == bufferType )
    {
        vidcBufType = VIDC_BUFFER_INPUT;
    }
    else
    {
        vidcBufType = VIDC_BUFFER_OUTPUT;
    }

    for ( const VideoFrameDescriptor_t &buf : buffers )
    {
        vidc_buffer_info_type buf_info = { VIDC_BUFFER_UNUSED, 0 };

        buf_info.buf_addr = static_cast<uint8_t *>( buf.pBuf );
#if defined( __QNXNTO__ )
        buf_info.buf_handle = (pmem_handle_t) buf.dmaHandle;
#else
        buf_info.buf_handle = (int)( buf.dmaHandle );
#endif
        buf_info.buf_type = vidcBufType;
        buf_info.contiguous = true;
        buf_info.buf_size = buf.size;

        QC_DEBUG( "FREE_BUFFER, i:%d, size:%d", i, buf_info.buf_size );
        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_FREE_BUFFER, (uint8_t *) ( &buf_info ),
                           sizeof( vidc_buffer_info_type ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "VIDC_IOCTL_FREE_BUFFER index=%" PRIu32 ", type:%" PRIu32
                      " failed! rc=0x%x, %s",
                      i, vidcBufType, rc, VidcErrToStr( vidc_status_type( rc ) ) );
        }
    }

    return ret;
}

QCStatus_e VidcDrvClient::EmptyBuffer( VideoFrameDescriptor &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t rc = 0;
    uint64_t handle = frameDesc.dmaHandle;
    uint64_t timestampUs = frameDesc.timestampNs / 1000;
    uint64_t appMarkData = frameDesc.appMarkData;
    vidc_frame_data_type frameData = {};

    QC_DEBUG( "%s-input-begin: handle 0x%x tsUs %" PRIu64 " markData %" PRIu64
              " buf_addr 0x%x dmaHandle 0x%x buf_size %" PRIu32,
              (m_encDecType == VIDEO_ENC) ? "enc" : "dec", handle, timestampUs, appMarkData,
              frameDesc.pBuf, frameDesc.dmaHandle, frameDesc.size );

    frameData.frm_clnt_data = handle;
    frameData.buf_type = VIDC_BUFFER_INPUT;
    frameData.frame_addr = static_cast<uint8_t *>( frameDesc.pBuf );
    frameData.alloc_len = frameDesc.size;
#if defined( __QNXNTO__ )
    frameData.frame_handle = (pmem_handle_t) handle;
#else
    frameData.frame_handle = (int) reinterpret_cast<uint64_t>( handle );
#endif
    frameData.data_len = frameDesc.size;
    frameData.timestamp = timestampUs;
    frameData.mark_data = (unsigned long) appMarkData;

    {
        std::unique_lock<std::mutex> auto_lock( m_inLock );

        auto it = m_inputMap.find( handle );

        if ( true == m_bInputDynamicMode )
        {
            if ( m_inputMap.end() != it )
            {
                QC_DEBUG( "find handle 0x%x", handle );
                if ( false == it->second.bUsedFlag )
                {
                    it->second.bUsedFlag = true;
                    it->second.inFrameDesc.timestampNs = 1000 * timestampUs;
                    it->second.inFrameDesc.appMarkData = appMarkData;
                }
                else
                {
                    QC_ERROR( "input buffer not available now!" );
                    ret = QC_STATUS_NOMEM;
                }
            }
            else if ( m_inputMap.size() < m_bufNum[VIDEO_CODEC_BUF_INPUT] )
            {
                auto result = m_inputMap.emplace(handle, VideoCodec_InputInfo_t());
                result.first->second.bUsedFlag = true;
                result.first->second.inFrameDesc = frameDesc;
                result.first->second.inFrameDesc.timestampNs = 1000 * timestampUs;
                result.first->second.inFrameDesc.appMarkData = appMarkData;
            }
            else
            {
                QC_ERROR( "No empty input buffer available!" );
                ret = QC_STATUS_NOMEM;
            }
        }
        else /* false == m_bInputDynamicMode */
        {
            if ( ( m_inputMap.end() != it ) && ( false == it->second.bUsedFlag ) )
            {
                QC_DEBUG( "find handle 0x%x", handle );
                it->second.bUsedFlag = true;
                it->second.inFrameDesc.timestampNs = 1000 * timestampUs;
                it->second.inFrameDesc.appMarkData = appMarkData;
            }
            else
            {
                QC_ERROR( "No empty input buffer available!" );
                ret = QC_STATUS_NOMEM;
            }
        }
    } /* m_inLock released */

    if ( QC_STATUS_OK == ret )
    {
        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_EMPTY_INPUT_BUFFER, (uint8_t *) ( &frameData ),
                           sizeof( frameData ), nullptr, 0 );
        if ( VIDC_ERR_NONE == rc )
        {
            QC_DEBUG( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER succeeded. frameTs: %" PRIu64
                      " inputMapSsize: %zu", timestampUs, m_inputMap.size() );
        }
        else
        {
            QC_ERROR( "SubmitInputFrame VIDC_IOCTL_EMPTY_INPUT_BUFFER failed! rc=0x%x", rc );
            std::unique_lock<std::mutex> auto_lock( m_inLock );
            m_inputMap[handle].bUsedFlag = false;
            ret = QC_STATUS_FAIL;
            /* m_inLock released */
        }
    }

    return ret;
}

QCStatus_e VidcDrvClient::FillBuffer( VideoFrameDescriptor &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t rc = 0;
    vidc_frame_data_type frameData = {};
    uint64_t handle = frameDesc.dmaHandle;

    QC_DEBUG( "fill buffer begin" );

    if ( QC_STATUS_OK == ret )
    {
        std::unique_lock<std::mutex> auto_lock( m_outLock );

        auto it = m_outputMap.find( handle );

        if ( true == m_bOutputDynamicMode )
        {
            if ( m_outputMap.end() != it )
            {
                QC_DEBUG( "find handle 0x%x", handle );
                if ( false == it->second.bUsedFlag )
                {
                    it->second.bUsedFlag = true;
                    it->second.outFrameDesc.timestampNs = frameDesc.timestampNs;
                    it->second.outFrameDesc.appMarkData = frameDesc.appMarkData;
                }
                else
                {
                    QC_ERROR( "input buffer not available now!" );
                    ret = QC_STATUS_NOMEM;
                }
            }
            else if ( m_outputMap.size() < m_bufNum[VIDEO_CODEC_BUF_OUTPUT] )
            {
                auto result = m_outputMap.emplace(handle, VideoCodec_OutputInfo_t());
                result.first->second.bUsedFlag = true;
                result.first->second.outFrameDesc = frameDesc;
                result.first->second.outFrameDesc.timestampNs = frameDesc.timestampNs;
                result.first->second.outFrameDesc.appMarkData = frameDesc.appMarkData;
            }
            else
            {
                QC_ERROR( "No empty input buffer available!" );
                ret = QC_STATUS_NOMEM;
            }
        }
        else /* false == m_bOutputDynamicMode */
        {
            if ( ( m_outputMap.end() != it ) && ( false == it->second.bUsedFlag ) )
            {
                QC_DEBUG( "find handle 0x%x", handle );
                it->second.bUsedFlag = true;
            }
            else
            {
                QC_ERROR( "No empty output buffer available!" );
                ret = QC_STATUS_NOMEM;
            }
        }
    } /* m_outLock released */

    if ( QC_STATUS_OK == ret )
    {
        frameData.buf_type = VIDC_BUFFER_OUTPUT;
        frameData.frame_addr = (uint8_t *) frameDesc.pBuf;
        frameData.alloc_len = frameDesc.size;
#if defined( __QNXNTO__ )
        frameData.frame_handle = (pmem_handle_t) handle;
#else
        frameData.frame_handle = (int) reinterpret_cast<uint64_t>( handle );
#endif
        frameData.frm_clnt_data = handle;

        QC_DEBUG( "FillBuffer: frame_handle=0x%x , frameData.frame_addr=0x%x "
                  "frameData.alloc_len %" PRIu32 " frameData.data_len=%" PRIu32,
                  handle, frameData.frame_addr, frameData.alloc_len, frameData.data_len );

        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_FILL_OUTPUT_BUFFER, (uint8_t *) ( &frameData ),
                           sizeof( frameData ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            QC_ERROR( "FillBuffer 0x%x failed rc 0x%x", handle, rc );
            m_outputMap[handle].bUsedFlag = false;
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "%s-output-begin, handle 0x%x",
                      (m_encDecType == VIDEO_ENC) ? "enc" : "dec", handle );
        }
    }

    QC_DEBUG( "fill buffer done" );

    return ret;
}

QCStatus_e VidcDrvClient::StopDecoder()
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t rc = 0;
    vidc_stop_mode_type stop_mode = VIDC_STOP_UNUSED;

    QC_DEBUG( "%s: Stopping vidc!", (m_encDecType == VIDEO_ENC) ? "enc" : "dec" );

    InitCmdCompleted();
    m_bCmdDrainReceived = false;
    rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_DRAIN, nullptr, 0, nullptr, 0 );
    if ( VIDC_ERR_NONE != rc )
    {
        QC_ERROR( "Stop vidc failed! rc=0x%x", rc );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        /* drain event received firstly, then receive last flag event */
        ret = WaitForCmdCompleted( VIDEO_CODEC_COMMAND_LAST_FLAG, WAIT_TIMEOUT_10_MSEC );
        if ( ( QC_STATUS_OK != ret ) || ( m_bCmdDrainReceived == false ) )
        {
            QC_ERROR( "WaitFor last flag fail or drain recv:%d", m_bCmdDrainReceived );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        InitCmdCompleted();
        stop_mode = VIDC_STOP_INPUT;
        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_STOP, (uint8 *) &stop_mode,
                           sizeof( vidc_stop_mode_type ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            QC_ERROR( "vidc stop input failed! rc=0x%x", rc );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            ret = WaitForCmdCompleted( VIDEO_CODEC_COMMAND_INPUT_STOP, WAIT_TIMEOUT_10_MSEC );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "WaitFor stop input timeout" );
                ret = QC_STATUS_FAIL;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        InitCmdCompleted();
        stop_mode = VIDC_STOP_OUTPUT;
        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_STOP, (uint8 *) &stop_mode,
                           sizeof( vidc_stop_mode_type ), nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            QC_ERROR( "vidc stop output failed! rc=0x%x", rc );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            ret = WaitForCmdCompleted( VIDEO_CODEC_COMMAND_OUTPUT_STOP, WAIT_TIMEOUT_10_MSEC );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "WaitFor stop output timeout" );
                ret = QC_STATUS_FAIL;
            }
        }
    }

    return ret;
}

QCStatus_e VidcDrvClient::StopEncoder()
{
    QCStatus_e ret = QC_STATUS_OK;

    InitCmdCompleted();
    int32_t rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_STOP, nullptr, 0, nullptr, 0 );
    if ( VIDC_ERR_NONE != rc )
    {
        QC_ERROR( "Stop vidc failed! rc=0x%x", rc );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        ret = WaitForCmdCompleted( VIDEO_CODEC_COMMAND_STOP, WAIT_TIMEOUT_10_MSEC );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "WaitFor stop output timeout" );
            ret = QC_STATUS_FAIL;
        }
    }

    QC_DEBUG( "enc: Stop vidc done" );
    return ret;
}

int VidcDrvClient::DeviceCbHandler( uint8_t *pMsg, uint32_t length )
{
    int ret = 0;

    if (nullptr == pMsg || length < sizeof(vidc_drv_msg_info_type))
    {
        QC_ERROR("Invalid callback parameters: pMsg=%p, length=%u", pMsg, length);
        ret = -1;
    }

    if (0 == ret)
    {
        vidc_drv_msg_info_type *pEvent = (vidc_drv_msg_info_type *) pMsg;
        vidc_frame_data_type *pFrameData = nullptr;

        switch ( pEvent->event_type )
        {
        case VIDC_EVT_RESP_FLUSH_INPUT_DONE:
            QC_DEBUG("Vidc event VIDC_EVT_RESP_FLUSH_INPUT_DONE");
            m_eventCb( VIDEO_CODEC_EVT_FLUSH_INPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_INPUT_RECONFIG:
            QC_DEBUG("Vidc event VIDC_EVT_INPUT_RECONFIG");
            QC_ERROR( "not support input-reconfig" );
            m_eventCb( VIDEO_CODEC_EVT_INPUT_RECONFIG, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_INPUT_DONE:
            QC_DEBUG("Vidc event VIDC_EVT_RESP_INPUT_DONE");
            pFrameData = &pEvent->payload.frame_data;
            if (nullptr == pFrameData || nullptr == pFrameData->frame_addr)
            {
                QC_ERROR("Invalid frame data in input done event");
                ret = -1;
                break;
            }
            QC_DEBUG( "%s-input-done: handle 0x%x",
                      (m_encDecType == VIDEO_ENC) ? "enc" : "dec", pFrameData->frm_clnt_data );
            {
                VideoCodec_InputInfo_t inputInfo;
                bool found = false;

                {
                    std::unique_lock<std::mutex> lck( m_inLock );
                    if ( m_inputMap.end() != m_inputMap.find( pFrameData->frm_clnt_data ) )
                    {
                        m_inputMap[pFrameData->frm_clnt_data].bUsedFlag = false;
                        inputInfo = m_inputMap[pFrameData->frm_clnt_data];
                        found = true;
                    }
                } // Release @lck

                if ( found )
                {
                    m_inputDoneCb( inputInfo.inFrameDesc, m_pAppPriv );
                }
                else
                {
                    QC_ERROR( "input handle = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_CODEC_EVT_ERROR, pEvent, m_pAppPriv );
                }
            }
            break;
        case VIDC_EVT_RESP_OUTPUT_DONE:
            QC_DEBUG("Vidc event VIDC_EVT_RESP_OUTPUT_DONE");
            pFrameData = &pEvent->payload.frame_data;
            QC_DEBUG( "%s-output-done: handle 0x%x ts %" PRIu64
                      " size %u alloc_len %u frameType %" PRIu64 " "
                      "pFrameData %p frame_addr %p flag 0x%x ",
                      (m_encDecType == VIDEO_ENC) ? "enc" : "dec",
                      pFrameData->frm_clnt_data, pFrameData->timestamp,
                      pFrameData->data_len, pFrameData->alloc_len, pFrameData->frame_type,
                      pFrameData, pFrameData->frame_addr, pFrameData->flags );

            if ( nullptr != pFrameData->frame_addr )
            {
                VideoCodec_OutputInfo_t outputInfo;
                bool found = false;
                {
                    std::unique_lock<std::mutex> lck( m_outLock );
                    if ( m_outputMap.end() != m_outputMap.find( pFrameData->frm_clnt_data ) )
                    {
                        m_outputMap[pFrameData->frm_clnt_data].bUsedFlag = false;
                        outputInfo = m_outputMap[pFrameData->frm_clnt_data];
                        found = true;
                    }
                    // release @lck
                }
                if ( found )
                {
                    QC_DEBUG( "Frame is ready - calling callback!" );
                    if ( (uint8_t *) outputInfo.outFrameDesc.pBuf == pFrameData->frame_addr )
                    {
                        outputInfo.outFrameDesc.validSize = pFrameData->data_len;
                        outputInfo.outFrameDesc.appMarkData = (uint64_t) pFrameData->mark_data;
                        outputInfo.outFrameDesc.timestampNs = pFrameData->timestamp * 1000;
                        outputInfo.outFrameDesc.frameFlag = pFrameData->flags;
                        outputInfo.outFrameDesc.frameType = pFrameData->frame_type;
                        m_outputDoneCb( outputInfo.outFrameDesc, m_pAppPriv );
                    }
                    else
                    {
                        QC_ERROR( "output handle = 0x%x, frame_addr %p does not match with "
                                        "buffer data %p",
                                        pFrameData->frm_clnt_data, pFrameData->frame_addr,
                                        (uint8_t *) outputInfo.outFrameDesc.pBuf);
                        m_eventCb( VIDEO_CODEC_EVT_ERROR, pEvent, m_pAppPriv );
                    }
                }
                else
                {
                    QC_ERROR( "output handle = 0x%x is invalid", pFrameData->frm_clnt_data );
                    m_eventCb( VIDEO_CODEC_EVT_ERROR, pEvent, m_pAppPriv );
                }
            }

            if ( ( pFrameData->flags & VIDC_FRAME_FLAG_EOS ) > 0 )
            {
                QC_WARN( "detected VIDC_FRAME_FLAG_EOS -- not possible for camera streaming" );
            }

            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT_DONE:
            QC_DEBUG("Vidc event VIDC_EVT_RESP_FLUSH_OUTPUT_DONE");
            m_eventCb( VIDEO_CODEC_EVT_FLUSH_OUTPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_FLUSH_OUTPUT2_DONE:
            QC_DEBUG("Vidc event VIDC_EVT_RESP_FLUSH_OUTPUT2_DONE");
            m_eventCb( VIDEO_CODEC_EVT_FLUSH_OUTPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_OUTPUT_RECONFIG:
            QC_INFO( "output-reconfig received" );
            m_eventCb( VIDEO_CODEC_EVT_OUTPUT_RECONFIG, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_INFO_OUTPUT_RECONFIG:
            QC_DEBUG("Vidc event VIDC_EVT_INFO_OUTPUT_RECONFIG");
            // smooth streaming case - notifies about resolution change (not handled)
            QC_ERROR( "not support info output-reconfig" );
            m_eventCb( VIDEO_CODEC_EVT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_HWFATAL:
            QC_ERROR( "hw fatal" );
            m_eventCb( VIDEO_CODEC_EVT_FATAL, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_ERR_CLIENTFATAL:
            QC_ERROR( "client fatal" );
            m_eventCb( VIDEO_CODEC_EVT_FATAL, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_START:
            QC_INFO( "resp-start received" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_START_ALL;
            m_eventCb( VIDEO_CODEC_EVT_RESP_START, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_START_INPUT_DONE:
            QC_INFO( "START_INPUT_DONE event received" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_START_INPUT;
            m_eventCb( VIDEO_CODEC_EVT_RESP_START_INPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_START_OUTPUT_DONE:
            QC_INFO( "START_OUTPUT_DONE event received" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_START_OUTPUT;
            m_eventCb( VIDEO_CODEC_EVT_RESP_START_OUTPUT_DONE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_PAUSE:
            QC_INFO( "RESP_PAUSE event received" );
            m_eventCb( VIDEO_CODEC_EVT_RESP_PAUSE, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_RESUME:
            QC_INFO( "RESP_RESUME event received" );
            m_eventCb( VIDEO_CODEC_EVT_RESP_RESUME, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_LOAD_RESOURCES:
            QC_INFO( "vidc event: load resources done" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_LOAD_RESOURCE;
            m_eventCb( VIDEO_CODEC_EVT_RESP_LOAD_RESOURCES, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_RELEASE_RESOURCES:
            QC_INFO( "vidc event: release resources done" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_RELEASE_RESOURCE;
            m_eventCb( VIDEO_CODEC_EVT_RESP_RELEASE_RESOURCES, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RELEASE_BUFFER_REFERENCE:
            QC_ERROR( "Release buffer reference event" );
            m_eventCb( VIDEO_CODEC_EVT_ERROR, pEvent, m_pAppPriv );
            break;
        case VIDC_EVT_RESP_DRAIN:
            QC_INFO( "vidc event: drain done" );
            m_bCmdDrainReceived = true;
            break;
        case VIDC_EVT_LAST_FLAG:
            QC_INFO( "vidc event: last flag" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_LAST_FLAG;
            break;
        case VIDC_EVT_RESP_STOP_INPUT_DONE:
            QC_INFO( "vidc event: stop input done" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_INPUT_STOP;
            break;
        case VIDC_EVT_RESP_STOP_OUTPUT_DONE:
            QC_INFO( "vidc event: stop output done" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_OUTPUT_STOP;
            break;
        case VIDC_EVT_RESP_STOP:
            QC_INFO( "vidc event: stop done" );
            m_cmdCompleted = VIDEO_CODEC_COMMAND_STOP;
            m_eventCb( VIDEO_CODEC_EVT_RESP_STOP, pEvent, m_pAppPriv );
            break;
        default:
            QC_ERROR( "Unknown event_type: %d", pEvent->event_type );
            ret = -1;
            break;
        }
    }

    return ret;
}

int VidcDrvClient::DeviceCallback( uint8_t *pMsg, uint32_t length, void *pCdata )
{
    VidcDrvClient *self = (VidcDrvClient *) pCdata;
    int ret;

    if (nullptr != self)
    {
        ret = self->DeviceCbHandler( pMsg, length );
    }
    else
    {
        ret = -1;
    }

    return ret;
}

QCStatus_e VidcDrvClient::GetDrvProperty( uint32_t id, uint32_t nPktSize, const uint8_t &pkt )
{
    int32_t rc = 0;
    QCStatus_e ret = QC_STATUS_OK;
    uint8_t dev_cmd_buffer[VIDEO_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    uint32_t nMsgSize = sizeof( vidc_property_hdr_type ) + nPktSize;
    vidc_property_id_type propId = (vidc_property_id_type) id;

    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    if ( nPktSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE || nMsgSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE )
    {
        printf( "GetDrvProperty nPktSize=0x%x is too large", nPktSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        (void) memcpy( pProp->payload, &pkt, nPktSize );
        pProp->prop_hdr.size = nPktSize;
        pProp->prop_hdr.prop_id = propId;

        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_GET_PROPERTY, dev_cmd_buffer, (int32_t) nMsgSize,
                           (uint8_t*) &pkt, nPktSize );
        if ( VIDC_ERR_NONE != rc )
        {
            printf( "GetDrvProperty propId=0x%x failed! rc=0x%x\n", propId, rc );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e VidcDrvClient::SetDrvProperty( uint32_t id, uint32_t nPktSize, const uint8_t &pkt )
{
    int32_t rc = 0;
    QCStatus_e ret = QC_STATUS_OK;
    uint8_t dev_cmd_buffer[VIDEO_MAX_DEV_CMD_BUFFER_SIZE] = { 0 };
    vidc_drv_property_type *pProp = (vidc_drv_property_type *) dev_cmd_buffer;
    uint32_t nMsgSize = sizeof( vidc_property_hdr_type ) + nPktSize;
    vidc_property_id_type propId = (vidc_property_id_type) id;

    // pProp->payload buffer is more than 1 byte as the struct defined;
    // this is the technique to handle variable name size and the struct memory
    // is more than the struct size
    if ( nPktSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE || nMsgSize > VIDEO_MAX_DEV_CMD_BUFFER_SIZE )
    {
        QC_ERROR( "SetDrvProperty nPktSize=0x%x is too large", nPktSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        (void) memcpy( pProp->payload, &pkt, nPktSize );

        pProp->prop_hdr.size = nPktSize;
        pProp->prop_hdr.prop_id = propId;

        rc = device_ioctl( m_pIoHandle, VIDC_IOCTL_SET_PROPERTY, dev_cmd_buffer, (int32_t) nMsgSize,
                           nullptr, 0 );
        if ( VIDC_ERR_NONE != rc )
        {
            QC_ERROR( "SetDrvProperty propId=0x%x failed! rc=0x%x, %s", propId, rc,
                      VidcErrToStr( vidc_status_type( rc ) ) );
            ret = QC_STATUS_FAIL;
        }
    }
    return ret;
}

QCStatus_e VidcDrvClient::WaitForCmdCompleted( VideoCodec_CommandType_e expectedCmd,
                                               int32_t timeoutMs )
{
    int32_t counter = 0;
    QCStatus_e ret = QC_STATUS_OK;

    while ( m_cmdCompleted != expectedCmd )
    {
        (void) MM_Timer_Sleep( 1 );
        counter++;
        if ( counter > timeoutMs )
        {
            QC_ERROR( "WaitForCmd %d timeout, %d ms used", expectedCmd, timeoutMs );
            ret = QC_STATUS_TIMEOUT;
            break;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "WaitForCmd %d done, %d ms used", expectedCmd, counter );
    }
    return ret;
}

}   // Node
}   // namespace QC
