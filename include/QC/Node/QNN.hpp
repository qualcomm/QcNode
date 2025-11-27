// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_QNN_HPP
#define QC_NODE_QNN_HPP

#include "QC/Node/NodeBase.hpp"
#include "QnnTypes.h"

namespace QC
{
namespace Node
{

/** @brief The QCNode QNN Version */
#define QCNODE_QNN_VERSION_MAJOR 2U
#define QCNODE_QNN_VERSION_MINOR 0U
#define QCNODE_QNN_VERSION_PATCH 4U

#define QCNODE_QNN_VERSION                                                                         \
    ( ( QCNODE_QNN_VERSION_MAJOR << 16U ) | ( QCNODE_QNN_VERSION_MINOR << 8U ) |                   \
      QCNODE_QNN_VERSION_PATCH )

/**
 * @brief Performance metrics for QNN model execution.
 *
 * This structure captures timing information related to the execution of a QNN model,
 * including total execution time and breakdowns by processor type.
 *
 * @param entireExecTime     Total execution time of the QNN model in microseconds (µs).
 *
 * @param rpcExecTimeCPU     Execution time (in microseconds) for the remote procedure call
 *                           on the CPU when the client invokes QnnGraph_execute.
 *
 * @param rpcExecTimeHTP     Execution time (in microseconds) for the remote procedure call
 *                           on the HTP (Hexagon Tensor Processor) when the client invokes
 *                           QnnGraph_execute.
 *
 * @param rpcExecTimeDSP     Execution time (in microseconds) for the remote procedure call
 *                           on the DSP or other accelerator when the client invokes
 *                           QnnGraph_execute.
 */
typedef struct
{
    uint64_t entireExecTime;
    uint64_t rpcExecTimeCPU;
    uint64_t rpcExecTimeHTP;
    uint64_t rpcExecTimeAcc;
} Qnn_Perf_t;


/**
 * @brief Represents the QNN implementation used by QnnConfig, QnnMonitor, and Node QNN.
 *
 * This class encapsulates the specific implementation details of the
 * QNN (Qualcomm Neural Network) runtime that are shared across configuration,
 * monitoring, and Node QNN.
 *
 * It serves as a central reference for components that need to interact with
 * the underlying QNN runtime implementation.
 */
class QnnImpl;

class QnnConfig : public NodeConfigBase
{
public:
    /**
     * @brief QnnConfig Constructor
     * @param[in] logger A reference to the logger to be shared and used by QnnConfig.
     * @param[in] pQnnImpl A pointer to the QnnImpl object to be used by QnnConfig.
     * @return None
     */
    QnnConfig( Logger &logger, QnnImpl *pQnnImpl )
        : NodeConfigBase( logger ),
          m_pQnnImpl( pQnnImpl )
    {}

    /**
     * @brief QnnConfig Destructor
     * @return None
     */
    ~QnnConfig() {}

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
     *        "logLevel": "The message log level, type: string,
     *                     options: [VERBOSE, DEBUG, INFO, WARN, ERROR],
     *                     default: ERROR"
     *        "processorType": "The processor type, type: string,
     *                          options: [ htp0, htp1, htp2, htp3, cpu, gpu ],
     *                          default: htp0",
     *        "coreIds": [ A list of uint32_t values representing core id ]
     *                   default: [0],
     *        "loadType": "The load type, type: string, options: [binary, library, buffer],
     *                     default: binary",
     *        "modelPath": "The QNN model file path, type: string, present if loadType is binary
     *                       or library",
     *        "contextBufferId": "The context buffer index in QCNodeInit::buffers,
     *                            type: uint32_t, present if loadType is buffer",
     *        "bufferIds": [ A list of uint32_t values representing the indices of buffers
     *                      in QCNodeInit::buffers ],
     *        "priority": "The QNN model scheduling priority, type: string,
     *                     options: [ low, normal, normal_high, high ], default: normal",
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
     *                   type: bool, default: false",
     *        "perfProfile": "Specifies perf profile to set, type: string,
     *                  options: [ low_balanced, balanced, default, high_performance,
     *                             sustained_high_performance, burst, low_power_saver,
     *                             power_saver, high_power_saver, extreme_power_saver ],
     *                  default: default",
     *        "weightSharingEnabled": "Enables weight sharing, type: bool, default: false",
     *        "extendedUdma": "Activates extended UDMA support, type: bool, default: false",
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
     * Use this API to query detailed input and output information from the loaded QNN model.
     */
    virtual const std::string &GetOptions();

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    virtual const QCNodeConfigBase_t &Get();

private:
    QCStatus_e VerifyStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ApplyDynamicConfig( DataTree &dt, std::string &errors );

    DataTree ConvertTensorInfoToJson( const Qnn_Tensor_t &info );
    std::vector<DataTree> ConvertTensorInfoListToJson( const std::vector<Qnn_Tensor_t> &infoList );

    QnnImpl *m_pQnnImpl = nullptr;
    bool m_bOptionsBuilt = false;
    std::string m_options;
};

// TODO: how to handle QnnMonitor
// logging is part of MonitoringIfs
class QnnMonitor : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief QnnMonitor Constructor
     * @param[in] logger A reference to the logger to be shared and used by QnnMonitor.
     * @param[in] qnn A reference to the QC QNN component to be used by QnnMonitor.
     * @return None
     */
    QnnMonitor( Logger &logger, QnnImpl *pQnnImpl ) : m_logger( logger ), m_pQnnImpl( pQnnImpl ) {}

    ~QnnMonitor() {}

    QnnMonitor( const QnnMonitor &other ) = delete;

    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    virtual const std::string &GetOptions();

    virtual const QCNodeMonitoringBase_t &Get();

    virtual uint32_t GetMaximalSize();
    virtual uint32_t GetCurrentSize();

    /**
     * @brief Places the QNN performance data into a user-provided buffer.
     * @param[in] pData The user-provided buffer to store the QNN performance data.
     * @param[inout] size The size of the buffer pData and returns the actual size of the placed
     * data.
     * @return QC_STATUS_OK on success, or an error code on failure.
     * @note pData is of type Qnn_Perf_t.
     * @example
     *   Qnn_Perf_t perf;
     *   QCNodeMonitoringIfs& monitorIfs = qnn.GetMonitoringIfs();
     *   QCStatus_e status = monitorIfs.Place( &perf, sizeof( perf ) );
     */
    virtual QCStatus_e Place( void *pData, uint32_t &size );

private:
    QnnImpl *m_pQnnImpl = nullptr;
    Logger &m_logger;
    std::string m_options;
};

class Qnn : public NodeBase
{
public:
    /**
     * @brief Qnn Constructor
     * @return None
     */
    Qnn();

    /**
     * @brief Qnn Destructor
     * @return None
     */
    ~Qnn();

    /**
     * @brief Initializes Node QNN.
     * @param[in] config The Node QNN configuration.
     * @note QCNodeInit::config - Refer to the comments of the API QnnConfig::VerifyAndSet.
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
     * @brief Starts the QNN Node.
     * @return QC_STATUS_OK on success; other status codes indicate failure.
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
     * @brief Stops the QNN Node.
     * @return QC_STATUS_OK on success; other status codes indicate failure.
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initializes the QNN Node and releases associated resources.
     *
     * This function performs cleanup operations for the QNN Node, including releasing
     * memory, deregistering buffers, and resetting internal states. It should be called
     * when the node is no longer needed.
     *
     * @return QC_STATUS_OK on success; other status codes indicate failure.
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Retrieves the current state of the QNN Node.
     *
     * This function returns the current operational state of the QNN Node,
     * which may indicate whether the node is initialized, running, stopped, or in an error state.
     *
     * @return The current state of the QNN Node.
     */
    virtual QCObjectState_e GetState();

private:
    QnnImpl *m_pQnnImpl;
    QnnConfig m_configIfs;
    QnnMonitor m_monitorIfs;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_QNN_HPP
