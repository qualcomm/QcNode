// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef _QC_SAMPLE_VIDEO_DEMUXER_HPP_
#define _QC_SAMPLE_VIDEO_DEMUXER_HPP_

#include "QC/sample/SampleIF.hpp"
#include "VidcDemuxer.hpp"

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleVideoDemuxer
///
/// SampleVideoDemuxer that to demonstate how to use the QC component VidcDemuxer
class SampleVideoDemuxer : public SampleIF
{
public:
    SampleVideoDemuxer();
    ~SampleVideoDemuxer();

    /// @brief Initialize the VidcDemuxer
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the VidcDemuxer
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the VidcDemuxer
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the VidcDemuxer
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    std::string m_videoFile;
    uint64_t m_frameIdx;
    uint32_t m_startTime;
    uint32_t m_playBackTime;
    uint32_t m_fps;
    bool m_bReplayMode;

    VidcDemuxer m_vidcDemuxer;
    VidcDemuxer_Config_t m_config;
    VidcDemuxer_VideoInfo_t m_videoInfo;

    uint32_t m_poolSize = 4;
    std::string m_topicName;

    std::thread m_thread;
    SharedBufferPool m_framePool;
    bool m_stop;

    DataPublisher<DataFrames_t> m_pub;


};   // class SampleVideoDemuxer

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_VIDEO_DEMUXER_HPP_
