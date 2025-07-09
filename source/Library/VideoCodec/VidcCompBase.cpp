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
#include "VidcCompBase.hpp"

namespace QC
{
namespace component
{

static constexpr int WAIT_TIMEOUT_1_MSEC = 1;

QCStatus_e VidcCompBase::Init( const char *pName, VideoEncDecType_e type, Logger_Level_e level )
{
    QCStatus_e ret = ComponentIF::Init( pName, level );

    if ( QC_STATUS_OK == ret )
    {
        m_encDecType = type;
        QC_INFO( "vidc-base init done" );
    }
    else
    {
        QC_ERROR( "vidc-base init failed." );
    }

    return ret;
}

QCStatus_e VidcCompBase::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "can not Start since Init Not ready!" );
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_STARTING;

        if ( VIDEO_ENC == m_encDecType )
        {
            ret = m_drvClient.StartDriver( VIDEO_CODEC_START_ALL );
        }
        else
        {
            ret = m_drvClient.StartDriver( VIDEO_CODEC_START_INPUT );
        }

        if ( QC_STATUS_OK != ret )
        {
            m_state = QC_OBJECT_STATE_ERROR;
            QC_ERROR( "start driver failed" );
        }
        else
        {
            QC_INFO( "start driver succeed" );
            ret = WaitForState( QC_OBJECT_STATE_RUNNING );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "VIDC_IOCTL_START START_INPUT failed!" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
        }
    }

    QC_INFO( "start done" );

    return ret;
}

QCStatus_e VidcCompBase::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_STOPING;

        if ( VIDEO_ENC == m_encDecType )
        {
            ret = m_drvClient.StopEncoder();
        }
        else
        {
            ret = m_drvClient.StopDecoder();
        }

        if ( QC_STATUS_OK == ret )
        {
            m_state = QC_OBJECT_STATE_READY;
        }
        else
        {
            m_state = QC_OBJECT_STATE_ERROR;
        }
    }

    return ret;
}

QCStatus_e VidcCompBase::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_ERROR != m_state ) )
    {
        ret = QC_STATUS_BAD_STATE;
    }

    if ( ( QC_STATUS_OK == ret ) && ( QC_OBJECT_STATE_READY == m_state ) )
    {
        QC_DEBUG( "Deiniting vidc" );
        m_state = QC_OBJECT_STATE_DEINITIALIZING;

        ret = m_drvClient.ReleaseResources();

        if ( QC_STATUS_OK != ret )
        {
            m_state = QC_OBJECT_STATE_ERROR;
        }
        else
        {
            ret = WaitForState( QC_OBJECT_STATE_INITIAL );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "WaitForState.state_initial.fail!" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = FreeInputBuffer();
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = FreeOutputBuffer();
    }

    if ( QC_STATUS_OK == ret )
    {
        m_drvClient.CloseDriver();
        QC_DEBUG( "Deinited component" );
        ret = ComponentIF::Deinit();
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Something wrong happened in Deinit" );
        m_state = QC_OBJECT_STATE_ERROR;
    }
    else
    {
        QC_INFO( "Deinited done" );
        m_state = QC_OBJECT_STATE_INITIAL;
    }

    return ret;
}

// only supported for non-dynamic mode
QCStatus_e VidcCompBase::GetInputBuffers( QCSharedBuffer_t *pInputList, uint32_t num )
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_DEBUG( "GetInputBuffers" );

    if ( m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] )
    {
        QC_ERROR( "not supported, dynamic mode, app knows buf list" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pInputList )
        {
            QC_ERROR( "pInputList is null pointer!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( num != m_bufNum[VIDEO_CODEC_BUF_INPUT] )
        {
            QC_ERROR( "provided array size %" PRIu32 " should be same to %" PRIu32, num,
                      m_bufNum[VIDEO_CODEC_BUF_INPUT] );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == m_pInputList )
        {
            QC_ERROR( "input buffer is not ready!" );
            ret = QC_STATUS_BAD_STATE;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( int i = 0; i < m_bufNum[VIDEO_CODEC_BUF_INPUT]; i++ )
        {
            pInputList[i] = m_pInputList[i];
        }
    }

    return ret;
}

QCStatus_e VidcCompBase::GetOutputBuffers( QCSharedBuffer_t *pOutputList, uint32_t num )
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_DEBUG( "GetOutputBuffers" );

    if ( m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] )
    {
        QC_ERROR( "not supported, dynamic mode, app knows buf list" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pOutputList )
        {
            QC_ERROR( "pOutputList is null pointer!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else if ( num != m_bufNum[VIDEO_CODEC_BUF_OUTPUT] )
        {
            QC_ERROR( "provided array size %" PRIu32 " should be same to %" PRIu32, num,
                      m_bufNum[VIDEO_CODEC_BUF_OUTPUT] );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == m_pOutputList )
        {
            QC_ERROR( "output buffer is not ready!" );
            ret = QC_STATUS_BAD_STATE;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( int i = 0; i < m_bufNum[VIDEO_CODEC_BUF_OUTPUT]; i++ )
        {
            pOutputList[i] = m_pOutputList[i];
        }
    }

    return ret;
}

QCStatus_e VidcCompBase::ValidateBuffer( const QCSharedBuffer_t *pBuffer,
                                         VideoCodec_BufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pBuffer )
    {
        QC_ERROR( "pBuffer is nullptr, invalid" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( pBuffer->imgProps.width != m_width ) || ( pBuffer->imgProps.height != m_height ) )
        {
            QC_ERROR( "pBuffer width %" PRIu32 " height %" PRIu32 " does not match m_width %" PRIu32
                      " m_height %" PRIu32,
                      pBuffer->imgProps.width, pBuffer->imgProps.height, m_width, m_height );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( bufferType == VIDEO_CODEC_BUF_INPUT )
        {
            if ( pBuffer->imgProps.format != m_inFormat )
            {
                QC_ERROR( "pBuffer format %d does not match m_inFormat %d",
                          pBuffer->imgProps.format, m_inFormat );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
        else
        {
            if ( pBuffer->imgProps.format != m_outFormat )
            {
                QC_ERROR( "pBuffer format %d does not match m_outFormat %d",
                          pBuffer->imgProps.format, m_outFormat );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
    }

    return ret;
}

QCStatus_e VidcCompBase::WaitForState( QCObjectState_e expectedState )
{
    int32_t counter = 0;
    QCStatus_e ret = QC_STATUS_OK;

    while ( m_state != expectedState )
    {
        (void) MM_Timer_Sleep( 1 );
        counter++;
        if ( counter > WAIT_TIMEOUT_1_MSEC )
        {
            QC_ERROR( "WaitForState timeout!" );
            ret = QC_STATUS_TIMEOUT;
            break;
        }
    }
    return ret;
}

QCStatus_e VidcCompBase::PostInit( QCSharedBuffer_t *pBufListIn, QCSharedBuffer_t *pBufListOut )
{
    QCStatus_e ret = m_drvClient.SetDynamicMode( VIDEO_CODEC_BUF_INPUT,
                                                 m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] );

    if ( QC_STATUS_OK == ret )
    {
        if ( false == m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] )
        {
            ret = InitBufferForNonDynamicMode( pBufListIn, VIDEO_CODEC_BUF_INPUT );
            if ( QC_STATUS_OK == ret )
            {
                ret = VidcCompBase::SetBuffer( VIDEO_CODEC_BUF_INPUT );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.SetDynamicMode( VIDEO_CODEC_BUF_OUTPUT,
                                          m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( false == m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] )
        {
            ret = InitBufferForNonDynamicMode( pBufListOut, VIDEO_CODEC_BUF_OUTPUT );

            if ( ( QC_STATUS_OK == ret ) && ( VIDEO_ENC == m_encDecType ) )
            {
                ret = VidcCompBase::SetBuffer( VIDEO_CODEC_BUF_OUTPUT );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.LoadResources();

        if ( QC_STATUS_OK == ret )
        {
            ret = WaitForState( QC_OBJECT_STATE_READY );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "VIDC_IOCTL_LOAD_RESOURCES WaitForState.state_ready fail!" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        PrintConfig();
    }
    else
    {
        QC_ERROR( "Something wrong happened in Init, Deiniting vidc" );
        m_state = QC_OBJECT_STATE_ERROR;
        (void) Deinit();
    }

    return ret;
}

QCStatus_e VidcCompBase::InitBufferForNonDynamicMode( QCSharedBuffer_t *pBufList,
                                                      VideoCodec_BufType_e bufferType )
{
    int32_t i;
    QCStatus_e ret = QC_STATUS_OK;
    QCSharedBuffer_t *bufList =
            (QCSharedBuffer_t *) malloc( m_bufNum[bufferType] * sizeof( QCSharedBuffer_t ) );

    if ( nullptr == bufList )
    {
        QC_ERROR( "bufList malloc failed! type:%" PRIu32, bufferType );
        ret = QC_STATUS_NOMEM;
    }
    else
    {
        QC_DEBUG( "Allocating %" PRIu32 " buffers succeed, type:%" PRIu32, m_bufNum[bufferType],
                  bufferType );

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

QCStatus_e VidcCompBase::AllocateSharedBuf( QCSharedBuffer_t &sharedBuffer,
                                            VideoCodec_BufAllocMode_e mode, int32_t bufSize,
                                            QCImageFormat_e format )
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_INFO( "Alloc buf width:%u height:%u format:%d size:%d", m_width, m_height, format, bufSize );

    if ( COMPRESSED_DATA_MODE == mode )
    {
        QCImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = m_width;
        imgProps.height = m_height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = bufSize;
        imgProps.format = format;
        ret = sharedBuffer.Allocate( &imgProps );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Alloc buf failed ret:%d, size:%d", ret, bufSize );
        }
    }
    else
    {
        ret = sharedBuffer.Allocate( m_width, m_height, format );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Alloc buf failed ret:%d, format:%d", ret, format );
        }
    }
    return ret;
}

QCStatus_e VidcCompBase::AllocateBuffer( VideoCodec_BufType_e bufferType )
{
    int32_t i;
    QCStatus_e ret = QC_STATUS_OK;
    int32_t bufCnt = m_bufNum[bufferType];
    int32_t bufSize = m_bufSize[bufferType];

    QC_DEBUG( "bufSize=%" PRId32 " bufCnt=%" PRId32, bufSize, bufCnt );

    for ( i = 0; i < bufCnt; i++ )
    {
        QCSharedBuffer_t sharedBuffer;
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

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Allocate inputBuffer failed %d for index=%" PRId32, ret, i );
                break;
            }
            else
            {
                m_pInputList[i] = sharedBuffer;
                QC_DEBUG( "m_pInputList[%" PRId32 "] 0x%x", i, &m_pInputList[i] );
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

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Allocate outputBuffer failed %d for index=%" PRId32, ret, i );
                break;
            }
            else
            {
                m_pOutputList[i] = sharedBuffer;
                QC_DEBUG( "m_pOutputList[%" PRId32 "] 0x%x", i, &m_pOutputList[i] );
            }
        }
    }

    return ret;
}

QCStatus_e VidcCompBase::SetBuffer( VideoCodec_BufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCSharedBuffer_t *bufList;

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

QCStatus_e VidcCompBase::NegotiateBufferReq( VideoCodec_BufType_e bufType )
{
    uint32_t bufNum = m_bufNum[bufType];
    uint32_t bufSize;
    bool isAppAlloc = false;

    if ( m_bDynamicMode[bufType] || m_bNonDynamicAppAllocBuffer[bufType] )
    {
        isAppAlloc = true;
    }

    QCStatus_e ret = m_drvClient.NegotiateBufferReq( bufType, bufNum, bufSize );
    if ( QC_STATUS_OK == ret )
    {
        if ( isAppAlloc && m_bufNum[bufType] < bufNum )
        {
            QC_ERROR( "buf-type:%d, app alloc count:%" PRIu32 ", but driver req count:%" PRIu32,
                      bufType, m_bufNum[bufType], bufNum );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            m_bufNum[bufType] = bufNum;
            m_bufSize[bufType] = bufSize;
            QC_INFO( "dec-buf-type:%d, num:%" PRIu32 ", size=%" PRIu32, bufType, m_bufNum[bufType],
                     m_bufSize[bufType] );
        }
    }
    return ret;
}

QCStatus_e VidcCompBase::FreeInputBuffer()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr != m_pInputList )   // it means non dynamic mode
    {
        QC_DEBUG( "FreeInputBuffer begin" );

        ret = m_drvClient.FreeBuffer( m_pInputList, m_bufNum[VIDEO_CODEC_BUF_INPUT],
                                      VIDEO_CODEC_BUF_INPUT );

        if ( QC_STATUS_OK == ret )
        {
            ret = FreeSharedBuf( VIDEO_CODEC_BUF_INPUT );

            QC_INFO( "FreeInputBuffer done" );
            free( m_pInputList );
            m_pInputList = nullptr;
        }
    }

    return ret;
}

QCStatus_e VidcCompBase::FreeOutputBuffer()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr != m_pOutputList )   // it means non dynamic mode
    {
        QC_DEBUG( "FreeOutputBuffer:" );

        ret = m_drvClient.FreeBuffer( m_pOutputList, m_bufNum[VIDEO_CODEC_BUF_OUTPUT],
                                      VIDEO_CODEC_BUF_OUTPUT );

        if ( QC_STATUS_OK == ret )
        {
            ret = FreeSharedBuf( VIDEO_CODEC_BUF_OUTPUT );

            QC_INFO( "Free m_pOutputList done" );
            free( m_pOutputList );
            m_pOutputList = nullptr;
        }
    }

    return ret;
}

/* free the memory allocated by codec component */
QCStatus_e VidcCompBase::FreeSharedBuf( VideoCodec_BufType_e bufType )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bDynamicMode[bufType] && false == m_bNonDynamicAppAllocBuffer[bufType] )
    {
        QCSharedBuffer_t *bufList;
        if ( VIDEO_CODEC_BUF_INPUT == bufType )
        {
            bufList = m_pInputList;
        }
        else
        {
            bufList = m_pOutputList;
        }

        QC_INFO( "Free shared buf, bufNum:%d, bufType:%d", m_bufNum[bufType], bufType );

        for ( int32_t i = 0; i < m_bufNum[bufType]; i++ )
        {
            QC_DEBUG( "Free shared buf, i:%d, 0x%x", i, &bufList[i] );
            QCStatus_e r = bufList[i].Free();
            if ( QC_STATUS_OK != r )
            {
                ret = r;
                QC_ERROR( "Free Shared Buffer failed, ret:%d", ret );
            }
        }
    }
    return ret;
}

vidc_color_format_type VidcCompBase::GetVidcFormat( QCImageFormat_e format )
{
    // only support NV12 and p010 now
    vidc_color_format_type ret = VIDC_COLOR_FORMAT_UNUSED;
    switch ( format )
    {
        case QC_IMAGE_FORMAT_NV12:
            ret = VIDC_COLOR_FORMAT_NV12;
            break;
        case QC_IMAGE_FORMAT_NV12_UBWC:
            ret = VIDC_COLOR_FORMAT_NV12_UBWC;
            break;
        case QC_IMAGE_FORMAT_P010:
            ret = VIDC_COLOR_FORMAT_NV12_P010;
            break;
        default:
            QC_ERROR( "format %d not supported", format );
            ret = VIDC_COLOR_FORMAT_UNUSED;
            break;
    }
    return ret;
}

void VidcCompBase::PrintConfig()
{
    if ( VIDEO_ENC == m_encDecType )
    {
        QC_DEBUG( "EncoderConfig:" );
    }
    else
    {
        QC_DEBUG( "DecoderConfig:" );
    }

    m_drvClient.PrintCodecConfig();

    QC_DEBUG( "FrameWidth = %" PRIu32, m_width );
    QC_DEBUG( "FrameHeight = %" PRIu32, m_height );
    QC_DEBUG( "Fps = %" PRIu32, m_frameRate );
    QC_DEBUG( "InBufCount = %" PRIu32 " [0 means minimum]", m_bufNum[VIDEO_CODEC_BUF_INPUT] );
    QC_DEBUG( "OutBufCount = %" PRIu32 " [0 means minimum]", m_bufNum[VIDEO_CODEC_BUF_OUTPUT] );
    QC_DEBUG( "InBufSize = %" PRIu32, m_bufSize[VIDEO_CODEC_BUF_INPUT] );
    QC_DEBUG( "OutBufSize = %" PRIu32, m_bufSize[VIDEO_CODEC_BUF_OUTPUT] );
}

void VidcCompBase::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload )
{
    QC_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );

    switch ( eventId )
    {
        case VIDEO_CODEC_EVT_FATAL:
            m_state = QC_OBJECT_STATE_ERROR;
            break;
        case VIDEO_CODEC_EVT_RESP_START:
            if ( QC_OBJECT_STATE_STARTING == m_state )
            {
                QC_INFO( "Started vidc event" );
                m_state = QC_OBJECT_STATE_RUNNING;   // used by encoder
            }
            else
            {
                QC_ERROR( "Started vidc from wrong state" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_LOAD_RESOURCES:
            if ( QC_OBJECT_STATE_INITIALIZING == m_state )
            {
                QC_INFO( "Loaded vidc resources event" );
                m_state = QC_OBJECT_STATE_READY;
            }
            else
            {
                QC_ERROR( "Loaded vidc resources from wrong state" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_RELEASE_RESOURCES:
            if ( QC_OBJECT_STATE_DEINITIALIZING == m_state )
            {
                QC_INFO( "Released vidc resources event" );
                m_state = QC_OBJECT_STATE_INITIAL;
            }
            else
            {
                QC_ERROR( "Released vidc resources from wrong state" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_STOP:
            if ( QC_OBJECT_STATE_STOPING == m_state )
            {
                QC_DEBUG( "Stopped vidc" );
                m_state = QC_OBJECT_STATE_READY;
            }
            else
            {
                QC_ERROR( "Stopped vidc from wrong state" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_PAUSE:
            if ( QC_OBJECT_STATE_PAUSING == m_state )
            {
                QC_INFO( "Paused vidc event" );
                m_state = QC_OBJECT_STATE_PAUSE;
            }
            else
            {
                QC_ERROR( "Paused vidc from wrong state" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_RESP_RESUME:
            if ( QC_OBJECT_STATE_RESUMING == m_state )
            {
                QC_INFO( "Resumed vidc event" );
                m_state = QC_OBJECT_STATE_RUNNING;
            }
            else
            {
                QC_ERROR( "Resumed vidc from wrong state" );
                m_state = QC_OBJECT_STATE_ERROR;
            }
            break;
        case VIDEO_CODEC_EVT_ERROR:
            QC_WARN( "normal error event" );
            break;
        default:
            QC_WARN( "Unknown event %d", eventId );
            break;
    }
}

}   // namespace component
}   // namespace QC
