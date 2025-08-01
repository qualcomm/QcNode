*Menu*:
- [1. Node Radar Overview](#1-node-radar-overview)
- [2. Node Radar Data Structures](#2-node-radar-data-structures)
  - [2.1 The details of RadarConfig_t](#21-the-details-of-radarconfig_t)
  - [2.2 The details of RadarConfigIfs](#22-the-details-of-radarconfigifs)
  - [2.3 The details of RadarMonitoringIfs](#23-the-details-of-radarmonitoringifs)
- [3. Node Radar APIs](#3-node-radar-apis)
- [4. Node Configuration](#4-node-configuration)
  - [4.1 JSON Configuration Structure](#41-json-configuration-structure)
  - [4.2 Configuration Parameters](#42-configuration-parameters)
  - [4.3 Buffer Management](#43-buffer-management)
- [5. Typical Use Case](#5-typical-use-case)
  - [5.1 Node Initialization](#51-node-initialization)
  - [5.2 Frame Processing](#52-frame-processing)
  - [5.3 Complete Example](#53-complete-example)

# 1. Node Radar Overview

The QC Node Radar is a high-level wrapper around the QC Radar component that provides a standardized Node interface for radar data processing within the QC framework. It abstracts the underlying radar service communication and provides a unified interface for radar data processing pipelines.

**Key Differences from Component Radar:**
- **JSON-based Configuration**: Uses DataTree for structured configuration instead of direct struct initialization
- **Frame Descriptor Processing**: Processes data through QCFrameDescriptorNodeIfs instead of direct buffer passing
- **Global Buffer ID Mapping**: Maps logical buffer names to global buffer IDs for flexible buffer management
- **Standardized Node Interface**: Implements common Node patterns for configuration, monitoring, and lifecycle management
- **Event Notification**: Provides event callbacks for processing status and state changes

The Node Radar follows the QC Node architecture pattern, providing:
- **Standardized Configuration**: JSON-based configuration with validation and error reporting
- **Buffer Management**: Automatic buffer descriptor handling and frame descriptor management
- **Service Integration**: Seamless integration with radar processing services through configurable endpoints
- **Performance Monitoring**: Built-in performance tracking and logging capabilities
- **State Management**: Proper lifecycle management with state tracking and error recovery

The Node Radar interfaces with external radar processing services through configurable service endpoints, making it suitable for various radar hardware and software implementations.

# 2. Node Radar Data Structures

## 2.1 The details of RadarConfig_t

RadarConfig_t is the main configuration structure for the Node Radar, extending QCNodeConfigBase_t:

- [RadarConfig_t](../include/QC/Node/Radar.hpp#L25)

```c++
typedef struct RadarConfig : public QCNodeConfigBase_t
{
    Radar_Config_t params;                              // Component-level radar configuration
    std::vector<uint32_t> bufferIds;                   // Buffer indices for initialization-time registration
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap; // Global buffer ID mapping for frame descriptors
    bool bDeRegisterAllBuffersWhenStop;                // Flag to deregister all buffers on stop
} RadarConfig_t;
```

**Key Parameters:**
- `params`: Contains the underlying component configuration including service name, timeout, buffer sizes, and performance settings
- `bufferIds`: Optional list of buffer indices from QCNodeInit::buffers to register during initialization
- `globalBufferIdMap`: Maps logical buffer names to global buffer IDs in frame descriptors (input at ID 0, output at ID 1 by default)
- `bDeRegisterAllBuffersWhenStop`: Controls automatic buffer deregistration behavior during stop operations

## 2.2 The details of RadarConfigIfs

RadarConfigIfs provides the configuration interface for Node Radar, handling JSON configuration parsing and validation:

- [RadarConfigIfs](../include/QC/Node/Radar.hpp#L35)

**Key Methods:**
- `VerifyAndSet()`: Parses and validates JSON configuration string with detailed error reporting
- `GetOptions()`: Returns available configuration options (currently empty for Radar)
- `Get()`: Returns the parsed configuration structure

**Configuration Validation:**
The configuration interface validates all required parameters including:
- Node name and ID presence and validity
- Buffer size constraints (must be > 0)
- Service name availability
- Timeout value validity (must be > 0)
- Global buffer ID mapping consistency
- Buffer ID references within valid ranges

## 2.3 The details of RadarMonitoringIfs

RadarMonitoringIfs provides monitoring capabilities for Node Radar:

- [RadarMonitoringIfs](../include/QC/Node/Radar.hpp#L75)

Currently provides basic monitoring interface structure. The monitoring interface returns UINT32_MAX for size queries, indicating unlimited monitoring data capacity. Advanced monitoring features can be extended through this interface.

# 3. Node Radar APIs

- [Initialize](../include/QC/Node/Radar.hpp#L108): Initialize the Node Radar with JSON configuration and optional buffers
- [GetConfigurationIfs](../include/QC/Node/Radar.hpp#L115): Get the configuration interface for JSON parsing and validation
- [GetMonitoringIfs](../include/QC/Node/Radar.hpp#L122): Get the monitoring interface for performance tracking
- [Start](../include/QC/Node/Radar.hpp#L129): Start the Node Radar processing pipeline
- [ProcessFrameDescriptor](../include/QC/Node/Radar.hpp#L140): Process frame descriptor with mapped input/output buffers
- [Stop](../include/QC/Node/Radar.hpp#L151): Stop the Node Radar processing pipeline
- [DeInitialize](../include/QC/Node/Radar.hpp#L158): Deinitialize and cleanup all resources
- [GetState](../include/QC/Node/Radar.hpp#L165): Get current state of the Node Radar (delegates to component state)

# 4. Node Configuration

## 4.1 JSON Configuration Structure

The Node Radar uses JSON configuration following the standard Node configuration pattern:

```json
{
  "static": {
    "name": "Radar Node unique name",
    "id": 0,
    "maxInputBufferSize": 2097152,
    "maxOutputBufferSize": 8388608,
    "serviceName": "/dev/radar0",
    "timeoutMs": 5000,
    "bEnablePerformanceLog": false,
    "bufferIds": [],
    "globalBufferIdMap": [
      {
        "name": "input",
        "id": 0
      },
      {
        "name": "output", 
        "id": 1
      }
    ],
    "deRegisterAllBuffersWhenStop": false
  }
}
```

## 4.2 Configuration Parameters

**Required Parameters:**
- `name`: Unique identifier for the Node instance (string, non-empty)
- `id`: Numeric ID for the Node instance (uint32_t, valid value)
- `maxInputBufferSize`: Maximum size for input buffers in bytes (uint32_t, > 0)
- `maxOutputBufferSize`: Maximum size for output buffers in bytes (uint32_t, > 0)
- `serviceName`: Path to the radar processing service (string, non-empty, e.g., "/dev/radar0")
- `timeoutMs`: Processing timeout in milliseconds (uint32_t, > 0)

**Optional Parameters:**
- `bEnablePerformanceLog`: Enable performance logging (bool, default: false)
- `bufferIds`: List of buffer indices for initialization-time registration (array of uint32_t, default: empty)
- `globalBufferIdMap`: Buffer mapping for frame descriptors (array, default: input=0, output=1)
- `deRegisterAllBuffersWhenStop`: Auto-deregister buffers on stop (bool, default: false)

**Configuration Validation:**
- All required parameters are validated for presence and valid ranges
- Service name is checked for non-empty string
- Buffer sizes must be positive values
- Timeout must be positive value
- Global buffer ID map entries must have valid names and IDs
- Buffer ID references must be within valid ranges

## 4.3 Buffer Management

The Node Radar supports two buffer management approaches:

**1. Frame Descriptor Buffers (Recommended):**
- Buffers provided through ProcessFrameDescriptor calls
- Dynamic buffer allocation per frame
- Automatic buffer lifecycle management
- Flexible buffer mapping through global buffer ID map

**2. Initialization-time Registration:**
- Buffers registered during Initialize() call using bufferIds configuration
- Static buffer pool approach
- Requires pre-allocated buffers in QCNodeInit::buffers
- Suitable for fixed buffer pool scenarios

**Global Buffer ID Mapping:**
- Maps logical buffer names to global buffer IDs in frame descriptors
- Default mapping: input buffer at global ID 0, output buffer at global ID 1
- Customizable through globalBufferIdMap configuration
- Enables flexible buffer routing and multi-stream processing

# 5. Typical Use Case

## 5.1 Node Initialization

```c++
#include "QC/Node/Radar.hpp"
#include "QC/Common/DataTree.hpp"

QC::Node::Radar radarNode;

// Setup configuration using DataTree
DataTree dt;
dt.Set<std::string>("static.name", "RadarProcessor");
dt.Set<uint32_t>("static.id", 0);
dt.Set<uint32_t>("static.maxInputBufferSize", 2097152);  // 2MB
dt.Set<uint32_t>("static.maxOutputBufferSize", 8388608); // 8MB
dt.Set<std::string>("static.serviceName", "/dev/radar0");
dt.Set<uint32_t>("static.timeoutMs", 5000);
dt.Set<bool>("static.bEnablePerformanceLog", true);

// Set empty buffer IDs for frame descriptor approach
std::vector<uint32_t> bufferIds;
dt.Set("static.bufferIds", bufferIds);

// Configure global buffer mapping
std::vector<DataTree> bufferMapDts;
DataTree inputMapDt;
inputMapDt.Set<std::string>("name", "input");
inputMapDt.Set<uint32_t>("id", 0);
bufferMapDts.push_back(inputMapDt);

DataTree outputMapDt;
outputMapDt.Set<std::string>("name", "output");
outputMapDt.Set<uint32_t>("id", 1);
bufferMapDts.push_back(outputMapDt);

dt.Set("static.globalBufferIdMap", bufferMapDts);
dt.Set<bool>("static.deRegisterAllBuffersWhenStop", false);

// Initialize Node
QCNodeInit_t config = { dt.Dump() };
QCStatus_e ret = radarNode.Initialize(config);
if (QC_STATUS_OK != ret) {
    // Handle initialization error
    std::cerr << "Radar Node initialization failed: " << ret << std::endl;
}
```

## 5.2 Frame Processing

```c++
// Start the Node
ret = radarNode.Start();
if (QC_STATUS_OK != ret) {
    std::cerr << "Radar Node start failed: " << ret << std::endl;
    return;
}

// Allocate input and output buffers
QCSharedBufferDescriptor_t inputBuffer;
ret = inputBuffer.buffer.Allocate(2097152); // 2MB input buffer
if (QC_STATUS_OK != ret) {
    std::cerr << "Input buffer allocation failed: " << ret << std::endl;
    return;
}

QCSharedBufferDescriptor_t outputBuffer;
ret = outputBuffer.buffer.Allocate(8388608); // 8MB output buffer
if (QC_STATUS_OK != ret) {
    std::cerr << "Output buffer allocation failed: " << ret << std::endl;
    return;
}

// Setup frame descriptor
QCSharedFrameDescriptorNode frameDesc(2);

// Set input buffer at global ID 0 (as configured in globalBufferIdMap)
inputBuffer.pBuf = inputBuffer.buffer.data();
inputBuffer.size = inputBuffer.buffer.size;
inputBuffer.name = "InputBuffer";
inputBuffer.type = QC_BUF_TENSOR;
ret = frameDesc.SetBuffer(0, inputBuffer);
if (QC_STATUS_OK != ret) {
    std::cerr << "Failed to set input buffer: " << ret << std::endl;
    return;
}

// Set output buffer at global ID 1 (as configured in globalBufferIdMap)
outputBuffer.pBuf = outputBuffer.buffer.data();
outputBuffer.size = outputBuffer.buffer.size;
outputBuffer.name = "OutputBuffer";
outputBuffer.type = QC_BUF_TENSOR;
ret = frameDesc.SetBuffer(1, outputBuffer);
if (QC_STATUS_OK != ret) {
    std::cerr << "Failed to set output buffer: " << ret << std::endl;
    return;
}

// Process frame
ret = radarNode.ProcessFrameDescriptor(frameDesc);
if (QC_STATUS_OK == ret) {
    std::cout << "Frame processed successfully" << std::endl;
} else {
    std::cerr << "Frame processing failed: " << ret << std::endl;
}

// Cleanup
ret = radarNode.Stop();
ret = radarNode.DeInitialize();
inputBuffer.buffer.Free();
outputBuffer.buffer.Free();
```

## 5.3 Complete Example

```c++
#include "QC/Node/Radar.hpp"
#include "QC/Common/DataTree.hpp"
#include <iostream>

void RadarProcessingExample()
{
    QCStatus_e ret = QC_STATUS_OK;
    QC::Node::Radar radarNode;

    // Step 1: Configure Node with JSON-based configuration
    DataTree dt;
    dt.Set<std::string>("static.name", "RadarExample");
    dt.Set<uint32_t>("static.id", 0);
    dt.Set<uint32_t>("static.maxInputBufferSize", 2097152);
    dt.Set<uint32_t>("static.maxOutputBufferSize", 8388608);
    dt.Set<std::string>("static.serviceName", "/dev/radar0");
    dt.Set<uint32_t>("static.timeoutMs", 5000);
    dt.Set<bool>("static.bEnablePerformanceLog", true);

    // Configure buffer mapping for frame descriptor processing
    std::vector<uint32_t> bufferIds;
    dt.Set("static.bufferIds", bufferIds);
    
    std::vector<DataTree> bufferMapDts;
    DataTree inputMapDt, outputMapDt;
    inputMapDt.Set<std::string>("name", "input");
    inputMapDt.Set<uint32_t>("id", 0);
    outputMapDt.Set<std::string>("name", "output");
    outputMapDt.Set<uint32_t>("id", 1);
    bufferMapDts.push_back(inputMapDt);
    bufferMapDts.push_back(outputMapDt);
    dt.Set("static.globalBufferIdMap", bufferMapDts);
    dt.Set<bool>("static.deRegisterAllBuffersWhenStop", false);

    // Step 2: Initialize Node
    QCNodeInit_t config = { dt.Dump() };
    ret = radarNode.Initialize(config);
    if (QC_STATUS_OK != ret) {
        std::cerr << "Node initialization failed: " << ret << std::endl;
        return;
    }

    // Step 3: Start processing
    ret = radarNode.Start();
    if (QC_STATUS_OK != ret) {
        std::cerr << "Node start failed: " << ret << std::endl;
        radarNode.DeInitialize();
        return;
    }

    // Step 4: Process multiple frames
    for (int i = 0; i < 10; i++) {
        // Allocate buffers for each frame
        QCSharedBufferDescriptor_t inputBuffer, outputBuffer;
        ret = inputBuffer.buffer.Allocate(2097152);
        if (QC_STATUS_OK != ret) {
            std::cerr << "Input buffer allocation failed: " << ret << std::endl;
            break;
        }
        
        ret = outputBuffer.buffer.Allocate(8388608);
        if (QC_STATUS_OK != ret) {
            std::cerr << "Output buffer allocation failed: " << ret << std::endl;
            inputBuffer.buffer.Free();
            break;
        }

        // Setup frame descriptor with proper buffer descriptors
        QCSharedFrameDescriptorNode frameDesc(2);
        
        // Configure input buffer descriptor
        inputBuffer.pBuf = inputBuffer.buffer.data();
        inputBuffer.size = inputBuffer.buffer.size;
        inputBuffer.name = "InputBuffer";
        inputBuffer.type = QC_BUF_TENSOR;
        
        // Configure output buffer descriptor
        outputBuffer.pBuf = outputBuffer.buffer.data();
        outputBuffer.size = outputBuffer.buffer.size;
        outputBuffer.name = "OutputBuffer";
        outputBuffer.type = QC_BUF_TENSOR;

        // Set buffers in frame descriptor using global buffer IDs
        ret = frameDesc.SetBuffer(0, inputBuffer);
        if (QC_STATUS_OK == ret) {
            ret = frameDesc.SetBuffer(1, outputBuffer);
        }

        if (QC_STATUS_OK == ret) {
            // Process frame through Node interface
            ret = radarNode.ProcessFrameDescriptor(frameDesc);
            if (QC_STATUS_OK == ret) {
                std::cout << "Frame " << i << " processed successfully" << std::endl;
            } else {
                std::cerr << "Frame " << i << " processing failed: " << ret << std::endl;
            }
        }

        // Cleanup frame buffers
        inputBuffer.buffer.Free();
        outputBuffer.buffer.Free();
    }

    // Step 5: Stop and cleanup
    ret = radarNode.Stop();
    if (QC_STATUS_OK != ret) {
        std::cerr << "Node stop failed: " << ret << std::endl;
    }
    
    ret = radarNode.DeInitialize();
    if (QC_STATUS_OK != ret) {
        std::cerr << "Node deinitialization failed: " << ret << std::endl;
    }
    
    std::cout << "Radar processing example completed" << std::endl;
}
```

**Key Features Demonstrated:**
- **JSON-based Configuration**: Using DataTree for structured configuration
- **Frame Descriptor Processing**: Buffer management through frame descriptors
- **Global Buffer ID Mapping**: Flexible buffer routing and identification
- **Error Handling**: Comprehensive error checking and recovery
- **Resource Management**: Proper buffer allocation, usage, and cleanup
- **Performance Monitoring**: Optional performance logging integration

**Service Dependencies:**
- Requires radar processing service at configured serviceName (e.g., "/dev/radar0")
- Service availability affects initialization and processing results
- Timeout configuration prevents indefinite blocking on service calls
- Service errors are propagated through return codes and event notifications

**Buffer Management Notes:**
- Frame descriptor approach provides maximum flexibility
- Global buffer ID mapping enables complex routing scenarios
- Buffer descriptors must be properly initialized with base class fields
- Memory management is caller's responsibility for frame descriptor buffers

Reference:
- [gtest_NodeRadar](../tests/unit_test/Node/Radar/gtest_NodeRadar.cpp)
- [SampleRadar](../tests/sample/source/SampleRadar.cpp)
- [Node Radar Header](../include/QC/Node/Radar.hpp)
- [Component Radar Documentation](./Radar.md)
