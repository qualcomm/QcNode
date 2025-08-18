// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QCNODE_SAMPLE_C2C_EXAMPLE_HPP
#define QCNODE_SAMPLE_C2C_EXAMPLE_HPP

#include "QC/sample/SampleIF.hpp"
extern "C"
{
#include <c2c.h>
}

namespace QC
{
namespace sample
{

/// @brief qcnode::sample::SampleC2C
///
/// SampleC2C that to demonstate how to chip to chip communicaiton over PCIe to pass camera frame
class SampleC2C : public SampleIF
{
public:
    SampleC2C();
    ~SampleC2C();

    /// @brief Initialize the C2C
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the C2C
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the C2C
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the C2C
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadPubMain();
    void ThreadSubMain();

    int WaitC2cMsg( void *buff, size_t size );

private:
    struct DmaInfo
    {
        void *pData;
        uint64_t dmaBufId;
        int dmaFd;
    };

private:
    std::string m_topicName;
    std::string m_buffersName;

    std::thread m_thread;
    bool m_stop;

    bool m_bIsPub = true;
    uint32_t m_queueDepth = 2;

    int m_c2cFd = -1;

    uint32_t m_width;
    uint32_t m_height;
    QCImageFormat_e m_format;

    uint32_t m_poolSize = 4;
    uint32_t m_channleId;

    SharedBufferPool m_imagePool;

    std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> m_buffers;
    std::vector<DmaInfo> m_dmaInfos;
    std::map<uint64_t, uint32_t> m_dmaIndexMap;


    DataSubscriber<DataFrames_t> m_sub;
    DataPublisher<DataFrames_t> m_pub;
};   // class SampleC2C

}   // namespace sample
}   // namespace QC

#endif   // QCNODE_SAMPLE_C2C_EXAMPLE_HPP
