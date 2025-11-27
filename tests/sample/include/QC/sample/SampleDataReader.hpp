// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_DATAREADER_HPP_
#define _QC_SAMPLE_DATAREADER_HPP_

#include "QC/sample/SampleIF.hpp"
#include <mutex>

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleDataReader
///
/// SampleDataReader that to demonstate how to use the QC component Remap
class SampleDataReader : public SampleIF
{
public:
    SampleDataReader();
    ~SampleDataReader();

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
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();
    QCStatus_e LoadImage( std::shared_ptr<SharedBuffer_t> image, std::string path );
    QCStatus_e LoadTensor( std::shared_ptr<SharedBuffer_t> tensor, std::string path );

private:
    typedef enum
    {
        DATA_READER_TYPE_IMAGE,
        DATA_READER_TYPE_TENSOR,
    } DataReaderType_e;

    typedef struct
    {
        DataReaderType_e type;
        QCImageFormat_e format;
        uint32_t width;
        uint32_t height;
        QCTensorProps_t tensorProps;
        std::string dataPath;
    } DataReaderConfig_t;

    float m_fps;
    std::vector<DataReaderConfig_t> m_configs;
    uint32_t m_numOfDataReaders;
    uint32_t m_offset;

    uint32_t m_poolSize = 4;
    std::string m_topicName;

    std::thread m_thread;
    std::vector<SharedBufferPool> m_bufferPools;
    bool m_stop;

    DataPublisher<DataFrames_t> m_pub;
    QCAllocationCache_e m_bufferCache = QC_CACHEABLE;

};   // class SampleDataReader

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_DATAREADER_HPP_
