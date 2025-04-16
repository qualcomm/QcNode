*Menu*:
- [1. Introduction](#1-introduction)
- [2. QnnRuntime Data Structures](#2-qnnruntime-data-structures)
- [3. QnnRuntime APIs](#3-qnnruntime-apis)
- [4. QnnRuntime Examples](#4-qnnruntime-examples)
  - [4.1 Set up QnnRuntime configuration](#41-set-up-qnnruntime-configuration)
  - [4.2 Load qnn model from configuration](#42-load-qnn-model-from-configuration)
  - [4.3 Get qnn input/output tensor information](#43-get-qnn-inputoutput-tensor-information)
  - [4.4 Use QnnRuntime::Execute to run qnn inference in sync mode](#44-use-qnnruntimeexecute-to-run-qnn-inference-in-sync-mode)
  - [4.5 Use QnnRuntime::Execute to run qnn inference in async mode](#45-use-qnnruntimeexecute-to-run-qnn-inference-in-async-mode)
  - [4.6 Register/DeRegister Buffers](#46-registerderegister-buffers)
- [5. References](#5-references)


# 1. Introduction
QnnRuntime is an AI infenrence framework designed to assist users in running QNN models on HTP, GPU, and CPU backends in real-time. It offers a user-friendly approach to facilitate this process:
- load model either from shared library or serialized context binary.
- get detailed input/output tensor information from given models.
- provide qnn performance information during qnn model execution.
- support zero copy mechanism to reduce run-time lantency as much as possible.

# 2. QnnRuntime Data Structures

- [QnnRuntime_Perf_t](../include/QC/component/QnnRuntime.hpp#L36)
- [QnnRuntime_UdoPackage_t](../include/QC/component/QnnRuntime.hpp#L43)
- [QnnRuntime_LoadType_e](../include/QC/component/QnnRuntime.hpp#L51)
- [QnnRuntime_Config_t](../include/QC/component/QnnRuntime.hpp#L66)
- [QnnRuntime_TensorInfo_t](../include/QC/component/QnnRuntime.hpp#L75)
- [QnnRuntime_TensorInfoList_t](../include/QC/component/QnnRuntime.hpp#L82)
- [QnnRuntime_OutputCallback_t](../include/QC/component/QnnRuntime.hpp#L94)
- [QnnRuntime_ErrorCallback_t](../include/QC/component/QnnRuntime.hpp#L102)

# 3. QnnRuntime APIs

- [QnnRuntime::Init](../include/QC/component/QnnRuntime.hpp#L121) Initialize QnnRuntime component

- [QnnRuntime::GetInputInfo](../include/QC/component/QnnRuntime.hpp#L129) Get input tensor information

- [QnnRuntime::GetOutputInfo](../include/QC/component/QnnRuntime.hpp#L136) Get output tensor information

- [QnnRuntime::RegisterCallback](../include/QC/component/QnnRuntime.hpp#L146) Register callback to use QNN graphExecuteAsync API

- [QnnRuntime::RegisterBuffers](../include/QC/component/QnnRuntime.hpp#L155) Register memory with specific shared buffers

- [QnnRuntime::Start](../include/QC/component/QnnRuntime.hpp#L162) Start the QnnRuntime object

- [QnnRuntime::EnablePerf](../include/QC/component/QnnRuntime.hpp#L168) Enable qnn performance calculation

- [QnnRuntime::Execute](../include/QC/component/QnnRuntime.hpp#L184) Execute qnn model with input and output buffer

- [QnnRuntime::GetPerf](../include/QC/component/QnnRuntime.hpp#L193) Get qnn latest performance data

- [QnnRuntime:DisablePerf](../include/QC/component/QnnRuntime.hpp#L199) Disable qnn performance calculation

- [QnnRuntime::Stop](../include/QC/component/QnnRuntime.hpp#L205) Stop the QnnRuntime object

- [QnnRuntime::DeRegisterBuffers](../include/QC/component/QnnRuntime.hpp#L213) DeRigister memory with specific shared buffers

- [QnnRuntime::Deinit](../include/QC/component/QnnRuntime.hpp#L220) Deinit the QnnRuntime object

# 4. QnnRuntime Examples

## 4.1 Set up QnnRuntime configuration

QnnRuntime configuration help user to set up necessary information before loading qnn model. It mainly includes:
- Specify qnn model file path
- Select qnn model loading type
- Select qnn backend type
- Select qnn priority
- Specify qnn customer op packakes
- Specify qnn model context buffer if loading from binary buffer

Please refer below code block:
```c++
QnnRuntime_Config_t qnnConfig;

// Specify qnn model file path
qnnConfig.modelPath = "data/centernet/program.bin";
// Select qnn model loading type
qnnConfig.loadType = QNNRUNTIME_LOAD_CONTEXT_BIN_FROM_FILE;

// Select qnn backend type
qnnConfig.processorType = QC_PROCESSOR_HTP0;
```

## 4.2 Load qnn model from configuration

Once the configuration setup is ready, we can call [QnnRuntime::Init](../include/QC/component/QnnRuntime.hpp#L121), and this API call will automatically load the QNN model. Please refer to the code block below to see how this can be achieved:

```c++
qnnRuntime.Init( pName, &qnnConfig );
```

## 4.3 Get qnn input/output tensor information

Before creating the appropriate input/output buffers, we need to obtain input/output tensor information based on the model details. QnnRuntime provides the [QnnRuntime::GetInputInfo](../include/QC/component/QnnRuntime.hpp#L129) and [QnnRuntime::GetOutputInfo](../include/QC/component/QnnRuntime.hpp#L136) APIs to help users get the necessary information. Please refer to the code block below to see how this can be achieved:

```c++
// Get input tensor information
QnnRuntime_TensorInfoList_t tensorInputList;
qnnRuntime.GetInputInfo( &tensorInputList );
// Allocate sharedBuffers for input buffer
const uint32_t inputNum = tensorInputList.num;
QCSharedBuffer_t inputs[inputNum];
for ( int i = 0; i < inputNum; ++i )
{
    const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
}

// get output tensor information
QnnRuntime_TensorInfoList_t tensorOutputList;
qnnRuntime.GetOutputInfo( &tensorOutputList );

// Allocate sharedBuffers for output buffer
const uint32_t outputNum = tensorOutputList.num;
QCSharedBuffer_t outputs[outputNum];
for ( int i = 0; i < outputNum; ++i )
{
    const auto ret = outputs[i].Allocate( &tensorOutputList.pInfo[i].properties );
}
```


## 4.4 Use QnnRuntime::Execute to run qnn inference in sync mode

Once the user successfully loads the QNN model and creates input/output buffers, it's time to feed them into the QNN context and execute QNN inference cycles. To achieve this with [QnnRuntime::Execute](../include/QC/component/QnnRuntime.hpp#L184), please refer to the code block below:

```c++
// Execute qnn model inference
// variables of inputs, inputNum, outputs, outputNum refer to 4.3
qnnRuntime.Execute( inputs, inputNum, outputs, outputNum );
```

Note that in synchronous mode, there is no need to call the [QnnRuntime::RegisterCallback](../include/QC/component/QnnRuntime.hpp#L146) API to register a callback

## 4.5 Use QnnRuntime::Execute to run qnn inference in async mode

The QNN asynchronous mode is supported by calling the [QnnRuntime::Execute](../include/QC/component/QnnRuntime.hpp#L184) API with the `pOutputPriv` set to a non-null value. However, before using this API in async mode, you must register the related callbacks through the [QnnRuntime::RegisterCallback](../include/QC/component/QnnRuntime.hpp#L146) API.

```c++
static void SampleQnn_OutputCallback( void *pAppPriv, void *pOutputPriv )
{
    std::condition_variable *pCondVar = (std::condition_variable *) pAppPriv;
    uint64_t *pAsyncResult = (uint64_t *) pOutputPriv;
    *pAsyncResult = 0;
    pCondVar->notify_one();
}

static void SampleQnn_ErrorCallback( void *pAppPriv, void *pOutputPriv,
                                     Qnn_NotifyStatus_t notifyStatus )
{
    std::condition_variable *pCondVar = (std::condition_variable *) pAppPriv;
    uint64_t *pAsyncResult = (uint64_t *) pOutputPriv;
    *pAsyncResult = notifyStatus.error;
    pCondVar->notify_one();
}

    // During Init
    // after qnnRuntime.Init API call, do the related callback register.
    ret = qnnRuntime.RegisterCallback( SampleQnn_OutputCallback, SampleQnn_ErrorCallback, &m_condVar );

    // During Running
    std::mutex mtx;
    uint64_t asyncResult = 0xdeadbeef;
    ret = qnnRuntime.Execute( inputs, inputNum, outputs, outputNum, &asyncResult );
    if ( QC_STATUS_OK == ret )
    {
        std::unique_lock<std::mutex> lock( mtx );
        // wait async output or error callback invoked
        (void) m_condVar.wait_for( lock, std::chrono::milliseconds( 1000 ) );
        if ( 0 != asyncResult ) // check result OK
        {
            ret = QC_STATUS_FAIL;
        }
    }
```

## 4.6 Register/DeRegister Buffers

Additionally, QnnRuntime provides independent interfaces [QnnRuntime::RegisterBuffers](../include/QC/component/QnnRuntime.hpp#L155) and [QnnRuntime::DeRegisterBuffers](../include/QC/component/QnnRuntime.hpp#L213) to register and deregister buffers. These interfaces act as a bridge, mapping buffer addresses between the CPU and HTP. This allows QNN inference to run on registered buffers without the need to create new buffer space for input and output data.

```c++
// Get input tensor information
QnnRuntime_TensorInfoList_t tensorInputList;
qnnRuntime.GetInputInfo( &tensorInputList );
// Allocate sharedBuffers for input buffer
const uint32_t inputNum = tensorInputList.num;
QCSharedBuffer_t inputs[inputNum];
for ( int i = 0; i < inputNum; ++i )
{
    const auto ret = inputs[i].Allocate( &tensorInputList.pInfo[i].properties );
}

// Register input buffers
qnnRuntime.RegisterBuffers(inputs,inputNum );

// DeRgister input buffers
qnnRuntime.DeRegisterBuffers(inputs,inputNum );
```

# 5. References

- [SampleQnn](../tests/sample/source/SampleQnn.cpp#L107).
- [gtest QnnRuntime](../tests/unit_test/components/QnnRuntime/gtest_QnnRuntime.cpp#L49).
