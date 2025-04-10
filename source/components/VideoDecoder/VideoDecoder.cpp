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

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/Types.hpp"
#include "ridehal/component/VideoDecoder.hpp"

namespace ridehal
{
namespace component
{

static constexpr uint16_t VIDEO_DECODER_DEFAULT_FRAME_RATE = 30;

#define VIDEO_DECODER_MAX_BUFFER_REQ 64
#define VIDEO_DECODER_MIN_BUFFER_REQ 4

RideHalError_e VideoDecoder::Init( const char *pName, const VideoDecoder_Config_t *pConfig,
                                   Logger_Level_e level )
{
    int32_t i;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = VidcCompBase::Init( pName, VIDEO_DEC, level );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ValidateConfig( pName, pConfig );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;

        RIDEHAL_INFO( "video-decoder %s init begin", pName );
        ret = InitFromConfig( pConfig );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_drvClient.Init( pName, level, VIDEO_DEC );
        ret = m_drvClient.OpenDriver( VideoDecoder::InFrameCallback, VideoDecoder::OutFrameCallback,
                                      VideoDecoder::EventCallback, (void *) this );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = InitDrvProperty();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = VidcCompBase::NegotiateBufferReq( VIDEO_CODEC_BUF_INPUT );
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_INPUT] )
    {
        for ( i = 0; i < m_bufNum[VIDEO_CODEC_BUF_INPUT]; i++ )
        {
            ret = CheckBuffer( &pConfig->pInputBufferList[i], VIDEO_CODEC_BUF_INPUT );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "validate VIDC_BUFFER_INPUT failed" );
                break;
            }
        }
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_OUTPUT] )
    {
        for ( i = 0; i < m_bufNum[VIDEO_CODEC_BUF_OUTPUT]; i++ )
        {
            ret = CheckBuffer( &pConfig->pOutputBufferList[i], VIDEO_CODEC_BUF_OUTPUT );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "validate VIDC_BUFFER_OUTPUT failed" );
                break;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = VidcCompBase::PostInit( pConfig->pInputBufferList, pConfig->pOutputBufferList );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "video-decoder init done" );
    }
    else
    {
        RIDEHAL_ERROR( "Something wrong happened in Init, Deiniting vidc" );
        m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        (void) Deinit();
    }

    return ret;
}

RideHalError_e VideoDecoder::InitDrvProperty()
{
    VidcCodecMeta_t meta;
    meta.width = m_width;
    meta.height = m_height;
    meta.frameRate = m_frameRate;
    if ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265 == m_inFormat )
    {
        meta.codecType = VIDEO_CODEC_H265;
    }
    else
    {
        meta.codecType = VIDEO_CODEC_H264;
    }

    RideHalError_e ret = m_drvClient.InitDriver( meta );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_COLOR_FORMAT" );
        vidc_color_format_config_type vidcColorFmt;
        vidcColorFmt.buf_type = VIDC_BUFFER_OUTPUT;
        vidcColorFmt.color_format = VidcCompBase::GetVidcFormat( m_outFormat );
        ret = m_drvClient.SetDrvProperty( VIDC_I_COLOR_FORMAT,
                                          sizeof( vidc_color_format_config_type ),
                                          (uint8_t *) &vidcColorFmt );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_DEC_ORDER_DECODE" );
        vidc_output_order_type order;
        order.output_order = VIDC_DEC_ORDER_DECODE;
        ret = m_drvClient.SetDrvProperty( VIDC_I_DEC_OUTPUT_ORDER, sizeof( vidc_output_order_type ),
                                          (uint8_t *) &order );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "Setting VIDC_I_DEC_CONT_ON_RECONFIG" );
        vidc_enable_type enable;
        enable.enable = true;
        ret = m_drvClient.SetDrvProperty( VIDC_I_DEC_CONT_ON_RECONFIG, sizeof( vidc_enable_type ),
                                          (uint8_t *) &enable );
    }

    return ret;
}

RideHalError_e VideoDecoder::CheckBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                          VideoCodec_BufType_e bufferType )
{
    RideHalError_e ret = ValidateBuffer( pBuffer, bufferType );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "validate buffer failed, type:%d", bufferType );
    }
    else
    {
        if ( bufferType == VIDEO_CODEC_BUF_INPUT )
        {
            if ( 0 == pBuffer->size )
            {
                RIDEHAL_INFO( "pBuffer size %zu is invalid", pBuffer->size );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
        }
        else
        {
            if ( pBuffer->size < (size_t) m_bufSize[VIDEO_CODEC_BUF_OUTPUT] )
            {
                RIDEHAL_ERROR( "pBuffer size %zu is smaller than vidcOutputBufferSize %" PRIu32,
                               pBuffer->size, m_bufSize[VIDEO_CODEC_BUF_OUTPUT] );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
        }
    }

    return ret;
}

RideHalError_e VideoDecoder::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( nullptr == m_inputDoneCb ) || ( nullptr == m_outputDoneCb ) )
    {
        RIDEHAL_ERROR( "Not start since callback is not registered!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = VidcCompBase::Start();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "dec start succeed" );
    }

    return ret;
}

RideHalError_e VideoDecoder::HandleOutputReconfig()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_OutputReconfigInprogress = true;

    RIDEHAL_INFO( "handle output-reconfig begin" );

    ret = VidcCompBase::NegotiateBufferReq( VIDEO_CODEC_BUF_OUTPUT );

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( false == m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] ) )
    {
        ret = VidcCompBase::SetBuffer( VIDEO_CODEC_BUF_OUTPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_drvClient.StartDriver( VIDEO_CODEC_START_OUTPUT );
    }

    return ret;
}

RideHalError_e VideoDecoder::FinishOutputReconfig()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
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
                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    break;
                }
            }
        }

        RIDEHAL_INFO( "handle output-reconfig done" );
    }
    return ret;
}

RideHalError_e VideoDecoder::SubmitInputFrame( const VideoDecoder_InputFrame_t *pInput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    const RideHal_SharedBuffer_t *inputBuffer = nullptr;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting inputBuffer since decoder is not running" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( nullptr == pInput ) || ( nullptr == pInput->sharedBuffer.data() ) ) )
    {
        RIDEHAL_ERROR( "Not submitting empty inputBuffer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        inputBuffer = &pInput->sharedBuffer;
        ret = CheckBuffer( inputBuffer, VIDEO_CODEC_BUF_INPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_drvClient.EmptyBuffer( &pInput->sharedBuffer, pInput->timestampNs / 1000,
                                       pInput->appMarkData );
    }

    RIDEHAL_DEBUG( "SubmitInputFrame done" );

    return ret;
}

/*
 * For output buffer none dynamic mode:
 *   this function must be called in OutFrameCallback when output buffer return back from driver.
 */
RideHalError_e VideoDecoder::SubmitOutputFrame( const VideoDecoder_OutputFrame_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    const RideHal_SharedBuffer_t *outputBuffer = nullptr;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_WARN( "Not submitting outputBuffer since decoder not running" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( false == m_OutputStarted )
    {
        RIDEHAL_ERROR( "output not started" );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else if ( m_OutputReconfigInprogress )
    {
        /* when handle output-reconfig, app submitOutputFrame should be forbiden */
        RIDEHAL_ERROR( "Not submitting outputBuffer since doing output reconfig" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( nullptr == pOutput ) )
    {
        RIDEHAL_ERROR( "Not submitting empty outputBuffer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_DEBUG( "check buffer begin" );
        outputBuffer = &pOutput->sharedBuffer;
        ret = CheckBuffer( outputBuffer, VIDEO_CODEC_BUF_OUTPUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_drvClient.FillBuffer( outputBuffer );
    }

    RIDEHAL_DEBUG( "SubmitOutputFrame done" );

    return ret;
}

RideHalError_e VideoDecoder::Stop()
{
    RideHalError_e ret = VidcCompBase::Stop();

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "dec stop succeed" );
    }
    else
    {
        RIDEHAL_ERROR( "dec stop failed" );
    }

    return ret;
}

RideHalError_e VideoDecoder::Deinit()
{
    RideHalError_e ret = VidcCompBase::Deinit();

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RIDEHAL_INFO( "dec Deinit succeed" );
    }
    else
    {
        RIDEHAL_ERROR( "dec Deinit failed" );
    }

    return ret;
}

RideHalError_e VideoDecoder::RegisterCallback( VideoDecoder_InFrameCallback_t inputDoneCb,
                                               VideoDecoder_OutFrameCallback_t outputDoneCb,
                                               VideoDecoder_EventCallback_t eventCb,
                                               void *pAppPriv )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( nullptr == inputDoneCb ) || ( nullptr == outputDoneCb ) )
    {
        RIDEHAL_ERROR( "callback is NULL pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_inputDoneCb = inputDoneCb;
        m_outputDoneCb = outputDoneCb;
        m_eventCb = eventCb;
        m_pAppPriv = pAppPriv;
    }
    return ret;
}

RideHalError_e VideoDecoder::ValidateConfig( const char *name, const VideoDecoder_Config_t *cfg )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == name || nullptr == cfg )
    {
        RIDEHAL_ERROR( "cfg is null pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        size_t len = strlen( name );
        if ( ( len < 2 ) || ( len > 128 ) )
        {
            RIDEHAL_ERROR( "name length %" PRIu32 " not in range [2, 16] ", len );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( cfg->width < 128 ) || ( cfg->height < 128 ) || ( cfg->width > 8192 ) ||
             ( cfg->height > 8192 ) )
        {
            RIDEHAL_ERROR( "width %" PRIu32 " height %" PRIu32 " not in [128, 8192] ", cfg->width,
                           cfg->height );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265 != cfg->inFormat ) &&
             ( RIDEHAL_IMAGE_FORMAT_COMPRESSED_H264 != cfg->inFormat ) )
        {
            RIDEHAL_ERROR( "input format: %d not supported!", cfg->inFormat );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( RIDEHAL_IMAGE_FORMAT_NV12 != cfg->outFormat ) &&
             ( RIDEHAL_IMAGE_FORMAT_NV12_UBWC != cfg->outFormat ) &&
             ( RIDEHAL_IMAGE_FORMAT_P010 != cfg->outFormat ) )
        {
            RIDEHAL_ERROR( "output format: %d not supported!", cfg->outFormat );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( cfg->numInputBuffer > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
           ( cfg->numInputBuffer < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        RIDEHAL_ERROR( "numInputBuffer: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       cfg->numInputBuffer, VIDEO_DECODER_MIN_BUFFER_REQ,
                       VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        m_bufNum[VIDEO_CODEC_BUF_INPUT] = 0;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) &&
         ( ( cfg->numOutputBuffer > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
           ( cfg->numOutputBuffer < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        RIDEHAL_ERROR( "numOutputBuffer: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                       "MAX_BUFFER_REQ %d) ",
                       cfg->numOutputBuffer, VIDEO_DECODER_MIN_BUFFER_REQ,
                       VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        m_bufNum[VIDEO_CODEC_BUF_OUTPUT] = 0;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == cfg->bInputDynamicMode ) &&
         ( nullptr != cfg->pInputBufferList ) )
    {
        RIDEHAL_ERROR( "should not provide inputbuffer in config in dynamic mode!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == cfg->bOutputDynamicMode ) &&
         ( nullptr != cfg->pOutputBufferList ) )
    {
        RIDEHAL_ERROR( "should not provide outputbuffer in config in dynamic mode!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e VideoDecoder::InitFromConfig( const VideoDecoder_Config_t *cfg )
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

    RIDEHAL_INFO( "dec-init: w:%u, h:%u, fps:%u, inbuf: dynamicMode:%d, num:%u, outbuf: "
                  "dynamicMode:%d, num:%u",
                  m_width, m_height, m_frameRate, m_bDynamicMode[VIDEO_CODEC_BUF_INPUT],
                  m_bufNum[VIDEO_CODEC_BUF_INPUT], m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT],
                  m_bufNum[VIDEO_CODEC_BUF_OUTPUT] );

    return RIDEHAL_ERROR_NONE;
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
        RIDEHAL_ERROR( "m_inputDoneCb is nullptr" );
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
        RIDEHAL_ERROR( "m_outputDoneCb is nullptr" );
    }
}

void VideoDecoder::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload )
{
    RIDEHAL_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );

    switch ( eventId )
    {
        case VIDEO_CODEC_EVT_OUTPUT_RECONFIG:
            if ( RIDEHAL_ERROR_NONE != HandleOutputReconfig() )
            {
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
                RIDEHAL_ERROR( "handle output-reconfig failed" );
            }
            break;
        case VIDEO_CODEC_EVT_RESP_START_INPUT_DONE:
            m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            break;
        case VIDEO_CODEC_EVT_RESP_START_OUTPUT_DONE:
            m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
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
}   // namespace ridehal

