// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_QNN_IMPL_HPP
#define QC_NODE_QNN_IMPL_HPP

#include "HTP/QnnHtpContext.h"
#include "HTP/QnnHtpDevice.h"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Infras/NodeTrace/NodeTrace.hpp"
#include "QC/Node/QNN.hpp"
#include "QnnInterface.h"
#include "QnnWrapperUtils.hpp"
#include "System/QnnSystemInterface.h"
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace QC
{
namespace Node
{
using namespace QC::Memory;

#ifndef QNN_NOTIFY_PARAM_NUM
/* @note this value must be power of 2 */
#define QNN_NOTIFY_PARAM_NUM 8u
#endif

#define QNN_NOTIFY_MAGIC ( 0x464900544F4E4E51ull )

#ifndef QNNIMPL_FRIEND_CLASS
#define QNNIMPL_FRIEND_CLASS()
#endif


typedef enum
{
    QNN_PROCESSOR_HTP0,
    QNN_PROCESSOR_HTP1,
    QNN_PROCESSOR_HTP2,
    QNN_PROCESSOR_HTP3,
    QNN_PROCESSOR_CPU,
    QNN_PROCESSOR_GPU,
    QNN_PROCESSOR_MAX
} Qnn_ProcessorType_e;

typedef enum
{
    QNN_PERF_PROFILE_EXTREME_POWER_SAVER,
    QNN_PERF_PROFILE_HIGH_POWER_SAVER,
    QNN_PERF_PROFILE_LOW_POWER_SAVER,
    QNN_PERF_PROFILE_POWER_SAVER,
    QNN_PERF_PROFILE_LOW_BALANCED,
    QNN_PERF_PROFILE_BALANCED,
    QNN_PERF_PROFILE_DEFAULT,
    QNN_PERF_PROFILE_HIGH_PERFORMANCE,
    QNN_PERF_PROFILE_SUSTAINED_HIGH_PERFORMANCE,
    QNN_PERF_PROFILE_BURST,
    QNN_PERF_PROFILE_MAX,
} Qnn_PerfProfile_e;

/**
 * @brief Represents metadata for a UDO (User-Defined Operation) package.
 *
 * This structure holds essential information required to load and interface
 * with a UDO library, including the name of the interface provider and the
 * path to the UDO shared library.
 *
 * @param interfaceProvider A string specifying the name of the interface provider
 *                          responsible for implementing the UDO functionality.
 * @param udoLibPath        A string representing the file system path to the
 *                          UDO shared library (.so/.dll) that contains the implementation.
 */
typedef struct
{
    std::string interfaceProvider;
    std::string udoLibPath;
} Qnn_UdoPackage_t;

/**
 * @brief Enumeration of QNN model loading methods.
 *
 * This enum defines the different ways a QNN model can be loaded into the runtime environment.
 *
 * @enum QNN_LOAD_CONTEXT_BIN_FROM_FILE
 *        Load the QNN model from a context binary file located on the file system.
 *
 * @enum QNN_LOAD_CONTEXT_BIN_FROM_BUFFER
 *        Load the QNN model from a context binary buffer in memory.
 *        This method is useful when the context binary file is encrypted.
 *        In such cases, the user must first decrypt the file into a memory buffer,
 *        which is then used to load the model.
 *
 * @enum QNN_LOAD_SHARED_LIBRARY_FROM_FILE
 *        Load the QNN model from a shared library file (.so) on the file system.
 */
typedef enum
{
    QNN_LOAD_CONTEXT_BIN_FROM_FILE,
    QNN_LOAD_CONTEXT_BIN_FROM_BUFFER,
    QNN_LOAD_SHARED_LIBRARY_FROM_FILE
} Qnn_LoadType_e;

/**
 * @brief Configuration structure for a QNN Node.
 *
 * This structure defines all parameters required to initialize and execute a QNN (Qualcomm Neural
 * Network) model within a node, including model loading options, memory buffers, processor
 * settings, and buffer mappings.
 *
 * @param loadType         Specifies the method used to load the QNN model.
 *                         Options include loading from a file, memory buffer, or shared library.
 *
 * @param modelPath        File system path to the QNN model context binary or shared library.
 *                         Used when the model is loaded from a file.
 *
 * @param contextBufferId  Index of the buffer in QCNodeInit::buffers used to load the QNN model
 *                         from memory. Typically used when the QNN context binary is encrypted
 *                         and decrypted by the user application.
 * @note                   This field is ignored if loadType is not
 *                         QNN_LOAD_CONTEXT_BIN_FROM_BUFFER.
 *
 * @param processorType    Specifies the target hardware processor on which the model will run.
 *                         Typical values include CPU, HTP (Hexagon Tensor Processor), or GPU.
 *
 * @param coreIds          A list of core IDs representing the target hardware processors
 *                         on which the model will be executed.
 *
 * @param priority         Execution priority for the QNN model. May influence scheduling
 *                         and resource allocation in multi-model or multi-threaded environments.
 *
 * @param udoPackages      A vector of UDO (User-Defined Operation) package descriptors required
 *                         by the model for custom operations.
 *
 * @param bufferIds        Indices of buffers in QCNodeInit::buffers provided by the user
 *                         application for use by QNN. These buffers are registered during the
 *                         initialization stage.
 * @note                   This field is optional. If empty, buffers will be registered when
 *                         the ProcessFrameDescriptor API is called.
 *
 * @param globalBufferIdMap Mapping of global buffer indices used to identify which buffers in
 *                          QCFrameDescriptorNodeIfs correspond to QNN inputs and outputs.
 * @note                   This field is optional. If empty, a default mapping is applied:
 *                         - Indices 0 to N-1: Inputs (for N input tensors)
 *                         - Indices N to N+M-1: Outputs (for M output tensors)
 *                         - Index N+M: Error data slot (used in asynchronous mode with a callback)
 *
 * @param bDeRegisterAllBuffersWhenStop If true, all registered buffers will be deregistered
 *                         when the QNN node's Stop API is called.
 *
 * @param perfProfile      Specifies the performance profile to be used.
 *
 * @param bWeightSharingEnabled This field sets the weight sharing which is by default false.
 *
 * @param bUseExtendedUdma This field enables preparing graphs, associated with this context, with
 *                      far-mapping enabled so that weights and spill/fill buffer are mapped to the
 *                      far region of the DSP which is helpful if PD's limited VA space is
 *                      exhausted. Total RAM usage may increase if used together with shared
 *                      weights. Only available for Hexagon arch v81 and above.
 */
typedef struct QnnImplConfig : public QCNodeConfigBase_t
{
    Qnn_LoadType_e loadType;
    std::string modelPath;
    uint32_t contextBufferId;
    Qnn_ProcessorType_e processorType;
    std::vector<uint32_t> coreIds;
    Qnn_Priority_t priority;
    std::vector<Qnn_UdoPackage_t> udoPackages;
    std::vector<uint32_t> bufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
    Qnn_PerfProfile_e perfProfile;
    bool bWeightSharingEnabled;
    bool bUseExtendedUdma;
} QnnImplConfig_t;

// TODO
typedef struct QnnImplMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bEnablePerf;
} QnnImplMonitorConfig_t;

// Graph Related Function Handle Types
typedef qnn_wrapper_api::ModelError_t ( *QnnImplComposeGraphsFnHandleType_t )(
        Qnn_BackendHandle_t, QNN_INTERFACE_VER_TYPE, Qnn_ContextHandle_t,
        const qnn_wrapper_api::GraphConfigInfo_t **, const uint32_t,
        qnn_wrapper_api::GraphInfo_t ***, uint32_t *, bool, QnnLog_Callback_t, QnnLog_Level_t );

typedef struct QnnImplFunctionPointers
{
    QnnImplComposeGraphsFnHandleType_t composeGraphsFnHandle;
    QNN_INTERFACE_VER_TYPE qnnInterface;
    QNN_SYSTEM_INTERFACE_VER_TYPE qnnSystemInterface;
} QnnImplFunctionPointers_t;

class QnnImpl
{
public:
    QnnImpl( QCNodeID_t &nodeId, Logger &logger )
        : m_nodeId( nodeId ),
          m_logger( logger ),
          m_state( QC_OBJECT_STATE_INITIAL ) {};
    QnnImplConfig_t &GetConfig() { return m_config; }
    QnnImplMonitorConfig_t &GetMonitorConfig() { return m_monitorConfig; }

    QCStatus_e Initialize( QCNodeEventCallBack_t callback,
                           std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );
    QCStatus_e Start();
    QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );
    QCStatus_e Stop();
    QCStatus_e DeInitialize();

    /**
     * @brief Enables performance measurement for QNN model execution.
     * @return QC_STATUS_OK on success; other status codes indicate failure.
     */
    QCStatus_e EnablePerf();

    /**
     * @brief Retrieves the most recent QNN performance data.
     * @param[out] perf Reference to a QNN performance data structure to be filled with results.
     * @return QC_STATUS_OK on success; other status codes indicate failure.
     */
    QCStatus_e GetPerf( Qnn_Perf_t &perf );

    /**
     * @brief Disables performance measurement for QNN model execution
     * return QC_STATUS_OK on success; other status codes indicate failure.
     */
    QCStatus_e DisablePerf();

    /**
     * @brief Retrieves the current state of the QNN Node.
     *
     * This function returns the current operational state of the QNN Node,
     * which may indicate whether the node is initialized, running, stopped, or in an error state.
     *
     * @return The current state of the QNN Node.
     */
    QCObjectState_e GetState();


    /**
     * @brief Retrieves metadata for all input tensors of the model.
     *
     * This function populates the provided vector with information about each input tensor,
     * including their names, types, dimensions, and quantization parameters.
     *
     * @param[out] inputTensors  A reference to a vector that will be filled with input tensor
     * metadata.
     * @return QC_STATUS_OK if the operation is successful; an appropriate error code otherwise.
     */
    QCStatus_e GetInputTensors( std::vector<Qnn_Tensor_t> &inputTensors );

    /**
     * @brief Retrieves metadata for all output tensors of the model.
     *
     * This function populates the provided vector with information about each output tensor,
     * including their names, types, dimensions, and quantization parameters.
     *
     * @param[out] inputTensors  A reference to a vector that will be filled with output tensor
     * metadata.
     * @return QC_STATUS_OK if the operation is successful; an appropriate error code otherwise.
     */
    QCStatus_e GetOutputTensors( std::vector<Qnn_Tensor_t> &outputTensors );

    /**
     * @brief Converts a QNN-defined data type to a QCNode-defined tensor type.
     *
     * This function maps the given QNN data type to its corresponding tensor type
     * as defined in the QCNode framework.
     *
     * @param[in] dataType  The data type defined by QNN (e.g., QNN_DATATYPE_FLOAT_32).
     * @return The corresponding tensor type defined in QCNode (QCTensorType_e).
     */
    QCTensorType_e SwitchFromQnnDataType( Qnn_DataType_t dataType );

    /**
     * @brief Converts a QCNode-defined tensor type to a QNN-defined data type.
     *
     * This function maps the specified tensor type from the QCNode framework
     * to its corresponding data type as defined by QNN.
     *
     * @param[in] tensorType  The tensor type defined in QCNode (QCTensorType_e).
     * @return The corresponding QNN data type (Qnn_DataType_t).
     */
    Qnn_DataType_t SwitchToQnnDataType( QCTensorType_e tensorType );

private:
    typedef struct
    {
        Qnn_MemHandle_t memHandle;
        size_t size;
        int32_t fd;
    } DmaMemInfo_t;

    typedef struct
    {
        uint64_t magic;
        QnnImpl *pSelf;
        QCFrameDescriptorNodeIfs *pFrameDesc;
    } NotifyParam_t;

    typedef struct NotifyParamQueue
    {
        NotifyParam_t notifyParam[QNN_NOTIFY_PARAM_NUM];
        uint16_t ring[QNN_NOTIFY_PARAM_NUM];
        uint16_t popIdx;
        uint16_t pushIdx;

    public:
        void Init();
        void Push( NotifyParam_t *pNotifyParam );
        NotifyParam_t *Pop();
    } NotifyParamQueue_t;

private:
    static void QnnNotifyFn( void *pNotifyParam, Qnn_NotifyStatus_t notifyStatus );
    void QnnNotifyFn( NotifyParam_t &notifyParam, Qnn_NotifyStatus_t notifyStatus );

    QnnLog_Level_t GetQnnLogLevel( Logger_Level_e level );
    uint32_t GetQnnDeviceId( Qnn_ProcessorType_e processorType );
    QCStatus_e LoadOpPackages( std::vector<Qnn_UdoPackage_t> &udoPackages );

    QCStatus_e CreateFromModelSo( std::string modelFile );
    QCStatus_e CreateFromBinaryBuffer( QCBufferDescriptorBase &bufDesc );
    QCStatus_e CreateFromBinaryFile( std::string modelFile );
    QCStatus_e SetupGlobalBufferIdMap();

    QCStatus_e ExtractProfilingEvent( QnnProfile_EventId_t profileEventId, Qnn_Perf_t &perf,
                                      bool &bPerfDataValid );

    QCStatus_e RemoteRegisterBuf( const TensorDescriptor_t &tensorDesc, int &fd );
    QCStatus_e RegisterBufferToHTP( const TensorDescriptor_t &tensorDesc,
                                    Qnn_MemHandle_t &memHandle );
    QCStatus_e GetMemHandle( const TensorDescriptor_t &tensorDesc, Qnn_MemHandle_t &memHandle );
    QCStatus_e RegisterTensor( const TensorDescriptor_t &tensorDesc );

    QCStatus_e ValidateTensor( const TensorDescriptor_t &tensorDesc, const Qnn_Tensor_t &tensor );

    void RemoteDeRegisterBuf( void *pData, size_t size );
    QCStatus_e DeRegisterAllBuffers();
    QCStatus_e Destroy();

    QCStatus_e GetQnnFunctionPointers( std::string backendPath, std::string modelPath,
                                       bool loadModelLib );
    QCStatus_e GetQnnSystemFunctionPointers( std::string systemLibraryPath );
    QCStatus_e DeepCopyQnnTensorInfo( Qnn_Tensor_t *dst, const Qnn_Tensor_t *src );
    QCStatus_e CopyTensorsInfo( const Qnn_Tensor_t *tensorsInfoSrc, Qnn_Tensor_t *&tensorWrappers,
                                uint32_t tensorsCount );
    QCStatus_e CopyGraphsInfo( const QnnSystemContext_GraphInfo_t *graphsInput,
                               const uint32_t numGraphs,
                               qnn_wrapper_api::GraphInfo_t **&graphsInfo );
    QCStatus_e CopyGraphsInfoV1( const QnnSystemContext_GraphInfoV1_t *graphInfoSrc,
                                 qnn_wrapper_api::GraphInfo_t *graphInfoDst );
    QCStatus_e CopyGraphsInfoV3( const QnnSystemContext_GraphInfoV3_t *graphInfoSrc,
                                 qnn_wrapper_api::GraphInfo_t *graphInfoDst );
    QCStatus_e CopyMetadataToGraphsInfo( const QnnSystemContext_BinaryInfo_t *binaryInfo,
                                         qnn_wrapper_api::GraphInfo_t **&graphsInfo,
                                         uint32_t &graphsCount );
    void FreeQnnTensor( Qnn_Tensor_t &tensor );
    void FreeQnnTensors( Qnn_Tensor_t *&tensors, uint32_t numTensors );
    QCStatus_e FreeGraphsInfo( qnn_wrapper_api::GraphInfoPtr_t **graphsInfo, uint32_t numGraphs );
    QCStatus_e SetHtpPerformanceMode();
    QCStatus_e SetPerformanceMode();
    bool IsHtpProcessor();

    static void QnnLog_Callback( const char *fmt, QnnLog_Level_t logLevel, uint64_t timestamp,
                                 va_list args );
    static size_t memscpy( void *dst, size_t dstSize, const void *src, size_t copySize );

private:
    QCNodeID_t &m_nodeId;
    Logger &m_logger;
    QnnImplConfig_t m_config;
    QnnImplMonitorConfig_t m_monitorConfig;
    QCObjectState_e m_state;

    QCNodeEventCallBack_t m_callback = nullptr;

    std::mutex m_lock;
    static std::mutex s_lock[QNN_PROCESSOR_MAX];
    static std::map<void *, int> s_dmaMemRefMap[QNN_PROCESSOR_MAX];
    static std::map<Qnn_ProcessorType_e, std::string> s_Backends;

    static constexpr size_t CONTEXT_CONFIG_SIZE = 3;

    Qnn_BackendHandle_t m_backendHandle = nullptr;
    Qnn_DeviceHandle_t m_deviceHandle = nullptr;
    void *m_backendLibraryHandle = nullptr;
    void *m_modelLibraryHandle = nullptr;
    void *m_systemLibraryHandle = nullptr;
    Qnn_ProfileHandle_t m_profileBackendHandle = nullptr;

    Qnn_LogHandle_t m_logHandle = nullptr;

    QnnImplFunctionPointers_t m_qnnFunctionPointers;

    const QnnBackend_Config_t **m_backendConfig = nullptr;
    QnnSystemContext_Handle_t m_systemContext = nullptr;
    Qnn_ContextHandle_t m_context = nullptr;
    QnnContext_Config_t *m_contextConfig[CONTEXT_CONFIG_SIZE + 1] = { nullptr };
    QnnContext_Config_t m_contextConfigArray[CONTEXT_CONFIG_SIZE];
    QnnHtpContext_CustomConfig_t m_useExtendedUdmaConfig;
    QnnHtpContext_CustomConfig_t m_weightSharingEnabledConfig;

    bool m_bLoadFromCachedBinary = false;

    qnn_wrapper_api::GraphInfo_t **m_graphsInfo = nullptr;
    uint32_t m_graphsCount = 0;

    const qnn_wrapper_api::GraphConfigInfo_t **m_graphConfigsInfo = nullptr;
    uint32_t m_graphConfigsInfoCount = 0;

    const QnnDevice_PlatformInfo_t *m_platformInfo = nullptr;

    NotifyParamQueue_t m_notifyParamQ;
    std::map<void *, DmaMemInfo_t> m_dmaMemInfoMap;

    uint32_t m_inputTensorNum;
    uint32_t m_outputTensorNum;
    uint32_t m_bufferDescNum;

    std::vector<uint32_t> m_powerConfigIds;
    QnnHtpDevice_PerfInfrastructure_t *m_perfInfra{ nullptr };

    std::vector<Qnn_Tensor_t> m_inputs;
    std::vector<Qnn_Tensor_t> m_outputs;

    QC_DECLARE_NODETRACE();

    QNNIMPL_FRIEND_CLASS();
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_QNN_IMPL_HPP
