// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_DATA_ONLINE_HPP
#define RIDEHAL_SAMPLE_DATA_ONLINE_HPP

#include "ridehal/sample/SampleIF.hpp"
#include <mutex>

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleDataOnline
///
/// SampleDataOnline that to demonstate how to use the RideHal component Remap
class SampleDataOnline : public SampleIF
{
public:
    SampleDataOnline();
    ~SampleDataOnline();

    /// @brief Initialize the remap
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the remap
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Start();

    /// @brief Stop the remap
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Stop();

    /// @brief deinitialize the remap
    /// @return RIDEHAL_ERROR_NONE on success, others on failure
    RideHalError_e Deinit();

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
        RideHal_BufferType_e dataType;
        uint32_t size;
        uint8_t reserved[120];
    };

    struct DataImageMeta
    {
        RideHal_BufferType_e dataType;
        uint32_t size;
        RideHal_ImageProps_t imageProps;
        uint8_t reserved[120 - sizeof( RideHal_ImageProps_t )];
    };

    struct DataTensorMeta
    {
        RideHal_BufferType_e dataType;
        uint32_t size;
        RideHal_TensorProps_t tensorProps;
        float quantScale;
        int32_t quantOffset;
        char name[64];
        uint8_t reserved[48 - sizeof( RideHal_TensorProps_t )];
    };

private:
    RideHalError_e ParseConfig( SampleConfig_t &config );
    RideHalError_e ReceiveData( Meta &meta );
    RideHalError_e ReplyModelInfo();
    RideHalError_e ReceiveMain();
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

    RideHal_BufferFlags_t m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;

    std::vector<uint8_t> m_payload;
};   // class SampleDataOnline

}   // namespace sample
}   // namespace ridehal

#endif   // RIDEHAL_SAMPLE_DATA_ONLINE_HPP
