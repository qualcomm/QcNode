// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_IF_HPP_
#define _QC_SAMPLE_IF_HPP_

#include "QC/common/Types.hpp"
#include "QC/infras/logger/Logger.hpp"
#include "QC/sample/DataBroker.hpp"
#include "QC/sample/DataTypes.hpp"
#include "QC/sample/Profiler.hpp"
#include "QC/sample/SysTrace.hpp"

#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined( WITH_RSM_V2 )
#include <rsm_client_v2.h>
#endif

using namespace QC::common;

namespace QC
{
namespace sample
{

typedef std::map<std::string, std::string> SampleConfig_t;

class SampleIF;

typedef SampleIF *( *Sample_CreateFunction_t )();

#define REGISTER_SAMPLE( name, class_name )                                                        \
    static SampleIF *CreatSample##name() { return new class_name(); }                              \
    class Register##class_name                                                                     \
    {                                                                                              \
    public:                                                                                        \
        Register##class_name() { SampleIF::RegisterSample( #name, CreatSample##name ); }           \
    };                                                                                             \
    const Register##class_name g_register##name;

/// @brief qcnode::sample::SampleIF
///
/// Sample Interface that to demonstate how to use the QC component
class SampleIF
{
public:
    SampleIF() = default;
    ~SampleIF() = default;

    /// @brief Initialize the sample
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    virtual QCStatus_e Init( std::string name, SampleConfig_t &config ) = 0;

    /// @brief Start the sample
    /// @return QC_STATUS_OK on success, others on failure
    virtual QCStatus_e Start() = 0;

    /// @brief Stop the sample
    /// @return QC_STATUS_OK on success, others on failure
    virtual QCStatus_e Stop() = 0;

    /// @brief deinitialize the sample
    /// @return QC_STATUS_OK on success, others on failure
    virtual QCStatus_e Deinit() = 0;

    const char *GetName();

    /// @brief create a QC sample from type name
    /// @param name the sample type name
    /// @return the sample pointer on success, nullptr on failure
    static SampleIF *Create( std::string name );

    /// @brief register a QC sample
    /// @param name the sample type name
    /// @param createFnc the function to create the sample
    static void RegisterSample( std::string name, Sample_CreateFunction_t createFnc );

    static QCStatus_e RegisterBuffers( std::string name, const QCSharedBuffer_t *pBuffers,
                                       uint32_t numBuffers );

    static QCStatus_e GetBuffers( std::string name, QCSharedBuffer_t *pBuffers,
                                  uint32_t numBuffers );

    static QCStatus_e DeRegisterBuffers( std::string name );


protected:
    QCStatus_e Init( std::string name );

    QCStatus_e Init( QCProcessorType_e processor );
    QCStatus_e Lock();
    QCStatus_e Unlock();

    uint32_t StringToU32( std::string strV );
    std::string Get( SampleConfig_t &config, std::string key, const char *defaultV );
    std::string Get( SampleConfig_t &config, std::string key, std::string defaultV );
    std::vector<std::string> Get( SampleConfig_t &config, std::string key,
                                  std::vector<std::string> defaultV );
    std::vector<uint32_t> Get( SampleConfig_t &config, std::string key,
                               std::vector<uint32_t> defaultV );
    std::vector<float> Get( SampleConfig_t &config, std::string key, std::vector<float> defaultV );
    int32_t Get( SampleConfig_t &config, std::string key, int32_t defaultV );
    uint32_t Get( SampleConfig_t &config, std::string key, uint32_t defaultV );
    float Get( SampleConfig_t &config, std::string key, float defaultV );
    QCImageFormat_e Get( SampleConfig_t &config, std::string key, QCImageFormat_e defaultV );
    QCTensorType_e Get( SampleConfig_t &config, std::string key, QCTensorType_e defaultV );
    QCProcessorType_e Get( SampleConfig_t &config, std::string key, QCProcessorType_e defaultV );
    bool Get( SampleConfig_t &config, std::string key, bool defaultV );
    QCStatus_e LoadFile( QCSharedBuffer_t buffer, std::string path );

protected:
    std::string m_name;
    Profiler m_profiler;
    SysTrace m_systrace;
    QC_DECLARE_LOGGER();

private:
#if defined( WITH_RSM_V2 )
    rsm_acquire_cmd_v2 m_acquireCmdV2;
    rsm_acquire_rsp_v2 m_acquireRspV2;
    rsm_handle m_handle = 0;
    bool m_bRsmDisabled = false;
#endif

    QCProcessorType_e m_processor = QC_PROCESSOR_MAX;

    static std::mutex s_locks[QC_PROCESSOR_MAX];

    static std::mutex s_bufMapLock;
    static std::map<std::string, std::vector<QCSharedBuffer_t>> s_bufferMaps;

private:
    static std::map<std::string, Sample_CreateFunction_t> s_SampleMap;
};   // class SampleIF

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_IF_HPP_
