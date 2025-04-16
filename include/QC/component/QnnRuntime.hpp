// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_QNN_RUNTIME_HPP
#define QC_QNN_RUNTIME_HPP

#include <map>
#include <string.h>
#include <string>
#include <vector>

#include "DataUtil.hpp"
#include "DynamicLoadUtil.hpp"
#include "QC/component/ComponentIF.hpp"

using namespace qnn::tools;
using namespace qnn::tools::sample_app;
using namespace QC::common;

namespace QC
{
namespace component
{

#ifndef QNNRUNTIME_NOTIFY_PARAM_NUM
/* @note this value must be power of 2 */
#define QNNRUNTIME_NOTIFY_PARAM_NUM 8
#endif

/** @brief QnnRuntime performance information */
typedef struct
{
    uint64_t entireExecTime; /**<qnn model entire execution time (ms) */
    uint64_t rpcExecTimeCPU; /**<execution time(ms) of remote procedure call on the CPU processor
                                when client invokes QnnGraph_execute. */
    uint64_t rpcExecTimeHTP; /**<execution time(ms) of remote procedure call on the HTP processor
                                when client invokes QnnGraph_execute. */
    uint64_t rpcExecTimeAcc; /**<execution time(ms) of remote procedure call on the accelerator when
                                client invokes QnnGraph_execute. */
} QnnRuntime_Perf_t;

/** @brief UDO package information */
typedef struct
{
    const char *interfaceProvider; /**<The name of the interface provider*/
    const char *udoLibPath;        /**<The UDO library path*/
} QnnRuntime_UdoPackage_t;

/** @brief Qnn model loading type */
typedef enum
{
    QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE,   /**<load qnn model from binary file*/
    QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_BUFFER, /**<load qnn model from bin buffer*/
    QNNRUNTIME_LOAD_SHARED_LIBRARY_FROM_FILE /**<load qnn model from .so file*/
} QnnRuntime_LoadType_e;

/** @brief QnnRuntime configuration*/
typedef struct
{
    QnnRuntime_LoadType_e loadType =
            QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE;   /**<Qnn model loading type*/
    const char *modelPath;                           /**<Qnn model path*/
    uint8_t *contextBuffer;                          /**<Pointer to qnn model context buffer */
    uint64_t contextSize;                            /**<qnn model context buffer size */
    QCProcessorType_e processorType;                 /**<Hardware compute processor that the QNN
                                                      ** model running on*/
    Qnn_Priority_t priority = QNN_PRIORITY_DEFAULT;  /**<Qnn priority */
    QnnRuntime_UdoPackage_t *pUdoPackages = nullptr; /**<The pointer to QnnRuntime udo package */
    int numOfUdoPackages = 0;                        /**<The number of udo packages */
} QnnRuntime_Config_t;

/** @brief QnnRuntime tensor information */
typedef struct
{
    const char *pName;          /**<The name of the tensor*/
    QCTensorProps_t properties; /**<The property of the tensor*/
    float quantScale;           /**<The value of the quantization scale*/
    int32_t quantOffset;        /**<The value of the quantization offset*/
} QnnRuntime_TensorInfo_t;

/** @brief The list of QnnRuntime tensor information */
typedef struct
{
    const QnnRuntime_TensorInfo_t *pInfo; /**<Pointer to QnnRuntime tensor information*/
    uint32_t num;                         /**<The number of tensors*/
} QnnRuntime_TensorInfoList_t;

/** @brief callback for QNN graphExecute done that output is ready.
 * @param[in] pAppPriv the private data to be used to identify the app instance
 * @param[in] pOutputPriv the private data associated with the output tensors
 * @return None
 */
typedef void ( *QnnRuntime_OutputCallback_t )( void *pAppPriv, void *pOutputPriv );

/** @brief callback for QNN graphExecute done but with error.
 * @param[in] pAppPriv the private data to be used to identify the app instance
 * @param[in] pOutputPriv the private data associated with the output tensors
 * @param[in] notifyStatus the QNN SDK specific notify status
 * @return None
 */
typedef void ( *QnnRuntime_ErrorCallback_t )( void *pAppPriv, void *pOutputPriv,
                                              Qnn_NotifyStatus_t notifyStatus );

/** @addtogroup QnnRuntime Functions
@{ */

class QnnRuntime : public ComponentIF
{
public:
    QnnRuntime();
    ~QnnRuntime();

    /**
     * @brief Initialize QnnRuntime component
     * @param[in] pName Component name
     * @param[in] pConfig QnnRuntime configuration
     * @param [in] level Logger level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const QnnRuntime_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Get Input tensor information
     * @param[out] pList Pointer to tensor info list
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetInputInfo( QnnRuntime_TensorInfoList_t *pList );

    /**
     * @brief Get output tensor information
     * @param[out] pList Pointer to tensor info list
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetOutputInfo( QnnRuntime_TensorInfoList_t *pList );

    /**
     * @brief register callback
     * @param outputCb callback for QNN graphExecuteAsync done that output is ready.
     * @param errorCb callback for QNN graphExecuteAsync done but with error.
     * @param pAppPriv the private data to be used to identify the app instance.
     * @note This API is optional if QNN graphExecuteAsync API was not used.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterCallback( QnnRuntime_OutputCallback_t outputCb,
                                 QnnRuntime_ErrorCallback_t errorCb, void *pAppPriv );

    /**
     * @brief Rigister memory with specific shared buffers
     * @param[in] pSharedBuffers Pointer to shared buffers
     * @param[in] numBuffers The number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterBuffers( const QCSharedBuffer_t *pSharedBuffers, uint32_t numBuffers );

    /**
     * @brief Start the QnnRuntime object
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start() final;

    /**
     * @brief Enable qnn performance calculation
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e EnablePerf();

    /**
     * @brief Execute qnn model with input and output buffers
     * @param[in] pInputs Pointer to input shared buffers
     * @param[in] numInputs The number of input shared buffers
     * @param[out] pOutputs Pointer to output shared buffer
     * @param[in] numOutputs The number of output shared buffers
     * @param[in] pOutputPriv the private data associated with the output tensors
     *   If pOutputPriv is nullptr, the QNN graphExecute will be called that when
     * this API returned, the inference is done.
     *   If pOutputPriv is not nullptr, the QNN graphExecuteAsync will be called and
     * this API will return immediately, once when the inference is done, through the
     * callback outputCb or errorCb to notifiy the inference status.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( const QCSharedBuffer_t *pInputs, uint32_t numInputs,
                        const QCSharedBuffer_t *pOutputs, uint32_t numOutputs,
                        void *pOutputPriv = nullptr );

    /**
     * @brief Get qnn latest performance data
     * @param[out] pPerf Pointer to QnnRuntime perf structure
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetPerf( QnnRuntime_Perf_t *pPerf );

    /**
     * @brief Disable qnn performance calculation
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DisablePerf();

    /**
     * @brief Stop the QnnRuntime object
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop() final;

    /**
     * @brief DeRigister memory with specific shared buffers
     * @param[in] pSharedBuffers Pointer to shared buffers
     * @param[in] numBuffers The number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeRegisterBuffers( const QCSharedBuffer_t *pSharedBuffers, uint32_t numBuffers );

    /**
     * @brief Deinit the QnnRuntime object
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit() final;

private:
    typedef struct
    {
        Qnn_MemHandle_t memHandle;
        size_t size;
        int32_t fd;
    } DmaMemInfo_t;

    typedef struct
    {
        QnnRuntime *self;
        void *pOutputPriv;
    } NotifyParam_t;

    typedef struct NotifyParamQueue
    {
        NotifyParam_t notifyParam[QNNRUNTIME_NOTIFY_PARAM_NUM];
        uint16_t ring[QNNRUNTIME_NOTIFY_PARAM_NUM];
        uint16_t popIdx;
        uint16_t pushIdx;

    public:
        void Init();
        void Push( NotifyParam_t *pNotifyParam );
        NotifyParam_t *Pop();
    } NotifyParamQueue_t;

private:
    /**
     * @brief Create qnn model from .so file
     * @param[in] modelFile model path
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e CreateFromModelSo( std::string modelFile );

    /**
     * @brief Create qnn model from .bin file
     * @param[in] modelFile model path
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e CreateFromBinaryFile( std::string modelFile );

    /**
     * @brief Create qnn model from binary buffer
     * @param[in] pBuffer The pointer ro buffer
     * @param[in] bufferSize The size of buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e CreateFromBinaryBuffer( uint8_t *pBuffer, uint64_t bufferSize );

    /**
     * @brief Load customer op package
     * @param[in] pUdoPackages The pointer to udo packages information
     * @param[in] numOfUdoPackages The number of udo packages
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e LoadOpPackages( QnnRuntime_UdoPackage_t *pUdoPackages, int numOfUdoPackages );

    /**
     * @brief FastRPC remote buffer register
     * @param[in] pSharedBuffer Pointer to shared buffer
     * @param[out] fd the returned FastRPC memory descriptor
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RemoteRegisterBuf( const QCSharedBuffer_t *pSharedBuffer, int &fd );

    /**
     * @brief FastRPC remote buffer deregister
     * @param[in] pData the buffer virtual address
     * @param[in] size the size of the buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RemoteDeRegisterBuf( void *pData, size_t size );

    /**
     * @brief Register Buffer to HTP and get the QNN memory handle
     * @param[in] pSharedBuffer pointer shared buffer
     * @param[out] pMemHandle pointer to QNN memory handle
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterBufferToHTP( const QCSharedBuffer_t *pSharedBuffer,
                                    Qnn_MemHandle_t *pMemHandle );

    /**
     * @brief Get a QNN memory handle
     * @param[in] pSharedBuffer pointer shared buffer
     * @param[out] pMemHandle pointer to QNN memory handle
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetMemHandle( const QCSharedBuffer_t *pSharedBuffer, Qnn_MemHandle_t *pMemHandle );

    /**
     * @brief DeRegister all the buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeRegisterAllBuffers();

    /**
     * @brief get input tensor info internally
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetInputInfo();

    /**
     * @brief get output tensor info internally
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetOutputInfo();

    /**
     * @brief Extract qnn profiling event
     * @param[in] profileEventId profiling event id
     * @param[out] pPerf Pointer to QnnRuntime perf structure
     * @param[out] bPerfDataValid has valid perf data or not
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e ExtractProfilingEvent( QnnProfile_EventId_t profileEventId, QnnRuntime_Perf_t *pPerf,
                                      bool &bPerfDataValid );
#ifdef QNNRUNTIME_UNIT_TEST
public:
#endif

    QCStatus_e CheckInputTensors( const QCSharedBuffer_t *pInputs, uint32_t numInputs );

    QCStatus_e CheckOutputTensors( const QCSharedBuffer_t *pOutputs, uint32_t numOutputs );

    QnnLog_Level_t GetQnnLogLevel( Logger_Level_e level );

    QCStatus_e Destroy();

    static void QnnNotifyFn( void *pNotifyParam, Qnn_NotifyStatus_t notifyStatus );

    void QnnNotifyFn( NotifyParam_t *pNotifyParam, Qnn_NotifyStatus_t notifyStatus );

#ifdef QNNRUNTIME_UNIT_TEST
public:
#endif

    /**
     * @brief Stwich qnn defined data type to qcnode defined tensor type
     * @param[in] dataType qnn defined data type
     * @return QCTensorType_e qcnode defined tensor type
     */
    QCTensorType_e SwitchFromQnnDataType( Qnn_DataType_t dataType );

    /**
     * @brief Stwich qcnode defined tensor type to qnn defined data type
     * @param[in] tensorType qcnode defined tensor type
     * @return Qnn_DataType_t qnn defined data type
     */
    Qnn_DataType_t SwitchToQnnDataType( QCTensorType_e tensorType );

private:
    static constexpr size_t CONTEXT_CONFIG_SIZE = 1;

    QCProcessorType_e m_backendType;
    int m_backendCoreId = 0;
    Qnn_BackendHandle_t m_backendHandle = nullptr;
    Qnn_DeviceHandle_t m_deviceHandle = nullptr;
    void *m_modelHandle = nullptr;
    Qnn_ProfileHandle_t m_profileBackendHandle = nullptr;

    Qnn_LogHandle_t m_logHandle = nullptr;

    QnnFunctionPointers m_qnnFunctionPointers;

    const QnnBackend_Config_t **m_backendConfig = nullptr;
    QnnSystemContext_Handle_t m_systemContext = nullptr;
    Qnn_ContextHandle_t m_context = nullptr;
    QnnContext_Config_t *m_contextConfig[CONTEXT_CONFIG_SIZE + 1] = { nullptr };
    QnnContext_Config_t m_contextConfigArray[CONTEXT_CONFIG_SIZE];

    bool m_bLoadFromCachedBinary = false;

    qnn_wrapper_api::GraphInfo_t **m_graphsInfo = nullptr;
    uint32_t m_graphsCount = 0;

    const qnn_wrapper_api::GraphConfigInfo_t **m_graphConfigsInfo = nullptr;
    uint32_t m_graphConfigsInfoCount = 0;

    const QnnDevice_PlatformInfo_t *m_platformInfo = nullptr;

    QnnRuntime_OutputCallback_t m_outputCb = nullptr;
    QnnRuntime_ErrorCallback_t m_errorCb = nullptr;
    void *m_pAppPriv = nullptr;
    NotifyParamQueue_t m_notifyParamQ;

    std::mutex m_lock;
    std::map<void *, DmaMemInfo_t> m_dmaMemInfoMap;
    QnnRuntime_TensorInfo_t *m_pInputTensor = nullptr;
    size_t m_inputTensorNum = 0;
    QnnRuntime_TensorInfo_t *m_pOutputTensor = nullptr;
    size_t m_outputTensorNum = 0;

    std::vector<Qnn_Tensor_t> m_inputs;
    std::vector<Qnn_Tensor_t> m_outputs;

    /* used to maintain the reference counter for the buffer that used among
     * different QNN instances on the some hardware processor */
    static std::mutex s_lock[QC_PROCESSOR_MAX];
    static std::map<void *, int> s_dmaMemRefMap[QC_PROCESSOR_MAX];
};   // QnnRuntime

}   // namespace component
}   // namespace QC
#endif   // QC_QNN_RUNTIME_HPP