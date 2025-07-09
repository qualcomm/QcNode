// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_QNN_HPP
#define QC_NODE_QNN_HPP

#include "QC/component/QnnRuntime.hpp"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{

using namespace QC::component;

/**
 * @brief QNN Node Configuration Data Structure
 * @param params The QC component QNN configuration data structure.
 * @param contextBufferId The context buffer index in QCNodeInit::buffers, specifying the buffer to
 * load a QNN model from memory provided by the user. Typically used when the QNN context binary
 * file is encrypted, and the user application decrypts and saves it into a memory buffer.
 * @note contextBufferId is not used when params.loadType is not
 * QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER.
 * @param bufferIds The indices of buffers in QCNodeInit::buffers provided by the user application
 * for use by QNN. These buffers will be registered into QNN during the initialization stage.
 * @note bufferIds are optional and can be empty, in which case the buffers will be registered into
 * QNN when the API ProcessFrameDescriptor is called.
 * @param globalBufferIdMap The global buffer index map used to identify which buffer in
 * QCFrameDescriptorNodeIfs is used for QNN input(s) and output(s).
 * @note globalBufferIdMap is optional and can be empty, in which case a default buffer index map
 * will be applied for QNN input(s) and output(s). For example, for a model with N inputs and M
 * outputs:
 * - The index 0 of QCFrameDescriptorNodeIfs will be input 0.
 * - The index 1 of QCFrameDescriptorNodeIfs will be input 1.
 * - ...
 * - The index N-1 of QCFrameDescriptorNodeIfs will be input N-1.
 * - The index N of QCFrameDescriptorNodeIfs will be output 0.
 * - The index N+1 of QCFrameDescriptorNodeIfs will be output 1.
 * - ...
 * - The index N+M-1 of QCFrameDescriptorNodeIfs will be output M-1.
 * - The index N+M of QCFrameDescriptorNodeIfs will be the slot for error data information if QNN
 *   asynchronous mode is used with the callback specified.
 * @param bDeRegisterAllBuffersWhenStop When the Stop API of the QNN node is called and
 * bDeRegisterAllBuffersWhenStop is true, deregister all buffers.
 */
typedef struct QnnConfig : public QCNodeConfigBase_t
{
    QnnRuntime_Config_t params;
    uint32_t contextBufferId;
    std::vector<uint32_t> bufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} QnnConfig_t;

class QnnConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief QnnConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by QnnConfigIfs.
     * @param[in] qnn A reference to the QC QNN component to be used by QnnConfigIfs.
     * @return None
     */
    QnnConfigIfs( Logger &logger, QnnRuntime &qnn ) : NodeConfigIfs( logger ), m_qnn( qnn ) {}

    /**
     * @brief QnnConfigIfs Destructor
     * @return None
     */
    ~QnnConfigIfs() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * @param[in] config The configuration string.
     * @param[out] errors The error string returned if there is an error.
     * @note The config is a JSON string according to the templates below.
     * 1. Static configuration used for initialization:
     *   {
     *     "static": {
     *        "name": "The Node unique name, type: string",
     *        "id": "The Node unique ID, type: uint32_t",
     *        "processorType": "The processor type, type: string, options: [htp0, htp1, cpu, gpu],
     *                          default: htp0",
     *        "loadType": "The load type, type: string, options: [binary, library, buffer],
     *                     default: binary",
     *        "modelPath": "The QNN model file path, type: string, present if loadType is binary
     *                       or library",
     *        "contextBufferId": "The context buffer index in QCNodeInit::buffers,
     *                            type: uint32_t, present if loadType is buffer",
     *        "bufferIds": [A list of uint32_t values representing the indices of buffers
     *                      in QCNodeInit::buffers],
     *        "priority": "The QNN model scheduling priority, type: string,
     *                     options: [low, normal, normal_high, high], default: normal",
     *        "udoPackages": [
     *           {
     *             "udoLibPath": "The UDO library file path, type: string",
     *             "interfaceProvider": "The name of the interface provider, type: string"
     *           }
     *        ],
     *        "globalBufferIdMap": [
     *           {
     *              "name": "The buffer name, type: string",
     *              "id": "The index to a buffer in QCFrameDescriptorNodeIfs"
     *           }
     *        ],
     *        "deRegisterAllBuffersWhenStop": "Flag to deregister all buffers when stopped,
     *                   type: bool, default: false"
     *     }
     *   }
     *   @note: The udoPackages is a list and is optional.
     * 2. Dynamic configuration used to change some runtime parameters:
     *   {
     *     "dynamic": {
     *        "enablePerf": "enable performance profiling, type: bool"
     *     }
     *   }
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get Configuration Options
     * @return A reference string to the JSON configuration options.
     * @note
     * TODO: Provide a more detailed introduction about the JSON configuration options.
     */
    virtual const std::string &GetOptions();

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    virtual const QCNodeConfigBase_t &Get() { return m_config; };

private:
    QCStatus_e VerifyStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ApplyDynamicConfig( DataTree &dt, std::string &errors );
    std::string ConvertTensorTypeToString( QCTensorType_e tensorType );
    DataTree ConvertTensorInfoToJson( const QnnRuntime_TensorInfo_t &info );
    std::vector<DataTree>
    ConvertTensorInfoListToJson( const QnnRuntime_TensorInfoList_t &infoList );

private:
    /**
     * @brief The Udo Package
     * @param interfaceProvider The name of the interface provider.
     * @param udoLibPath The UDO library path.
     */
    typedef struct
    {
        std::string interfaceProvider;
        std::string udoLibPath;
    } UdoPackage_t;

private:
    QnnRuntime &m_qnn;
    bool m_bOptionsBuilt = false;
    std::string m_options;
    QnnConfig_t m_config;
    std::string m_modelPath;
    std::vector<UdoPackage_t> m_udoPkgs;
    std::vector<QnnRuntime_UdoPackage_t> m_udoPackages;
};

// TODO: how to handle QnnMonitorConfig
typedef struct QnnMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} QnnMonitorConfig_t;

// TODO: how to handle QnnMonitoringIfs
// logging is part of MonitoringIfs
class QnnMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief QnnMonitoringIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by QnnMonitoringIfs.
     * @param[in] qnn A reference to the QC QNN component to be used by QnnMonitoringIfs.
     * @return None
     */
    QnnMonitoringIfs( Logger &logger, QnnRuntime &qnn ) : m_logger( logger ), m_qnn( qnn ) {}

    ~QnnMonitoringIfs() {}

    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    virtual const std::string &GetOptions() { return m_options; }

    virtual const QCNodeMonitoringBase_t &Get() { return m_config; };

    virtual uint32_t GetMaximalSize();
    virtual uint32_t GetCurrentSize();

    /**
     * @brief Places the QNN performance data into a user-provided buffer.
     * @param[in] pData The user-provided buffer to store the QNN performance data.
     * @param[inout] size The size of the buffer pData and returns the actual size of the placed
     * data.
     * @return QC_STATUS_OK on success, or an error code on failure.
     * @note pData is of type QnnRuntime_Perf_t.
     * @example
     *   QnnRuntime_Perf_t perf;
     *   QCNodeMonitoringIfs& monitorIfs = qnn.GetMonitoringIfs();
     *   QCStatus_e status = monitorIfs.Place( &perf, sizeof( perf ) );
     */
    virtual QCStatus_e Place( void *pData, uint32_t &size );

private:
    QnnRuntime &m_qnn;
    Logger &m_logger;
    std::string m_options;
    QnnMonitorConfig_t m_config;
};

class Qnn : public NodeBase
{
public:
    /**
     * @brief Qnn Constructor
     * @return None
     */
    Qnn() : m_configIfs( m_logger, m_qnn ), m_monitorIfs( m_logger, m_qnn ) {};

    /**
     * @brief Qnn Destructor
     * @return None
     */
    ~Qnn() {};

    /**
     * @brief Initializes Node QNN.
     * @param[in] config The Node QNN configuration.
     * @note QCNodeInit::config - Refer to the comments of the API QnnConfigIfs::VerifyAndSet.
     * @note QCNodeInit::callback - The user application callback to notify the status of
     * the API ProcessFrameDescriptor. This is optional. If provided, the API ProcessFrameDescriptor
     * will be asynchronous; otherwise, the API ProcessFrameDescriptor will be synchronous.
     * @note QCNodeInit::buffers - Buffers provided by the user application. The buffers can be
     * provided for two purposes:
     * - 1. A buffer provided to store the decrypted QNN context binary (see
     *      QnnConfig::contextBufferId).
     * - 2. Shared buffers to be registered into QNN (see QnnConfig::bufferIds).
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node QNN configuration interface.
     * @return A reference to the Node QNN configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node QNN monitoring interface.
     * @return A reference to the Node QNN monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node QNN
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note If the QCNodeInit::callback is provided, this ProcessFrameDescriptor call will
     * return immediately after queuing the job into the QNN engine. Once the inference job
     * is completed, the callback will be invoked, making this an asynchronous API.
     * If the QCNodeInit::callback is not provided, this API is synchronous. When it
     * returns without error, it means the inference job is done and the output buffer will be
     * filled with valid output data.
     * @note The configuration globalBufferIdMap determines which buffers are input and which
     * are output.
     * @example For a QNN model with N inputs and M outputs:
     * - The globalBufferIdMap[0].globalBufferId of QCFrameDescriptorNodeIfs will be input 0.
     * - The globalBufferIdMap[1].globalBufferId of QCFrameDescriptorNodeIfs will be input 1.
     * - ...
     * - The globalBufferIdMap[N-1].globalBufferId of QCFrameDescriptorNodeIfs will be input N-1.
     * - The globalBufferIdMap[N].globalBufferId of QCFrameDescriptorNodeIfs will be output 0.
     * - The globalBufferIdMap[N+1].globalBufferId of QCFrameDescriptorNodeIfs will be output 1.
     * - ...
     * - The globalBufferIdMap[N+M-1].globalBufferId of QCFrameDescriptorNodeIfs will be output M-1.
     * - The globalBufferIdMap[N+M].globalBufferId of QCFrameDescriptorNodeIfs will be the
     *   slot for error data information if QNN asynchronous mode is used with the callback
     *   specified.
     * @note This API is not thread-safe. Avoid calling the ProcessFrameDescriptor API
     * on the same instance from multiple threads simultaneously.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the Node QNN
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node QNN
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the Node QNN
     * @return The current state of the Node QNN
     */
    virtual QCObjectState_e GetState() { return static_cast<QCObjectState_e>( m_qnn.GetState() ); }

private:
    static void OutputCallback( void *pAppPriv, void *pOutputPriv );
    static void ErrorCallback( void *pAppPriv, void *pOutputPriv, Qnn_NotifyStatus_t notifyStatus );
    void OutputCallback( void *pOutputPriv );
    void ErrorCallback( void *pOutputPriv, Qnn_NotifyStatus_t notifyStatus );

    QCStatus_e SetupGlobalBufferIdMap( const QnnConfig_t &cfg );

private:
    QnnRuntime m_qnn;
    QnnConfigIfs m_configIfs;
    QnnMonitoringIfs m_monitorIfs;

    bool m_bDeRegisterAllBuffersWhenStop = false;

    uint32_t m_inputNum;
    uint32_t m_outputNum;

    uint32_t m_bufferDescNum;

    std::vector<QCNodeBufferMapEntry_t> m_globalBufferIdMap;
    QCNodeEventCallBack_t m_callback = nullptr;
    uint64_t m_ticketId = 1;
    std::mutex m_lock;
    std::map<uint64_t, QCFrameDescriptorNodeIfs *> m_frameDescMap;
    QCSharedFrameDescriptorNodePool *m_pFrameDescPool = nullptr;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_QNN_HPP
