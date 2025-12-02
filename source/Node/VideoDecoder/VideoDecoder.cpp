// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <unistd.h>
#include <cmath>
#include <string>
#include <iostream>

#include "QC/Common/Types.hpp"
#include "QC/Node/VideoDecoder.hpp"

#define VIDEO_DECODER_MAX_BUFFER_REQ 64
#define VIDEO_DECODER_MIN_BUFFER_REQ 4

#define VIDEO_DECODER_MIN_RESOLUTION 128
#define VIDEO_DECODER_MAX_RESOLUTION 8192

namespace QC
{
namespace Node
{

QCStatus_e VideoDecoder::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    bool bBaseVidcInitDone = false;

    m_callback = config.callback;

    m_OutputStarted = false;
    m_OutputReconfigInprogress = false;

    status = m_configIfs.VerifyAndSet( config.config, errors );
    if ( QC_STATUS_OK == status )
    {
        m_pConfig = dynamic_cast<const VideoDecoder_Config_t *>( &m_configIfs.Get() );
        m_name = m_pConfig->nodeId.name;
        status = VidcNodeBase::Init( *m_pConfig );
    }
    else
    {
        QC_ERROR( "config error: %s", errors.c_str() );
    }

    if ( QC_STATUS_OK == status )
    {
        QC_INFO( "%s: initialization begin", m_name.c_str() );
        bBaseVidcInitDone = true;
    }
    else
    {
        QC_ERROR( "video-decoder %s node base init error: %d", m_name.c_str(), status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = ValidateConfig( );
    }
    else {
        QC_ERROR( "video-decoder %s vidcnodebase init error: %d", m_name.c_str(), status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        QC_INFO( "video-decoder %s init begin", m_name.c_str() );
    }

    if ( QC_STATUS_OK == status )
    {
        m_drvClient.Init( m_name, m_logger.GetLevel(), VIDEO_DEC, *m_pConfig );
        status = m_drvClient.OpenDriver(InFrameCallback, OutFrameCallback, EventCallback, this);
    }
    else
    {
        QC_ERROR( "video-decoder %s set profile level error: %d", m_name.c_str(), status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = InitDrvProperty();
    }
    else
    {
        QC_ERROR( "Something wrong happened in driver init error %d, Deiniting vidc", status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = NegotiateBufferReq( VIDEO_CODEC_BUF_INPUT );
    } else {
        QC_ERROR( "Something wrong happened in driver InitDrvProperty error %d, Deiniting vidc",
                  status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = AllocateBuffer(config.buffers, 0, VIDEO_CODEC_BUF_INPUT);
        if ( QC_STATUS_OK == status ) {
            for ( auto &buffer : m_inputBufferList )
            {
                status = CheckBuffer(buffer, VIDEO_CODEC_BUF_INPUT);
                if (QC_STATUS_OK != status)
                    break;
            }

            if (QC_STATUS_OK != status)
            {
                QC_ERROR( "Input buffer descriptors are not properly initialized, Diniting vidc");
                m_state = QC_OBJECT_STATE_ERROR;
            }
        }

        if (QC_STATUS_OK == status)
        {
            status = AllocateBuffer(config.buffers,
                                    m_pConfig->bInputDynamicMode ? 0 : m_pConfig->numInputBufferReq,
                                    VIDEO_CODEC_BUF_OUTPUT);
            if ( QC_STATUS_OK == status ) {
                for ( auto &buffer : m_outputBufferList )
                {
                    status = CheckBuffer(buffer, VIDEO_CODEC_BUF_OUTPUT);
                    if (QC_STATUS_OK != status)
                        break;
                }

                if (QC_STATUS_OK != status)
                {
                    QC_ERROR( "Output buffer descriptors are not properly initializded, Diniting vidc");
                    m_state = QC_OBJECT_STATE_ERROR;
                }
            }
        }

        if (QC_STATUS_OK == status)
        {
            status = SetBuffer(VIDEO_CODEC_BUF_INPUT);
        }
    }
    else
    {
        QC_ERROR( "Something wrong happened in buffer allocation or SetBuffer, Deiniting vidc" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = PostInit( );
        QC_INFO( "video-decoder init done" );
    }

    if ( QC_STATUS_OK == status )
    {
        status = ValidateBuffers( );
    }
    else {
        QC_ERROR( "video-decoder %s buffers init error: %d", m_name.c_str(), status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if (QC_STATUS_OK != status)
    {
        QC_ERROR( "video-decoder initialization failed, cleaning up resources" );

        // Clean up buffers first
        if ( !m_outputBufferList.empty() )
        {
            (void) FreeOutputBuffers();
            m_outputBufferList.clear();
        }
        if ( !m_inputBufferList.empty() )
        {
            (void) FreeInputBuffers();
            m_inputBufferList.clear();
        }

        // Close driver if it was opened
        if (m_drvClient.GetType() == VIDEO_ENC || m_drvClient.GetType() == VIDEO_DEC)
        {
            m_drvClient.CloseDriver();
        }

        m_state = QC_OBJECT_STATE_ERROR;

        if ( bBaseVidcInitDone )
        {
            (void) VidcNodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e VideoDecoder::Start()
{
    QCStatus_e ret = VidcNodeBase::Start();

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "dec start succeed" );
    }

    return ret;
}

QCStatus_e VideoDecoder::Stop()
{
    QCStatus_e ret = VidcNodeBase::Stop();

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

QCStatus_e VideoDecoder::InitDrvProperty()
{
    VidcCodecMeta_t meta;
    meta.width = m_pConfig->width;
    meta.height = m_pConfig->height;
    meta.frameRate = m_pConfig->frameRate;
    if ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_pConfig->inFormat )
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
        vidcColorFmt.color_format = VidcNodeBase::GetVidcFormat( m_pConfig->outFormat );
        ret = m_drvClient.SetDrvProperty( VIDC_I_COLOR_FORMAT,
                                          sizeof( vidc_color_format_config_type ),
                                          (uint8_t &) vidcColorFmt );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_DEC_ORDER_DECODE" );
        vidc_output_order_type order;
        order.output_order = VIDC_DEC_ORDER_DECODE;
        ret = m_drvClient.SetDrvProperty( VIDC_I_DEC_OUTPUT_ORDER, sizeof( vidc_output_order_type ),
                                          (uint8_t &) order );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_DEC_CONT_ON_RECONFIG" );
        vidc_enable_type enable;
        enable.enable = true;
        ret = m_drvClient.SetDrvProperty( VIDC_I_DEC_CONT_ON_RECONFIG, sizeof( vidc_enable_type ),
                                          (uint8_t &) enable );
    }

    return ret;
}

QCStatus_e VideoDecoder::ValidateConfig( )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == m_pConfig )
    {
        QC_ERROR( "m_pConfig is null pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( m_pConfig->width < VIDEO_DECODER_MIN_RESOLUTION ) ||
             ( m_pConfig->height < VIDEO_DECODER_MIN_RESOLUTION ) ||
             ( m_pConfig->width > VIDEO_DECODER_MAX_RESOLUTION ) ||
             ( m_pConfig->height > VIDEO_DECODER_MAX_RESOLUTION ) )
        {
            QC_ERROR( "width %" PRIu32 " height %" PRIu32 " not in [%s, %d] ",
                      m_pConfig->width, m_pConfig->height,
                      VIDEO_DECODER_MIN_RESOLUTION, VIDEO_DECODER_MAX_RESOLUTION);
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( ( QC_STATUS_OK == ret ) && ( QC_IMAGE_FORMAT_COMPRESSED_H265 != m_pConfig->inFormat ) &&
         ( QC_IMAGE_FORMAT_COMPRESSED_H264 != m_pConfig->inFormat ) )
    {
        QC_ERROR( "input format: %d not supported!", m_pConfig->inFormat );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( QC_IMAGE_FORMAT_NV12 != m_pConfig->outFormat ) &&
         ( QC_IMAGE_FORMAT_P010 != m_pConfig->outFormat ) )
    {
        QC_ERROR( "output format: %d not supported!", m_pConfig->outFormat );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( m_pConfig->numInputBufferReq > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
                    ( m_pConfig->numInputBufferReq < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numInputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  m_pConfig->numInputBufferReq, VIDEO_DECODER_MIN_BUFFER_REQ,
                  VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( m_pConfig->numOutputBufferReq > VIDEO_DECODER_MAX_BUFFER_REQ ) ||
                    ( m_pConfig->numOutputBufferReq < VIDEO_DECODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numOutputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  m_pConfig->numOutputBufferReq, VIDEO_DECODER_MIN_BUFFER_REQ,
                  VIDEO_DECODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e VideoDecoder::CheckBuffer( const VideoFrameDescriptor_t &frameDesc,
                                      VideoCodec_BufType_e bufferType )
{
    QCStatus_e ret = ValidateBuffer( frameDesc, bufferType );

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "validate buffer failed, type:%d", bufferType );
    }
    else
    {
        if ( bufferType == VIDEO_CODEC_BUF_INPUT )
        {
            if ( 0 == frameDesc.size )
            {
                QC_INFO( "pBuffer size %zu is invalid", frameDesc.size );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
        else
        {
            if ( frameDesc.size < (size_t) m_bufSize[VIDEO_CODEC_BUF_OUTPUT] )
            {
                QC_ERROR( "pBuffer size %zu is smaller than vidcOutputBufferSize %" PRIu32,
                          frameDesc.size, m_bufSize[VIDEO_CODEC_BUF_OUTPUT] );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
    }

    return ret;
}

QCStatus_e VideoDecoder::HandleOutputReconfig()
{
    QCStatus_e ret = QC_STATUS_OK;

    std::lock_guard<std::mutex> lock(m_reconfigMutex);

    if ( m_OutputReconfigInprogress )
     {
         QC_WARN("Output reconfig already in progress, ignoring duplicate event");
         ret = QC_STATUS_BAD_STATE;
     }

    if ( QC_STATUS_OK == ret )
    {
        m_OutputReconfigInprogress = true;

        QC_INFO( "handle output-reconfig begin" );

        ret = NegotiateBufferReq( VIDEO_CODEC_BUF_OUTPUT );

        if ( ( QC_STATUS_OK == ret ) && ( false == m_pConfig->bOutputDynamicMode ) )
        {
            ret = VidcNodeBase::SetBuffer( VIDEO_CODEC_BUF_OUTPUT );
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = m_drvClient.StartDriver( VIDEO_CODEC_START_OUTPUT );
        }

        if (QC_STATUS_OK != ret)
        {
            m_OutputReconfigInprogress = false;
            QC_ERROR( "Output reconfig failed, state may be inconsistent" );
        }
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

        if ( false == m_pConfig->bOutputDynamicMode )
        {
            for ( auto &buffer : m_outputBufferList )
            {
                ret = SubmitOutputFrame( buffer );
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

QCStatus_e VideoDecoder::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    // INPUT:
    QCBufferDescriptorBase_t &inBufDesc = frameDesc.GetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID );
    VideoFrameDescriptor_t *pInSharedBuffer = dynamic_cast<VideoFrameDescriptor_t*>( &inBufDesc );

    // OUTPUT:
    QCBufferDescriptorBase_t &outBufDesc = frameDesc.GetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID );
    VideoFrameDescriptor_t *pOutSharedBuffer = dynamic_cast<VideoFrameDescriptor_t*>( &outBufDesc );

    if ((nullptr == pInSharedBuffer) && (nullptr == pOutSharedBuffer))
    {
        status = QC_STATUS_INVALID_BUF;
    }

    if (QC_STATUS_OK == status)
    {
        if (nullptr != pInSharedBuffer)
        {
            status = SubmitInputFrame( *pInSharedBuffer );
            QC_DEBUG( "submit input frame done: status=%d", status );
        }
    }

    if (QC_STATUS_OK == status)
    {
        if (nullptr != pOutSharedBuffer)
        {
            status = SubmitOutputFrame( *pOutSharedBuffer );
            QC_DEBUG( "submit output frame done: status=%d", status );
        }
    }

    return status;
}

QCStatus_e VideoDecoder::SubmitInputFrame( VideoFrameDescriptor_t &inFrameDesc )
{
    QC_DEBUG( "submit input frame descriptor begin: type=%d, size=%lu", inFrameDesc.type,
              inFrameDesc.size );

    QCStatus_e ret = ValidateFrameSubmission( inFrameDesc, VIDEO_CODEC_BUF_INPUT, true);

    if ( QC_STATUS_OK == ret )
    {
        ret = CheckBuffer( inFrameDesc, VIDEO_CODEC_BUF_INPUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.EmptyBuffer( inFrameDesc );
    }

    QC_DEBUG( "SubmitInputFrame done" );

    return ret;
}

QCStatus_e VideoDecoder::SubmitOutputFrame( VideoFrameDescriptor_t &outFrameDesc )
{
    QC_DEBUG( "submit output frame descriptor begin: type=%d, size=%lu", outFrameDesc.type,
              outFrameDesc.size );

    QCStatus_e ret = ValidateFrameSubmission( outFrameDesc, VIDEO_CODEC_BUF_OUTPUT, false);

    if ( QC_STATUS_OK == ret )
    {
        ret = CheckBuffer( outFrameDesc, VIDEO_CODEC_BUF_OUTPUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.FillBuffer( outFrameDesc );
    }

    QC_DEBUG( "SubmitOutputFrame done" );

    return ret;
}

void VideoDecoder::InFrameCallback(VideoFrameDescriptor_t &inFrameDesc )
{
    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID + 1 );

    frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, inFrameDesc );

    if ( m_callback ) {
        QCNodeEventInfo_t evtInfo( frameDesc, m_configIfs.Get().nodeId, QC_STATUS_OK, GetState() );
        m_callback( evtInfo );
    }
}

void VideoDecoder::OutFrameCallback( VideoFrameDescriptor_t &outFrameDesc )
{
    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID + 1 );

    frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, outFrameDesc );

    if ( m_callback ) {
        QCNodeEventInfo_t evtInfo( frameDesc, m_configIfs.Get().nodeId, QC_STATUS_OK, GetState() );
        m_callback( evtInfo );
    }
}

void VideoDecoder::EventCallback( VideoDecoder_EventType_e eventId, const void *pEvent )
{
    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_DECODER_EVENT_BUFF_ID + 1 );

    switch (eventId)
    {
    case VIDEO_CODEC_EVT_OUTPUT_RECONFIG:
        if (QC_STATUS_OK != HandleOutputReconfig())
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
        VidcNodeBase::EventCallback( eventId, pEvent );
        break;
    }

    if ( m_callback ) {
        NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_DECODER_EVENT_BUFF_ID + 1 );
        QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK, GetState() );

        QCBufferDescriptorBase_t errDesc;
        errDesc.name = std::to_string( eventId );
        errDesc.pBuf = const_cast<void *>( pEvent );
        errDesc.size = sizeof( vidc_drv_msg_info_type );
        frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, errDesc );

        m_callback( evtInfo );
    }
}

QCStatus_e VideoDecoderConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    return QC_STATUS_OK;
}

QCStatus_e VideoDecoderConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e VideoDecoderConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = VidcNodeBaseConfigIfs::VerifyAndSet( config, errors, m_config );

    if ( QC_STATUS_OK == status )
    {
        DataTree dt;
        status = m_dataTree.Get( "static", dt );
        if ( QC_STATUS_OK == status )
        {
            status = ParseStaticConfig( dt, errors );
        }
        else
        {
            status = m_dataTree.Get( "dynamic", dt );
            if ( QC_STATUS_OK == status )
            {
                status = ApplyDynamicConfig( dt, errors );
            }
        }
    }

    return status;
}

const std::string &VideoDecoderConfigIfs::GetOptions()
{
    return m_options = "{}";
}

void VideoDecoder::InFrameCallback( VideoFrameDescriptor_t &inFrameDesc, void *pPrivData )
{
    VideoDecoder *nvd = reinterpret_cast<VideoDecoder *>( pPrivData );
    if ( nvd != nullptr )
    {
        nvd->InFrameCallback( inFrameDesc );
    }
    else
    {
        QC_LOG_ERROR( "VideoDecoder_InFrameCallback: pPrivData param is invalid" );
    }
}

void VideoDecoder::OutFrameCallback( VideoFrameDescriptor_t &outFrameDesc, void *pPrivData )
{
    VideoDecoder *nvd = reinterpret_cast<VideoDecoder *>( pPrivData );
    if ( nvd != nullptr )
    {
        nvd->OutFrameCallback( outFrameDesc );
    }
    else
    {
        QC_LOG_ERROR( "VideoDecoder_OutFrameCallback: pPrivData param is invalid" );
    }
}

void VideoDecoder::EventCallback( VideoCodec_EventType_e eventId, const void *pEvent,
                                  void *pPrivData )
{
    VideoDecoder *nvd = reinterpret_cast<VideoDecoder *>( pPrivData );
    if ( nvd != nullptr )
    {
        nvd->EventCallback( eventId, pEvent );
    }
    else
    {
        QC_LOG_ERROR( "VideoDecoder_EventCallback: pPrivData param is invalid" );
    }
}

}   // namespace Node
}   // namespace QC
