// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_POST_PROC_CENTERNET_HPP_
#define _RIDEHAL_SAMPLE_POST_PROC_CENTERNET_HPP_

#include "OpenclIface.hpp"
#include "ridehal/sample/SampleIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::OpenclIface;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SamplePostProcCenternet
///
/// SamplePostProcCenternet that to demonstate how to do QNN output post processing for Centernet
class SamplePostProcCenternet : public SampleIF
{
public:
    SamplePostProcCenternet();
    ~SamplePostProcCenternet();

    /// @brief Initialize the PostProc
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the PostProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the PostProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the PostProc
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    RideHalError_e RegisterInputBuffers( DataFrames_t &tensors );
    RideHalError_e RegisterOutputBuffers();
    void PostProcCPU( DataFrames_t &inputsFrame );
    RideHalError_e PostProcCL( DataFrames_t &tensors );
    void SetCLParams();
    void NMS( std::vector<Road2DObject_t> &boxes, float thres );
    float ComputeIou( const Road2DObject_t &box1, const Road2DObject_t &box2 );

private:
    std::string m_inputTopicName;
    std::string m_outputTopicName;

    std::thread m_thread;
    bool m_stop;
    RideHal_ProcessorType_e m_processor;

    uint32_t m_roiX = 0;
    uint32_t m_roiY = 0;
    uint32_t m_camWidth = 1920;
    uint32_t m_camHeight = 1024;

    // the score and NMS threshold value used for the NMS post processing
    float m_scoreThreshold = 0.6;
    float m_NMSThreshold = 0.6;

    // max object num
    const uint32_t MAX_OBJ_NUM = 1000;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
    float m_hmScale = 0.0;
    int32_t m_hmOffset = 0;
    float m_whScale = 0.0;
    int32_t m_whOffset = 0;
    float m_regScale = 0.0;
    int32_t m_regOffset = 0;
    uint32_t m_classNum = 0;
    uint32_t m_maxObjNum = MAX_OBJ_NUM;
    uint32_t m_kernelSize = 7;

    RideHal_SharedBuffer_t m_outputClsIdBuf;
    RideHal_SharedBuffer_t m_outputProbBuf;
    RideHal_SharedBuffer_t m_outputCoordsBuf;

    cl_mem m_clInputHMBuf;
    cl_mem m_clInputWHBuf;
    cl_mem m_clInputRegBuf;
    cl_mem m_clOutputClsIdBuf;
    cl_mem m_clOutputProbBuf;
    cl_mem m_clOutputCoordsBuf;

    OpenclSrv m_OpenclSrvObj;
    cl_kernel m_kernel;
    OpenclIfcae_Arg_t m_openclArgs[22];

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<Road2DObjects_t> m_pub;
};   // class SamplePostProcCenternet

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_POST_PROC_CENTERNET_HPP_
