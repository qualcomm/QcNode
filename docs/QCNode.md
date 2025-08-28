*Menu*:
- [🚗 **QCNode - Qualcomm Compute Node**](#-qcnode---qualcomm-compute-node)
  - [Overview](#overview)
  - [🧩 Architecture](#-architecture)
  - [🔧 API Summary](#-api-summary)
    - [Lifecycle Management](#lifecycle-management)
      - [`Initialize(QCNodeInit_t &config)`](#initializeqcnodeinit_t-config)
      - [`Start()`](#start)
      - [`Stop()`](#stop)
      - [`DeInitialize()`](#deinitialize)
    - [Frame Processing](#frame-processing)
      - [`ProcessFrameDescriptor(QCFrameDescriptorNodeIfs &frameDesc)`](#processframedescriptorqcframedescriptornodeifs-framedesc)
    - [State \& Interface Access](#state--interface-access)
      - [`GetState()`](#getstate)
      - [`GetConfigurationIfs()`](#getconfigurationifs)
      - [`GetMonitoringIfs()`](#getmonitoringifs)

---

# 🚗 **QCNode - Qualcomm Compute Node**

## Overview

**QCNode** (Qualcomm Compute Node) is part of Qualcomm's ADAS hardware abstraction layer. It provides a **service-oriented**, **user-friendly API** to access Qualcomm's ADAS hardware accelerators, enabling efficient execution of perception and computer vision tasks.

QCNode is designed to simplify integration and development by offering modular components and utilities tailored for ADAS workloads.

---

## 🧩 Architecture

All QCNode implementations must inherit from the abstract base class `QCNodeIfs`. This interface defines a **unified API** for:

- Lifecycle management
- Frame descriptor processing
- Configuration and monitoring access

This ensures consistency and interoperability across all QCNode components.

---

## 🔧 API Summary

### Lifecycle Management

#### `Initialize(QCNodeInit_t &config)`
Initializes the node with the provided configuration.

- **Parameters**: `config` – Initialization parameters
- **Returns**: `QCStatus_e` – Status of the operation
- **Usage**: Must be called before `Start()`

---

#### `Start()`
Activates the node, making it ready to process data.

- **Returns**: `QCStatus_e`
- **Usage**: Typically follows a successful `Initialize()`

---

#### `Stop()`
Gracefully halts node operations.

- **Returns**: `QCStatus_e`
- **Usage**: Should be followed by `DeInitialize()` if shutting down

---

#### `DeInitialize()`
Cleans up resources and prepares the node for shutdown.

- **Returns**: `QCStatus_e`
- **Usage**: Called after `Stop()`

---

### Frame Processing

#### `ProcessFrameDescriptor(QCFrameDescriptorNodeIfs &frameDesc)`
Handles a frame descriptor either by queuing it for asynchronous processing or by processing it immediately, depending on the node's internal design and capabilities.

- **Parameters**: `frameDesc` – Frame descriptor to be processed, for more details of `frameDesc`, refer [Node Frame Descriptor](./buffer.md#qcnode-node-frame-descriptor).
- **Returns**: `QCStatus_e` – Processing status
- **Usage**: Used during active node operation

---

### State & Interface Access

#### `GetState()`
Retrieves the current state of the node.

- **Returns**: `QCObjectState_e` – e.g., Initialized, Running, Stopped
- **Usage**: Useful for monitoring and control logic

---

#### `GetConfigurationIfs()`
Provides access to the node’s configuration interface.

- **Returns**: `QCNodeConfigIfs&`
- **Usage**: Allows querying or updating configuration parameters

---

#### `GetMonitoringIfs()`
Provides access to the node’s monitoring interface.

- **Returns**: `QCNodeMonitoringIfs&`
- **Usage**: Enables retrieval of runtime metrics and status indicators

---