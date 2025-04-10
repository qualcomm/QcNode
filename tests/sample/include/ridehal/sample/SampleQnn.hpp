// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_QNN_HPP_
#define _RIDEHAL_SAMPLE_QNN_HPP_

#include "ridehal/component/QnnRuntime.hpp"
#include "ridehal/sample/SampleIF.hpp"
using namespace ridehal::common;
using namespace ridehal::component;

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleQnn
///
/// SampleQnn that to demonstate how to use the RideHal component QnnRuntime
class SampleQnn : public SampleIF
{
public:
    SampleQnn();
    ~SampleQnn();

    /// @brief Initialize the QnnRuntime
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the QnnRuntime
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the QnnRuntime
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the QnnRuntime
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

private:
    typedef enum
    {
        SAMPLE_QNN_IMAGE_CONVERT_DEFAULT,
        SAMPLE_QNN_IMAGE_CONVERT_GRAY,
        SAMPLE_QNN_IMAGE_CONVERT_CHROMA_FIRST,
    } SampleQnn_ImageConvertType_e;

    typedef struct
    {
        std::string interfaceProvider; /**<name of interface provider*/
        std::string udoLibPath;        /**<uod library path*/
    } UdoPackage_t;

private:
    QnnRuntime_Config_t m_config;
    SampleQnn_ImageConvertType_e m_imageConvertType = SAMPLE_QNN_IMAGE_CONVERT_DEFAULT;
    uint32_t m_poolSize = 4;
    bool m_bAsync = false;
    std::condition_variable m_condVar;

    std::string m_inputTopicName;
    std::string m_outputTopicName;
    std::string m_modelInOutInfoTopicName;

    std::string m_modelPath;
    std::vector<UdoPackage_t> m_udoPkgs;
    std::vector<QnnRuntime_UdoPackage_t> m_opPackagePaths;
    std::thread m_thread;
    bool m_stop;

    QnnRuntime_TensorInfoList_t m_inputInfoList;
    QnnRuntime_TensorInfoList_t m_outputInfoList;

    std::vector<SharedBufferPool> m_tensorPools;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;
    DataPublisher<ModelInOutInfo_t> m_modelInOutInfoPub;

    QnnRuntime m_qnn;
};   // class SampleQnn

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_QNN_HPP_
