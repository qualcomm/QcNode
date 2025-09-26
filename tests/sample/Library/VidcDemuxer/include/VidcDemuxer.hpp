// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_SAMPLE_VIDC_DEMUXER_HPP
#define QC_SAMPLE_VIDC_DEMUXER_HPP

#include <condition_variable>
#include <filesource.h>
#include <mutex>
#include <queue>
#include <stdlib.h>
#include <unordered_map>
#include <vidc_types.h>

#ifndef _VIDC_LRH_LINUX_
#include <ioctlClient.h>
#else
#include <cstring>
#include <vidc_client.h>
#endif
#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"

namespace QC
{
namespace sample
{

using namespace QC::Memory;

typedef struct
{
    const char *pVideoFileName;
    uint64_t startFrameIdx;

} VidcDemuxer_Config_t;

typedef struct
{
    uint32_t trackId;
    uint32_t frameWidth;
    uint32_t frameHeight;
    uint32_t maxFrameSize;
    float frameRate;
    vidc_codec_type codecType;
    QCImageFormat_e format;
} VidcDemuxer_VideoInfo_t;

typedef struct
{
    uint64_t startTime;
    uint64_t endTime;
    uint64_t delta;
    uint32_t sync;
    uint32_t flag;
} VidcDemuxer_FrameInfo_t;

class VidcDemuxer
{
public:
    VidcDemuxer();
    ~VidcDemuxer();

    QCStatus_e Init( VidcDemuxer_Config_t *pConfig );
    QCStatus_e DeInit();
    QCStatus_e GetFrame( QCBufferDescriptorBase_t &bufDesc, VidcDemuxer_FrameInfo_t &frameInfo );
    QCStatus_e GetVideoInfo( VidcDemuxer_VideoInfo_t &videoInfo );
    QCStatus_e SetPlayTime( int64_t time );

private:
    QCStatus_e OpenFile( wchar_t *videoFile );
    QCStatus_e GetVideoTrackInfo( VidcDemuxer_VideoInfo_t &videoInfo );
    QCStatus_e SelectVideoTrackId( uint32 trackId );
    QCStatus_e GetMaxFrameBufferSize( uint32 trackId, uint32_t &maxFrameSize );

    QCStatus_e ProcessFormatBlock( uint32 trackId, vidc_frame_data_type *pFrame );
    QCStatus_e GetNextMediaSample( uint32 id, vidc_frame_data_type *pFrame,
                                   VidcDemuxer_FrameInfo_t &frameInfo );
    QCImageFormat_e GetQCImageFormat( vidc_codec_type codecType );

    static void FileSourceCallback( FileSourceCallBackStatus eStatus, void *pClientData );

private:
    FileSource m_fileSource;
    wchar_t *m_videoFile = nullptr;

    bool m_bFileOpen;
    bool m_bSendCodecConfig;
    bool m_bKeyFrameFound;
    bool m_bInitialized = false;

    uint64_t m_frameIdx;
    uint64_t m_startFrameIdx;

    VidcDemuxer_VideoInfo_t m_videoInfo;

    std::unordered_map<void *, vidc_frame_data_type> m_frameBufferMap;
};

}   // namespace sample
}   // namespace QC
#endif /* QC_SAMPLE_VIDC_DEMUXER_HPP */
