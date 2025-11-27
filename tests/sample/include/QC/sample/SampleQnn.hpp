// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_QNN_HPP
#define QC_SAMPLE_QNN_HPP

#include "QC/Node/QNN.hpp"
#include "QC/sample/SampleIF.hpp"

namespace QC
{
namespace sample
{
using namespace QC::Node;

/// @brief qcnode::sample::SampleQnn
///
/// SampleQnn that to demonstate how to use the QC Node Qnn
class SampleQnn : public SampleIF
{
public:
    SampleQnn();
    ~SampleQnn();

    /// @brief Initialize the QC Node Qnn
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the QC Node Qnn
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the QC Node Qnn
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the QC Node Qnn
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

    /**
     * @brief Retrieves the version identifier of the Node QNN.
     */
    const uint32_t GetVersion() const;

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

    QCStatus_e ConvertDtToInfo( DataTree &dt, TensorInfo_t &info );

    void EventCallback( const QCNodeEventInfo_t &info );

private:
    typedef enum
    {
        SAMPLE_QNN_IMAGE_CONVERT_DEFAULT,
        SAMPLE_QNN_IMAGE_CONVERT_GRAY,
        SAMPLE_QNN_IMAGE_CONVERT_CHROMA_FIRST,
    } SampleQnnImageConvertType_e;

private:
    SampleQnnImageConvertType_e m_imageConvertType = SAMPLE_QNN_IMAGE_CONVERT_DEFAULT;
    uint32_t m_poolSize = 4;
    bool m_bAsync = false;
    std::condition_variable m_condVar;

    std::string m_inputTopicName;
    std::string m_outputTopicName;
    std::string m_modelInOutInfoTopicName;

    std::string m_modelPath;
    std::thread m_thread;
    bool m_stop;

    std::vector<TensorInfo_t> m_inputsInfo;
    std::vector<TensorInfo_t> m_outputsInfo;

    std::vector<SharedBufferPool> m_tensorPools;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;
    DataPublisher<ModelInOutInfo_t> m_modelInOutInfoPub;

    DataTree m_dataTree;
    QCNodeInit_t m_nodeCfg;
    Qnn m_qnn;
    uint64_t m_asyncResult;
    QCProcessorType_e m_processor;
    int m_rsmPriority;

    NodeFrameDescriptorPool *m_pFrameDescPool = nullptr;
};   // class SampleQnn

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_QNN_HPP
