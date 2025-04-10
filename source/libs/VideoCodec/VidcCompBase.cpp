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

#include "VidcCompBase.hpp"
#include "ridehal/common/Logger.hpp"
#include "ridehal/common/Types.hpp"

namespace ridehal
{
namespace component
{

static constexpr int WAIT_TIMEOUT_1_MSEC = 1;

RideHalError_e VidcCompBase::Init( const char *pName, VideoEncDecType_e type, Logger_Level_e level )
{
    RideHalError_e ret = ComponentIF::Init( pName, level );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_encDecType = type;
        RIDEHAL_INFO( "vidc-base init done" );
    }
    else
    {
        RIDEHAL_ERROR( "vidc-base init failed." );
    }

    return ret;
}

RideHalError_e VidcCompBase::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "can not Start since Init Not ready!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_STARTING;

        if ( VIDEO_ENC == m_encDecType )
        {
            ret = m_drvClient.StartDriver( VIDEO_CODEC_START_ALL );
        }
        else
        {
            ret = m_drvClient.StartDriver( VIDEO_CODEC_START_INPUT );
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            RIDEHAL_ERROR( "start driver failed" );
        }
        else
        {
            RIDEHAL_INFO( "start driver succeed" );
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_RUNNING );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "VIDC_IOCTL_START START_INPUT failed!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
        }
    }

    RIDEHAL_INFO( "start done" );

    return ret;
}

RideHalError_e VidcCompBase::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_state = RIDEHAL_COMPONENT_STATE_STOPING;

        if ( VIDEO_ENC == m_encDecType )
        {
            ret = m_drvClient.StopEncoder();
        }
        else
        {
            ret = m_drvClient.StopDecoder();
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_state = RIDEHAL_COMPONENT_STATE_READY;
        }
        else
        {
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
    }

    return ret;
}

RideHalError_e VidcCompBase::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_ERROR != m_state ) )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( ( RIDEHAL_ERROR_NONE == ret ) && ( RIDEHAL_COMPONENT_STATE_READY == m_state ) )
    {
        RIDEHAL_DEBUG( "Deiniting vidc" );
        m_state = RIDEHAL_COMPONENT_STATE_DEINITIALIZING;

        ret = m_drvClient.ReleaseResources();

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        }
        else
        {
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_INITIAL );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "WaitForState.state_initial.fail!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = FreeInputBuffer();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = FreeOutputBuffer();
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_drvClient.CloseDriver();
        RIDEHAL_DEBUG( "Deinited component" );
        ret = ComponentIF::Deinit();
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Something wrong happened in Deinit" );
        m_state = RIDEHAL_COMPONENT_STATE_ERROR;
    }
    else
    {
        RIDEHAL_INFO( "Deinited done" );
        m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
    }

    return ret;
}

// only supported for non-dynamic mode
RideHalError_e VidcCompBase::GetInputBuffers( RideHal_SharedBuffer_t *pInputList, uint32_t num )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetInputBuffers" );

    if ( m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] )
    {
        RIDEHAL_ERROR( "not supported, dynamic mode, app knows buf list" );
        ret = RIDEHAL_ERROR_OUT_OF_BOUND;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == pInputList )
        {
            RIDEHAL_ERROR( "pInputList is null pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( num != m_bufNum[VIDEO_CODEC_BUF_INPUT] )
        {
            RIDEHAL_ERROR( "provided array size %" PRIu32 " should be same to %" PRIu32, num,
                           m_bufNum[VIDEO_CODEC_BUF_INPUT] );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == m_pInputList )
        {
            RIDEHAL_ERROR( "input buffer is not ready!" );
            ret = RIDEHAL_ERROR_BAD_STATE;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( int i = 0; i < m_bufNum[VIDEO_CODEC_BUF_INPUT]; i++ )
        {
            pInputList[i] = m_pInputList[i];
        }
    }

    return ret;
}

RideHalError_e VidcCompBase::GetOutputBuffers( RideHal_SharedBuffer_t *pOutputList, uint32_t num )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_DEBUG( "GetOutputBuffers" );

    if ( m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] )
    {
        RIDEHAL_ERROR( "not supported, dynamic mode, app knows buf list" );
        ret = RIDEHAL_ERROR_OUT_OF_BOUND;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == pOutputList )
        {
            RIDEHAL_ERROR( "pOutputList is null pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( num != m_bufNum[VIDEO_CODEC_BUF_OUTPUT] )
        {
            RIDEHAL_ERROR( "provided array size %" PRIu32 " should be same to %" PRIu32, num,
                           m_bufNum[VIDEO_CODEC_BUF_OUTPUT] );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == m_pOutputList )
        {
            RIDEHAL_ERROR( "output buffer is not ready!" );
            ret = RIDEHAL_ERROR_BAD_STATE;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( int i = 0; i < m_bufNum[VIDEO_CODEC_BUF_OUTPUT]; i++ )
        {
            pOutputList[i] = m_pOutputList[i];
        }
    }

    return ret;
}

RideHalError_e VidcCompBase::ValidateBuffer( const RideHal_SharedBuffer_t *pBuffer,
                                             VideoCodec_BufType_e bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pBuffer )
    {
        RIDEHAL_ERROR( "pBuffer is nullptr, invalid" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( ( pBuffer->imgProps.width != m_width ) || ( pBuffer->imgProps.height != m_height ) )
        {
            RIDEHAL_ERROR( "pBuffer width %" PRIu32 " height %" PRIu32
                           " is not match m_width %" PRIu32 " m_height %" PRIu32,
                           pBuffer->imgProps.width, pBuffer->imgProps.height, m_width, m_height );
            ret = RIDEHAL_ERROR_INVALID_BUF;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( bufferType == VIDEO_CODEC_BUF_INPUT )
        {
            if ( pBuffer->imgProps.format != m_inFormat )
            {
                RIDEHAL_ERROR( "pBuffer format %d  is not match m_inFormat %d",
                               pBuffer->imgProps.format, m_inFormat );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
        }
        else
        {
            if ( pBuffer->imgProps.format != m_outFormat )
            {
                RIDEHAL_ERROR( "pBuffer format %d  is not match m_outFormat %d",
                               pBuffer->imgProps.format, m_outFormat );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
        }
    }

    return ret;
}

RideHalError_e VidcCompBase::WaitForState( RideHal_ComponentState_t expectedState )
{
    int32_t counter = 0;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    while ( m_state != expectedState )
    {
        (void) MM_Timer_Sleep( 1 );
        counter++;
        if ( counter > WAIT_TIMEOUT_1_MSEC )
        {
            RIDEHAL_ERROR( "WaitForState timeout!" );
            ret = RIDEHAL_ERROR_TIMEOUT;
            break;
        }
    }
    return ret;
}

RideHalError_e VidcCompBase::PostInit( RideHal_SharedBuffer_t *pBufListIn,
                                       RideHal_SharedBuffer_t *pBufListOut )
{
    RideHalError_e ret = m_drvClient.SetDynamicMode( VIDEO_CODEC_BUF_INPUT,
                                                     m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( false == m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] )
        {
            ret = InitBufferForNonDynamicMode( pBufListIn, VIDEO_CODEC_BUF_INPUT );
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                ret = VidcCompBase::SetBuffer( VIDEO_CODEC_BUF_INPUT );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_drvClient.SetDynamicMode( VIDEO_CODEC_BUF_OUTPUT,
                                          m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( false == m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] )
        {
            ret = InitBufferForNonDynamicMode( pBufListOut, VIDEO_CODEC_BUF_OUTPUT );

            if ( ( RIDEHAL_ERROR_NONE == ret ) && ( VIDEO_ENC == m_encDecType ) )
            {
                ret = VidcCompBase::SetBuffer( VIDEO_CODEC_BUF_OUTPUT );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_drvClient.LoadResources();

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = WaitForState( RIDEHAL_COMPONENT_STATE_READY );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "VIDC_IOCTL_LOAD_RESOURCES WaitForState.state_ready fail!" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        PrintConfig();
    }
    else
    {
        RIDEHAL_ERROR( "Something wrong happened in Init, Deiniting vidc" );
        m_state = RIDEHAL_COMPONENT_STATE_ERROR;
        (void) Deinit();
    }

    return ret;
}

RideHalError_e VidcCompBase::InitBufferForNonDynamicMode( RideHal_SharedBuffer_t *pBufList,
                                                          VideoCodec_BufType_e bufferType )
{
    int32_t i;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHal_SharedBuffer_t *bufList = (RideHal_SharedBuffer_t *) malloc(
            m_bufNum[bufferType] * sizeof( RideHal_SharedBuffer_t ) );

    if ( nullptr == bufList )
    {
        RIDEHAL_ERROR( "bufList malloc failed! type:%" PRIu32, bufferType );
        ret = RIDEHAL_ERROR_NOMEM;
    }
    else
    {
        RIDEHAL_DEBUG( "Allocating %" PRIu32 " buffers succeed, type:%" PRIu32,
                       m_bufNum[bufferType], bufferType );

        if ( VIDEO_CODEC_BUF_INPUT == bufferType )
        {
            m_pInputList = bufList;
        }
        else
        {
            m_pOutputList = bufList;
        }

        if ( m_bNonDynamicAppAllocBuffer[bufferType] )
        {
            /* use the buffer list allocated by app */
            for ( i = 0; i < m_bufNum[bufferType]; i++ )
            {
                bufList[i] = pBufList[i];
            }
        }
        else
        {
            /* allocate the buffer list in component */
            ret = AllocateBuffer( bufferType );
        }
    }

    return ret;
}

RideHalError_e VidcCompBase::AllocateSharedBuf( RideHal_SharedBuffer_t &sharedBuffer,
                                                VideoCodec_BufAllocMode_e mode, int32_t bufSize,
                                                RideHal_ImageFormat_e format )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    RIDEHAL_INFO( "Alloc buf width:%u height:%u format:%d size:%d", m_width, m_height, format,
                  bufSize );

    if ( COMPRESSED_DATA_MODE == mode )
    {
        RideHal_ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = m_width;
        imgProps.height = m_height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = bufSize;
        imgProps.format = format;
        ret = sharedBuffer.Allocate( &imgProps );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Alloc buf failed ret:%d, size:%d", ret, bufSize );
        }
    }
    else
    {
        ret = sharedBuffer.Allocate( m_width, m_height, format );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Alloc buf failed ret:%d, format:%d", ret, format );
        }
    }
    return ret;
}

RideHalError_e VidcCompBase::AllocateBuffer( VideoCodec_BufType_e bufferType )
{
    int32_t i;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int32_t bufCnt = m_bufNum[bufferType];
    int32_t bufSize = m_bufSize[bufferType];

    RIDEHAL_DEBUG( "bufSize=%" PRId32 " bufCnt=%" PRId32, bufSize, bufCnt );

    for ( i = 0; i < bufCnt; i++ )
    {
        RideHal_SharedBuffer_t sharedBuffer;
        if ( VIDEO_CODEC_BUF_INPUT == bufferType )
        {
            if ( VIDEO_DEC == m_encDecType )
            {
                ret = AllocateSharedBuf( sharedBuffer, COMPRESSED_DATA_MODE, bufSize, m_inFormat );
            }
            else
            {
                ret = AllocateSharedBuf( sharedBuffer, YUV_OR_RGB_MODE, bufSize, m_inFormat );
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Allocate inputBuffer failed %d for index=%" PRId32, ret, i );
                break;
            }
            else
            {
                m_pInputList[i] = sharedBuffer;
                RIDEHAL_DEBUG( "m_pInputList[%" PRId32 "] 0x%x", i, &m_pInputList[i] );
            }
        }
        else if ( VIDEO_CODEC_BUF_OUTPUT == bufferType )
        {
            if ( VIDEO_DEC == m_encDecType )
            {
                ret = AllocateSharedBuf( sharedBuffer, YUV_OR_RGB_MODE, bufSize, m_outFormat );
            }
            else
            {
                ret = AllocateSharedBuf( sharedBuffer, COMPRESSED_DATA_MODE, bufSize, m_outFormat );
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Allocate outputBuffer failed %d for index=%" PRId32, ret, i );
                break;
            }
            else
            {
                m_pOutputList[i] = sharedBuffer;
                RIDEHAL_DEBUG( "m_pOutputList[%" PRId32 "] 0x%x", i, &m_pOutputList[i] );
            }
        }
    }

    return ret;
}

RideHalError_e VidcCompBase::SetBuffer( VideoCodec_BufType_e bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    RideHal_SharedBuffer_t *bufList;

    if ( VIDEO_CODEC_BUF_INPUT == bufferType )
    {
        bufList = m_pInputList;
    }
    else
    {
        bufList = m_pOutputList;
    }

    ret = m_drvClient.SetBuffer( bufferType, bufList, m_bufNum[bufferType], m_bufSize[bufferType] );

    return ret;
}

RideHalError_e VidcCompBase::NegotiateBufferReq( VideoCodec_BufType_e bufType )
{
    uint32_t bufNum = m_bufNum[bufType];
    uint32_t bufSize;
    bool isAppAlloc = false;

    if ( m_bDynamicMode[bufType] || m_bNonDynamicAppAllocBuffer[bufType] )
    {
        isAppAlloc = true;
    }

    RideHalError_e ret = m_drvClient.NegotiateBufferReq( bufType, bufNum, bufSize );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( isAppAlloc && m_bufNum[bufType] < bufNum )
        {
            RIDEHAL_ERROR( "buf-type:%d, app alloc count:%" PRIu32
                           ", but driver req count:%" PRIu32,
                           bufType, m_bufNum[bufType], bufNum );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            m_bufNum[bufType] = bufNum;
            m_bufSize[bufType] = bufSize;
            RIDEHAL_INFO( "dec-buf-type:%d, num:%" PRIu32 ", size=%" PRIu32, bufType,
                          m_bufNum[bufType], m_bufSize[bufType] );
        }
    }
    return ret;
}

RideHalError_e VidcCompBase::FreeInputBuffer()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr != m_pInputList )   // it means non dynamic mode
    {
        RIDEHAL_DEBUG( "FreeInputBuffer begin" );

        ret = m_drvClient.FreeBuffer( m_pInputList, m_bufNum[VIDEO_CODEC_BUF_INPUT],
                                      VIDEO_CODEC_BUF_INPUT );

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = FreeSharedBuf( VIDEO_CODEC_BUF_INPUT );

            RIDEHAL_INFO( "FreeInputBuffer done" );
            free( m_pInputList );
            m_pInputList = nullptr;
        }
    }

    return ret;
}

RideHalError_e VidcCompBase::FreeOutputBuffer()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr != m_pOutputList )   // it means non dynamic mode
    {
        RIDEHAL_DEBUG( "FreeOutputBuffer:" );

        ret = m_drvClient.FreeBuffer( m_pOutputList, m_bufNum[VIDEO_CODEC_BUF_OUTPUT],
                                      VIDEO_CODEC_BUF_OUTPUT );

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            ret = FreeSharedBuf( VIDEO_CODEC_BUF_OUTPUT );

            RIDEHAL_INFO( "Free m_pOutputList done" );
            free( m_pOutputList );
            m_pOutputList = nullptr;
        }
    }

    return ret;
}

/* free the memory allocated by codec component */
RideHalError_e VidcCompBase::FreeSharedBuf( VideoCodec_BufType_e bufType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( false == m_bDynamicMode[bufType] && false == m_bNonDynamicAppAllocBuffer[bufType] )
    {
        RideHal_SharedBuffer_t *bufList;
        if ( VIDEO_CODEC_BUF_INPUT == bufType )
        {
            bufList = m_pInputList;
        }
        else
        {
            bufList = m_pOutputList;
        }

        RIDEHAL_INFO( "Free shared buf, bufNum:%d, bufType:%d", m_bufNum[bufType], bufType );

        for ( int32_t i = 0; i < m_bufNum[bufType]; i++ )
        {
            RIDEHAL_DEBUG( "Free shared buf, i:%d, 0x%x", i, &bufList[i] );
            RideHalError_e r = bufList[i].Free();
            if ( RIDEHAL_ERROR_NONE != r )
            {
                ret = r;
                RIDEHAL_ERROR( "Free Shared Buffer failed, ret:%d", ret );
            }
        }
    }
    return ret;
}

vidc_color_format_type VidcCompBase::GetVidcFormat( RideHal_ImageFormat_e format )
{
    // only support NV12 and p010 now
    vidc_color_format_type ret = VIDC_COLOR_FORMAT_UNUSED;
    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_NV12:
            ret = VIDC_COLOR_FORMAT_NV12;
            break;
        case RIDEHAL_IMAGE_FORMAT_NV12_UBWC:
            ret = VIDC_COLOR_FORMAT_NV12_UBWC;
            break;
        case RIDEHAL_IMAGE_FORMAT_P010:
            ret = VIDC_COLOR_FORMAT_NV12_P010;
            break;
        default:
            RIDEHAL_ERROR( "format %d not supported", format );
            ret = VIDC_COLOR_FORMAT_UNUSED;
            break;
    }
    return ret;
}

void VidcCompBase::PrintConfig()
{
    if ( VIDEO_ENC == m_encDecType )
    {
        RIDEHAL_DEBUG( "EncoderConfig:" );
    }
    else
    {
        RIDEHAL_DEBUG( "DecoderConfig:" );
    }

    m_drvClient.PrintCodecConfig();

    RIDEHAL_DEBUG( "FrameWidth = %" PRIu32, m_width );
    RIDEHAL_DEBUG( "FrameHeight = %" PRIu32, m_height );
    RIDEHAL_DEBUG( "Fps = %" PRIu32, m_frameRate );
    RIDEHAL_DEBUG( "InBufCount = %" PRIu32 " [0 means minimum]", m_bufNum[VIDEO_CODEC_BUF_INPUT] );
    RIDEHAL_DEBUG( "OutBufCount = %" PRIu32 " [0 means minimum]",
                   m_bufNum[VIDEO_CODEC_BUF_OUTPUT] );
    RIDEHAL_DEBUG( "InBufSize = %" PRIu32, m_bufSize[VIDEO_CODEC_BUF_INPUT] );
    RIDEHAL_DEBUG( "OutBufSize = %" PRIu32, m_bufSize[VIDEO_CODEC_BUF_OUTPUT] );
}

void VidcCompBase::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload )
{
    RIDEHAL_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );

    switch ( eventId )
    {
        case VIDEO_CODEC_EVT_FATAL:
            m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            break;
        case VIDEO_CODEC_EVT_RESP_START:
            if ( RIDEHAL_COMPONENT_STATE_STARTING == m_state )
            {
                RIDEHAL_INFO( "Started vidc event" );
                m_state = RIDEHAL_COMPONENT_STATE_RUNNING;   // used by encoder
            }
            else
            {
                RIDEHAL_ERROR( "Started vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_LOAD_RESOURCES:
            if ( RIDEHAL_COMPONENT_STATE_INITIALIZING == m_state )
            {
                RIDEHAL_INFO( "Loaded vidc resources event" );
                m_state = RIDEHAL_COMPONENT_STATE_READY;
            }
            else
            {
                RIDEHAL_ERROR( "Loaded vidc resources from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_RELEASE_RESOURCES:
            if ( RIDEHAL_COMPONENT_STATE_DEINITIALIZING == m_state )
            {
                RIDEHAL_INFO( "Released vidc resources event" );
                m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
            }
            else
            {
                RIDEHAL_ERROR( "Released vidc resources from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_STOP:
            if ( RIDEHAL_COMPONENT_STATE_STOPING == m_state )
            {
                RIDEHAL_DEBUG( "Stopped vidc" );
                m_state = RIDEHAL_COMPONENT_STATE_READY;
            }
            else
            {
                RIDEHAL_ERROR( "Stopped vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_PAUSE:
            if ( RIDEHAL_COMPONENT_STATE_PAUSING == m_state )
            {
                RIDEHAL_INFO( "Paused vidc event" );
                m_state = RIDEHAL_COMPONENT_STATE_PAUSE;
            }
            else
            {
                RIDEHAL_ERROR( "Paused vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_RESUME:
            if ( RIDEHAL_COMPONENT_STATE_RESUMING == m_state )
            {
                RIDEHAL_INFO( "Resumed vidc event" );
                m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            }
            else
            {
                RIDEHAL_ERROR( "Resumed vidc from wrong state" );
                m_state = RIDEHAL_COMPONENT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_ERROR:
            RIDEHAL_WARN( "normal error event" );
            break;
        default:
            RIDEHAL_WARN( "Unknown event %d", eventId );
            break;
    }
}

}   // namespace component
}   // namespace ridehal

