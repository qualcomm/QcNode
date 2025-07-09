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
#include "QC/component/VideoDecoder.hpp"
#include "QC/Infras/Log/Logger.hpp"

namespace QC
{
namespace component
{

static constexpr uint16_t VIDEO_DECODER_DEFAULT_FRAME_RATE = 30;

#define VIDEO_DECODER_MAX_BUFFER_REQ 64
#define VIDEO_DECODER_MIN_BUFFER_REQ 4

QCStatus_e VideoDecoder::Init( const char *pName, const VideoDecoder_Config_t *pConfig,
                               Logger_Level_e level )
{
    int32_t i;
    QCStatus_e ret = QC_STATUS_OK;

    ret = VidcCompBase::Init( pName, VIDEO_DEC, level );

    if ( QC_STATUS_OK == ret )
    {
        ret = ValidateConfig( pName, pConfig );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;

        QC_INFO( "video-decoder %s init begin", pName );
        ret = InitFromConfig( pConfig );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_drvClient.Init( pName, level, VIDEO_DEC );
        ret = m_drvClient.OpenDriver( VideoDecoder::InFrameCallback, VideoDecoder::OutFrameCallback,
                                      VideoDecoder::EventCallback, (void *) this );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = InitDrvProperty();
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = VidcCompBase::NegotiateBufferReq( VIDEO_CODEC_BUF_INPUT );
    }

    if ( ( QC_STATUS_OK == ret ) && m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_INPUT] )
    {
        for ( i = 0; i < m_bufNum[VIDEO_CODEC_BUF_INPUT]; i++ )
        {
            ret = CheckBuffer( &pConfig->pInputBufferList[i], VIDEO_CODEC_BUF_INPUT );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "validate VIDC_BUFFER_INPUT failed" );
                break;
            }
        }
    }

    if ( ( QC_STATUS_OK == ret ) && m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_OUTPUT] )
    {
        for ( i = 0; i < m_bufNum[VIDEO_CODEC_BUF_OUTPUT]; i++ )
        {
            ret = CheckBuffer( &pConfig->pOutputBufferList[i], VIDEO_CODEC_BUF_OUTPUT );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "validate VIDC_BUFFER_OUTPUT failed" );
                break;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = VidcCompBase::PostInit( pConfig->pInputBufferList, pConfig->pOutputBufferList );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "video-decoder init done" );
    }
    else
    {
        QC_ERROR( "Something wrong happened in Init, Deiniting vidc" );
        m_state = QC_OBJECT_STATE_ERROR;
        (void) Deinit();
    }

    return ret;
}

QCStatus_e VideoDecoder::InitDrvProperty()
{
    VidcCodecMeta_t meta;
    meta.width = m_width;
    meta.height = m_height;
    meta.frameRate = m_frameRate;
    if ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_inFormat )
    {
        meta.codecType = VIDEO_CODEC_H265;
    }
    else
    {
        meta.codecType = VIDEO_CODEC_H264;
    }

    QCStatus_e ret = m_drvClient.InitDriver( meta );
    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_COLOR_FORMAT" );
        vidc_color_format_config_type vidcColorFmt;
        vidcColorFmt.buf_type = VIDC_BUFFER_OUTPUT;
        vidcColorFmt.color_format = VidcCompBase::GetVidcFormat( m_outFormat );
        ret = m_drvClient.SetDrvProperty( VIDC_I_COLOR_FORMAT,
                                          sizeof( vidc_color_format_config_type ),
                                          (uint8_t *) &vidcColorFmt );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_DEC_ORDER_DECODE" );
        vidc_output_order_type order;
        order.output_order = VIDC_DEC_ORDER_DECODE;
        ret = m_drvClient.SetDrvProperty( VIDC_I_DEC_OUTPUT_ORDER, sizeof( vidc_output_order_type ),
                                          (uint8_t *) &order );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_DEC_CONT_ON_RECONFIG" );
        vidc_enable_type enable;
        enable.enable = true;
        ret = m_drvClient.SetDrvProperty( VIDC_I_DEC_CONT_ON_RECONFIG, sizeof( vidc_enable_type ),
                                          (uint8_t *) &enable );
    }

    return ret;
}

QCStatus_e VideoDecoder::CheckBuffer( const QCSharedBuffer_t *pBuffer,
                                      VideoCodec_BufType_e bufferType )
{
    QCStatus_e ret = ValidateBuffer( pBuffer, bufferType );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "validate buffer failed, type:%d", bufferType );
    }
    else
    {
        if ( bufferType == VIDEO_CODEC_BUF_INPUT )
        {
            if ( 0 == pBuffer->size )
            {
                QC_INFO( "pBuffer size %zu is invalid", pBuffer->size );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
        else
        {
            if ( pBuffer->size < (size_t) m_bufSize[VIDEO_CODEC_BUF_OUTPUT] )
            {
                QC_ERROR( "pBuffer size %zu is smaller than vidcOutputBufferSize %" PRIu32,
                          pBuffer->size, m_bufSize[VIDEO_CODEC_BUF_OUTPUT] );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
    }

    return ret;
}

QCStatus_e VideoDecoder::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( nullptr == m_inputDoneCb ) || ( nullptr == m_outputDoneCb ) )
    {
        QC_ERROR( "Not start since callback is not registered!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = VidcCompBase::Start();
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "dec start succeed" );
    }

    return ret;
}

QCStatus_e VideoDecoder::HandleOutputReconfig()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_OutputReconfigInprogress = true;

    QC_INFO( "handle output-reconfig begin" );

    ret = VidcCompBase::NegotiateBufferReq( VIDEO_CODEC_BUF_OUTPUT );

    if ( ( QC_STATUS_OK == ret ) && ( false == m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] ) )
    {
        ret = VidcCompBase::SetBuffer( VIDEO_CODEC_BUF_OUTPUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.StartDriver( VIDEO_CODEC_START_OUTPUT );
    }

    return ret;
}

QCStatus_e VideoDecoder::FinishOutputReconfig()
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t i;

    if ( m_OutputReconfigInprogress )
    {
        m_OutputReconfigInprogress = false;

        if ( false == m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] )
        {
            for ( i = 0; i < m_bufNum[VIDEO_CODEC_BUF_OUTPUT]; i++ )
            {
                VideoDecoder_OutputFrame_t outputFrame;
                outputFrame.sharedBuffer = m_pOutputList[i];
                ret = SubmitOutputFrame( &outputFrame );
                if ( QC_STATUS_OK != ret )
                {
                    break;
                }
            }
        }

        QC_INFO( "handle output-reconfig done" );
    }
    return ret;
}

QCStatus_e VideoDecoder::SubmitInputFrame( const VideoDecoder_InputFrame_t *pInput )
{
    QCStatus_e ret = QC_STATUS_OK;
    const QCSharedBuffer_t *inputBuffer = nullptr;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_WARN( "Not submitting inputBuffer since decoder is not running" );
        ret = QC_STATUS_BAD_STATE;
    }

    if ( ( QC_STATUS_OK == ret ) &&
         ( ( nullptr == pInput ) || ( nullptr == pInput->sharedBuffer.data() ) ) )
    {
        QC_ERROR( "Not submitting empty inputBuffer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        inputBuffer = &pInput->sharedBuffer;
        ret = CheckBuffer( inputBuffer, VIDEO_CODEC_BUF_INPUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.EmptyBuffer( &pInput->sharedBuffer, pInput->timestampNs / 1000,
                                       pInput->appMarkData );
    }

    QC_DEBUG( "SubmitInputFrame done" );

    return ret;
}

/*
 * For output buffer none dynamic mode:
 *   this function must be called in OutFrameCallback when output buffer return back from driver.
 */
QCStatus_e VideoDecoder::SubmitOutputFrame( const VideoDecoder_OutputFrame_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;
    const QCSharedBuffer_t *outputBuffer = nullptr;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_WARN( "Not submitting outputBuffer since decoder not running" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( false == m_OutputStarted )
    {
        QC_ERROR( "output not started" );
        ret = QC_STATUS_FAIL;
    }
    else if ( m_OutputReconfigInprogress )
    {
        /* when handle output-reconfig, app submitOutputFrame should be forbiden */
        QC_ERROR( "Not submitting outputBuffer since doing output reconfig" );
        ret = QC_STATUS_FAIL;
    }

    if ( ( QC_STATUS_OK == ret ) && ( nullptr == pOutput ) )
    {
        QC_ERROR( "Not submitting empty outputBuffer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "check buffer begin" );
        outputBuffer = &pOutput->sharedBuffer;
        ret = CheckBuffer( outputBuffer, VIDEO_CODEC_BUF_OUTPUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.FillBuffer( outputBuffer );
    }

    QC_DEBUG( "SubmitOutputFrame done" );

    return ret;
}

QCStatus_e VideoDecoder::Stop()
{
    QCStatus_e ret = VidcCompBase::Stop();

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "dec stop succeed" );
    }
    else
    {
        QC_ERROR( "dec stop failed" );
    }

    return ret;
}

QCStatus_e VideoDecoder::Deinit()
{
    QCStatus_e ret = VidcCompBase::Deinit();

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "dec Deinit succeed" );
    }
    else
    {
        QC_ERROR( "dec Deinit failed" );
    }

    return ret;
}

QCStatus_e VideoDecoder::RegisterCallback( VideoDecoder_InFrameCallback_t inputDoneCb,
                                           VideoDecoder_OutFrameCallback_t outputDoneCb,
                                           VideoDecoder_EventCallback_t eventCb, void *pAppPriv )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( nullptr == inputDoneCb ) || ( nullptr == outputDoneCb ) )
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
    }
    return ret;
}

QCStatus_e VideoDecoder::ValidateConfig( const char *name, const VideoDecoder_Config_t *cfg )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == name || nullptr == cfg )
    {
        QC_ERROR( "cfg is null pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        size_t len = strlen( name );
        if ( ( len < 2 ) || ( len > 128 ) )
        {
            QC_ERROR( "name length %" PRIu32 " not in range [2, 16] ", len );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( cfg->width < 128 ) || ( cfg->height < 128 ) || ( cfg->width > 8192 ) ||
             ( cfg->height > 8192 ) )
        {
            QC_ERROR( "width %" PRIu32 " height %" PRIu32 " not in [128, 8192] ", cfg->width,
                      cfg->height );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( QC_IMAGE_FORMAT_COMPRESSED_H265 != cfg->inFormat ) &&
             ( QC_IMAGE_FORMAT_COMPRESSED_H264 != cfg->inFormat ) )
        {
            QC_ERROR( "input format: %d not supported!", cfg->inFormat );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( QC_IMAGE_FORMAT_NV12 != cfg->outFormat ) &&
             ( QC_IMAGE_FORMAT_NV12_UBWC != cfg->outFormat ) &&
             ( QC_IMAGE_FORMAT_P010 != cfg->outFormat ) )
        {
            QC_ERROR( "output format: %d not supported!", cfg->outFormat );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( cfg->numInputBuffer > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
                                      ( cfg->numInputBuffer < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numInputBuffer: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  cfg->numInputBuffer, VIDEO_DECODER_MIN_BUFFER_REQ, VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
        m_bufNum[VIDEO_CODEC_BUF_INPUT] = 0;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( cfg->numOutputBuffer > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
                                      ( cfg->numOutputBuffer < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numOutputBuffer: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  cfg->numOutputBuffer, VIDEO_DECODER_MIN_BUFFER_REQ,
                  VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
        m_bufNum[VIDEO_CODEC_BUF_OUTPUT] = 0;
    }

    if ( ( QC_STATUS_OK == ret ) && ( true == cfg->bInputDynamicMode ) &&
         ( nullptr != cfg->pInputBufferList ) )
    {
        QC_ERROR( "should not provide inputbuffer in config in dynamic mode!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( true == cfg->bOutputDynamicMode ) &&
         ( nullptr != cfg->pOutputBufferList ) )
    {
        QC_ERROR( "should not provide outputbuffer in config in dynamic mode!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e VideoDecoder::InitFromConfig( const VideoDecoder_Config_t *cfg )
{
    m_width = cfg->width;
    m_height = cfg->height;
    m_frameRate = ( cfg->frameRate > 0 ) ? cfg->frameRate : VIDEO_DECODER_DEFAULT_FRAME_RATE;

    m_inFormat = cfg->inFormat;
    m_outFormat = cfg->outFormat;
    m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] = cfg->bInputDynamicMode;
    m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] = cfg->bOutputDynamicMode;
    m_bufNum[VIDEO_CODEC_BUF_INPUT] = cfg->numInputBuffer;
    m_bufNum[VIDEO_CODEC_BUF_OUTPUT] = cfg->numOutputBuffer;

    if ( ( false == m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] ) &&
         ( nullptr != cfg->pInputBufferList ) )
    {
        m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_INPUT] = true;
    }
    else
    {
        m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_INPUT] = false;
    }
    if ( ( false == m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] ) &&
         ( nullptr != cfg->pOutputBufferList ) )
    {
        m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_OUTPUT] = true;
    }
    else
    {
        m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_OUTPUT] = false;
    }

    QC_INFO( "dec-init: w:%u, h:%u, fps:%u, inbuf: dynamicMode:%d, num:%u, outbuf: "
             "dynamicMode:%d, num:%u",
             m_width, m_height, m_frameRate, m_bDynamicMode[VIDEO_CODEC_BUF_INPUT],
             m_bufNum[VIDEO_CODEC_BUF_INPUT], m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT],
             m_bufNum[VIDEO_CODEC_BUF_OUTPUT] );

    return QC_STATUS_OK;
}

void VideoDecoder::InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame )
{
    VideoDecoder_InputFrame_t inputFrame;
    inputFrame.sharedBuffer = pInputFrame->sharedBuffer;
    inputFrame.timestampNs = pInputFrame->timestampUs * 1000;
    inputFrame.appMarkData = pInputFrame->appMarkData;
    if ( nullptr != m_inputDoneCb )
    {
        m_inputDoneCb( &inputFrame, m_pAppPriv );
    }
    else
    {
        QC_ERROR( "m_inputDoneCb is nullptr" );
    }
}

void VideoDecoder::OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame )
{
    VideoDecoder_OutputFrame_t outputFrame;
    outputFrame.sharedBuffer = pOutputFrame->sharedBuffer;
    outputFrame.appMarkData = pOutputFrame->appMarkData;
    outputFrame.timestampNs = pOutputFrame->timestampUs * 1000;   // convert us to ns
    outputFrame.frameFlag = pOutputFrame->frameFlag;
    if ( nullptr != m_outputDoneCb )
    {
        m_outputDoneCb( &outputFrame, m_pAppPriv );
    }
    else
    {
        QC_ERROR( "m_outputDoneCb is nullptr" );
    }
}

void VideoDecoder::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload )
{
    QC_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );

    switch ( eventId )
    {
        case VIDEO_CODEC_EVT_OUTPUT_RECONFIG:
            if ( QC_STATUS_OK != HandleOutputReconfig() )
            {
                m_state = QC_OBJECT_STATE_ERROR;
                QC_ERROR( "handle output-reconfig failed" );
            }
            break;
        case VIDEO_CODEC_EVT_RESP_START_INPUT_DONE:
            m_state = QC_OBJECT_STATE_RUNNING;
            break;
        case VIDEO_CODEC_EVT_RESP_START_OUTPUT_DONE:
            m_state = QC_OBJECT_STATE_RUNNING;
            m_OutputStarted = true;
            (void) FinishOutputReconfig();
            break;
        default:
            VidcCompBase::EventCallback( eventId, pPayload );
            break;
    }

    if ( m_eventCb )
    {
        m_eventCb( eventId, pPayload, m_pAppPriv );
    }
}

void VideoDecoder::InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame, void *pPrivData )
{
    VideoDecoder *self = (VideoDecoder *) pPrivData;
    self->InFrameCallback( pInputFrame );
}

void VideoDecoder::OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame, void *pPrivData )
{
    VideoDecoder *self = (VideoDecoder *) pPrivData;
    self->OutFrameCallback( pOutputFrame );
}

void VideoDecoder::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload,
                                  void *pPrivData )
{
    VideoDecoder *self = (VideoDecoder *) pPrivData;
    self->EventCallback( eventId, pPayload );
}

}   // namespace component
}   // namespace QC
