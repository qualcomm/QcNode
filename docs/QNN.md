*Menu*:
- [1. Overview](#1-overview)
    - [Key Features](#key-features)
- [2. QNN Configuraion](#2-qnn-configuraion)
  - [2.1 QNN Static JSON Configuration](#21-qnn-static-json-configuration)
  - [2.2 QNN Dynamic JSON Configuration](#22-qnn-dynamic-json-configuration)
- [3. QNN APIs](#3-qnn-apis)
  - [3.1 QCNode QNN APIS](#31-qcnode-qnn-apis)
  - [3.2 QCNode Configuration Interfaces](#32-qcnode-configuration-interfaces)
- [4. Typical QNN API Usage Examples](#4-typical-qnn-api-usage-examples)
  - [4.1 Load from Context Binary File and Run in Synchronous Mode](#41-load-from-context-binary-file-and-run-in-synchronous-mode)
  - [4.2 Load from Context Binary File and Run in Asynchronous Mode](#42-load-from-context-binary-file-and-run-in-asynchronous-mode)
- [5. References](#5-references)


# 1. Overview

**QCNode QNN** provides user-friendly APIs that simplifies the usage of the QNN SDK, enabling users to efficiently run QNN models on HTP, GPU, and CPU backends.

### Key Features

- **Flexible Model Loading**
  Load QNN model from a serialized context binary, a decrypted context buffer, or a shared library.

- **Comprehensive Tensor Information**
  Retrieve detailed input and output tensor metadata from the loaded model.

- **Performance Insights**
  Access runtime performance metrics during QNN model execution to aid in profiling and optimization.

- **Zero-Copy Support**
  Leverage zero-copy mechanisms to minimize runtime latency and maximize throughput.

# 2. QNN Configuraion

## 2.1 QNN Static JSON Configuration

| Parameter  | Required  | Type        | Description            |
|------------|-----------|-------------|------------------------|
| `name`     | true      | string      | The Node unique name.  |
| `id`       | true      | uint32_t    | The Node unique ID.    |
| `logLevel` | false     | string      | The message log level. <br> Options: `VERBOSE`, `DEBUG`, `INFO`, `WARN`, `ERROR` <br> Default: `ERROR`   |
| `processorType` | false | string     | The processor type. <br> Options: `htp0`, `htp1`, `htp2`, `htp3`, `cpu`, `gpu` <br> Default: `htp0` |
| `coreIds`  | false     | uint32_t[]  | A list of core IDs. <br> Default: `[0]` |
| `loadType` | false     | string      | The load type. <br> Options: `binary`, `library`, `buffer` <br> Default: `binary` |
| `modelPath` | depends  | string      | The QNN model file path (required if `loadType` is `binary` or `library`) |
| `contextBufferId` | depends | uint32_t | Context buffer index in `QCNodeInit::buffers` (required if `loadType` is `buffer`) |
| `bufferIds` | false    | uint32_t[]  | List of buffer indices in `QCNodeInit::buffers`  |
| `priority`  | false    | string      | QNN model scheduling priority. <br> Options: `low`, `normal`, `normal_high`, `high` <br> Default: `normal`           |
| `udoPackages` | depends | object[]    | List of UDO packages. <br> Each object contains:<br> - `udoLibPath` (string)<br> - `interfaceProvider` (string) |
| `globalBufferIdMap` | false | object[] | Mapping of buffer names to buffer indices in `QCFrameDescriptorNodeIfs`. <br>Each object contains:<br> - `name` (string)<br> - `id` (uint32_t)   |
| `deRegisterAllBuffersWhenStop` | false | bool     | Flag to deregister all buffers when stopped      <br>Default: `false` |
| `perfProfile` | false | string     | Specifies perf profile to set. <br> Options: `low_balanced`, `balanced`, `default`, `high_performance`, `sustained_high_performance`, `burst`, `low_power_saver`, `power_saver`, `high_power_saver`, `extreme_power_saver` <br> Default: `default` |

- Example Configurations
  - Load QNN Model from a Serialized Context Binary
    ```json
    {
      "static": {
        "name": "QNN0",
        "id": 0,
        "loadType": "binary",
        "modelPath": "data/centernet/program.bin",
        "processorType": "htp0"
      }
    }
    ```
    Refer to [QNN SANITY_General](../tests/unit_test/Node/QNN/gtest_NodeQnn.cpp#L107) for more details..

  - Load QNN Model from a Serialized Context Buffer
    ```json
    {
      "static": {
        "name": "QNN0",
        "id": 0,
        "loadType": "buffer",
        "contextBufferId": 0,
        "processorType": "htp0"
      }
    }
    ```
    Refer to [QNN CreateModelFromBuffer](../tests/unit_test/Node/QNN/gtest_NodeQnn.cpp#L198) for more details. The `contextBufferId` was used together with the `QCNodeInit_t::buffers` to create a QNN model.

  - Load QNN Model with UDO package
    ```json
    {
    "static": {
      "id": 0,
      "loadType": "binary",
      "modelPath": "data/bevdet/program.bin",
      "name": "QNN0",
      "processorType": "htp0",
      "udoPackages": [
        {
          "interfaceProvider": "AutoAiswOpPackageInterfaceProvider",
          "udoLibPath": "libQnnAutoAiswOpPackage.so"
        }
      ]
    }
    ```
    Refer to [QNN LoadOpPackage](../tests/unit_test/Node/QNN/gtest_NodeQnn.cpp#L665) for more details.

## 2.2 QNN Dynamic JSON Configuration

| Parameter  | Required  | Type        | Description            |
|------------|-----------|-------------|------------------------|
| `enablePerf` | false   | bool        | enable performance profiling.  |

- Example Configurations
  - Enable Performance Profiling
  ```json
  {
    "dynamic": {
      "enablePerf": true
    }
  }
  ```
  Refer to [QNN Perf](../tests/unit_test/Node/QNN/gtest_NodeQnn.cpp#L345) for more details. 

# 3. QNN APIs

## 3.1 QCNode QNN APIS

- [Qnn::Initialize](../include/QC/Node/QNN.hpp#L227) Initialize QNN node

- [Qnn::GetConfigurationIfs](../include/QC/Node/QNN.hpp#L233) Get QNN configuration interfaces

- [Qnn::GetMonitoringIfs](../include/QC/Node/QNN.hpp#L239) Get QNN monitoring interfaces

- [Qnn::Start](../include/QC/Node/QNN.hpp#L245) Start the QNN node

- [Qnn::ProcessFrameDescriptor](../include/QC/Node/QNN.hpp#L274) Execute QNN model with input and output buffers

- [Qnn::Stop](../include/QC/Node/QNN.hpp#L280) Stop the QNN node

- [Qnn::DeInitialize](../include/QC/Node/QNN.hpp#L291) Deinit the QNN node

## 3.2 QCNode Configuration Interfaces

- [QnnConfig::GetOptions](../include/QC/Node/QNN.hpp#L132) Get Configuration Options
  - Use this API to query detailed input and output information from the loaded QNN model.
    - Below was a example output for QNN model `centernet`
      ```json
      {
        "model": {
          "inputs": [
            {
              "dims": [1, 800, 1152, 3],
              "name": "input",
              "quantOffset": -114,
              "quantScale": 0.01865844801068306,
              "quantType": "scale_offset",
              "type": "ufixed_point8"
            }
          ],
          "outputs": [
            {
              "dims": [1,200,288,80],
              "name": "_256",
              "quantOffset": -247,
              "quantScale": 0.0852336436510086,
              "quantType": "scale_offset",
              "type": "ufixed_point8"
            },
            {
              "dims": [1,200,288,2],
              "name": "_259",
              "quantOffset": 0,
              "quantScale": 0.5239613056182861,
              "quantType": "scale_offset",
              "type": "ufixed_point8"
            },
            {
              "dims": [1,200,288,2],
              "name": "_262",
              "quantOffset": -20,
              "quantScale": 0.004786043893545866,
              "quantType": "scale_offset",
              "type": "ufixed_point8"
            }
          ]
        }
      }
      ```

# 4. Typical QNN API Usage Examples

## 4.1 Load from Context Binary File and Run in Synchronous Mode

```c++
// include the QCNode QNN header files
#include "QC/Node/QNN.hpp"
using namespace QC::Node;

#include "QC/Infras/Memory/Utils/TensorAllocator.hpp"
using namespace QC::Memory;

class MyQnnApp {
private:
  Qnn m_qnn; // Create a QNN node.
  TensorAllocator m_ta( "TENSOR" ); // Create a Tensor allocator.
  TensorDescriptor_t m_inputDesc;
  TensorDescriptor_t m_output0Desc;
  TensorDescriptor_t m_output1Desc;
  TensorDescriptor_t m_output2Desc;
  QCSharedFrameDescriptorNode m_frameDesc( 5 );

  public:
  void Init() {
    // Initialize the QNN node.
    QCNodeInit_t config = {
      R"({
        "static": {
          "name": "QNN0",
          "id": 0,
          "loadType": "binary",
          "modelPath": "data/centernet/program.bin",
          "processorType": "htp0"
        }
      })"
    };

    status = m_qnn.Initialize(config);

    // Get the options that contain the detailed input and
    // output information of the QNN model.
    // This step was optional as generaly this was known by
    // default when the QNN model was compiled.
    QCNodeConfigIfs &cfgIfs = m_qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    // Allocate input buffers, for high efficiency pipeline,
    // should allocate a set of buffers.
    // Here this was demo code, just use 1 buffer.
    status = m_ta.Allocate( TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 800, 1152, 3 } ), m_inputDesc );
    status = m_frameDesc.SetBuffer( 0, m_inputDesc );

    // Allocate output buffers.
    status = m_ta.Allocate(
        TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 200, 288, 80 } ),
        m_output0Desc );
    status = m_frameDesc.SetBuffer( 1, m_output0Desc );
    status = m_ta.Allocate(
        TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 200, 288, 2 } ),
        m_output1Desc );
    status = m_frameDesc.SetBuffer( 2, m_output1Desc );
    status = m_ta.Allocate(
        TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 200, 288, 2 } ),
        m_output2Desc );
    status = m_frameDesc.SetBuffer( 3, m_output2Desc );

    // Start the QNN node.
    status = qnn.Start();
  }

  void Run() {
    // When input was ready in m_inputDesc buffer, call the
    // following function to process the frame descriptor.
    status = qnn.ProcessFrameDescriptor( m_frameDesc );
    // The output will be ready when here in
    // m_output0Desc/m_output1Desc/m_output2Desc buffer.
  }

  void Deinit() {
    // Stop the QNN node.
    status = qnn.Stop();

    // Deinitialize the QNN node.
    status = qnn.DeInitialize();
  }
};
```

## 4.2 Load from Context Binary File and Run in Asynchronous Mode

```c++
// include the QCNode QNN header files.
#include "QC/Node/QNN.hpp"
using namespace QC::Node;

#include "QC/Infras/Memory/Utils/TensorAllocator.hpp"
using namespace QC::Memory;

#include <functional>

class MyQnnApp {
private:
  Qnn m_qnn; // Create a QNN node.
  TensorAllocator m_ta( "TENSOR" ); // Create a Tensor allocator.
  TensorDescriptor_t m_inputDesc;
  TensorDescriptor_t m_output0Desc;
  TensorDescriptor_t m_output1Desc;
  TensorDescriptor_t m_output2Desc;
  QCSharedFrameDescriptorNode m_frameDesc( 5 );

  public:
  void Init() {
    // Initialize the QNN node
    QCNodeInit_t config = {
      R"({
        "static": {
          "name": "QNN0",
          "id": 0,
          "loadType": "binary",
          "modelPath": "data/centernet/program.bin",
          "processorType": "htp0"
        }
      })"
    };
    using std::placeholders::_1;
    config.callback = std::bind( &MyQnnApp::EventCallback, this, _1 );

    status = m_qnn.Initialize(config);

    // Get the options that contain the detailed input and
    // output information of the QNN model.
    // This step was optional as generaly this was known by
    // default when the QNN model was compiled.
    QCNodeConfigIfs &cfgIfs = m_qnn.GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    // Allocate input buffers, for high efficiency pipeline,
    // should allocate a set of buffers.
    // Here this was demo code, just use 1 buffer.
    status = m_ta.Allocate( TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 800, 1152, 3 } ), m_inputDesc );
    status = m_frameDesc.SetBuffer( 0, m_inputDesc );

    // Allocate output buffers.
    status = m_ta.Allocate(
        TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 200, 288, 80 } ),
        m_output0Desc );
    status = m_frameDesc.SetBuffer( 1, m_output0Desc );
    status = m_ta.Allocate(
        TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 200, 288, 2 } ),
        m_output1Desc );
    status = m_frameDesc.SetBuffer( 2, m_output1Desc );
    status = m_ta.Allocate(
        TensorProps_t( QC_TENSOR_TYPE_UFIXED_POINT_8, { 1, 200, 288, 2 } ),
        m_output2Desc );
    status = m_frameDesc.SetBuffer( 3, m_output2Desc );

    // Start the QNN node.
    status = qnn.Start();
  }

  void EventCallback( const QCNodeEventInfo_t &info ) {
    if ( QC_STATUS_OK == info.status )
    {
        // When here, the output is ready.
        // The info.frameDesc will be exactly the reference to m_frameDesc.
    }
    else
    {
        // When here, the QNN inference failed.
        // The info.frameDesc will hole a QNN error code.
        QCBufferDescriptorBase_t &bufDesc = info.frameDesc.GetBuffer( 0 );
        Qnn_NotifyStatus_t *pQnnStatus = (Qnn_NotifyStatus_t *) bufDesc.pBuf;
        printf( "QNN callback with error %" PRIu64, pQnnStatus->error );
    }
  }

  void Run() {
    // When input was ready in m_inputDesc buffer, call the
    // following function to process the frame descriptor.
    status = qnn.ProcessFrameDescriptor( m_frameDesc );
    // When here, the output is not ready, the output will
    // be ready when EventCallback was invokded with status OK.
  }

  void Deinit() {
    // Stop the QNN node.
    status = qnn.Stop();

    // Deinitialize the QNN node.
    status = qnn.DeInitialize();
  }
};
```


# 5. References

- [SampleQnn](../tests/sample/source/SampleQnn.cpp#L474).
- [gtest QNN](../tests/unit_test/Node/QNN/gtest_NodeQnn.cpp#L124).
