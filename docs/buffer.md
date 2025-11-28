*Menu*:
- [1. QCNode Buffer Related Types](#1-qcnode-buffer-related-types)
  - [QCNode Buffer properties](#qcnode-buffer-properties)
    - [The details of image properties.](#the-details-of-image-properties)
  - [QCNode Buffer Descriptors](#qcnode-buffer-descriptors)
    - [The details of QCNode Buffer Descriptors.](#the-details-of-qcnode-buffer-descriptors)
    - [`BufferDescriptor_t` Overview](#bufferdescriptor_t-overview)
    - [Key Concepts](#key-concepts)
    - [Typical Usage](#typical-usage)
    - [Special Use Case: BEV AI Model](#special-use-case-bev-ai-model)
    - [Memory Allocation Recommendation](#memory-allocation-recommendation)
  - [QCNode Node Frame Descriptor](#qcnode-node-frame-descriptor)
    - [Global Buffer Mapping in NodeFrameDescriptor](#global-buffer-mapping-in-nodeframedescriptor)
    - [Example: Buffer Usage in a Simple QCNode Pipeline](#example-buffer-usage-in-a-simple-qcnode-pipeline)
      - [Camera Node](#camera-node)
      - [CL2DFlex Node](#cl2dflex-node)
      - [QNN Node (Centernet)](#qnn-node-centernet)
      - [VideoEncoder Node](#videoencoder-node)
    - [⚠️ Application Responsibility \& Limitations in Sample Code](#️-application-responsibility--limitations-in-sample-code)
- [2. QCNode buffer related APIs](#2-qcnode-buffer-related-apis)
- [3. QCNode Buffer Descriptor Examples](#3-qcnode-buffer-descriptor-examples)
  - [3.1 A ImageDescriptor\_t image for BEV kind of AI model](#31-a-imagedescriptor_t-image-for-bev-kind-of-ai-model)
  - [3.2 Allocate buffers to hold images](#32-allocate-buffers-to-hold-images)
  - [3.3 Allocate Tensor](#33-allocate-tensor)
  - [3.4 Convert Image to Tensor](#34-convert-image-to-tensor)
    - [3.4.1 Convert the RGB Image to the Tensor](#341-convert-the-rgb-image-to-the-tensor)
    - [3.4.2 Convert the NV12/P010 Image to the Luma and Chroma Tensor](#342-convert-the-nv12p010-image-to-the-luma-and-chroma-tensor)

# 1. QCNode Buffer Related Types

## QCNode Buffer properties

These properties define the memory allocation strategy required to fulfill the specific needs of each QCNode. They guide the buffer manager in selecting the appropriate allocation method and configuring the buffer layout accordingly.

  - [BufferProps_t](../include/QC/Infras/Memory/BufferDescriptor.hpp#L25)
  - [ImageBasicProps_t](../include/QC/Infras/Memory/ImageDescriptor.hpp#L28)
  - [ImageProps_t](../include/QC/Infras/Memory/ImageDescriptor.hpp#L113)
  - [TensorProps_t](../include/QC/Infras/Memory/TensorDescriptor.hpp#L28)

### The details of image properties.

Due to hardware constraints, the actual buffer used to store an image may have alignment padding along its width and height. This padding is primarily required for zero-copy operations, enabling the buffer to be shared with the hardware accelerator. However, for an image with certain width and height, it can has no padding at all.

And the below picture shows a case what's the actual buffer looks like for an image format such as NV12 that has 2 planes, the black area is padding space thus not valid pixels.

![Image format with 2 plane](./images/image-prop-2-plane.jpg)

For each plane, it may have paddings along width and height, it may also has paddings between the 2 planes. And some extra paddings is also needed at the end of the each plane.

And the below picture shows a case what's the actual buffer looks like for an image format such as RGB that has 1 plane.

![Image format with 1 plane](./images/image-prop-1-plane.jpg)

Thus now, it's easy to understand those members of the type [ImageProps_t](../include/QC/Infras/Memory/ImageDescriptor.hpp#L113) except batchSize.

For the batchSize, it was generally designed for the BEV kind of AI models, check below section [3.1](#31-a-imagedescriptor_t-image-for-bev-kind-of-ai-model).

For or the compressed image with the format H264 or H265, and the code [SANITY_CompressedImageAllocateByProps](../tests/unit_test/Infras/Memory/gtest_Memory.cpp#L340) which gives an example that how to allocate a buffer for a compressed image and this is the only way. And please note that for the compressed image, the member stride/actualHeight will be invalid and should not be used.

## QCNode Buffer Descriptors

The following descriptor types define the structure and metadata of buffers used by QCNode. Each descriptor corresponds to a specific buffer format and plays a critical role in managing memory and data layout.

  - [BufferDescriptor_t](../include/QC/Infras/Memory/BufferDescriptor.hpp#L72)
  - [ImageDescriptor_t](../include/QC/Infras/Memory/ImageDescriptor.hpp#L176)
  - [TensorDescriptor_t](../include/QC/Infras/Memory/TensorDescriptor.hpp#L77)

### The details of QCNode Buffer Descriptors.

```mermaid
classDiagram
    class QCBufferDescriptorBase_t {
        +name
        +pBuf
        +size
        +dmaHandle
        +pid
        +type
        +allocatorType
        +cache
        +alignment
    }

    class BufferDescriptor_t {
        +validSize
        +offset
    }

    class ImageDescriptor_t {
        +format
        +width
        +height
        +stride[QC_NUM_IMAGE_PLANES]
        +actualHeight[QC_NUM_IMAGE_PLANES]
        +planeBufSize[QC_NUM_IMAGE_PLANES]
        +numPlanes
    }

    class TensorDescriptor_t {
        +tensorType
        +dims[QC_NUM_TENSOR_DIMS]
        +numDims
    }

    QCBufferDescriptorBase_t <|-- BufferDescriptor_t
    BufferDescriptor_t <|-- ImageDescriptor_t
    BufferDescriptor_t <|-- TensorDescriptor_t
```

### `BufferDescriptor_t` Overview

The `BufferDescriptor_t` is a data structure used to represent a portion of DMA memory that can be shared between `QCNode` instances for **zero-copy** purposes.

### Key Concepts

- The base class [QCBufferDescriptorBase_t](../include/QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp#L115) represents a **single continuous** DMA memory block from the user's perspective.  
  > ⚠️ Physically, the memory may not be continuous.

- The derived structure `BufferDescriptor_t` includes:
  - [`offset`](../include/QC/Infras/Memory/BufferDescriptor.hpp#L83): Indicates the starting point within the DMA memory.
  - [`validSize`](../include/QC/Infras/Memory/BufferDescriptor.hpp#L82): Specifies the size of the valid memory region.

These members define the **actual memory location and size** within the DMA block.

### Typical Usage

In most cases, `BufferDescriptor_t` represents the **entire DMA memory**, where:
- `offset = 0`
- `validSize = size`

### Special Use Case: BEV AI Model

A notable exception occurs in the BEV (Bird’s Eye View) type AI model. In this scenario, the `ShareBufferMiddle` represents **only the middle portion** of the DMA memory.

For details, refer to section [3.1](#31-a-imagedescriptor_t-image-for-bev-kind-of-ai-model).

### Memory Allocation Recommendation

It is **strongly recommended** to use the APIs provided by the sample [`BufferManager`](../tests/sample/include/QC/sample/BufferManager.hpp#L56) for memory allocation and deallocation.

However, it is also acceptable to use platform-specific DMA-related APIs, such as:
- **PMEM** on QNX
- **dma-buf** on Linux

> ⚠️ In these cases, the user application is responsible for correctly assigning values to each member of the `BufferDescriptor_t`.

And in fact, the BufferManager APIs are based on the platform DMA related APIs (PMEM for QNX, dma-buf for Linux). 
  - For QNX, check [PMEMAllocator](../source/Infras/Memory/PMEMAllocator.cpp).
  - For Linux, check [DMABUFFAllocator](../source/Infras/Memory/DMABUFFAllocator.cpp).

```c
// For case that using PMEM or dma-buf to allocate memory,
// now have the virtual address pBuf and the uint64 dmaHandle.
// for QNX, the dmaHandle is cast from pmem_handle_t.
// for Linux, the dmaHandle is cast from int.

ImageDescriptor_t imgDesc;

imgDesc.pBuf = pBuf;
imgDesc.dmaHandle = dmaHandle;
imgDesc.size = size;
imgDesc.pid = static_cast<uint64_t>( getpid() );
imgDesc.allocatorType = QC_MEMORY_ALLOCATOR_DMA;
imgDesc.cache = QC_CACHEABLE;
imgDesc.alignment = 4096;
imgDesc.validSize = size;
imgDesc.offset = 0;
imgDesc.type = QC_BUFFER_TYPE_IMAGE;
imgDesc.imgProps.format = format;
imgDesc.imgProps.batchSize = batchSize;
imgDesc.imgProps.width = width;
imgDesc.imgProps.height = height;
imgDesc.imgProps.numPlanes = numPlanes;
imgDesc.imgProps.stride[0] = stride0;
imgDesc.imgProps.actualHeight[0] = actualHeight0;
imgDesc.imgProps.planeBufSize[0] = stride0*actualHeight0;
...
imgDesc.imgProps.stride[numPlanes-1] = strideX;
imgDesc.imgProps.actualHeight[numPlanes-1] = actualHeightX;
imgDesc.imgProps.planeBufSize[numPlanes-1] = strideX*actualHeight;

// and then this can be feed into a QCNode
```

And another thing, the Buffer Descriptor can be shared between QCNode, but it has no life cycle management ability. Here, the QCNode Sample Application has a demo that using C++ std::shared_ptr to demonstrate that how to do the buffer life cycle management between the nodes that running in the same process but in different threads, refer [The QCNode Sample Buffer Life Cycle Management](./sample-buffer-life-cycle-management.md).

## QCNode Node Frame Descriptor

NodeFrameDescriptor is a concrete implementation of QCFrameDescriptorNodeIfs used by QCNode. It encapsulates a collection of buffer descriptors that represent DMA-accessible memory regions for raw data, images, or tensors.

The role of each buffer descriptor in `NodeFrameDescriptor`—whether it serves as an input, output, or parameter—is determined by the specific QCNode implementation based on its buffer index, referred to as `globalBufferId`.

  - [NodeFrameDescriptor](../include/QC/Node/NodeFrameDescriptor.hpp#L45)
    - [GetBuffer](../include/QC/Node/NodeFrameDescriptor.hpp#L94)
    - [SetBuffer](../include/QC/Node/NodeFrameDescriptor.hpp#L110)
    - [Clear](../include/QC/Node/NodeFrameDescriptor.hpp#L128)

### Global Buffer Mapping in NodeFrameDescriptor

The user application can implement its own version of `NodeFrameDescriptor` tailored to its specific needs. The QCNode framework is designed to support a model where a **single `NodeFrameDescriptor` instance** is shared across multiple nodes in a processing pipeline. In this design, each node must know which buffer indices—referred to as `globalBufferId`s—it should interact with. This mapping of buffer roles (e.g., input, output, parameter) is defined in a **global buffer map**, which should be provided to each node during the initialization phase via a **JSON configuration string**.

### Example: Buffer Usage in a Simple QCNode Pipeline

Consider a simple pipeline:

```mermaid
graph LR
    A[Camera] --> B[CL2DFlex]
    B --> C["QNN e.g., Centernet"]
    A --> D[VideoEncoder]
```

In this setup, we can define a shared `NodeFrameDescriptor` with the following buffer layout:

```
[cam_img, cl_rgb, heatmap, wh, reg, hevc]
```

```mermaid
graph LR
    subgraph NodeFrameDescriptor
        B0["0: cam_img"]
        B1["1: cl_rgb"]
        B2["2: heatmap"]
        B3["3: wh"]
        B4["4: reg"]
        B5["5: hevc"]
    end

    CameraNode["Camera Node"] -->|writes to| B0
    B0 -->|read by| CL2DFlexNode["CL2DFlex Node"]
    CL2DFlexNode -->|writes to| B1
    B1 -->|read by| QNNNode["QNN Node"]
    QNNNode -->|writes to| B2
    QNNNode -->|writes to| B3
    QNNNode -->|writes to| B4
    B0 -->|read by| VideoEncoderNode["VideoEncoder Node"]
    VideoEncoderNode -->|writes to| B5
```

#### Camera Node
- **Buffer Index 0 (`cam_img`)**:  
  The Camera node writes its output image to this buffer.

#### CL2DFlex Node
- **Input**:
  Reads from **buffer index 0**, which contains the image produced by the Camera.
- **Output**:
  Writes the preprocessed RGB image to **buffer index 1 (`cl_rgb`)**.

#### QNN Node (Centernet)
- **Input**:
  Reads from **buffer index 1**, the RGB image produced by CL2DFlex.
- **Outputs**:
  - **Buffer index 2 (`heatmap`)**: Centernet heatmap output  
  - **Buffer index 3 (`wh`)**: Width-height regression output  
  - **Buffer index 4 (`reg`)**: Offset regression output
- For details, refer [QNN globalBufferIdMap configuration](../include/QC/Node/QNN.hpp#L105)

#### VideoEncoder Node
- **Input**:
  Reads from **buffer index 0**, which contains the image produced by the Camera.
- **Output**:
  Writes the compressed hevc image to **buffer index 5 (`hevc`)**.

### ⚠️ Application Responsibility & Limitations in Sample Code

To correctly use a shared `NodeFrameDescriptor` across multiple QCNodes in a pipeline, the **application must be aware of the graph topology**—specifically, the number of buffer descriptors required and their roles (input, output, parameter) at each stage. This knowledge is essential to correctly size and populate the `NodeFrameDescriptor`.

In the current QCNode source code, the **`QCNodeSampleApp` does not support this shared descriptor model**. Instead, each sample application demonstrates a single QCNode in isolation, using a `NodeFrameDescriptor` that contains **only the buffer descriptors relevant to that specific node**.

As a result:
- The shared buffer model is not demonstrated in the sample apps.
- Developers integrating multiple nodes must manually manage the buffer layout and ensure consistency across nodes.
- The global buffer map must be defined and passed during initialization, but this feature is **not yet fully supported by all QCNode implementations**.

---

# 2. QCNode buffer related APIs

- [BufferManager::Allocate](../tests/sample/include/QC/sample/BufferManager.hpp#L56)
  - This API performs buffer allocation based on the input properties. Internally, it delegates the task to one of the following private methods of BufferManager, depending on the buffer type:
    - [BufferManager::AllocateBinary](../tests/sample/include/QC/sample/BufferManager.hpp#L79)
    - [BufferManager::AllocateBasicImage](../tests/sample/include/QC/sample/BufferManager.hpp#L96)
    - [BufferManager::AllocateImage](../tests/sample/include/QC/sample/BufferManager.hpp#L125)
    - [BufferManager::AllocateTensor](../tests/sample/include/QC/sample/BufferManager.hpp#L147)
- [BufferManager::Free](../tests/sample/include/QC/sample/BufferManager.hpp#L63)
- [ImageDescriptor::GetImageDesc](../include/QC/Infras/Memory/ImageDescriptor.hpp#L248): Gets a new buffer descriptor that represents the image batches specified by batchOffset and batchSize.
- [BufferDescriptor::GetDataPtr](../include/QC/Infras/Memory/BufferDescriptor.hpp#L118): Returns a pointer to the valid data in the buffer.
- [BufferDescriptor::GetDataSize](../include/QC/Infras/Memory/BufferDescriptor.hpp#L129): Returns the size of valid data in the buffer.
- [ImageDescriptor::ImageToTensor](../include/QC/Infras/Memory/ImageDescriptor.hpp#L228): 1 plane image to tensor
- [ImageDescriptor::ImageToTensor](../include/QC/Infras/Memory/ImageDescriptor.hpp#L237): 2 plane yuv image to luma and chroma tensor

- Memory Map/UnMap for QNX
  - [MemoryMap](../include/QC/Infras/Memory/PMEMUtils.hpp#L44): Memory Map a DMA memory allocated by the other process.
  - [MemoryUnMap](../include/QC/Infras/Memory/PMEMUtils.hpp#L54): Un-Import a DMA memory allocated by the other process.
- Memory Map/UnMap for Linux
  - [MemoryMap](../include/QC/Infras/Memory/DMABUFFUtils.hpp#L43): Memory Map a DMA memory allocated by the other process.
  - [MemoryUnMap](../include/QC/Infras/Memory/DMABUFFUtils.hpp#L53): Un-Import a DMA memory allocated by the other process.
  
# 3. QCNode Buffer Descriptor Examples

For an ADAS perception application, the buffers are generally allocated during the initialization phase and then on ping-pong used during running, and only will be released when the application exit.

## 3.1 A ImageDescriptor_t image for BEV kind of AI model

Generally, for the BEV kind of AI models, it was that multiple cameras’ frame are preprocessed and saved into 1 RGB buffer, and generally it was 6 or 7 cameras, but here gives an example with 3 cameras case.

![3-batch-rgb-image](./images/3-batch-rgb-image.jpg)

The [SANITY_ImageAllocateRGBByProps](../tests/unit_test/Infras/Memory/gtest_Memory.cpp#L293) demonstrate that how to allocate such a batched image(batchSize=3), the imgDescAll will represent the whole buffer that contain the 3 RGB images. And use the API [GetImageDesc](../include/QC/Infras/Memory/ImageDescriptor.hpp#L248) to get a shared buffer descriptor imgDescMiddle to represent the middle front camera RGB image.

Thus, the imgDescAll can be feed into the BEV kind of the AI models, and the imgDescMiddle can be feed into a traffic light detection AI model for example, thus for the traffic light detection AI model, it doesn't need another pre-processing to convert the front camera frame to RGB, just reused the middle portion of the imgDescAll to save computing resource.


## 3.2 Allocate buffers to hold images

The [SANITY_ImageAllocateByWHF](../tests/unit_test/Infras/Memory/gtest_Memory.cpp#L104) demonstrate that how to allocate 1 camera buffer for format UYVY or NV12, it was through using API "[Allocate](../tests/sample/include/QC/sample/BufferManager.hpp#L56)" to allocate an image with the best alignment that can be shared between CPU/GPU/VPU/HTP, etc.

But if want to allocate a list of ping-pong buffers, the usage is generally as below.

```c++
class AUserClass
{
public:
    QCStatus_e Init( void )
    {
        QCNodeID_t nodeId;
        nodeId.name = "DEMO"; /* the name must be unique */
        nodeId.id = 0;        /* the nodeId must be unique */
        nodeId.type = QC_NODE_TYPE_CL_2D_FLEX; /* use the correct type accordingly */
        m_pBufMgr = BufferManager::Get( nodeId );
        QCStatus_e ret = QC_STATUS_OK;
        // allocate the 4 ping-pong buffers
        for ( int i = 0; ( i < 4 ) && ( QC_STATUS_OK == ret ); i++ )
        {
            ret = m_pBufMgr->Allocate( ImageBasicProps_t( 3840, 2160, QC_IMAGE_FORMAT_NV12 ), m_buffers[i] );
        }
        // init the QCNode CL2DFlex
        return ret;
    }

    QCStatus_e Run( ImageDescriptor_t &input )
    {
        QCSharedFrameDescriptorNode frameDesc( 2 );
        ImageDescriptor_t &output = m_buffers[m_index];
        // for each run, ping-pong use each buffer
        // do process of the pInput, such as using CL2DFlex to do color conversion from UYVY to NV12
        frameDesc.SetBuffer( 0, input );
        frameDesc.SetBuffer( 1, output );
        ret = cl2dflex.ProcessFrameDescriptor( frameDesc );
        m_index++;
        if ( m_index > 4 )
        {
            m_index = 0;
        }
    }

    QCStatus_e Deinit( void )
    {
        QCStatus_e ret = QC_STATUS_OK;

        // release the 4 ping-pong buffers
        for ( int i = 0; ( i < 4 ) && ( QC_STATUS_OK == ret ); i++ )
        {
            ret = m_pBufMgr->Free( m_buffers[i] );
        }

        BufferManager::Put(m_pBufMgr);
        m_pBufMgr = nullptr;

        return ret;
    }

private:
    CL2DFlex m_cl2d;
    ImageDescriptor_t m_buffers[4];   // A case that want 4 ping-pong buffers.
    BufferManager *m_pBufMgr = nullptr;
    uint32_t m_index = 0;
}
```

But consideration of the life cycle manegement, the implementation will be totally different for the sharing between threads in the same process or between processes.

And the QCNode Sample [SharedBufferPool](../tests/sample/include/QC/sample/SharedBufferPool.hpp#L126) gives a demo that how to create a ping-pong buffer pool that the buffer can be shared between threads in the process, for more details, check [The QCNode Sample Buffer Life Cycle Management](./sample-buffer-life-cycle-management.md).

## 3.3 Allocate Tensor

The [SANITY_TensorAllocate](../tests/unit_test/Infras/Memory/gtest_Memory.cpp#L365) demonstrate that how to allocate buffer for Tensor.


## 3.4 Convert Image to Tensor

Here for the node QNN, the inputs/outputs of this node must be Tensor not Image. So the shared buffer Image must be converted into Tensor.

### 3.4.1 Convert the RGB Image to the Tensor

For QNN with RGB or normalized RGB as input, here this API [ImageToTensor](../include/QC/Infras/Memory/ImageDescriptor.hpp##L228) can be used to convert the RGB Image to a Tensor.

- Refer [SampleQnn ThreadMain](../tests/sample/source/SampleQnn.cpp#L416).
- Refer [gtest SANITY_ImageAllocateByWHF](../tests/unit_test/Infras/Memory/gtest_Memory.cpp#L104).

### 3.4.2 Convert the NV12/P010 Image to the Luma and Chroma Tensor

For QNN with NV12 or P010 as input, here this API [ImageToTensor](../include/QC/Infras/Memory/ImageDescriptor.hpp##L237) can be used to convert the NV12/P010 Image to the Luma and Chroma Tensor.

- Refer [gtest L2_Image2Tensor](../tests/unit_test/Infras/Memory/gtest_Memory.cpp#L895).


