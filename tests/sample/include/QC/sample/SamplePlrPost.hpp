// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_PLRPOST_HPP_
#define _QC_SAMPLE_PLRPOST_HPP_

#include "QC/component/PostCenterPoint.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC;
using namespace QC::component;

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SamplePlrPost
///
/// SamplePlrPost that to demonstate how to use the QC component PostCenterPoint
class SamplePlrPost : public SampleIF
{
public:
    SamplePlrPost();
    ~SamplePlrPost();

    /// @brief Initialize the PostCenterPoint
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the PostCenterPoint
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the PostCenterPoint
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the PostCenterPoint
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    Point2D_t ProjectToImage( Point2D_t &pt, Point2D_t &center, float yaw );

private:
    PostCenterPoint_Config_t m_config = { QC_PROCESSOR_HTP0, 0 };
    uint32_t m_poolSize = 4;

    std::string m_inputLidarTopicName;
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_objsPool;
    bool m_stop;

    /**< parameter that used to project the BEV bbox into the pre-generated pointclound image */
    uint32_t m_offsetX;
    uint32_t m_offsetY;
    float m_ratioW;
    float m_ratioH;

    bool m_bDebug = false;

    DataSubscriber<DataFrames_t> m_lidarSub;
    DataSubscriber<DataFrames_t> m_infSub;
    DataPublisher<Road2DObjects_t> m_pub;

    std::vector<uint32_t> m_indexs = { 3, 0, 1, 4, 2 };

    PostCenterPoint m_plrPost;
};   // class SamplePlrPost

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_PLRPOST_HPP_
