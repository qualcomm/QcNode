// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_OPTICALFLOW_VIZ_HPP
#define QC_SAMPLE_OPTICALFLOW_VIZ_HPP

#include "OpenclIface.hpp"
#include "QC/sample/SampleIF.hpp"

using namespace QC;
using namespace QC::libs::OpenclIface;

namespace QC
{
namespace sample
{

#define MVCOLOR_MAXCOLS 60

/**
 * @brief Max value of Motion vector from EVA. Used for MV color
 *        saturation.
 */
typedef struct MvColorMvMaxValue
{
    uint32_t nMvX; /*< max value of MV in horizontal direction */
    uint32_t nMvY; /*< max value of MV in vertical direction */
} MvColorMvMaxValue_t;


/// @brief qcnode::sample::SampleOpticalFlowViz
///
/// SampleOpticalFlowViz that to demonstate how to decoding the output of the QC component
/// OpticalFlow
class SampleOpticalFlowViz : public SampleIF
{
public:
    SampleOpticalFlowViz();
    ~SampleOpticalFlowViz();

    /// @brief Initialize the OpticalFlow Viz
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the OpticalFlow Viz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the OpticalFlow Viz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the OpticalFlow Viz
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

    void MvSetcols( uint32_t r, uint32_t g, uint32_t b, uint32_t k );
    void MvMakeColorWheel( void );
    QCStatus_e MvComputeColor( float fx, float fy, uint8_t *pix );

    QCStatus_e ConvertToRgbCPU( QCBufferDescriptorBase_t &Mv, QCBufferDescriptorBase_t &MvConf,
                                QCBufferDescriptorBase_t &RGB );
    QCStatus_e ConvertToRgbGPU( QCBufferDescriptorBase_t &Mv, QCBufferDescriptorBase_t &MvConf,
                                QCBufferDescriptorBase_t &RGB );

private:
    uint32_t m_width;
    uint32_t m_height;

    uint32_t m_poolSize = 4;

    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    SharedBufferPool m_rgbPool;
    bool m_stop;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    uint32_t m_ncols;
    uint8_t m_nColorwheel[MVCOLOR_MAXCOLS][3];
    BufferDescriptor_t m_colorwheelBuf;
    cl_mem m_clMemColorWheel;

    MvColorMvMaxValue_t m_sLiveMaxMvValue = { 0, 0 };   // full opaque
    MvColorMvMaxValue_t m_sMaxMvValue = { 128, 64 };    // semi-dense
    uint32_t m_nMagThreshold = 0;
    bool m_bSaturateColor = true;
    uint8_t m_nTransparency = 0xFF;
    uint8_t m_nConfMapThreshold = 150;

    QCProcessorType_e m_processor;
    OpenclSrv m_openclSrvObj;
    cl_kernel m_kernel;

    BufferManager *m_pBufMgr = nullptr;
};   // class SampleOpticalFlowViz

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_OPTICALFLOW_VIZ_HPP
