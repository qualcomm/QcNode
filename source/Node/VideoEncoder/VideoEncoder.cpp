// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include <cmath>
#include <unistd.h>

#include "QC/Common/Types.hpp"
#include "QC/Node/VideoEncoder.hpp"

namespace QC
{
namespace Node
{

using namespace QC;
using namespace QC::component;

QCStatus_e VideoEncoder::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    const QCNodeConfigBase_t &cfg = m_configIfs.Get();
    const VideoEncoder_Config_t *pConfig =
            dynamic_cast<const VideoEncoder_Config_t *>( &cfg );
    bool bNodeBaseInitDone = false;
    bool bVideInitDone = false;

    m_callback = config.callback;

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

    QC_INFO( "initialization begin" );

    if ( QC_STATUS_OK == status )
    {
        status = m_vide.Init( m_nodeId.name.c_str(), &pConfig->params );
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
        status = m_vide.RegisterCallback( NVE_InFrameCallback, NVE_OutFrameCallback,
                                          NVE_EventCallback, this );
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

    QC_INFO( "initialization done" );

    return status;
}

QCStatus_e VideoEncoder::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    QCStatus_e status2;

    status2 = m_vide.Deinit();
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

QCStatus_e VideoEncoder::Start()
{
    return m_vide.Start();
}

QCStatus_e VideoEncoder::Stop()
{
    return m_vide.Stop();
}

QCStatus_e VideoEncoder::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    // INPUT:
    QCBufferDescriptorBase_t &inBufDesc =
                    frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID );
    const QCSharedVideoFrameDescriptor_t *pInSharedBuffer =
                    dynamic_cast<QCSharedVideoFrameDescriptor_t*>( &inBufDesc );

    // OUTPUT:
    QCBufferDescriptorBase_t &outBufDesc =
                    frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID );
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

QCStatus_e VideoEncoder::SubmitInputFrame( const QCSharedVideoFrameDescriptor *pInput )
{
    QC_DEBUG( "submit input frame begin: type=%d, size=%lu", pInput->buffer.type,
              pInput->buffer.size );

    const VideoEncoder_InputFrame_t inputFrame = { .sharedBuffer = pInput->buffer,
                                                   .timestampNs = pInput->timestampNs,
                                                   .appMarkData = pInput->appMarkData };

    return m_vide.SubmitInputFrame( &inputFrame );
}

QCStatus_e VideoEncoder::SubmitOutputFrame( const QCSharedVideoFrameDescriptor *pOutput )
{
    QC_DEBUG( "submit output frame begin: type=%d, size=%lu", pOutput->buffer.type,
              pOutput->buffer.size );

    const VideoEncoder_OutputFrame_t outputFrame = { .sharedBuffer = pOutput->buffer,
                                                     .timestampNs = pOutput->timestampNs,
                                                     .appMarkData = pOutput->appMarkData,
                                                     .frameFlag = pOutput->frameFlag,
                                                     .frameType = pOutput->frameType };

    return m_vide.SubmitOutputFrame( &outputFrame );
}

void VideoEncoder::InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame )
{
    QCSharedVideoFrameDescriptor buffDesc;
    buffDesc.buffer = pInputFrame->sharedBuffer;
    buffDesc.appMarkData = pInputFrame->appMarkData;
    buffDesc.timestampNs = pInputFrame->timestampNs;

    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID + 1 );
    frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID, buffDesc );

    QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK,
                               m_vide.GetState() );

    m_callback( evtInfo );
}

void VideoEncoder::OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame )
{
    QCSharedVideoFrameDescriptor buffDesc;
    buffDesc.buffer = pOutputFrame->sharedBuffer;
    buffDesc.frameFlag = pOutputFrame->frameFlag;
    buffDesc.frameType = pOutputFrame->frameType;
    buffDesc.appMarkData = pOutputFrame->appMarkData;
    buffDesc.timestampNs = pOutputFrame->timestampNs;

    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID + 1 );
    frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, buffDesc );

    QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK,
                               m_vide.GetState() );

    m_callback( evtInfo );
}

void VideoEncoder::EventCallback( const VideoEncoder_EventType_e eventId, const void *pEvent )
{
    QCSharedFrameDescriptorNode frameDesc( QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID + 1 );
    QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK,
                               m_vide.GetState() );

    QCBufferDescriptorBase_t errDesc;
    errDesc.name = "VideoEncoder ERROR: " + std::to_string( eventId );
    errDesc.pBuf = const_cast<void *>( pEvent );
    errDesc.size = sizeof( vidc_drv_msg_info_type );
    frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, errDesc );

    m_callback( evtInfo );
}

QCStatus_e VideoEncoderConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = VerifyStaticConfig( dt, errors );

    if ( QC_STATUS_OK == status )
    {
        m_config.nodeId.name = dt.Get<std::string>( "name", "unnamed" );
        m_config.nodeId.id = dt.Get<uint32_t>( "id", 0 );
        m_config.params.width = dt.Get<uint32_t>( "width", 0 );
        m_config.params.height = dt.Get<uint32_t>( "height", 0 );
        m_config.params.bitRate = dt.Get<uint32_t>( "bitRate", 0 );
        m_config.params.gop = dt.Get<uint32_t>( "gop", 0 );
        m_config.params.frameRate = dt.Get<uint32_t>( "frameRate", 0 );

        m_config.params.bInputDynamicMode = dt.Get<bool>( "inputDynamicMode", true );
        m_config.params.bOutputDynamicMode = dt.Get<bool>( "outputDynamicMode", false );
        m_config.params.numInputBufferReq = dt.Get<uint32_t>( "numInputBufferReq", 4 );
        m_config.params.numOutputBufferReq = dt.Get<uint32_t>( "numOutputBufferReq", 4 );

        std::string rateCtlMode = dt.Get<std::string>( "rateControlMode", "CBR_CFR" );
        if ("CBR_CFR" == rateCtlMode)
        {
            m_config.params.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
        }
        else if ("CBR_VFR" == rateCtlMode)
        {
            m_config.params.rateControlMode = VIDEO_ENCODER_RCM_CBR_VFR;
        }
        else if ("VBR_CFR" == rateCtlMode)
        {
            m_config.params.rateControlMode = VIDEO_ENCODER_RCM_VBR_CFR;
        }
        else if ("VBR_VFR" == rateCtlMode)
        {
            m_config.params.rateControlMode = VIDEO_ENCODER_RCM_VBR_VFR;
        }
        else if ("UNUSED" == rateCtlMode)
        {
            m_config.params.rateControlMode = VIDEO_ENCODER_RCM_UNUSED;
        }
        else
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }

        std::string profile = dt.Get<std::string>( "profile", "HEVC_MAIN" );
        if ("H264_BASELINE" == profile) {
            m_config.params.profile = VIDEO_ENCODER_PROFILE_H264_BASELINE;
        }
        else if ("H264_HIGH" == profile)
        {
            m_config.params.profile = VIDEO_ENCODER_PROFILE_H264_HIGH;
        }
        else if ("H264_MAIN" == profile)
        {
            m_config.params.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
        }
        else if ("HEVC_MAIN" == profile)
        {
            m_config.params.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
        }
        else if ("HEVC_MAIN10" == profile)
        {
            m_config.params.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN10;
        } else {
            status = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.params.inFormat =
                dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );

        if (QC_IMAGE_FORMAT_MAX == m_config.params.inFormat)
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }

        m_config.params.outFormat =
                dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H265 );

        if (QC_IMAGE_FORMAT_MAX == m_config.params.outFormat)
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    return status;
}

QCStatus_e VideoEncoderConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e VideoEncoderConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
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

QCStatus_e VideoEncoderConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    bool inDyn = dt.Get<bool>( "inputDynamicMode", true );
    bool outDyn = dt.Get<bool>( "outputDynamicMode", true );

    if ( ( false == inDyn ) && ( dt.Get<uint32_t>( "numInputBufferReq", 0 ) ) == 0 )
    {
        errors += "the number of input buffers is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( false == outDyn && dt.Get<uint32_t>( "numOutputBufferReq", 0 ) == 0 )
    {
        errors += "the number of output buffers is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "width", 0 ) == 0 )
    {
        errors += "the width is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "height", 0 ) == 0 )
    {
        errors += "the height is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    if ( dt.Get<uint32_t>( "bitrate", 0 ) == 0 )
    {
        errors += "the bit rate is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "frameRate", 0 ) == 0 )
    {
        errors += "the frame rate is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string rateCtlMode = m_dataTree.Get<std::string>( "rateControlMode", "UNUSED" );

    if ( ( "CBR_CFR" != rateCtlMode ) && ( "CBR_VFR" != rateCtlMode ) &&
         ( "VBR_CFR" != rateCtlMode ) && ( "VBR_VFR" != rateCtlMode ) &&
         ( "UNUSED" != rateCtlMode ) )
    {
        errors += "the rate control is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string profile = m_dataTree.Get<std::string>( "profile", "H264_MAIN" );

    if ( ( "CBR_CFR" != profile ) && ( "H264_BASELINE" != profile ) && ( "H264_HIGH" != profile ) &&
         ( "H264_MAIN" != profile ) && ( "HEVC_MAIN" != profile ) && ( "HEVC_MAIN10" != profile ) )
    {
        errors += "the video profile is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
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

const std::string &VideoEncoderConfigIfs::GetOptions()
{
    return m_options = "{}";
}

void VideoEncoder::NVE_InFrameCallback( const VideoEncoder_InputFrame_t *pInputFrame,
                                            void *pPrivData )
{
    VideoEncoder *pNve = reinterpret_cast<VideoEncoder *>( pPrivData );
    if ( pNve != nullptr )
    {
        pNve->InFrameCallback( pInputFrame );
    }
    else
    {
        QC_LOG_ERROR( "VideoEncoder_InFrameCallback: pPrivData param is invalid" );
    }
}

void VideoEncoder::NVE_OutFrameCallback( const VideoEncoder_OutputFrame_t *pOutputFrame,
                                             void *pPrivData )
{
    VideoEncoder *pNve = reinterpret_cast<VideoEncoder *>( pPrivData );
    if ( pNve != nullptr )
    {
        pNve->OutFrameCallback( pOutputFrame );
    }
    else
    {
        QC_LOG_ERROR( "VideoEncoder_OutFrameCallback: pPrivData param is invalid" );
    }
}

void VideoEncoder::NVE_EventCallback( const VideoEncoder_EventType_e eventId,
                                          const void *pEvent, void *pPrivData )
{
    VideoEncoder *pNve = reinterpret_cast<VideoEncoder *>( pPrivData );
    if ( pNve != nullptr )
    {
        pNve->EventCallback( eventId, pEvent );
    }
    else
    {
        QC_LOG_ERROR( "VideoEncoder_EventCallback: pPrivData param is invalid" );
    }
}

}   // namespace Node
}   // namespace QC
