// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef _QC_SAMPLE_IF_HPP_
#define _QC_SAMPLE_IF_HPP_

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/sample/DataBroker.hpp"
#include "QC/sample/DataTypes.hpp"
#include "QC/sample/Profiler.hpp"
#include "QC/sample/SysTrace.hpp"

#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined( WITH_RSM_V2 )
#include <rsm_client_v2.h>
#endif

namespace QC
{
namespace sample
{

typedef std::map<std::string, std::string> SampleConfig_t;

class SampleIF;

typedef SampleIF *( *Sample_CreateFunction_t )();

#define REGISTER_SAMPLE( name, class_name )                                                        \
    static SampleIF *CreatSample##name()                                                           \
    {                                                                                              \
        return new class_name();                                                                   \
    }                                                                                              \
    class Register##class_name                                                                     \
    {                                                                                              \
    public:                                                                                        \
        Register##class_name()                                                                     \
        {                                                                                          \
            SampleIF::RegisterSample( #name, CreatSample##name );                                  \
        }                                                                                          \
    };                                                                                             \
    const Register##class_name g_register##name;

/// @brief qcnode::sample::SampleIF
///
/// Sample Interface that to demonstate how to use the QC component
class SampleIF
{
public:
    SampleIF();
    ~SampleIF() = default;

    /**
     * @brief Initializes the QCNode sample instance.
     *
     * Performs setup and configuration for the sample using the provided unique instance name
     * and configuration key-value map. This method is typically called during the sample's
     * lifecycle initialization phase and must be implemented by derived classes.
     *
     * @param[in] name The unique name identifying the sample instance.
     * @param[in] config A reference to a key-value map containing configuration parameters
     *                   specific to the sample.
     *
     * @return QC_STATUS_OK on successful initialization; otherwise, returns an error code
     *         indicating the reason for failure.
     */
    virtual QCStatus_e Init( std::string name, SampleConfig_t &config ) = 0;

    /**
     * @brief Starts the execution of the QCNode sample.
     *
     * Transitions the sample into its active or running state, initiating any internal processes
     * or operations required for its functionality. This method is typically called after
     * successful initialization and must be implemented by derived classes.
     *
     * @return QC_STATUS_OK if the sample starts successfully; otherwise, returns an error code
     *         indicating the reason for failure.
     */
    virtual QCStatus_e Start() = 0;

    /**
     * @brief Stops the execution of the QC sample.
     *
     * Transitions the sample out of its active state, halting any ongoing processes or operations.
     * This method is typically used during shutdown or when the sample needs to be paused or reset.
     * Must be implemented by derived classes to handle sample-specific stop logic.
     *
     * @return QC_STATUS_OK if the sample stops successfully; otherwise, returns an error code
     *         indicating the reason for failure.
     */
    virtual QCStatus_e Stop() = 0;

    /**
     * @brief Deinitializes the QC sample instance.
     *
     * Performs cleanup and resource release for the sample, reversing any setup done during
     * initialization. This method is typically called during shutdown or reconfiguration,
     * and must be implemented by derived classes to handle sample-specific deinitialization logic.
     *
     * @return QC_STATUS_OK if deinitialization succeeds; otherwise, returns an error code
     *         indicating the reason for failure.
     */
    virtual QCStatus_e Deinit() = 0;


    /**
     * @brief Retrieves the unique name of the QC sample instance.
     *
     * Returns the identifier assigned to the sample, which can be used for logging,
     * debugging, or referencing the sample within the system. The returned string is
     * a constant character pointer and should not be modified by the caller.
     *
     * @return A constant character pointer representing the sample's unique name.
     */
    const char *GetName() const;

    /**
     * @brief Creates a QCNode sample instance based on the specified sample type name.
     *
     * Looks up the registered creation function associated with the given name and invokes it
     * to instantiate a new sample object. If the name is not found or the creation fails,
     * the method returns a nullptr.
     *
     * @param[in] name The sample type name used to identify the corresponding creation function.
     * @return A pointer to the created sample instance on success; nullptr if creation fails.
     */
    static SampleIF *Create( std::string name );

    /**
     * @brief Registers a QCNode sample creation function under a specified sample type name.
     *
     * Associates a sample type name with a corresponding creation function, enabling dynamic
     * instantiation of sample objects based on their registered identifiers. This mechanism
     * supports extensibility and modularity in sample management.
     *
     * @param[in] name The unique name identifying the sample type.
     * @param[in] createFnc The function used to create instances of the specified sample type.
     */
    static void RegisterSample( std::string name, Sample_CreateFunction_t createFnc );

    /**
     * @brief Registers a list of shared buffer descriptors into the global buffer map.
     *
     * Associates a unique name with a collection of buffer descriptors, enabling centralized
     * management and retrieval of shared buffers. This mechanism allows other sample nodes
     * to access the buffers of their preceding nodes during initialization via the GetBuffers API.
     *
     * @param[in] name A unique identifier for the buffer group being registered.
     * @param[in] buffers A reference to a vector containing references to shared buffer
     *                    descriptors. These descriptors must remain valid for the duration of
     *                    their registration.
     *
     * @return QC_STATUS_OK if registration succeeds; otherwise, returns an error code indicating
     *         the reason for failure, such as duplicate registration or invalid input.
     */
    static QCStatus_e
    RegisterBuffers( std::string name,
                     std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> buffers );

    /**
     * @brief Retrieves a list of shared buffer descriptors from the global buffer map.
     *
     * This method looks up the buffer descriptors associated with the specified name and populates
     * the provided vector with references to those descriptors. It enables downstream nodes or
     * components to access buffers registered by preceding nodes during initialization.
     *
     * @param[in] name The unique identifier used to locate the registered buffer group.
     * @param[out] buffers A reference to a vector that will be populated with references to the
     *                     corresponding shared buffer descriptors.
     *
     * @return QC_STATUS_OK if retrieval is successful; otherwise, returns an error code indicating
     *         the reason for failure, such as missing registration or invalid input.
     */
    static QCStatus_e
    GetBuffers( std::string name,
                std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers );

    /**
     * @brief De-registers a buffer group from the global buffer map.
     *
     * Removes the association of the specified buffer group name from the global registry,
     * making the corresponding buffer descriptors inaccessible via the GetBuffers API.
     * This is typically used during shutdown, cleanup, or reinitialization phases to
     * release buffer resources and avoid stale references.
     *
     * @param[in] name The unique identifier of the buffer group to be removed.
     *
     * @return QC_STATUS_OK if deregistration succeeds; otherwise, returns an error code indicating
     *         the reason for failure, such as the name not being found in the registry.
     */
    static QCStatus_e DeRegisterBuffers( std::string name );


protected:
    QCStatus_e Init( std::string name, QCNodeType_e type = QC_NODE_TYPE_CUSTOM_0 );

    QCStatus_e Init( QCProcessorType_e processor, int rsmPriority = 0 );
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
    QCStatus_e LoadFile( QCBufferDescriptorBase_t &bufferDesc, std::string path );

protected:
    std::string m_name;
    QCNodeID_t m_nodeId;
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
    static std::map<std::string, std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>>>
            s_bufferMaps;

private:
    static std::map<std::string, Sample_CreateFunction_t> s_SampleMap;
    static std::atomic<uint32_t> s_nodeId;
};   // class SampleIF

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_IF_HPP_
