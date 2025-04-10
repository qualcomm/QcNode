// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef RIDEHAL_SAMPLE_VIDC_DEMUXER_HPP
#define RIDEHAL_SAMPLE_VIDC_DEMUXER_HPP

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
#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"

using namespace ridehal::common;

namespace ridehal
{
namespace sample
{

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
    RideHal_ImageFormat_e format;
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

    RideHalError_e Init( VidcDemuxer_Config_t *pConfig );
    RideHalError_e DeInit();
    RideHalError_e GetFrame( RideHal_SharedBuffer_t *pSharedBuffer,
                             VidcDemuxer_FrameInfo_t &frameInfo );
    RideHalError_e GetVideoInfo( VidcDemuxer_VideoInfo_t &videoInfo );
    RideHalError_e SetPlayTime( int64_t time );

private:
    RideHalError_e OpenFile( wchar_t *videoFile );
    RideHalError_e GetVideoTrackInfo( VidcDemuxer_VideoInfo_t &videoInfo );
    RideHalError_e SelectVideoTrackId( uint32 trackId );
    RideHalError_e GetMaxFrameBufferSize( uint32 trackId, uint32_t &maxFrameSize );

    RideHalError_e ProcessFormatBlock( uint32 trackId, vidc_frame_data_type *pFrame );
    RideHalError_e GetNextMediaSample( uint32 id, vidc_frame_data_type *pFrame,
                                       VidcDemuxer_FrameInfo_t &frameInfo );
    RideHal_ImageFormat_e GetRideHalImageFormat( vidc_codec_type codecType );

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
}   // namespace ridehal
#endif /* RIDEHAL_SAMPLE_VIDC_DEMUXER_HPP */

