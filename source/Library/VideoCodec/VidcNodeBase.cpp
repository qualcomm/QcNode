// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include <MMTimer.h>
#include <vidc_types.h>
#include <vidc_ioctl.h>

#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <cstring>
#include <vidc_client.h>
#endif

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "VidcNodeBase.hpp"

namespace QC::Node
{

static constexpr int WAIT_TIMEOUT_1_MSEC = 1;

QCStatus_e VidcNodeBase::Init( const VidcNodeBase_Config_t& config )
{
    QCStatus_e status = NodeBase::Init( config.nodeId );
    std::string errors;
    bool bBaseVidcInitDone = false;

    if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        status = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == status )
    {
        m_pConfig = &config;
        m_name = config.nodeId.name;
        QC_INFO( "vidc-base init done" );
    }
    else
    {
        QC_ERROR( "vidc-base init failed." );
    }

    return status;
}

QCStatus_e VidcNodeBaseConfigIfs::VerifyAndSet( const std::string cfg, std::string &errors,
                                                VidcNodeBase_Config_t& config )
{
    QCStatus_e status = NodeConfigIfs::VerifyAndSet( cfg, errors );

    if (QC_STATUS_OK == status)
    {
        DataTree dt;
        status = m_dataTree.Get( "static", dt );
        if ( QC_STATUS_OK == status )
        {
            status = ParseStaticConfig( dt, errors, config );
        }
        else
        {
            status = m_dataTree.Get( "dynamic", dt );
            if ( QC_STATUS_OK == status )
            {
                status = ApplyDynamicConfig( dt, errors, config );
            }
        }
    }

    if (QC_STATUS_OK == status) {
        QC_DEBUG( "FrameWidth = %" PRIu32, config.width );
        QC_DEBUG( "FrameHeight = %" PRIu32, config.height );
        QC_DEBUG( "frameRate = %" PRIu32, config.frameRate );
        QC_DEBUG( "bInputDynamicMode = %d", config.bInputDynamicMode );
        QC_DEBUG( "bOutputDynamicMode = %d", config.bOutputDynamicMode );
        QC_DEBUG( "numInputBufferReq = %" PRIu32 " ", config.numInputBufferReq );
        QC_DEBUG( "numOutputBufferReq = %" PRIu32 " ", config.numOutputBufferReq );
        QC_DEBUG( "inFormat = %d", config.inFormat );
        QC_DEBUG( "outFormat = %d", config.outFormat );
    }

    return status;
}

QCStatus_e VidcNodeBaseConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors,
                                                     VidcNodeBase_Config_t &config)
{
    QCStatus_e status = QC_STATUS_OK;

    config.nodeId.name = dt.Get<std::string>( "name", "unnamed" );
    config.nodeId.id = dt.Get<uint32_t>( "id", 0 );
    config.width = dt.Get<uint32_t>( "width", 0 );
    config.height = dt.Get<uint32_t>( "height", 0 );
    config.frameRate = dt.Get<uint32_t>( "frameRate", 0 );

    config.bInputDynamicMode = dt.Get<bool>( "bInputDynamicMode", true );
    config.bOutputDynamicMode = dt.Get<bool>( "bOutputDynamicMode", false );
    config.numInputBufferReq = dt.Get<uint32_t>( "numInputBufferReq", 0 );
    config.numOutputBufferReq = dt.Get<uint32_t>( "numOutputBufferReq", 0 );

    config.inFormat =
                    dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_MAX );
    config.outFormat =
                    dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_MAX );

    if ( config.inFormat == QC_IMAGE_FORMAT_COMPRESSED_MAX )
    {
        errors += "the input format is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( config.outFormat == QC_IMAGE_FORMAT_COMPRESSED_MAX )
    {
        errors += "the output format is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( false == config.bInputDynamicMode && config.numInputBufferReq == 0 )
    {
        errors += "the number of input buffers is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( false == config.bInputDynamicMode && config.numOutputBufferReq == 0 )
    {
        errors += "the number of output buffers is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( config.width == 0 )
    {
        errors += "the width is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( config.height == 0 )
    {
        errors += "the height is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( config.frameRate == 0 )
    {
        errors += "the frame rate is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_IMAGE_FORMAT_MAX == config.inFormat )
    {
        errors += "the input image format is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_IMAGE_FORMAT_COMPRESSED_MAX == config.outFormat )
    {
        errors += "the output image format is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if (QC_STATUS_OK != status)
        printf("errors: %s", errors.c_str());

    return status;
}

QCStatus_e VidcNodeBaseConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors,
                                                      VidcNodeBase_Config_t &config)
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e VidcNodeBase::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "cannot Start since Init Not ready!" );
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_STARTING;

        if ( VIDEO_ENC == m_drvClient.GetType() )
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

    return ret;
}

QCStatus_e VidcNodeBase::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_STOPING;

        if ( VIDEO_ENC == m_drvClient.GetType() )
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

QCStatus_e VidcNodeBase::DeInitialize()
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

QCStatus_e VidcNodeBase::ValidateBuffer( const VideoFrameDescriptor &vidFrmDesc,
                                         VideoCodec_BufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_DEBUG( "check buffer begin" );

    if ( QC_STATUS_OK == ret )
    {
        if ( ( vidFrmDesc.width != m_pConfig->width ) || ( vidFrmDesc.height != m_pConfig->height ) )
        {
            QC_ERROR( "pBuffer width %" PRIu32 " height %" PRIu32 " does not match m_width %" PRIu32
                      " m_height %" PRIu32,
                      vidFrmDesc.width, vidFrmDesc.height, m_pConfig->width, m_pConfig->height );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( bufferType == VIDEO_CODEC_BUF_INPUT )
        {
            if ( vidFrmDesc.format != m_pConfig->inFormat )
            {
                QC_ERROR( "pBuffer format %d does not match m_inFormat %d",
                          vidFrmDesc.format, m_pConfig->inFormat );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
        else
        {
            if ( vidFrmDesc.format != m_pConfig->outFormat )
            {
                QC_ERROR( "pBuffer format %d does not match m_outFormat %d",
                          vidFrmDesc.format, m_pConfig->outFormat );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
    }

    QC_DEBUG( "check buffer done" );

    return ret;
}

QCStatus_e VidcNodeBase::ValidateBuffers( )
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_DEBUG( "check buffers begin" );

    if ( ( QC_STATUS_OK == ret ) && ( true == m_pConfig->bInputDynamicMode ) &&
         ( !m_inputBufferList.empty() ) )
    {
        QC_ERROR( "should not provide input buffer in config in dynamic mode!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_STATUS_OK == ret ) && ( false == m_pConfig->bInputDynamicMode ) &&
              ( m_inputBufferList.empty() ) )
    {
        QC_ERROR( "should provide input buffer in config in non-dynamic mode!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( true == m_pConfig->bOutputDynamicMode ) &&
         ( !m_outputBufferList.empty() ) )
    {
        QC_ERROR( "should not provide output buffer in config in dynamic mode!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_STATUS_OK == ret ) && ( false == m_pConfig->bOutputDynamicMode ) &&
              ( m_outputBufferList.empty() ) )
    {
        QC_ERROR( "should provide output buffer in config in non-dynamic mode!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    QC_DEBUG( "check buffers done" );

    return ret;
}

QCStatus_e VidcNodeBase::ValidateFrameSubmission( const VideoFrameDescriptor_t &frameDesc,
                                                  VideoCodec_BufType_e bufferType,
                                                  bool requireNonZeroSize )
{
    QCStatus_e ret = QC_STATUS_OK;
    const char* inOutStr;

    QC_DEBUG( "validate frame submission begin" );

    if ( VIDEO_CODEC_BUF_INPUT == bufferType )
        inOutStr = "inputBuffer";
    else
        inOutStr = "outputBuffer";

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_WARN( "Not submitting %s since encoder is shutting down!", inOutStr );
        ret = QC_STATUS_BAD_STATE;
    }

    if ( ( QC_STATUS_OK == ret ) && ( nullptr == frameDesc.pBuf) )
    {
        QC_ERROR( "Not submitting empty %s!", inOutStr );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( 0 == frameDesc.size) && requireNonZeroSize ) )
    {
        QC_ERROR( "Not submitting empty %s!", inOutStr );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    QC_DEBUG( "validate frame submission end" );

    return ret;
}

QCStatus_e VidcNodeBase::WaitForState( QCObjectState_e expectedState )
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

QCStatus_e VidcNodeBase::PostInit( void )
{
    QCStatus_e ret = m_drvClient.LoadResources();

    if ( QC_STATUS_OK == ret )
    {
        ret = WaitForState( QC_OBJECT_STATE_READY );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "VIDC_IOCTL_LOAD_RESOURCES WaitForState.state_ready fail!" );
            m_state = QC_OBJECT_STATE_ERROR;
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
        (void) DeInitialize();
    }

    return ret;
}

QCStatus_e VidcNodeBase::InitBufferForNonDynamicMode( const std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers,
                                                      uint32_t bufferIdx, VideoCodec_BufType_e bufferType )
{
    int32_t i = 0, bufNum;
    QCStatus_e ret = QC_STATUS_OK;

    if (VIDEO_CODEC_BUF_INPUT == bufferType) {
        bufNum = m_pConfig->numInputBufferReq;
    }
    else { /* VIDEO_CODEC_BUF_OUTPUT */
        bufNum = m_pConfig->numOutputBufferReq;
    }

    if (VIDEO_CODEC_BUF_INPUT == bufferType) {
        for ( QCBufferDescriptorBase_t &buf : buffers)
        {
            if (i >= bufferIdx && i < bufferIdx + bufNum ) {
                VideoFrameDescriptor_t& vidcBuf = static_cast<VideoFrameDescriptor_t&>(buf);
                m_inputBufferList.push_back(vidcBuf);
            }
            i++;
        }
    }
    else { /* VIDEO_CODEC_BUF_OUTPUT */
        for ( QCBufferDescriptorBase_t &buf : buffers)
        {
            if (i >= bufferIdx && i < bufferIdx + bufNum ) {
                VideoFrameDescriptor_t& vidcBuf = static_cast<VideoFrameDescriptor_t&>(buf);
                m_outputBufferList.push_back(vidcBuf);
            }
            i++;
        }
    }

    return ret;
}

QCStatus_e VidcNodeBase::AllocateBuffer( const std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers,
                                         uint32_t bufferIdx, VideoCodec_BufType_e bufferType )
{
    QCStatus_e status = QC_STATUS_OK;
    VideoFrameDescriptor *vidFrmDescList = nullptr;
    uint32_t bufNum, bufSize, bufNumGiven = buffers.size() - bufferIdx;
    bool dynamicMode = false;

    if ( VIDEO_CODEC_BUF_INPUT == bufferType )
    {
        bufNum = m_pConfig->numInputBufferReq;
        bufSize = m_bufSize[VIDEO_CODEC_BUF_INPUT];
        dynamicMode = m_pConfig->bInputDynamicMode;
    }
    else
    {
        bufNum = m_pConfig->numOutputBufferReq;
        bufSize = m_bufSize[VIDEO_CODEC_BUF_OUTPUT];
        dynamicMode = m_pConfig->bOutputDynamicMode;
    }

    if (false == dynamicMode) {
        if (bufNum > bufNumGiven) {
            QC_ERROR( "Insufficient number of buffer descriptors (need %u given %u)",
                      bufNum, bufNumGiven );
            status = QC_STATUS_NOMEM;
        }
        else {
            for ( unsigned int i = 0; i < bufNum; i++)
            {
                const QCBufferDescriptorBase_t &bufdesc = buffers[i];
                if (bufSize > bufdesc.size) {
                    QC_ERROR( "Size of buffer of descriptor is too small (need %u bytes given %u bytes)",
                              bufSize, bufdesc.size );
                    status = QC_STATUS_NOMEM;
                }
            }
        }

        if (QC_STATUS_OK == status ) {
            QC_INFO( "Set %u %s buffer descriptors for non-dynamic mode", bufNum,
                     (VIDEO_CODEC_BUF_INPUT == bufferType) ? "input" : "output" );

            status = InitBufferForNonDynamicMode(buffers, bufferIdx, bufferType);
        }
    }

    return status;
}

QCStatus_e VidcNodeBase::SetBuffer( VideoCodec_BufType_e bufferType )
{
    QCStatus_e status = QC_STATUS_OK;
    std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> *vidFrmDescList = nullptr;
    bool dynamicMode = false;

    if ( VIDEO_CODEC_BUF_INPUT == bufferType )
    {
        vidFrmDescList = &m_inputBufferList;
        dynamicMode = m_pConfig->bInputDynamicMode;
    }
    else
    {
        vidFrmDescList = &m_outputBufferList;
        dynamicMode = m_pConfig->bOutputDynamicMode;
    }

    if (QC_STATUS_OK == status)
    {
        status = m_drvClient.SetDynamicMode( bufferType, dynamicMode );
    }

    if (QC_STATUS_OK == status && false == dynamicMode)
    {
        status = m_drvClient.SetBuffer( bufferType, *vidFrmDescList );
        if (QC_STATUS_OK == status)
        {
            QC_DEBUG( "Set %s buffers succeed", (VIDEO_CODEC_BUF_INPUT == bufferType) ? "input" : "output");
        }
    }

    return status;
}

QCStatus_e VidcNodeBase::NegotiateBufferReq( VideoCodec_BufType_e bufType )
{
    uint32_t bufSize = 0, bufNum;
    int32_t bufNumAvail;
    bool dynamicMode = false;
    std::vector<std::reference_wrapper<VideoFrameDescriptor_t>> *vidFrmDescList = nullptr;

    if ( VIDEO_CODEC_BUF_INPUT == bufType )
    {
        vidFrmDescList = &m_inputBufferList;
        bufNumAvail = m_pConfig->numInputBufferReq;
        dynamicMode = m_pConfig->bInputDynamicMode;
    }
    else
    {
        vidFrmDescList = &m_outputBufferList;
        bufNumAvail = m_pConfig->numOutputBufferReq;
        dynamicMode = m_pConfig->bOutputDynamicMode;
    }

    bufNum = bufNumAvail;

    QCStatus_e ret = m_drvClient.NegotiateBufferReq( bufType, bufNum, bufSize );
    if ( QC_STATUS_OK == ret )
    {
        if ( bufNumAvail < bufNum )
        {
            QC_ERROR( "%s-buf-type:%d, app alloc count:%" PRIu32 ", but driver req count:%" PRIu32,
                      (m_drvClient.GetType() == VIDEO_ENC) ? "enc" : "dec", bufType, bufNumAvail,
                                      bufNum );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            m_bufSize[bufType] = bufSize;
            QC_INFO( "%s-buf-type:%d, num:%" PRIu32 ", size=%" PRIu32,
                     (m_drvClient.GetType() == VIDEO_ENC) ? "enc" : "dec", bufType,
                                     vidFrmDescList->size(), m_bufSize[bufType] );
        }
    } else {
        QC_ERROR( "driver client NegotiateBufferReq returned an error: %d", ret );
    }

    return ret;
}

QCStatus_e VidcNodeBase::FreeInputBuffer()
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( auto buffer : m_inputBufferList )   // it means non dynamic mode
    {
        QC_DEBUG( "FreeInputBuffer begin" );
        ret = m_drvClient.FreeBuffer( VIDEO_CODEC_BUF_INPUT, m_inputBufferList );
    }

    return ret;
}

QCStatus_e VidcNodeBase::FreeOutputBuffer()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( !m_outputBufferList.empty() )   // it means non dynamic mode
    {
        QC_DEBUG( "FreeOutputBuffer begin" );
        ret = m_drvClient.FreeBuffer( VIDEO_CODEC_BUF_OUTPUT, m_outputBufferList );
        QC_DEBUG( "FreeOutputBuffer end" );
    }

    return ret;
}

vidc_color_format_type VidcNodeBase::GetVidcFormat( QCImageFormat_e format )
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

void VidcNodeBase::PrintConfig()
{
    if ( VIDEO_ENC == m_drvClient.GetType() )
    {
        QC_DEBUG( "EncoderConfig:" );
    }
    else
    {
        QC_DEBUG( "DecoderConfig:" );
    }

    m_drvClient.PrintCodecConfig();

    QC_DEBUG( "FrameWidth = %" PRIu32, m_pConfig->width );
    QC_DEBUG( "FrameHeight = %" PRIu32, m_pConfig->height );
    QC_DEBUG( "Fps = %" PRIu32, m_pConfig->frameRate );
    QC_DEBUG( "InBufCount = %" PRIu32 " [0 means minimum]", m_inputBufferList.size() );
    QC_DEBUG( "OutBufCount = %" PRIu32 " [0 means minimum]", m_outputBufferList.size() );
    QC_DEBUG( "InBufSize = %" PRIu32, m_bufSize[VIDEO_CODEC_BUF_INPUT] );
    QC_DEBUG( "OutBufSize = %" PRIu32, m_bufSize[VIDEO_CODEC_BUF_OUTPUT] );
}

void VidcNodeBase::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload )
{
    QC_DEBUG( "VidcNodeBase::EventCallback: Received event: %d, pPayload:%p", eventId, pPayload );

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

}   // namespace QC::Node
