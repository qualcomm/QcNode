// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_DATA_ONLINE_HPP
#define QC_SAMPLE_DATA_ONLINE_HPP

#include "QC/sample/SampleIF.hpp"
#include <mutex>

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleDataOnline
///
/// SampleDataOnline that to demonstate how to use the QC component Remap
class SampleDataOnline : public SampleIF
{
public:
    SampleDataOnline();
    ~SampleDataOnline();

    /// @brief Initialize the remap
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the remap
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the remap
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the remap
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    enum class MetaCommand : uint32_t
    {
        META_COMMAND_DATA,
        META_COMMAND_MODEL_INFO,
    };

    struct Meta
    { /* total 128 bytes */
        uint64_t payloadSize;
        uint64_t Id;
        uint64_t timestamp;
        uint32_t numOfDatas;
        MetaCommand command;
        uint8_t reserved[96];
    };

    struct MetaModelInfo
    { /* total 128 bytes */
        uint64_t payloadSize;
        uint64_t Id;
        uint64_t timestamp;
        uint32_t numOfDatas;
        MetaCommand command;
        uint32_t numInputs;
        uint32_t numOuputs;
        uint8_t reserved[88];
    };

    struct DataMeta
    {
        QCBufferType_e dataType;
        uint32_t size;
        uint8_t reserved[120];
    };

    struct DataImageMeta
    {
        QCBufferType_e dataType;
        uint32_t size;
        QCImageProps_t imageProps;
        uint8_t reserved[120 - sizeof( QCImageProps_t )];
    };

    struct DataTensorMeta
    {
        QCBufferType_e dataType;
        uint32_t size;
        QCTensorProps_t tensorProps;
        float quantScale;
        int32_t quantOffset;
        char name[64];
        uint8_t reserved[48 - sizeof( QCTensorProps_t )];
    };

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    QCStatus_e ReceiveData( Meta &meta );
    QCStatus_e ReplyModelInfo();
    QCStatus_e ReceiveMain();
    void ThreadSubMain();
    void ThreadPubMain();

private:
    uint32_t m_poolSize = 4;

    int m_port = 6666;

    int m_server = -1;
    int m_client = -1;
    bool m_bOnline = false;

    ModelInOutInfo_t m_modelInOutInfo;
    bool m_bHasInfInfo = false;

    std::string m_inputTopicName;
    std::string m_modelInOutInfoTopicName;
    std::string m_outputTopicName;

    std::thread m_subThread;
    std::thread m_pubThread;
    std::vector<SharedBufferPool> m_bufferPools;
    bool m_stop;

    uint32_t m_timeout = 2;

    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;

    DataSubscriber<ModelInOutInfo_t> m_modelInOutInfoSub;

    QCBufferFlags_t m_bufferFlags = QC_BUFFER_FLAGS_CACHE_WB_WA;

    std::vector<uint8_t> m_payload;
};   // class SampleDataOnline

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_DATA_ONLINE_HPP
