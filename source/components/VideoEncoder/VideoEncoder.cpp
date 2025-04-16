// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include <MMTimer.h>
#include <cmath>
#include <malloc.h>

#include "QC/common/Types.hpp"
#include "QC/component/VideoEncoder.hpp"

namespace QC
{
namespace component
{

#define VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I 30
#define VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I 0
#define VIDEO_ENCODER_DEFAULT_IDR_PERIOD 1
#define VIDEO_ENCODER_DEFAULT_BIT_RATE 64000
#define VIDEO_ENCODER_DEFAULT_FRAME_RATE 30

#define VIDEO_ENCODER_MAX_BUFFER_REQ 64
#define VIDEO_ENCODER_MIN_BUFFER_REQ 4

#define ARRAY_SIZE( a ) ( sizeof( ( a ) ) / sizeof( ( a )[0] ) )

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

static const ProfileLevelTableRef_t s_profileLevelTables[] = {
        { s_profileLevelH264BaseLineTable, ARRAY_SIZE( s_profileLevelH264BaseLineTable ) },
        { s_profileLevelH264HighTable, ARRAY_SIZE( s_profileLevelH264HighTable ) },
        { s_profileLevelH264MainTable, ARRAY_SIZE( s_profileLevelH264MainTable ) },
        { s_profileLevelHevcMainTable, ARRAY_SIZE( s_profileLevelHevcMainTable ) },
        { s_profileLevelHevcMain10Table, ARRAY_SIZE( s_profileLevelHevcMain10Table ) } };


QCStatus_e VideoEncoder::InitFromConfig( const VideoEncoder_Config_t *cfg )
{
    m_width = cfg->width;
    m_height = cfg->height;

    m_bitRate = ( cfg->bitRate > 0 ) ? cfg->bitRate : VIDEO_ENCODER_DEFAULT_BIT_RATE;
    m_frameRate = ( cfg->frameRate > 0 ) ? cfg->frameRate : VIDEO_ENCODER_DEFAULT_FRAME_RATE;
    m_gop = ( cfg->gop > 0 ) ? cfg->gop : VIDEO_ENCODER_DEFAULT_NUM_P_BET_2I;

    m_rateControl = cfg->rateControlMode;

    m_inFormat = cfg->inFormat;
    m_outFormat = cfg->outFormat;

    m_bEnableSyncFrameSeq = cfg->bSyncFrameSeqHdr;
    m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] = cfg->bInputDynamicMode;
    m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] = cfg->bOutputDynamicMode;
    m_bufNum[VIDEO_CODEC_BUF_INPUT] = cfg->numInputBufferReq;
    m_bufNum[VIDEO_CODEC_BUF_OUTPUT] = cfg->numOutputBufferReq;

    if ( ( false == m_bDynamicMode[VIDEO_CODEC_BUF_INPUT] ) &&
         ( nullptr != cfg->pInputBufferList ) )
    {
        m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_INPUT] = true;
    }
    else
    {
        /* For init() may be called multiple times, so should assign default value everytime
         * when this function called.
         */
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

    QC_INFO( "enc-init: w:%u, h:%u, fps:%u, inbuf: dynamicMode:%d, num:%u, appAloc:%d; outbuf: "
             "dynamicMode:%d, num:%u, appAloc:%d",
             m_width, m_height, m_frameRate, m_bDynamicMode[VIDEO_CODEC_BUF_INPUT],
             m_bufNum[VIDEO_CODEC_BUF_INPUT], m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_INPUT],
             m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT], m_bufNum[VIDEO_CODEC_BUF_OUTPUT],
             m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_OUTPUT] );

    return QC_STATUS_OK;
}

QCStatus_e VideoEncoder::Init( const char *pName, const VideoEncoder_Config_t *pConfig,
                               Logger_Level_e level )
{
    int i;
    QCStatus_e ret = QC_STATUS_OK;

    ret = VidcCompBase::Init( pName, VIDEO_ENC, level );

    if ( QC_STATUS_OK == ret )
    {
        ret = ValidateConfig( pName, pConfig );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        QC_INFO( "video-encoder %s init begin", pName );
        ret = InitFromConfig( pConfig );
    }

    if ( QC_STATUS_OK == ret )
    {
        /* init profile */
        ret = SetVidcProfileLevel( pConfig->profile );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_drvClient.Init( pName, level, VIDEO_ENC );
        ret = m_drvClient.OpenDriver( VideoEncoder::InFrameCallback, VideoEncoder::OutFrameCallback,
                                      VideoEncoder::EventCallback, (void *) this );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = InitDrvProperty();
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = GetInputInformation();
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = VidcCompBase::NegotiateBufferReq( VIDEO_CODEC_BUF_INPUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = VidcCompBase::NegotiateBufferReq( VIDEO_CODEC_BUF_OUTPUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_INPUT] )
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
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( m_bNonDynamicAppAllocBuffer[VIDEO_CODEC_BUF_OUTPUT] )
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
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = VidcCompBase::PostInit( pConfig->pInputBufferList, pConfig->pOutputBufferList );
    }

    if ( QC_STATUS_OK == ret )
    {
        PrintEncoderConfig();
        QC_INFO( "video-encoder init done" );
    }
    else
    {
        QC_ERROR( "Something wrong happened in Init, Deiniting vidc" );
        m_state = QC_OBJECT_STATE_ERROR;
        (void) Deinit();
    }

    return ret;
}

QCStatus_e VideoEncoder::InitDrvProperty()
{
    QCStatus_e ret = QC_STATUS_OK;

    VidcCodecMeta_t meta;
    meta.width = m_width;
    meta.height = m_height;
    meta.frameRate = m_frameRate;
    if ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_outFormat )
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
        vidc_rate_control_mode_type rateControl = (vidc_rate_control_mode_type) m_rateControl;

        QC_DEBUG( "Setting VIDC_I_ENC_RATE_CONTROL" );
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_RATE_CONTROL,
                                          sizeof( vidc_rate_control_mode_type ),
                                          (uint8_t *) ( &rateControl ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        /**< Configures the uncompressed Buffer format */
        vidc_color_format_config_type colorFormatConfig;
        colorFormatConfig.buf_type = VIDC_BUFFER_INPUT;
        colorFormatConfig.color_format = VidcCompBase::GetVidcFormat( m_inFormat );

        QC_DEBUG( "Setting VIDC_I_COLOR_FORMAT" );
        ret = m_drvClient.SetDrvProperty( VIDC_I_COLOR_FORMAT,
                                          sizeof( vidc_color_format_config_type ),
                                          (uint8_t *) ( &colorFormatConfig ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting GOP" );
        m_vidcEncoderData.iPeriod.p_frames = m_gop;
        m_vidcEncoderData.iPeriod.b_frames = VIDEO_ENCODER_DEFAULT_NUM_B_BET_2I;
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_INTRA_PERIOD, sizeof( vidc_iperiod_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.iPeriod ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting ENC_IDR_PERIOD" );
        m_vidcEncoderData.idrPeriod.idr_period = VIDEO_ENCODER_DEFAULT_IDR_PERIOD;
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_IDR_PERIOD, sizeof( vidc_idr_period_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.idrPeriod ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_TARGET_BITRATE" );
        m_vidcEncoderData.bitrate.target_bitrate = m_bitRate;
        ret = m_drvClient.SetDrvProperty( VIDC_I_TARGET_BITRATE, sizeof( vidc_target_bitrate_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.bitrate ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_ENC_SYNC_FRAME_SEQ_HDR" );
        m_vidcEncoderData.enableSyncFrameSeq.enable = m_bEnableSyncFrameSeq;
        ret = m_drvClient.SetDrvProperty( VIDC_I_ENC_SYNC_FRAME_SEQ_HDR, sizeof( vidc_enable_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.enableSyncFrameSeq ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_PROFILE" );
        ret = m_drvClient.SetDrvProperty( VIDC_I_PROFILE, sizeof( vidc_profile_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.profile ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_LEVEL" );
        ret = m_drvClient.SetDrvProperty( VIDC_I_LEVEL, sizeof( vidc_level_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.level ) );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_DEBUG( "Setting VIDC_I_VPE_SPATIAL_TRANSFORM" );
        vidc_spatial_transform_type vidcSpatialTransform = { VIDC_ROTATE_NONE, VIDC_FLIP_NONE };
        ret = m_drvClient.SetDrvProperty( VIDC_I_VPE_SPATIAL_TRANSFORM,
                                          sizeof( vidc_spatial_transform_type ),
                                          (uint8_t *) ( &vidcSpatialTransform ) );
    }

    return ret;
}

QCStatus_e VideoEncoder::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t i;

    if ( ( nullptr == m_inputDoneCb ) || ( nullptr == m_outputDoneCb ) )
    {
        QC_ERROR( "Not start since callback is not registered!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = VidcCompBase::Start();
    }

    if ( ( QC_STATUS_OK == ret ) && ( false == m_bDynamicMode[VIDEO_CODEC_BUF_OUTPUT] ) )
    {
        QC_INFO( "submit output frame begin, cnt:%u", m_bufNum[VIDEO_CODEC_BUF_OUTPUT] );
        for ( i = 0; i < m_bufNum[VIDEO_CODEC_BUF_OUTPUT]; i++ )
        {
            VideoEncoder_OutputFrame_t outputFrame;
            outputFrame.sharedBuffer = m_pOutputList[i];
            ret = SubmitOutputFrame( &outputFrame );
        }
        QC_INFO( "submit output frame done" );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "enc start succeed" );
    }

    return ret;
}

QCStatus_e VideoEncoder::SubmitInputFrame( const VideoEncoder_InputFrame_t *pInput )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_WARN( "Not submitting inputBuffer since encoder is shutting down!" );
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
        ret = CheckBuffer( &pInput->sharedBuffer, VIDEO_CODEC_BUF_INPUT );
    }

    if ( ( QC_STATUS_OK == ret ) && ( nullptr != pInput->pOnTheFlyCmd ) )
    {
        int32_t i;
        for ( i = 0; i < pInput->numCmd; i++ )
        {
            ret = Configure( &pInput->pOnTheFlyCmd[i] );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "config propID %d failed", pInput->pOnTheFlyCmd[i].propID );
                break;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_drvClient.EmptyBuffer( &pInput->sharedBuffer, pInput->timestampNs / 1000,
                                       pInput->appMarkData );
    }

    QC_DEBUG( "SubmitInputFrame done" );

    return ret;
}

QCStatus_e VideoEncoder::SubmitOutputFrame( const VideoEncoder_OutputFrame_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;
    const QCSharedBuffer_t *outputBuffer = nullptr;

    QC_DEBUG( "SubmitOutputFrame in" );

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_WARN( "Not submitting outputBuffer since encoder is shutting down!" );
        ret = QC_STATUS_BAD_STATE;
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
        QC_DEBUG( "fill buffer begin" );
        ret = m_drvClient.FillBuffer( outputBuffer );
        QC_DEBUG( "fill buffer done" );
    }

    return ret;
}

QCStatus_e VideoEncoder::Stop()
{
    QCStatus_e ret = VidcCompBase::Stop();

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

QCStatus_e VideoEncoder::Deinit()
{
    QCStatus_e ret = VidcCompBase::Deinit();

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "enc Deinit succeed" );
    }
    else
    {
        QC_ERROR( "enc Deinit failed" );
    }

    return ret;
}

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

void VideoEncoder::PrintEncoderConfig()
{
    if ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_outFormat )
    {
        QC_DEBUG( "EncoderConfig: Codec = H265" );
        QC_DEBUG( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        QC_DEBUG( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }
    else
    {
        QC_DEBUG( "EncoderConfig: Codec = H264" );
        QC_DEBUG( "EncoderConfig: Profile = 0x%x", m_vidcEncoderData.profile.profile );
        QC_DEBUG( "EncoderConfig: Level = 0x%x", m_vidcEncoderData.level.level );
    }

    QC_DEBUG( "EncoderConfig: RateControl = 0x%x ", m_rateControl );
    QC_DEBUG( "EncoderConfig: BitRate = %" PRIu32, m_vidcEncoderData.bitrate.target_bitrate );
    QC_DEBUG( "EncoderConfig: NumPframes = %" PRIu32, m_vidcEncoderData.iPeriod.p_frames );
    QC_DEBUG( "EncoderConfig: NumBframes = %" PRIu32, m_vidcEncoderData.iPeriod.b_frames );
    QC_DEBUG( "EncoderConfig: IdrPeriod = %" PRIu32, m_vidcEncoderData.idrPeriod.idr_period );
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

        if ( ( QC_IMAGE_FORMAT_COMPRESSED_H264 == m_outFormat ) &&
             ( profile <= VIDEO_ENCODER_PROFILE_H264_MAIN ) )
        {
            mbPerFrame = ( ( m_height + 15 ) >> 4 ) * ( ( m_width + 15 ) >> 4 );
            mbPerSec = mbPerFrame * m_frameRate;
            QC_DEBUG( "mbPerFrame %" PRIu32 " mbPerSec %" PRIu32, mbPerFrame, mbPerSec );
            for ( i = 0; ( i < num ) && ( false == bFindFlag ); i++ )
            {
                if ( mbPerFrame <= pTable[i].maxFrameSize )
                {
                    if ( mbPerSec <= pTable[i].maxSizePerSec )
                    {
                        if ( m_bitRate <= pTable[i].maxBitRate )
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
        else if ( ( QC_IMAGE_FORMAT_COMPRESSED_H265 == m_outFormat ) &&
                  ( profile >= VIDEO_ENCODER_PROFILE_HEVC_MAIN ) )
        {
            samplePerFrame = (uint64_t) m_height * m_width;
            samplePerSec = samplePerFrame * m_frameRate;
            QC_DEBUG( "samplePerFrame %" PRIu64 " samplePerSec %" PRIu64, samplePerFrame,
                      samplePerSec );
            for ( i = 0; ( i < num ) && ( false == bFindFlag ); i++ )
            {
                if ( samplePerFrame <= pTable[i].maxFrameSize )
                {
                    if ( samplePerSec <= pTable[i].maxSizePerSec )
                    {
                        if ( m_bitRate <= pTable[i].maxBitRate )
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
            QC_ERROR( "m_outFormat %d not match with profile %d", m_outFormat, profile );
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

QCStatus_e VideoEncoder::ValidateConfig( const char *name, const VideoEncoder_Config_t *cfg )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == cfg )
    {
        QC_ERROR( "cfg is null pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
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

    if ( ( QC_STATUS_OK == ret ) &&
         ( VIDC_COLOR_FORMAT_UNUSED == VidcCompBase::GetVidcFormat( cfg->inFormat ) ) )
    {
        QC_ERROR( "input format: %d not supported!", cfg->inFormat );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( VIDEO_ENCODER_RCM_UNUSED == cfg->rateControlMode ) )
    {
        QC_ERROR( "rate control mode: 0x%x not supported!", cfg->rateControlMode );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( QC_IMAGE_FORMAT_COMPRESSED_H265 != cfg->outFormat ) &&
         ( QC_IMAGE_FORMAT_COMPRESSED_H264 != cfg->outFormat ) )
    {
        QC_ERROR( "output format: %d not supported!", cfg->outFormat );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( cfg->numInputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ) ||
                                      ( cfg->numInputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numInputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  cfg->numInputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                  VIDEO_ENCODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( QC_STATUS_OK == ret ) && ( ( cfg->numOutputBufferReq > VIDEO_ENCODER_MAX_BUFFER_REQ ) ||
                                      ( cfg->numOutputBufferReq < VIDEO_ENCODER_MIN_BUFFER_REQ ) ) )
    {
        QC_ERROR( "numOutputBufferReq: %" PRIu32 " too small or too large! (MIN_BUFFER_REQ %d, "
                  "MAX_BUFFER_REQ %d) ",
                  cfg->numOutputBufferReq, VIDEO_ENCODER_MIN_BUFFER_REQ,
                  VIDEO_ENCODER_MAX_BUFFER_REQ );
        ret = QC_STATUS_BAD_ARGUMENTS;
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

QCStatus_e VideoEncoder::CheckBuffer( const QCSharedBuffer_t *pBuffer,
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
            if ( ( pBuffer->imgProps.stride[0] != m_vidcEncoderData.planeDefY.actual_stride ) ||
                 ( pBuffer->imgProps.stride[1] != m_vidcEncoderData.planeDefUV.actual_stride ) )
            {
                QC_ERROR( "pBuffer stride [%" PRIu32 "][%" PRIu32
                          "] is not same with actual stride [%" PRIu32 "][%" PRIu32 "] ",
                          pBuffer->imgProps.stride[0], pBuffer->imgProps.stride[1],
                          m_vidcEncoderData.planeDefY.actual_stride,
                          m_vidcEncoderData.planeDefUV.actual_stride );
                ret = QC_STATUS_INVALID_BUF;
            }
            if ( (size_t) m_bufSize[VIDEO_CODEC_BUF_INPUT] > pBuffer->size )
            {
                QC_ERROR( "pBuffer size %zu is smaller than Input BufSize %" PRIu32, pBuffer->size,
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

    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.frameRate.buf_type = VIDC_BUFFER_INPUT;
        ret = m_drvClient.GetDrvProperty( VIDC_I_FRAME_RATE, sizeof( vidc_frame_rate_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.frameRate ) );
        if ( m_vidcEncoderData.frameRate.fps_numerator != m_frameRate )
        {
            QC_ERROR( " frameRate: set:%u, get:%u is different! ", m_frameRate,
                      m_vidcEncoderData.frameRate.fps_numerator );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.planeDefY.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefY.plane_index = 1;
        ret = m_drvClient.GetDrvProperty( VIDC_I_PLANE_DEF, sizeof( vidc_plane_def_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.planeDefY ) );
    }
    if ( QC_STATUS_OK == ret )
    {
        m_vidcEncoderData.planeDefUV.buf_type = VIDC_BUFFER_INPUT;
        m_vidcEncoderData.planeDefUV.plane_index = 2;
        ret = m_drvClient.GetDrvProperty( VIDC_I_PLANE_DEF, sizeof( vidc_plane_def_type ),
                                          (uint8_t *) ( &m_vidcEncoderData.planeDefUV ) );
    }

    return ret;
}

QCStatus_e VideoEncoder::RegisterCallback( VideoEncoder_InFrameCallback_t inputDoneCb,
                                           VideoEncoder_OutFrameCallback_t outputDoneCb,
                                           VideoEncoder_EventCallback_t eventCb, void *pAppPriv )
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

void VideoEncoder::InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame )
{
    VideoEncoder_InputFrame_t inputFrame;
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

void VideoEncoder::OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame )
{
    VideoEncoder_OutputFrame_t outputFrame;
    outputFrame.sharedBuffer = pOutputFrame->sharedBuffer;
    outputFrame.appMarkData = pOutputFrame->appMarkData;
    outputFrame.timestampNs = pOutputFrame->timestampUs * 1000;   // convert us to ns
    outputFrame.frameFlag = pOutputFrame->frameFlag;
    outputFrame.frameType = (VideoEncoder_FrameType_e) pOutputFrame->frameType;

    if ( nullptr != m_outputDoneCb )
    {
        m_outputDoneCb( &outputFrame, m_pAppPriv );
    }
    else
    {
        QC_ERROR( "m_outputDoneCb is nullptr" );
    }
}

void VideoEncoder::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload )
{
    QC_INFO( "Received event: %d, pPayload:%p\n", eventId, pPayload );

    VidcCompBase::EventCallback( eventId, pPayload );

    if ( m_eventCb )
    {
        m_eventCb( (VideoEncoder_EventType_e) eventId, pPayload, m_pAppPriv );
    }
}

void VideoEncoder::InFrameCallback( const VideoCodec_InputFrame_t *pInputFrame, void *pPrivData )
{
    VideoEncoder *self = (VideoEncoder *) pPrivData;
    self->InFrameCallback( pInputFrame );
}

void VideoEncoder::OutFrameCallback( const VideoCodec_OutputFrame_t *pOutputFrame, void *pPrivData )
{
    VideoEncoder *self = (VideoEncoder *) pPrivData;
    self->OutFrameCallback( pOutputFrame );
}

void VideoEncoder::EventCallback( const VideoCodec_EventType_e eventId, const void *pPayload,
                                  void *pPrivData )
{
    VideoEncoder *self = (VideoEncoder *) pPrivData;
    self->EventCallback( eventId, pPayload );
}

}   // namespace component
}   // namespace QC
