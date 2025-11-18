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

#define ARRAY_SIZE( a ) ( sizeof( ( a ) ) / sizeof( ( a )[0] ) )

#define VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I 30
#define VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I 0
#define VIDEO_ENCODER_DEFAULT_IDR_PERIOD 1
#define VIDEO_ENCODER_DEFAULT_BIT_RATE 64000
#define VIDEO_ENCODER_DEFAULT_FRAME_RATE 30

#define VIDEO_ENCODER_MAX_BUFFER_REQ 64
#define VIDEO_ENCODER_MIN_BUFFER_REQ 4

#define VIDEO_ENCODER_MIN_RESOLUTION 128
#define VIDEO_ENCODER_MAX_RESOLUTION 8192

typedef struct
{
    uint32_t maxFrameSize;
    uint32_t maxSizePerSec;
    uint32_t maxBitRate;
    uint32_t level;
    uint32_t profile;
} ProfileLevel_t;

typedef struct
{
    const ProfileLevel_t *pTable;
    uint32_t num;
} ProfileLevelTableRef_t;

static const ProfileLevel_t s_profileLevelH264BaseLineTable[] = {
        /*max mb per frame, max mb per sec, max bitrate, level, profile*/
        { 99, 1485, 64000, VIDC_LEVEL_H264_1, VIDC_PROFILE_H264_BASELINE },
        { 99, 1485, 128000, VIDC_LEVEL_H264_1b, VIDC_PROFILE_H264_BASELINE },
        { 396, 3000, 192000, VIDC_LEVEL_H264_1p1, VIDC_PROFILE_H264_BASELINE },
        { 396, 6000, 384000, VIDC_LEVEL_H264_1p2, VIDC_PROFILE_H264_BASELINE },
        { 396, 11880, 768000, VIDC_LEVEL_H264_1p3, VIDC_PROFILE_H264_BASELINE },
        { 396, 11880, 2000000, VIDC_LEVEL_H264_2, VIDC_PROFILE_H264_BASELINE },
        { 792, 19800, 4000000, VIDC_LEVEL_H264_2p1, VIDC_PROFILE_H264_BASELINE },
        { 1620, 20250, 4000000, VIDC_LEVEL_H264_2p2, VIDC_PROFILE_H264_BASELINE },
        { 1620, 40500, 10000000, VIDC_LEVEL_H264_3, VIDC_PROFILE_H264_BASELINE },
        { 3600, 108000, 14000000, VIDC_LEVEL_H264_3p1, VIDC_PROFILE_H264_BASELINE },
        { 5120, 216000, 20000000, VIDC_LEVEL_H264_3p2, VIDC_PROFILE_H264_BASELINE },
        { 8192, 245760, 20000000, VIDC_LEVEL_H264_4, VIDC_PROFILE_H264_BASELINE },
        { 8192, 245760, 50000000, VIDC_LEVEL_H264_4p1, VIDC_PROFILE_H264_BASELINE },
        { 8704, 522240, 50000000, VIDC_LEVEL_H264_4p2, VIDC_PROFILE_H264_BASELINE },
        { 22080, 589824, 135000000, VIDC_LEVEL_H264_5, VIDC_PROFILE_H264_BASELINE },
        { 36864, 983040, 240000000, VIDC_LEVEL_H264_5p1, VIDC_PROFILE_H264_BASELINE } };
static const ProfileLevel_t s_profileLevelH264HighTable[] = {
        /*max mb per frame, max mb per sec, max bitrate, level, profile*/
        { 99, 1485, 80000, VIDC_LEVEL_H264_1, VIDC_PROFILE_H264_HIGH },
        { 99, 1485, 200000, VIDC_LEVEL_H264_1b, VIDC_PROFILE_H264_HIGH },
        { 396, 3000, 300000, VIDC_LEVEL_H264_1p1, VIDC_PROFILE_H264_HIGH },
        { 396, 6000, 600000, VIDC_LEVEL_H264_1p2, VIDC_PROFILE_H264_HIGH },
        { 396, 11880, 1200000, VIDC_LEVEL_H264_1p3, VIDC_PROFILE_H264_HIGH },
        { 396, 11880, 3125000, VIDC_LEVEL_H264_2, VIDC_PROFILE_H264_HIGH },
        { 792, 19800, 6250000, VIDC_LEVEL_H264_2p1, VIDC_PROFILE_H264_HIGH },
        { 1620, 20250, 6250000, VIDC_LEVEL_H264_2p2, VIDC_PROFILE_H264_HIGH },
        { 1620, 40500, 15625000, VIDC_LEVEL_H264_3, VIDC_PROFILE_H264_HIGH },
        { 3600, 108000, 21875000, VIDC_LEVEL_H264_3p1, VIDC_PROFILE_H264_HIGH },
        { 5120, 216000, 31250000, VIDC_LEVEL_H264_3p2, VIDC_PROFILE_H264_HIGH },
        { 8192, 245760, 31250000, VIDC_LEVEL_H264_4, VIDC_PROFILE_H264_HIGH },
        { 8192, 245760, 62500000, VIDC_LEVEL_H264_4p1, VIDC_PROFILE_H264_HIGH },
        { 8704, 522240, 62500000, VIDC_LEVEL_H264_4p2, VIDC_PROFILE_H264_HIGH },
        { 22080, 589824, 168750000, VIDC_LEVEL_H264_5, VIDC_PROFILE_H264_HIGH },
        { 36864, 983040, 300000000, VIDC_LEVEL_H264_5p1, VIDC_PROFILE_H264_HIGH } };
static const ProfileLevel_t s_profileLevelH264MainTable[] = {
        /*max mb per frame, max mb per sec, max bitrate, level, profile*/
        { 99, 1485, 64000, VIDC_LEVEL_H264_1, VIDC_PROFILE_H264_MAIN },
        { 99, 1485, 128000, VIDC_LEVEL_H264_1b, VIDC_PROFILE_H264_MAIN },
        { 396, 3000, 192000, VIDC_LEVEL_H264_1p1, VIDC_PROFILE_H264_MAIN },
        { 396, 6000, 384000, VIDC_LEVEL_H264_1p2, VIDC_PROFILE_H264_MAIN },
        { 396, 11880, 768000, VIDC_LEVEL_H264_1p3, VIDC_PROFILE_H264_MAIN },
        { 396, 11880, 2000000, VIDC_LEVEL_H264_2, VIDC_PROFILE_H264_MAIN },
        { 792, 19800, 4000000, VIDC_LEVEL_H264_2p1, VIDC_PROFILE_H264_MAIN },
        { 1620, 20250, 4000000, VIDC_LEVEL_H264_2p2, VIDC_PROFILE_H264_MAIN },
        { 1620, 40500, 10000000, VIDC_LEVEL_H264_3, VIDC_PROFILE_H264_MAIN },
        { 3600, 108000, 14000000, VIDC_LEVEL_H264_3p1, VIDC_PROFILE_H264_MAIN },
        { 5120, 216000, 20000000, VIDC_LEVEL_H264_3p2, VIDC_PROFILE_H264_MAIN },
        { 8192, 245760, 20000000, VIDC_LEVEL_H264_4, VIDC_PROFILE_H264_MAIN },
        { 8192, 245760, 50000000, VIDC_LEVEL_H264_4p1, VIDC_PROFILE_H264_MAIN },
        { 8704, 522240, 50000000, VIDC_LEVEL_H264_4p2, VIDC_PROFILE_H264_MAIN },
        { 22080, 589824, 135000000, VIDC_LEVEL_H264_5, VIDC_PROFILE_H264_MAIN },
        { 36864, 983040, 240000000, VIDC_LEVEL_H264_5p1, VIDC_PROFILE_H264_MAIN } };
static const ProfileLevel_t s_profileLevelHevcMainTable[] = {
        /*max sample per frame, max sample per sec, max bitrate, level, profile*/
        { 36864, 552960, 128000, VIDC_LEVEL_HEVC_1, VIDC_PROFILE_HEVC_MAIN },
        { 122880, 3686440, 1500000, VIDC_LEVEL_HEVC_2, VIDC_PROFILE_HEVC_MAIN },
        { 245760, 7372800, 3000000, VIDC_LEVEL_HEVC_21, VIDC_PROFILE_HEVC_MAIN },
        { 552960, 16588800, 6000000, VIDC_LEVEL_HEVC_3, VIDC_PROFILE_HEVC_MAIN },
        { 983040, 33177600, 10000000, VIDC_LEVEL_HEVC_31, VIDC_PROFILE_HEVC_MAIN },
        { 2228224, 66846720, 12000000, VIDC_LEVEL_HEVC_4, VIDC_PROFILE_HEVC_MAIN },
        { 2228224, 133693440, 20000000, VIDC_LEVEL_HEVC_41, VIDC_PROFILE_HEVC_MAIN },
        { 8912896, 267386880, 25000000, VIDC_LEVEL_HEVC_5, VIDC_PROFILE_HEVC_MAIN },
        { 8912896, 534773760, 40000000, VIDC_LEVEL_HEVC_51, VIDC_PROFILE_HEVC_MAIN },
        { 8912896, 1069547520, 60000000, VIDC_LEVEL_HEVC_52, VIDC_PROFILE_HEVC_MAIN },
        { 35651584, 1069547520, 60000000, VIDC_LEVEL_HEVC_6, VIDC_PROFILE_HEVC_MAIN } };
static const ProfileLevel_t s_profileLevelHevcMain10Table[] = {
        /*max sample per frame, max sample per sec, max bitrate, level, profile*/
        { 36864, 552960, 128000, VIDC_LEVEL_HEVC_1, VIDC_PROFILE_HEVC_MAIN10 },
        { 122880, 3686440, 1500000, VIDC_LEVEL_HEVC_2, VIDC_PROFILE_HEVC_MAIN10 },
        { 245760, 7372800, 3000000, VIDC_LEVEL_HEVC_21, VIDC_PROFILE_HEVC_MAIN10 },
        { 552960, 16588800, 6000000, VIDC_LEVEL_HEVC_3, VIDC_PROFILE_HEVC_MAIN10 },
        { 983040, 33177600, 10000000, VIDC_LEVEL_HEVC_31, VIDC_PROFILE_HEVC_MAIN10 },
        { 2228224, 66846720, 12000000, VIDC_LEVEL_HEVC_4, VIDC_PROFILE_HEVC_MAIN10 },
        { 2228224, 133693440, 20000000, VIDC_LEVEL_HEVC_41, VIDC_PROFILE_HEVC_MAIN10 },
        { 8912896, 267386880, 25000000, VIDC_LEVEL_HEVC_5, VIDC_PROFILE_HEVC_MAIN10 },
        { 8912896, 534773760, 40000000, VIDC_LEVEL_HEVC_51, VIDC_PROFILE_HEVC_MAIN10 },
        { 8912896, 1069547520, 60000000, VIDC_LEVEL_HEVC_52, VIDC_PROFILE_HEVC_MAIN10 },
        { 35651584, 1069547520, 60000000, VIDC_LEVEL_HEVC_6, VIDC_PROFILE_HEVC_MAIN10 } };

static const ProfileLevelTableRef_t s_profileLevelTables[] =
                {
        { s_profileLevelH264BaseLineTable, ARRAY_SIZE( s_profileLevelH264BaseLineTable ) },
        { s_profileLevelH264HighTable, ARRAY_SIZE( s_profileLevelH264HighTable ) },
        { s_profileLevelH264MainTable, ARRAY_SIZE( s_profileLevelH264MainTable ) },
        { s_profileLevelHevcMainTable, ARRAY_SIZE( s_profileLevelHevcMainTable ) },
        { s_profileLevelHevcMain10Table, ARRAY_SIZE( s_profileLevelHevcMain10Table ) } };

QCStatus_e VideoEncoder::Initialize( QCNodeInit_t &config )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string errors;
    bool bBaseVidcInitDone = false;
    uint32_t bufferIdx = 0;

    m_vidcEncoderData = {};
    m_vidcEncoderData.iPeriod.p_frames = VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I;
    m_vidcEncoderData.iPeriod.b_frames = VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I;
    m_vidcEncoderData.idrPeriod.idr_period = VIDEO_ENCODER_DEFAULT_IDR_PERIOD;
    m_vidcEncoderData.bitrate.target_bitrate = VIDEO_ENCODER_DEFAULT_BIT_RATE;
    m_vidcEncoderData.frameRate.fps_denominator = 1;

    status = m_configIfs.VerifyAndSet( config.config, errors );
    if ( QC_STATUS_OK == status )
    {
        m_pConfig = dynamic_cast<const VideoEncoder_Config_t *>( &m_configIfs.Get() );
        m_name = m_pConfig->nodeId.name;
        m_callback = config.callback;
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
        QC_ERROR( "video-encoder %s node base init error: %d", m_name.c_str(), status );
    }

    if ( QC_STATUS_OK == status )
    {
        status = ValidateConfig( );
    }
    else {
        QC_ERROR( "video-encoder %s vidcnodebase init error: %d", m_name.c_str(), status );
    }

    if ( QC_STATUS_OK == status )
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        QC_INFO( "video-encoder %s init begin", m_name.c_str() );
    }

    if ( QC_STATUS_OK == status )
    {
        /* init profile */
        status = SetVidcProfileLevel( m_pConfig->profile );
    }

    if ( QC_STATUS_OK == status )
    {
        m_drvClient.Init( m_name.c_str(), m_logger.GetLevel(), VIDEO_ENC, *m_pConfig );
        status = m_drvClient.OpenDriver(InFrameCallback, OutFrameCallback, EventCallback, this);
    }
    else
    {
        QC_ERROR( "video-encoder %s set profile level error: %d", m_name.c_str(), status );
    }

    if ( QC_STATUS_OK == status )
    {
        status = InitDrvProperty();
    }
    else
    {
        QC_ERROR( "Something wrong happened in driver init error %d, Deiniting vidc", status );
    }

    if ( QC_STATUS_OK == status )
    {
        status = GetInputInformation();
    } else {
        QC_ERROR( "Something wrong happened in driver InitDrvProperty error %d, Deiniting vidc",
                  status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = NegotiateBufferReq( VIDEO_CODEC_BUF_INPUT );
    } else {
        QC_ERROR( "Something wrong happened in driver GetInputInformation error %d, Deiniting vidc",
                  status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = NegotiateBufferReq( VIDEO_CODEC_BUF_OUTPUT );
    }
    else
    {
        QC_ERROR( "Something wrong happened in driver NegotiateBufferReq for input, Deiniting vidc" );
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
            if (QC_STATUS_OK == status)
            {
                status = SetBuffer(VIDEO_CODEC_BUF_OUTPUT);
            }
        }
    }
    else
    {
        QC_ERROR( "Something wrong happened in driver NegotiateBufferReq for output, Deiniting vidc" );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if ( QC_STATUS_OK == status )
    {
        status = PostInit( );
        PrintEncoderConfig();
        QC_INFO( "video-encoder init done" );
    }

    if ( QC_STATUS_OK == status )
    {
        status = ValidateBuffers( );
    }
    else
    {
        QC_ERROR( "video-encoder %s buffers init error: %d", m_name.c_str(), status );
        m_state = QC_OBJECT_STATE_ERROR;
    }

    if (QC_STATUS_OK != status)
    {
        QC_ERROR( "video-encoder initialization failed, cleaning up resources" );

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

        m_drvClient.CloseDriver();

        m_state = QC_OBJECT_STATE_ERROR;

        if ( bBaseVidcInitDone )
        {
            (void) VidcNodeBase::DeInitialize();
        }
    }

    return status;
}

QCStatus_e VideoEncoder::Start()
{
    QCStatus_e ret = VidcNodeBase::Start();

    if ( ( QC_STATUS_OK == ret ) && ( !m_outputBufferList.empty() ) )
    {
        QC_INFO( "submit non-dynamic output frame begin, cnt:%u", m_outputBufferList.size() );
        for ( VideoFrameDescriptor_t buffer : m_outputBufferList )
        {
            ret = ValidateBuffer( buffer, VIDEO_CODEC_BUF_OUTPUT );
            if (QC_STATUS_OK == ret)
                ret = m_drvClient.FillBuffer( buffer );
        }
        QC_INFO( "submit non-dynamic output frame done" );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "enc start succeed" );
    }

    return ret;
}

QCStatus_e VideoEncoder::Stop()
{
    QCStatus_e ret = VidcNodeBase::Stop();

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "enc stop succeed" );
    }
    else
    {
        QC_ERROR( "enc stop failed" );
    }

    return ret;
}

QCStatus_e VideoEncoder::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e status = QC_STATUS_OK;

    // INPUT:
    QCBufferDescriptorBase_t &inBufDesc = frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID );
    VideoFrameDescriptor_t *pInSharedBuffer = dynamic_cast<VideoFrameDescriptor_t*>( &inBufDesc );

    // OUTPUT:
    QCBufferDescriptorBase_t &outBufDesc = frameDesc.GetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID );
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

QCStatus_e VideoEncoder::SubmitInputFrame( VideoFrameDescriptor_t &inFrameDesc )
{
    QC_DEBUG( "submit input frame descriptor begin: type=%d, size=%zu", inFrameDesc.type,
              inFrameDesc.size );

    QCStatus_e ret = ValidateFrameSubmission( inFrameDesc, VIDEO_CODEC_BUF_INPUT, true);

    if ( QC_STATUS_OK == ret )
    {
        ret = CheckBuffer( inFrameDesc, VIDEO_CODEC_BUF_INPUT );
    }

    /* TODO: Check out Configure() and pOnTheFlyCmd */
#if 0
    if ( ( QC_STATUS_OK == ret ) && ( nullptr != inFrameDesc->pOnTheFlyCmd ) )
    {
        int32_t i;
        for ( i = 0; i < inFrameDesc->numCmd; i++ )
        {
            ret = Configure( &inFrameDesc->pOnTheFlyCmd[i] );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "config propID %d failed", inFrameDesc->pOnTheFlyCmd[i].propID );
                break;
            }
        }
    }
#endif

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.EmptyBuffer( inFrameDesc );
    }

    return ret;
}

QCStatus_e VideoEncoder::SubmitOutputFrame( VideoFrameDescriptor_t &outFrameDesc )
{
    QC_DEBUG( "submit output frame descriptor begin: type=%d, size=%zu", outFrameDesc.type,
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

    return ret;
}

QCStatus_e VideoEncoder::InitDrvProperty()
{
    QCStatus_e ret = QC_STATUS_OK;

    VidcCodecMeta_t meta;
    meta.width = m_pConfig->width;
    meta.height = m_pConfig->height;
    meta.frameRate = m_pConfig->frameRate;

    if ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_pConfig->outFormat )
    {
        meta.codecType = VIDEO_CODEC_H265;
    }
    else
    {
        meta.codecType = VIDEO_CODEC_H264;
    }

    ret = m_drvClient.InitDriver( meta );

    if ( QC_STATUS_OK == ret )
    {
        /**< The encoder rate control type */
        vidc_rate_control_mode_type rateControl =
                        static_cast<vidc_rate_control_mode_type>( m_pConfig->rateControlMode );

        QC_INFO( "Setting VIDC_I_ENC_RATE_CONTROL 0x%x", rateControl );
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_RATE_CONTROL,
                                          sizeof( vidc_rate_control_mode_type ),
                                          (uint8_t &) ( rateControl ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        /**< Configures the uncompressed Buffer format */
        vidc_color_format_config_type colorFormatConfig;
        colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        colorFormatConfig.color_format = VidcNodeBase::GetVidcFormat( m_pConfig->inFormat );
        QC_INFO( "Setting VIDC_I_COLOR_FORMAT %d", colorFormatConfig.color_format );
        ret = m_drvClient.SetDrvProperty( VIDC_I_COLOR_FORMAT,
                                          sizeof( vidc_color_format_config_type ),
                                          (uint8_t &) ( colorFormatConfig ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.iPeriod.p_frames = m_pConfig->gop;
        m_vidcEncoderData.iPeriod.b_frames = VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I;
        QC_INFO( "Setting GOP %d",  m_pConfig->gop);
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_INTRA_PERIOD, sizeof( vidc_iperiod_type ),
                                          (uint8_t &) ( m_vidcEncoderData.iPeriod ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.idrPeriod.idr_period = VIDEO_ENCODER_DEFAULT_IDR_PERIOD;
        QC_INFO( "Setting ENC_IDR_PERIOD %d", m_vidcEncoderData.idrPeriod );
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_IDR_PERIOD, sizeof( vidc_idr_period_type ),
                                          (uint8_t &) ( m_vidcEncoderData.idrPeriod ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.bitrate.target_bitrate = m_pConfig->bitRate;
        QC_INFO( "Setting VIDC_I_TARGET_BITRATE %d", m_vidcEncoderData.bitrate);
        ret = m_drvClient.SetDrvProperty( VIDC_I_TARGET_BITRATE, sizeof( vidc_target_bitrate_type ),
                                          (uint8_t &) ( m_vidcEncoderData.bitrate ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.enableSyncFrameSeq.enable = m_pConfig->bSyncFrameSeqHdr;
        QC_INFO( "Setting VIDC_I_ENC_SYNC_FRAME_SEQ_HDR %d", m_vidcEncoderData.enableSyncFrameSeq.enable );
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_SYNC_FRAME_SEQ_HDR, sizeof( vidc_enable_type ),
                                          (uint8_t &) ( m_vidcEncoderData.enableSyncFrameSeq ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "Setting VIDC_I_PROFILE %d", m_vidcEncoderData.profile);
        ret = m_drvClient.SetDrvProperty( VIDC_I_PROFILE, sizeof( vidc_profile_type ),
                                          (uint8_t &) ( m_vidcEncoderData.profile ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "Setting VIDC_I_LEVEL %d", m_vidcEncoderData.level );
        ret = m_drvClient.SetDrvProperty( VIDC_I_LEVEL, sizeof( vidc_level_type ),
                                          (uint8_t &) ( m_vidcEncoderData.level ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "Setting VIDC_I_VPE_SPATIAL_TRANSFORM" );
        vidc_spatial_transform_type vidcSpatialTransform = { VIDC_ROTATE_NONE, VIDC_FLIP_NONE };
        ret = m_drvClient.SetDrvProperty( VIDC_I_VPE_SPATIAL_TRANSFORM,
                                          sizeof( vidc_spatial_transform_type ),
                                          (uint8_t &) ( vidcSpatialTransform ) );
    }

    return ret;
}

#if 0
QCStatus_e VideoEncoder::Configure( const VideoEncoder_OnTheFlyCmd_t *pCmd )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_RUNNING != m_state ) && ( QC_OBJECT_STATE_READY != m_state ) )
    {
        QC_WARN( "Not Configure since encoder is not ready or running!" );
        ret = QC_STATUS_BAD_STATE;
    }

    if ( ( QC_STATUS_OK == ret ) && ( nullptr == pCmd ) )
    {
        QC_ERROR( "pCmd is NULL pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "Configuring vidc" );
        switch ( pCmd->propID )
        {
            case VIDEO_ENCODER_PROP_BITRATE:
                QC_DEBUG( "Setting VIDC_I_TARGET_BITRATE %" PRIu32, m_bitRate );
                m_bitRate = pCmd->value;
                m_vidcEncoderData.bitrate.target_bitrate = m_bitRate;
                ret = m_drvClient.SetDrvProperty( VIDC_I_TARGET_BITRATE,
                                                  sizeof( vidc_target_bitrate_type ),
                                                  (uint8_t *) ( &m_vidcEncoderData.bitrate ) );
                break;
            case VIDEO_ENCODER_PROP_FRAME_RATE:
                m_frameRate = pCmd->value;
                QC_DEBUG( "Setting VIDC_I_FRAME_RATE.VIDC_BUFFER_OUTPUT %" PRIu32, m_frameRate );
                m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_OUTPUT;
                m_vidcEncoderData.frameRate.fps_numerator = m_frameRate;
                m_vidcEncoderData.frameRate.fps_denominator = 1;
                ret = m_drvClient.SetDrvProperty( VIDC_I_FRAME_RATE, sizeof( vidc_frame_rate_type ),
                                                  (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
                break;
            default:
                QC_ERROR( "propID %d not supported", pCmd->propID );
                ret = QC_STATUS_BAD_ARGUMENTS;
                break;
        }
    }
    return ret;
}
#endif

void VideoEncoder::PrintEncoderConfig()
{
    if ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_pConfig->outFormat )
    {
        QC_INFO( "EncoderConfig: Codec = H265" );
        QC_INFO( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        QC_INFO( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }
    else
    {
        QC_INFO( "EncoderConfig: Codec = H264" );
        QC_INFO( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        QC_INFO( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }

    QC_INFO( "EncoderConfig: RateControl = 0x%x ", m_pConfig->rateControlMode );
    QC_INFO( "EncoderConfig: BitRate = %" PRIu32, m_vidcEncoderData.bitrate.target_bitrate );
    QC_INFO( "EncoderConfig: NumPframes = %" PRIu32, m_vidcEncoderData.iPeriod.p_frames );
    QC_INFO( "EncoderConfig: NumBframes = %" PRIu32, m_vidcEncoderData.iPeriod.b_frames );
    QC_INFO( "EncoderConfig: IdrPeriod = %" PRIu32, m_vidcEncoderData.idrPeriod.idr_period );
}

QCStatus_e VideoEncoder::SetVidcProfileLevel( VideoEncoder_Profile_e profile )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint32_t i, num = 0;
    const ProfileLevel_t *pTable = nullptr;
    uint32_t mbPerFrame = 0, mbPerSec = 0;
    uint64_t samplePerFrame = 0, samplePerSec = 0;
    bool bFindFlag = false;
    m_vidcEncoderData.level.level = 0;
    m_vidcEncoderData.profile.profile = 0;

    if ( profile < VIDEO_ENCODER_PROFILE_MAX )
    {
        pTable = s_profileLevelTables[profile].pTable;
        num = s_profileLevelTables[profile].num;

        if ( ( QC_IMAGE_FORMAT_COMPRESSED_H264 == m_pConfig->outFormat ) &&
             ( m_pConfig->profile <= VIDEO_ENCODER_PROFILE_H264_MAIN ) )
        {
            mbPerFrame = ( ( m_pConfig->height + 15 ) >> 4 ) * ( ( m_pConfig->width + 15 ) >> 4 );
            mbPerSec = mbPerFrame * m_pConfig->frameRate;
            QC_DEBUG( "mbPerFrame %" PRIu32 " mbPerSec %" PRIu32, mbPerFrame, mbPerSec );
            for ( i = 0; ( i < num ) && ( false == bFindFlag ); i++ )
            {
                if ( mbPerFrame <= pTable[i].maxFrameSize )
                {
                    if ( mbPerSec <= pTable[i].maxSizePerSec )
                    {
                        if ( m_pConfig->bitRate <= pTable[i].maxBitRate )
                        {
                            QC_DEBUG( "set level = %" PRIu32 ", profile = %" PRIu32,
                                      pTable[i].level, pTable[i].profile );
                            m_vidcEncoderData.level.level = pTable[i].level;
                            m_vidcEncoderData.profile.profile = pTable[i].profile;
                        }
                    }
                }
            }
        }
        else if ( ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_pConfig->outFormat ) &&
                  ( profile >= VIDEO_ENCODER_PROFILE_HEVC_MAIN ) )
        {
            samplePerFrame = (uint64_t) m_pConfig->height * m_pConfig->width;
            samplePerSec = samplePerFrame * m_pConfig->frameRate;
            QC_DEBUG( "samplePerFrame %" PRIu64 " samplePerSec %" PRIu64, samplePerFrame,
                      samplePerSec );
            for ( i = 0; ( i < num ) && ( false == bFindFlag ); i++ )
            {
                if ( samplePerFrame <= pTable[i].maxFrameSize )
                {
                    if ( samplePerSec <= pTable[i].maxSizePerSec )
                    {
                        if ( m_pConfig->bitRate <= pTable[i].maxBitRate )
                        {
                            QC_DEBUG( "set level = %" PRIu32 ", profile = %" PRIu32,
                                      pTable[i].level, pTable[i].profile );
                            m_vidcEncoderData.level.level = pTable[i].level;
                            m_vidcEncoderData.profile.profile = pTable[i].profile;
                        }
                    }
                }
            }
        }
        else
        {
            QC_ERROR( "m_outFormat %d not match with profile %d", m_pConfig->outFormat, profile );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( ( 0 == m_vidcEncoderData.level.level ) || ( 0 == m_vidcEncoderData.profile.profile ) )
    {
        QC_ERROR( "profile %d not supported", profile );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e VideoEncoder::ValidateConfig( )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == m_pConfig )
    {
        QC_ERROR( "m_pConfig is null pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( m_pConfig->width < VIDEO_ENCODER_MIN_RESOLUTION ) ||
             ( m_pConfig->height < VIDEO_ENCODER_MIN_RESOLUTION ) ||
             ( m_pConfig->width > VIDEO_ENCODER_MAX_RESOLUTION ) ||
             ( m_pConfig->height > VIDEO_ENCODER_MAX_RESOLUTION ) )
        {
            QC_ERROR( "width %" PRIu32 " height %" PRIu32 " not in [%s, %d] ",
                      m_pConfig->width, m_pConfig->height,
                      VIDEO_ENCODER_MIN_RESOLUTION, VIDEO_ENCODER_MAX_RESOLUTION);
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( ( QC_STATUS_OK == ret ) &&
         ( VIDC_COLOR_FORMAT_UNUSED == VidcNodeBase::GetVidcFormat( m_pConfig->inFormat ) ) )
    {
        QC_ERROR( "input format: %d not supported!", m_pConfig->inFormat );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( VIDEO_ENCODER_RCM_UNUSED == m_pConfig->rateControlMode ) )
    {
        QC_ERROR( "rate control mode: 0x%x not supported!", m_pConfig->rateControlMode );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( QC_IMAGE_FORMAT_COMPRESSED_H265 != m_pConfig->outFormat ) &&
         ( QC_IMAGE_FORMAT_COMPRESSED_H264 != m_pConfig->outFormat ) )
    {
        QC_ERROR( "output format: %d not supported!", m_pConfig->outFormat );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( m_pConfig->numInputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ) ||
                                      ( m_pConfig->numInputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numInputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  m_pConfig->numInputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                  VIDEO_ENCODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( m_pConfig->numOutputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ) ||
                                      ( m_pConfig->numOutputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numOutputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  m_pConfig->numOutputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                  VIDEO_ENCODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e VideoEncoder::CheckBuffer( const VideoFrameDescriptor_t &vidFrmDesc,
                                      VideoCodec_BufType_e bufferType )
{
    QCStatus_e ret = ValidateBuffer( vidFrmDesc, bufferType );

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "validate buffer failed, type:%d", bufferType );
    }
    else
    {
        if ( bufferType == VIDEO_CODEC_BUF_INPUT )
        {
            if ( ( vidFrmDesc.stride[0] != m_vidcEncoderData.planeDefY.actual_stride ) ||
                 ( vidFrmDesc.stride[1] != m_vidcEncoderData.planeDefUV.actual_stride ) )
            {
                QC_ERROR( "pBuffer stride [%" PRIu32 "][%" PRIu32
                          "] is not same with actual stride [%" PRIu32 "][%" PRIu32 "] ",
                          vidFrmDesc.stride[0], vidFrmDesc.stride[1],
                          m_vidcEncoderData.planeDefY.actual_stride,
                          m_vidcEncoderData.planeDefUV.actual_stride );
                ret = QC_STATUS_INVALID_BUF;
            }
            if ( (size_t) m_bufSize[VIDEO_CODEC_BUF_INPUT] > vidFrmDesc.size )
            {
                QC_ERROR( "pBuffer size %zu is smaller than Input BufSize %" PRIu32, vidFrmDesc.size,
                          m_bufSize[VIDEO_CODEC_BUF_INPUT] );
                ret = QC_STATUS_INVALID_BUF;
            }
        }
    }

    return ret;
}

QCStatus_e VideoEncoder::GetInputInformation()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
    ret = m_drvClient.GetDrvProperty( VIDC_I_FRAME_RATE, sizeof( vidc_frame_rate_type ),
                                      (uint8_t &) ( m_vidcEncoderData.frameRate ) );
    if ( m_vidcEncoderData.frameRate.fps_numerator != m_pConfig->frameRate )
    {
        QC_ERROR( " frameRate: set:%u, get:%u is different! ", m_pConfig->frameRate,
                  m_vidcEncoderData.frameRate.fps_numerator );
        ret = QC_STATUS_FAIL;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.planeDefY.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefY.plane_index = 1;
        ret = m_drvClient.GetDrvProperty( VIDC_I_PLANE_DEF, sizeof( vidc_plane_def_type ),
                                          (uint8_t &) ( m_vidcEncoderData.planeDefY ) );
    }
    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.planeDefUV.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefUV.plane_index = 2;
        ret = m_drvClient.GetDrvProperty( VIDC_I_PLANE_DEF, sizeof( vidc_plane_def_type ),
                                          (uint8_t &) ( m_vidcEncoderData.planeDefUV ) );
    }

    return ret;
}

void VideoEncoder::InFrameCallback( VideoFrameDescriptor_t &inFrameDesc )
{
    if ( m_callback ) {
        NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID + 1 );
        frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID, inFrameDesc );

        QCNodeEventInfo_t evtInfo( frameDesc, m_configIfs.Get().nodeId, QC_STATUS_OK, GetState() );
        m_callback( evtInfo );
    }
}

void VideoEncoder::OutFrameCallback( VideoFrameDescriptor_t &outFrameDesc )
{
    if ( m_callback ) {
        NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID + 1 );
        frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, outFrameDesc );

        QCNodeEventInfo_t evtInfo( frameDesc, m_configIfs.Get().nodeId, QC_STATUS_OK, GetState() );
        m_callback( evtInfo );
    }
}

void VideoEncoder::EventCallback( VideoEncoder_EventType_e eventId, const void *pEvent )
{
    VidcNodeBase::EventCallback( eventId, pEvent );

    if ( m_callback ) {
        NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID + 1 );
        QCNodeEventInfo_t evtInfo( frameDesc, m_nodeId, QC_STATUS_OK, GetState() );

        QCBufferDescriptorBase_t errDesc;
        errDesc.name = std::to_string( eventId );
        errDesc.pBuf = const_cast<void *>( pEvent );
        errDesc.size = sizeof( vidc_drv_msg_info_type );
        frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, errDesc );

        m_callback( evtInfo );
    }
}

QCStatus_e VideoEncoderConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    m_config.bitRate = dt.Get<uint32_t>( "bitrate", VIDEO_ENCODER_DEFAULT_BIT_RATE );
    m_config.gop = dt.Get<uint32_t>( "gop", VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I );

    std::string rateCtlMode = dt.Get<std::string>( "rateControlMode", "CBR_CFR" );
    if ("CBR_CFR" == rateCtlMode)
    {
        m_config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    }
    else if ("CBR_VFR" == rateCtlMode)
    {
        m_config.rateControlMode = VIDEO_ENCODER_RCM_CBR_VFR;
    }
    else if ("VBR_CFR" == rateCtlMode)
    {
        m_config.rateControlMode = VIDEO_ENCODER_RCM_VBR_CFR;
    }
    else if ("VBR_VFR" == rateCtlMode)
    {
        m_config.rateControlMode = VIDEO_ENCODER_RCM_VBR_VFR;
    }
    else if ("UNUSED" == rateCtlMode)
    {
        m_config.rateControlMode = VIDEO_ENCODER_RCM_UNUSED;
    }
    else
    {
        errors += "the video profile is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    std::string profile = dt.Get<std::string>( "profile", "HEVC_MAIN" );
    if ("H264_BASELINE" == profile) {
        m_config.profile = VIDEO_ENCODER_PROFILE_H264_BASELINE;
    }
    else if ("H264_HIGH" == profile)
    {
        m_config.profile = VIDEO_ENCODER_PROFILE_H264_HIGH;
    }
    else if ("H264_MAIN" == profile)
    {
        m_config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    }
    else if ("HEVC_MAIN" == profile)
    {
        m_config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    }
    else if ("HEVC_MAIN10" == profile)
    {
        m_config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN10;
    } else {
        errors += "the video profile is invalid, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if (QC_STATUS_OK != status)
        QC_ERROR("errors: %s", errors.c_str());

    QC_INFO( "enc-init: w:%u, h:%u, fps:%u, bitRate:%u, gop:%u, inbuf: dynamicMode:%d, num:%u; outbuf: "
             "dynamicMode:%d, num:%u",
             m_config.width, m_config.height, m_config.frameRate, m_config.bitRate, m_config.gop,
             m_config.bInputDynamicMode, m_config.numInputBufferReq,
             m_config.bOutputDynamicMode, m_config.numOutputBufferReq);

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

void VideoEncoder::InFrameCallback( VideoFrameDescriptor_t &inFrameDesc,
                                    void *pPrivData )
{
    VideoEncoder *pNve = reinterpret_cast<VideoEncoder *>( pPrivData );
    if ( pNve != nullptr )
    {
        pNve->InFrameCallback( inFrameDesc );
    }
    else
    {
        QC_LOG_ERROR( "VideoEncoder_InFrameCallback: pPrivData param is invalid" );
    }
}

void VideoEncoder::OutFrameCallback( VideoFrameDescriptor_t &outFrameDesc,
                                     void *pPrivData )
{
    VideoEncoder *pNve = reinterpret_cast<VideoEncoder *>( pPrivData );
    if ( pNve != nullptr )
    {
        pNve->OutFrameCallback( outFrameDesc );
    }
    else
    {
        QC_LOG_ERROR( "VideoEncoder_OutFrameCallback: pPrivData param is invalid" );
    }
}

void VideoEncoder::EventCallback( VideoCodec_EventType_e eventId,
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
