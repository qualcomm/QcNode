// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _RIDEHAL_SAMPLE_DATAREADER_HPP_
#define _RIDEHAL_SAMPLE_DATAREADER_HPP_

#include "ridehal/sample/SampleIF.hpp"
#include <mutex>

namespace ridehal
{
namespace sample
{

/// @brief ridehal::sample::SampleDataReader
///
/// SampleDataReader that to demonstate how to use the RideHal component Remap
class SampleDataReader : public SampleIF
{
public:
    SampleDataReader();
    ~SampleDataReader();

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
    RideHalError_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    RideHalError_e LoadImage( std::shared_ptr<SharedBuffer_t> image, std::string path );
    RideHalError_e LoadTensor( std::shared_ptr<SharedBuffer_t> tensor, std::string path );

private:
    typedef enum
    {
        DATA_READER_TYPE_IMAGE,
        DATA_READER_TYPE_TENSOR,
    } DataReaderType_e;

    typedef struct
    {
        DataReaderType_e type;
        RideHal_ImageFormat_e format;
        uint32_t width;
        uint32_t height;
        RideHal_TensorProps_t tensorProps;
        std::string dataPath;
    } DataReaderConfig_t;

    uint32_t m_fps;
    std::vector<DataReaderConfig_t> m_configs;
    uint32_t m_numOfDataReaders;

    uint32_t m_poolSize = 4;
    std::string m_topicName;

    std::thread m_thread;
    std::vector<SharedBufferPool> m_bufferPools;
    bool m_stop;

    DataPublisher<DataFrames_t> m_pub;
    RideHal_BufferFlags_t m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;

};   // class SampleDataReader

}   // namespace sample
}   // namespace ridehal

#endif   // _RIDEHAL_SAMPLE_DATAREADER_HPP_
