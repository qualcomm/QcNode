// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "VidcDemuxer.hpp"
#include <filesource.h>
#include <malloc.h>
#include <stdlib.h>
#include <thread>

namespace QC
{
namespace sample
{

VidcDemuxer::VidcDemuxer() : m_fileSource( FileSourceCallback, this, false ) {}

VidcDemuxer::~VidcDemuxer() {}

QCStatus_e VidcDemuxer::Init( VidcDemuxer_Config_t *pConfig )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_bFileOpen = false;
    m_bInitialized = false;
    m_bSendCodecConfig = true;
    m_bKeyFrameFound = false;
    m_frameIdx = 0;
    m_startFrameIdx = pConfig->startFrameIdx;

    const char *pVideoFile = pConfig->pVideoFileName;
    m_videoFile = (wchar_t *) malloc( sizeof( wchar_t ) * ( strlen( pVideoFile ) + 1 ) );
    memset( m_videoFile, 0, sizeof( m_videoFile ) );
    mbstowcs( m_videoFile, pVideoFile, strlen( pVideoFile ) );
    if ( m_videoFile == nullptr )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "VidcDemuxer::Init Error: could not get video file: %s", pVideoFile );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = OpenFile( m_videoFile );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = GetVideoTrackInfo( m_videoInfo );
    }
    if ( QC_STATUS_OK == ret )
    {
        ret = SelectVideoTrackId( m_videoInfo.trackId );
    }
    if ( QC_STATUS_OK == ret )
    {
        ret = GetMaxFrameBufferSize( m_videoInfo.trackId, m_videoInfo.maxFrameSize );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bInitialized = true;
    }

    return ret;
}

QCStatus_e VidcDemuxer::DeInit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bInitialized )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_LOG_ERROR( "VidcDemuxer::DeInit Error: demuxer not initialzed" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( m_bFileOpen )
        {
            m_fileSource.CleanupResource();
            m_fileSource.CloseFile();
            m_bFileOpen = false;
        }

        if ( m_videoFile != nullptr )
        {
            free( m_videoFile );
            m_videoFile = nullptr;
        }
        m_bInitialized = false;
    }

    return ret;
}

QCStatus_e VidcDemuxer::GetFrame( QCBufferDescriptorBase_t &bufDesc,
                                  VidcDemuxer_FrameInfo_t &frameInfo )
{
    QCStatus_e ret = QC_STATUS_OK;
    vidc_frame_data_type frame;
    ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );

    if ( false == m_bInitialized )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_LOG_ERROR( "VidcDemuxer::Process Error: demuxer not initialzed" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( nullptr == pImage ) || ( nullptr == pImage->pBuf ) )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_LOG_ERROR( "VidcDemuxer::Process Error: buffer is invalid" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        (void) memset( &frame, 0, sizeof( vidc_frame_data_type ) );
        frame.frame_addr = (uint8_t *) pImage->GetDataPtr();
        frame.data_len = pImage->GetDataSize();
        frame.alloc_len = pImage->size;

        if ( m_bSendCodecConfig == true )
        {
            ret = ProcessFormatBlock( m_videoInfo.trackId, &frame );
            if ( QC_STATUS_OK == ret )
            {
                m_bSendCodecConfig = false;
            }
            else
            {
                QC_LOG_ERROR( "VidcDemuxer::Process Error: failed to process format " );
            }
        }
        else
        {
            ret = GetNextMediaSample( m_videoInfo.trackId, &frame, frameInfo );
            if ( ( QC_STATUS_OK == ret ) && ( frame.data_len > 0 ) )
            {
                if ( m_bKeyFrameFound == false )
                {
                    m_bKeyFrameFound = frameInfo.sync == 1;
                }
                if ( m_bKeyFrameFound == true )
                {
                    frame.timestamp = frameInfo.startTime;
                    frame.flags = VIDC_FRAME_FLAG_ENDOFFRAME;
                }
                else
                {
                    frame.data_len = 0;
                    QC_LOG_DEBUG(
                            "VidcDemuxer::Process Debug: drop frame data until key frame found" );
                }
            }
            else if ( QC_STATUS_OUT_OF_BOUND == ret )
            {
                QC_LOG_DEBUG( "VidcDemuxer::Process Debug: end of video stream" );
            }
            else
            {
                frame.data_len = 0;
                ret = QC_STATUS_FAIL;
                QC_LOG_ERROR( "VidcDemuxer::Process Error: failed to get next media sample" );
            }
        }
    }

    return ret;
}


QCStatus_e VidcDemuxer::GetVideoInfo( VidcDemuxer_VideoInfo_t &videoInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bInitialized )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_LOG_ERROR( "VidcDemuxer::GetVideoInfo Error: demuxer not initialzed" );
    }

    if ( QC_STATUS_OK == ret )
    {
        videoInfo.trackId = m_videoInfo.trackId;
        videoInfo.frameWidth = m_videoInfo.frameWidth;
        videoInfo.frameHeight = m_videoInfo.frameHeight;
        videoInfo.maxFrameSize = m_videoInfo.maxFrameSize;
        videoInfo.frameRate = m_videoInfo.frameRate;
        videoInfo.codecType = m_videoInfo.codecType;
        videoInfo.format = GetQCImageFormat( m_videoInfo.codecType );
    }

    return ret;
}

QCStatus_e VidcDemuxer::SetPlayTime( int64_t time )
{
    QCStatus_e ret = QC_STATUS_OK;
    FileSourceStatus status = FILE_SOURCE_SUCCESS;

    if ( !m_bFileOpen )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "VidcDemuxer::SetPlayTime Error: file source not open" );
    }

    if ( QC_STATUS_OK == ret )
    {
        status = m_fileSource.SeekAbsolutePosition( m_videoInfo.trackId, time );
        if ( FILE_SOURCE_SUCCESS != status )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::SetPlayTime Error: could not seek time position" );
        }
    }

    return ret;
}

QCStatus_e VidcDemuxer::OpenFile( wchar_t *videoFile )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( m_bFileOpen )
    {
        ret = QC_STATUS_ALREADY;
        QC_LOG_INFO( "VidcDemuxer::OpenFile Info: file source already open" );
    }

    if ( QC_STATUS_OK == ret )
    {
        FileSourceStatus status = m_fileSource.OpenFile( videoFile, videoFile, videoFile );
        if ( FILE_SOURCE_SUCCESS != status )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::OpenFile Error: could not open video file: %s",
                          &videoFile );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bFileOpen = true;
    }

    return ret;
}

QCStatus_e VidcDemuxer::GetVideoTrackInfo( VidcDemuxer_VideoInfo_t &videoInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    int trackCnt = 0;
    int maxFrameBufferSize = 0;
    MediaTrackInfo trackInfo = {};
    FileSourceTrackIdInfoType *trackInfoPtr = nullptr;
    FileSourceStatus status = FILE_SOURCE_SUCCESS;

    if ( !m_bFileOpen )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "VidcDemuxer::GetVideoTrackInfo Error: file source not open" );
    }

    if ( QC_STATUS_OK == ret )
    {
        trackCnt = m_fileSource.GetWholeTracksIDList( nullptr );
        if ( trackCnt <= 0 )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::GetVideoTrackInfo Error: could not get track id list" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        trackInfoPtr = (FileSourceTrackIdInfoType *) malloc( trackCnt *
                                                             sizeof( FileSourceTrackIdInfoType ) );
        if ( trackInfoPtr == nullptr )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::GetVideoTrackInfo Error: could not allocate memory" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_fileSource.GetWholeTracksIDList( trackInfoPtr );
        for ( int i = 0; i < trackCnt; i++ )
        {
            if ( trackInfoPtr[i].majorType == FILE_SOURCE_MJ_TYPE_VIDEO )
            {
                memset( &trackInfo, 0, sizeof( trackInfo ) );
                status = m_fileSource.GetMediaTrackInfo( trackInfoPtr[i].id, &trackInfo );
                if ( FILE_SOURCE_SUCCESS == status )
                {
                    videoInfo.trackId = trackInfo.videoTrackInfo.id;
                    videoInfo.frameWidth = trackInfo.videoTrackInfo.frameWidth;
                    videoInfo.frameHeight = trackInfo.videoTrackInfo.frameHeight;
                    videoInfo.frameRate = trackInfo.videoTrackInfo.frameRate;
                    if ( FILE_SOURCE_MN_TYPE_H264 == trackInfo.videoTrackInfo.videoCodec )
                    {
                        videoInfo.codecType = VIDC_CODEC_H264;
                    }
                    else if ( FILE_SOURCE_MN_TYPE_HEVC == trackInfo.videoTrackInfo.videoCodec )
                    {
                        videoInfo.codecType = VIDC_CODEC_HEVC;
                    }
                    else
                    {
                        ret = QC_STATUS_UNSUPPORTED;
                        QC_LOG_ERROR( "VidcDemuxer::GetVideoTrackInfo Error: unsupported "
                                      "codec type" );
                    }
                }
            }
        }
    }

    if ( trackInfoPtr != nullptr )
    {
        free( trackInfoPtr );
        trackInfoPtr = nullptr;
    }

    return ret;
}

QCStatus_e VidcDemuxer::SelectVideoTrackId( uint32 trackId )
{
    QCStatus_e ret = QC_STATUS_OK;
    FileSourceStatus status = FILE_SOURCE_SUCCESS;

    if ( !m_bFileOpen )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "VidcDemuxer::SelectVideoTrackId Error: file source not open" );
    }

    if ( QC_STATUS_OK == ret )
    {
        status = m_fileSource.SetSelectedTrackID( trackId );
        if ( FILE_SOURCE_SUCCESS != status )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::SelectVideoTrackId Error: failed to select track id %u",
                          trackId );
        }
    }

    return ret;
}

QCStatus_e VidcDemuxer::GetMaxFrameBufferSize( uint32 trackId, uint32_t &maxFrameSize )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( !m_bFileOpen )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "VidcDemuxer::GetMaxFrameBufferSize Error: file source not open" );
    }

    if ( QC_STATUS_OK == ret )
    {
        maxFrameSize = m_fileSource.GetTrackMaxFrameBufferSize( trackId );
    }

    return ret;
}

QCStatus_e VidcDemuxer::ProcessFormatBlock( uint32 trackId, vidc_frame_data_type *pFrame )
{
    QCStatus_e ret = QC_STATUS_OK;

    uint32_t bufferSize = 0;
    FileSourceStatus status = FILE_SOURCE_SUCCESS;

    if ( !m_bFileOpen )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "VidcDemuxer::ProcessFormatBlock Error: file source not open" );
    }

    if ( QC_STATUS_OK == ret )
    {
        status = m_fileSource.GetFormatBlock( trackId, nullptr, &bufferSize );
        if ( status != FILE_SOURCE_SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::ProcessFormatBlock Error: falied to get format block "
                          "buffer size" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( bufferSize == 0 )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::ProcessFormatBlock Error: buffer size is 0" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        pFrame->data_len = bufferSize;
        status = m_fileSource.GetFormatBlock( trackId, pFrame->frame_addr, &bufferSize );
        if ( FILE_SOURCE_SUCCESS == status )
        {
            pFrame->flags = VIDC_FRAME_FLAG_CODECCONFIG;
        }
        else
        {
            pFrame->data_len = 0;
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::ProcessFormatBlock Error: failed to get format block" );
        }
    }

    return ret;
}

QCStatus_e VidcDemuxer::GetNextMediaSample( uint32 trackId, vidc_frame_data_type *pFrame,
                                            VidcDemuxer_FrameInfo_t &frameInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    FileSourceMediaStatus status = FILE_SOURCE_DATA_OK;
    FileSourceSampleInfo fileSourceSampleInfo;

    if ( !m_bFileOpen )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "VidcDemuxer::GetNextMediaSample Error: file source not open" );
    }

    if ( QC_STATUS_OK == ret )
    {
        status = m_fileSource.GetNextMediaSample( trackId, pFrame->frame_addr, &pFrame->data_len,
                                                  fileSourceSampleInfo );

        if ( status == FILE_SOURCE_DATA_OK )
        {
            frameInfo.startTime = fileSourceSampleInfo.startTime;
            frameInfo.endTime = fileSourceSampleInfo.endTime;
            frameInfo.delta = fileSourceSampleInfo.delta;
            frameInfo.sync = fileSourceSampleInfo.sync;
        }
        else if ( status == FILE_SOURCE_DATA_END )
        {
            ret = QC_STATUS_OUT_OF_BOUND;
            QC_LOG_DEBUG( "VidcDemuxer::GetNextMediaSample Debug: end of video stream" );
        }
        else
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "VidcDemuxer::GetNextMediaSample Error: failed to get media sample" );
        }
    }

    return ret;
}

QCImageFormat_e VidcDemuxer::GetQCImageFormat( vidc_codec_type codecType )
{
    QCImageFormat_e format = QC_IMAGE_FORMAT_COMPRESSED_MAX;
    switch ( codecType )
    {
        case VIDC_CODEC_H264:
            format = QC_IMAGE_FORMAT_COMPRESSED_H264;
            break;
        case VIDC_CODEC_HEVC:
            format = QC_IMAGE_FORMAT_COMPRESSED_H265;
            break;
        default:
            QC_LOG_ERROR( "codecType %d not supported", codecType );
            break;
    }

    return format;
}

void VidcDemuxer::FileSourceCallback( FileSourceCallBackStatus eStatus, void *pClientData )
{
    VidcDemuxer *thisPtr = (VidcDemuxer *) pClientData;
    switch ( eStatus )
    {
        case FILE_SOURCE_OPEN_COMPLETE:
            QC_LOG_INFO( "VidcDemuxer::fileSourceCallback Info: FILE_SOURCE_OPEN_COMPLETE" );
            break;
        case FILE_SOURCE_OPEN_FAIL:
            QC_LOG_ERROR( "VidcDemuxer::fileSourceCallback Error: FILE_SOURCE_OPEN_FAIL" );
            break;
        case FILE_SOURCE_SEEK_COMPLETE:
            QC_LOG_INFO( "VidcDemuxer::fileSourceCallback Info: FILE_SOURCE_SEEK_COMPLETE" );
            break;
        case FILE_SOURCE_SEEK_FAIL:
            QC_LOG_ERROR( "VidcDemuxer::fileSourceCallback Error: FILE_SOURCE_SEEK_FAIL" );
            break;
        case FILE_SOURCE_ERROR_ABORT:
            QC_LOG_ERROR( "VidcDemuxer::fileSourceCallback Error: FILE_SOURCE_ERROR_ABORT" );
            break;
        default:
            QC_LOG_ERROR( "VidcDemuxer::fileSourceCallback Error: unknown status %d", eStatus );
            break;
    }
}

}   // namespace sample
}   // namespace QC
