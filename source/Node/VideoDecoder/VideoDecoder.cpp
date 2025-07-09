// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include <cmath>
#include <unistd.h>

#include "QC/Common/Types.hpp"
#include "QC/Node/VideoDecoder.hpp"

namespace QC
{
namespace Node
{

using namespace QC;
using namespace QC::component;

QCStatus_e VideoDecoder::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const VideoDecoder_Config_t *pConfig =
                    dynamic_cast<const VideoDecoder_Config_t *>( &cfg );
    bool bNodeBaseInitDone = false;
    bool bVideInitDone = false;

    m_callback = config.callback;

    QC_DEBUG( "initialization begin" );

    status = m_configIfs.VerifyAndSet( config.config, errors );
    if ( QC_STATUS_OK == status )
    {
        status = NodeBase::Init( cfg.nodeId );
        bNodeBaseInitDone = true;
    }
    else
    {
        QC_ERROR( "config error: %s", errors.c_str() );
    }

    if ( QC_STATUS_OK == status )
    {
        status = (QCStatus_e) m_vide.Init( m_nodeId.name.c_str(), &pConfig->params );
        if ( QC_STATUS_OK == status )
        {
            bVideInitDone = true;
        }
        else
        {
            QC_ERROR( "init error: %d", status );
        }
    }
    else
    {
        QC_ERROR( "NodeBase init error: %d", status );
    }

    if ( QC_STATUS_OK == status )
    {
        status = static_cast<QCStatus_e>( m_vide.RegisterCallback(
                                                                   NVD_InFrameCallback,
                                                                   NVD_OutFrameCallback,
                                                                   NVD_EventCallback,
                                                                   this ) );
    }
    else
    {
        QC_ERROR( "callback registration error: %d", status );
    }

    if ( QC_STATUS_OK != status )
    {   // do error clean up
        if ( bVideInitDone )
        {
            m_vide.Deinit();
        }
        if ( bNodeBaseInitDone )
        {
            (void) NodeBase::DeInitialize();
        }
    }

    QC_DEBUG( "initialization done" );

    return status;
}

QCStatus_e VideoDecoder::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = (QCStatus_e) m_vide.Deinit();
    if ( QC_STATUS_OK == status2 )
    {
        status = status2;
    }

    status2 = NodeBase::DeInitialize();
    if ( QC_STATUS_OK == status2 )
    {
        status = status2;
    }

    return status;
}

QCStatus_e VideoDecoder::Start()
{
    return static_cast<QCStatus_e>( m_vide.Start() );
}

QCStatus_e VideoDecoder::Stop()
{
    return static_cast<QCStatus_e>( m_vide.Stop() );
}

QCObjectState_e VideoDecoder::GetState()
{
    return (QCObjectState_e) m_vide.GetState();
}

QCStatus_e VideoDecoder::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    // INPUT:
    QCBufferDescriptorBase_t &inBufDesc =
                    frameDesc.GetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID );
    const QCSharedVideoFrameDescriptor_t *pInSharedBuffer =
                    dynamic_cast<QCSharedVideoFrameDescriptor_t*>( &inBufDesc );

    // OUTPUT:
    QCBufferDescriptorBase_t &outBufDesc =
                    frameDesc.GetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID );
    const QCSharedVideoFrameDescriptor_t *pOutSharedBuffer =
                    dynamic_cast<QCSharedVideoFrameDescriptor_t*>( &outBufDesc );

    if ((nullptr == pInSharedBuffer) && (nullptr == pOutSharedBuffer))
    {
        status = QC_STATUS_INVALID_BUF;
    }

    if (QC_STATUS_OK == status)
    {
        if (nullptr != pInSharedBuffer)
        {
            status = SubmitInputFrame( pInSharedBuffer );
            QC_DEBUG( "submit input frame done: status=%d", status );
        }
    }

    if (QC_STATUS_OK == status)
    {
        if (nullptr != pOutSharedBuffer)
        {
            status = SubmitOutputFrame( pOutSharedBuffer );
            QC_DEBUG( "submit output frame done: status=%d", status );
        }
    }

    return status;
}

QCStatus_e VideoDecoder::SubmitInputFrame( const QCSharedVideoFrameDescriptor *pInput )
{
    QC_DEBUG( "submit input frame begin: type=%d, size=%lu", pInput->buffer.type,
              pInput->buffer.size );

    const VideoDecoder_InputFrame_t inputFrame = { .sharedBuffer = pInput->buffer,
                                                   .timestampNs = pInput->timestampNs,
                                                   .appMarkData = pInput->appMarkData };

    return m_vide.SubmitInputFrame( &inputFrame );
}

QCStatus_e VideoDecoder::SubmitOutputFrame( const QCSharedVideoFrameDescriptor *pOutput )
{
    QC_DEBUG( "submit output frame begin: type=%d, size=%lu", pOutput->buffer.type,
              pOutput->buffer.size );

    const VideoDecoder_OutputFrame_t outputFrame = { .sharedBuffer = pOutput->buffer,
                                                     .timestampNs = pOutput->timestampNs,
                                                     .appMarkData = pOutput->appMarkData,
                                                     .frameFlag = pOutput->frameFlag };

    return m_vide.SubmitOutputFrame( &outputFrame );
}

void VideoDecoder::InFrameCallback(const VideoDecoder_InputFrame_t *pInputFrame )
{
    QCSharedVideoFrameDescriptor buffDesc;
    buffDesc.buffer = pInputFrame->sharedBuffer;
    buffDesc.appMarkData = pInputFrame->appMarkData;
    buffDesc.timestampNs = pInputFrame->timestampNs;

    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID + 1 );
    frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, buffDesc );

    QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK, m_vide.GetState() );

    m_callback( evtInfo );
}

void VideoDecoder::OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame )
{
    QCSharedVideoFrameDescriptor buffDesc;
    buffDesc.buffer = pOutputFrame->sharedBuffer;
    buffDesc.frameFlag = pOutputFrame->frameFlag;
    buffDesc.appMarkData = pOutputFrame->appMarkData;
    buffDesc.timestampNs = pOutputFrame->timestampNs;

    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID + 1 );
    frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, buffDesc );

    QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK, m_vide.GetState() );

    m_callback( evtInfo );
}

void VideoDecoder::EventCallback( const VideoDecoder_EventType_e eventId, const void *pEvent )
{
    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_DECODER_EVENT_BUFF_ID + 1 );
    QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK,
                               m_vide.GetState() );

    QCBufferDescriptorBase_t errDesc;
    errDesc.name = "VideoDecoder ERROR: " + std::to_string( eventId );
    errDesc.pBuf = const_cast<void *>( pEvent );
    errDesc.size = sizeof( vidc_drv_msg_info_type );
    frameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_EVENT_BUFF_ID, errDesc );

    m_callback( evtInfo );
}

QCStatus_e VideoDecoderConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = VerifyStaticConfig( dt, errors );

    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "unnamed" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", 0 );
        m_config.params.width = dt.Get<uint32_t>( "width", 0 );
        m_config.params.height = dt.Get<uint32_t>( "height", 0 );
        m_config.params.frameRate = dt.Get<uint32_t>( "frameRate", 0 );

        m_config.params.bInputDynamicMode = dt.Get<bool>( "inputDynamicMode", true );
        m_config.params.bOutputDynamicMode = dt.Get<bool>( "outputDynamicMode", false );
        m_config.params.numInputBuffer = dt.Get<uint32_t>( "numInputBuffer", 0 );
        m_config.params.numOutputBuffer = dt.Get<uint32_t>( "numOutputBuffer", 0 );

        m_config.params.inFormat =
                dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H265 );

        if (QC_IMAGE_FORMAT_MAX == m_config.params.inFormat)
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.params.outFormat =
                dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_NV12 );

        if (QC_IMAGE_FORMAT_MAX == m_config.params.outFormat)
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    return status;
}

QCStatus_e VideoDecoderConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e VideoDecoderConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = NodeConfigIfs::VerifyAndSet( config, errors );

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

QCStatus_e VideoDecoderConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    bool inDyn = dt.Get<bool>( "inputDynamicMode", true );
    bool outDyn = dt.Get<bool>( "outputDynamicMode", false );

    if ( false == inDyn && dt.Get<uint32_t>( "numInputBuffer", 0 ) == 0 )
    {
        errors += "the number of input buffers is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    } else {

    }

    if ( false == outDyn && dt.Get<uint32_t>( "numOutputBuffer", 0 ) == 0 )
    {
        errors += "the number of output buffers is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    } else {

    }

    if ( dt.Get<uint32_t>( "width", 0 ) == 0 )
    {
        errors += "the width is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    } else {

    }

    if ( dt.Get<uint32_t>( "height", 0 ) == 0 )
    {
        errors += "the height is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    } else {

    }

    if ( dt.Get<uint32_t>( "frameRate", 0 ) == 0 )
    {
        errors += "the frame rate is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    } else {

    }

    if ( QC_IMAGE_FORMAT_MAX ==
         m_dataTree.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 ) )
    {
        errors += "the input image format is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_IMAGE_FORMAT_COMPRESSED_MAX ==
         m_dataTree.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H265 ) )
    {
        errors += "the output image format is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

const std::string &VideoDecoderConfigIfs::GetOptions()
{
    return m_options = "{}";
}

void VideoDecoder::NVD_InFrameCallback( const VideoDecoder_InputFrame_t *pInputFrame,
                                        void *pPrivData )
{
    VideoDecoder *nvd = reinterpret_cast<VideoDecoder *>( pPrivData );
    if ( nvd != nullptr )
    {
        nvd->InFrameCallback( pInputFrame );
    }
    else
    {
        QC_LOG_ERROR( "VideoDecoder_InFrameCallback: pPrivData param is invalid" );
    }
}

void VideoDecoder::NVD_OutFrameCallback( const VideoDecoder_OutputFrame_t *pOutputFrame,
                                         void *pPrivData )
{
    VideoDecoder *nvd = reinterpret_cast<VideoDecoder *>( pPrivData );
    if ( nvd != nullptr )
    {
        nvd->OutFrameCallback( pOutputFrame );
    }
    else
    {
        QC_LOG_ERROR( "VideoDecoder_OutFrameCallback: pPrivData param is invalid" );
    }
}

void VideoDecoder::NVD_EventCallback( const VideoDecoder_EventType_e eventId,
                                      const void *pEvent, void *pPrivData )
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
