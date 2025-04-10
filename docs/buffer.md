*Menu*:
- [1. RideHal Buffer Data Structures](#1-ridehal-buffer-data-structures)
  - [1.1 The details of image properties.](#11-the-details-of-image-properties)
  - [1.2 The details of RideHal_SharedBuffer_t.](#12-the-details-of-ridehal_sharedbuffer_t)
- [2. RideHal buffer APIs](#2-ridehal-buffer-apis)
- [3. RideHal_SharedBuffer_t Examples](#3-ridehal_sharedbuffer_t-examples)
  - [3.1 A RideHal_SharedBuffer_t image for BEV kind of AI model](#31-a-ridehal_sharedbuffer_t-image-for-bev-kind-of-ai-model)
  - [3.2 Allocate buffers to hold images](#32-allocate-buffers-to-hold-images)
  - [3.3 Allocate Tensor](#33-allocate-tensor)
  - [3.4 Convert Image to Tensor](#34-convert-image-to-tensor)
    - [3.4.1 Convert the RGB Image to the Tensor](#341-convert-the-rgb-image-to-the-tensor)
    - [3.4.2 Convert the NV12/P010 Image to the Luma and Chroma Tensor](#342-convert-the-nv12p010-image-to-the-luma-and-chroma-tensor)

# 1. RideHal Buffer Data Structures

- [RideHal_Buffer_t](../include/ridehal/common/Types.hpp#L86)
- [RideHal_ImageProps_t](../include/ridehal/common/Types.hpp#L115)
- [RideHal_TensorProps_t](../include/ridehal/common/Types.hpp#L166)
- [RideHal_SharedBuffer_t](../include/ridehal/common/SharedBuffer.hpp#L15)

## 1.1 The details of image properties.

Due to hardware constraints, the actual buffer used to store an image may have alignment padding along its width and height. This padding is primarily for zero-copy operations, enabling the buffer to be shared with the hardware accelerator. However, for an image with certain width and height, it can has no padding at all.

And the below picture shows a case what's the actual buffer looks like for an image format such as NV12 that has 2 planes, the black area is padding space thus not valid pixels.

![Image format with 2 plane](./images/image-prop-2-plane.jpg)

For each plane, it may have paddings along width and height, it may also has paddings between the 2 planes. And some extra paddings is also needed at the end of the each plane.

And the below picture shows a case what's the actual buffer looks like for an image format such as RGB that has 1 plane.

![Image format with 1 plane](./images/image-prop-1-plane.jpg)

Thus now, it's easy to understand those members of the type [RideHal_ImageProps_t](../include/ridehal/common/Types.hpp#L118) except batchSize.

For the batchSize, it was generally designed for the BEV kind of AI models, check below section [3.1](#31-a-ridehal_sharedbuffer_t-image-for-bev-kind-of-ai-model).

For or the compressed image with the format H264 or H265, and the code [SANITY_CompressedImageAllocateByProps](../tests/unit_test/buffer/gtest_Buffer.cpp#L222) which gives an example that how to allocate a buffer for a compressed image and this is the only way. And please note that for the compressed image, the member stride/actualHeight will be invalid and should not be used.

# 1.2 The details of RideHal_SharedBuffer_t.

The RideHal_SharedBuffer_t is a data structure to represent a DMA memory portion that can be shared between components for zero copy purpose. Please note that, its member ["buffer"](../include/ridehal/common/SharedBuffer.hpp#L17) represent a single continuous(from user space of view, physically it's maybe not continuous.) DMA memory, and with its member ["offset"](../include/ridehal/common/SharedBuffer.hpp#L19) and ["size"](../include/ridehal/common/SharedBuffer.hpp#L18) to represent the actual memory location and size in the single DMA memory. But general case is that the RideHal_SharedBuffer_t will represent all the memory represent by its member ["buffer"](../include/ridehal/common/SharedBuffer.hpp#L17), but there is a typical use case for the BEV kind of AI model, please check section [3.1](#31-a-ridehal_sharedbuffer_t-image-for-bev-kind-of-ai-model) which the ShareBufferMiddle just represent the middle portion of the DMA memory.

And it's strongly recommended that to use the APIs of RideHal_SharedBuffer_t to do memory allocation and free, but it's also OK to use any kind of the platform DMA related APIs (PMEM for QNX, dma-buf for Linux), but in this case, the user application need to assign the right value to each member of the RideHal_SharedBuffer_t.

And in fact, the RideHal_SharedBuffer_t APIs are based on the platform DMA related APIs (PMEM for QNX, dma-buf for Linux). 
  - For QNX, check [RideHal_DmaAllocate](../source/common/Buffer_QNX.cpp#L30) and [RideHal_DmaFree](../source/common/Buffer_QNX.cpp#L82).
  - For Linux, check [RideHal_DmaAllocate](../source/common/Buffer_Linux.cpp#L62) and [RideHal_DmaFree](../source/common/Buffer_Linux.cpp#L138).

```c
// For case that using PMEM or dma-buf to allocate memory,
// now have the virtual address pData and the uint64 dmaHandle.
// for QNX, the dmaHandle is cast from pmem_handle_t.
// for Linux, the dmaHandle is cast from int.

RideHal_SharedBuffer_t shareBuffer;

shareBuffer.buffer.pData = pData;
shareBuffer.buffer.dmaHandle = dmaHandle;
shareBuffer.buffer.size = size;
shareBuffer.buffer.id = (uint64)-1; // ignore this
shareBuffer.buffer.pid = static_cast<uint64_t>( getpid() );
shareBuffer.buffer.usage = RIDEHAL_BUFFER_USAGE_DEFAULT;
shareBuffer.buffer.flags = 0;
shareBuffer.size = size;
shareBuffer.offset = 0;
shareBuffer.type = RIDEHAL_BUFFER_TYPE_IMAGE;
shareBuffer.imgProps.format = format;
shareBuffer.imgProps.batchSize = batchSize;
shareBuffer.imgProps.width = width;
shareBuffer.imgProps.height = height;
shareBuffer.imgProps.numPlanes = numPlanes;
shareBuffer.imgProps.stride[0] = stride0;
shareBuffer.imgProps.actualHeight[0] = actualHeight0;
shareBuffer.imgProps.planeBufSize[0] = stride0*actualHeight0;
...
shareBuffer.imgProps.stride[numPlanes-1] = strideX;
shareBuffer.imgProps.actualHeight[numPlanes-1] = actualHeightX;
shareBuffer.imgProps.planeBufSize[numPlanes-1] = strideX*actualHeight;

// and then this can be feed into a RideHal Component

remap.Execute(&shareBuffer, 1, ...);
```

And another thing, the RideHal_SharedBuffer_t can be shared between components, but it has no life cycle management ability. Here, the RideHal Sample Application has a demo that using C++ std::shared_ptr to demonstrate that how to do the buffer life cycle management between the components that running in the same process but in different threads, refer [The RideHal Sample Buffer Life Cycle Management](./sample-buffer-life-cycle-management.md).

# 2. RideHal buffer APIs

- [Allocate an image with the best alignment that can be shared between CPU/GPU/VPU/HTP, etc](../include/ridehal/common/SharedBuffer.hpp#L63)

- [Allocate a batched images with the best alignment that can be shared between CPU/GPU/VPU/HTP, etc](../include/ridehal/common/SharedBuffer.hpp#L78)

- [Allocate an image(s) with specified image properties](../include/ridehal/common/SharedBuffer.hpp#L90)

- [Allocate a tensor with specified tensor properties](../include/ridehal/common/SharedBuffer.hpp#L101)

- [Free](../include/ridehal/common/SharedBuffer.hpp#L109)

- [GetSharedBuffer](../include/ridehal/common/SharedBuffer.hpp#L119): Get a shared buffer descriptor that represent the DMA memory portion specified by batchOffset and batchSize

- [data](../include/ridehal/common/SharedBuffer.hpp#L126): return the actual shared buffer virtual address

- [ImageToTensor](../include/ridehal/common/SharedBuffer.hpp#L135): 1 plane image to tensor

- [ImageToTensor](../include/ridehal/common/SharedBuffer.hpp#L146): 2 plane yuv image to luma and chroma tensor

- [Import](../include/ridehal/common/SharedBuffer.hpp#L157): Import a DMA memory allocated by the other process.

- [UnImport](../include/ridehal/common/SharedBuffer.hpp#L163): Un-Import a DMA memory allocated by the other process.

# 3. RideHal_SharedBuffer_t Examples

For an ADAS perception application, the buffers are generally allocated during the initialization phase and then on ping-pong used during running, and only will be released when the application exit.

## 3.1 A RideHal_SharedBuffer_t image for BEV kind of AI model

Generally, for the BEV kind of AI models, it was that multiple cameras’ frame are preprocessed and saved into 1 RGB buffer, and generally it was 6 or 7 cameras, but here gives an example with 3 cameras case.

![3-batch-rgb-image](./images/3-batch-rgb-image.jpg)

The [SANITY_ImageAllocateRGBByProps](../tests/unit_test/buffer/gtest_Buffer.cpp#L168) demonstrate that how to allocate such a batched image(batchSize=3), the ShareBufferAll will represent the whole buffer that contain the 3 RGB images. And use the API [GetSharedBuffer](../include/ridehal/common/SharedBuffer.hpp#L101) to get a shared buffer descriptor ShareBufferMiddle to represent the middle front camera RGB image.

Thus, the SharedBufferAll can be feed into the BEV kind of the AI models, and the ShareBufferMiddle can be feed into a traffic light detection AI model for example, thus for the traffic light detection AI model, it doesn't need another pre-processing to convert the front camera frame to RGB, just reused the middle portion of the SharedBufferAll to save computing resource.


## 3.2 Allocate buffers to hold images

The [SANITY_ImageAllocateByWHF](../tests/unit_test/buffer/gtest_Buffer.cpp#L12) demonstrate that how to allocate 1 camera buffer for format UYVY or NV12, it was through using API "[Allocate](../include/ridehal/common/SharedBuffer.hpp#L63)" to allocate an image with the best alignment that can be shared between CPU/GPU/VPU/HTP, etc.

But if want to allocate a list of ping-pong buffers, the usage is generally as below.

```c++
class AUserClass
{
public:
    RideHalError_e Init( void )
    {
        RideHalError_e ret = RIDEHAL_ERROR_NONE;
        // allocate the 4 ping-pong buffers
        for ( int i = 0; ( i < 4 ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
        {
            ret = m_buffers.Allocate( 3840, 2160, RIDEHAL_IMAGE_FORMAT_NV12 );
        }

        return ret;
    }

    RideHalError_e Run( RideHal_SharedBuffer_t *pInput )
    {
        RideHal_SharedBuffer_t *pBuffer = m_buffers[m_index];
        // for each run, ping-pong use each buffer
        // do process of the pInput, such as using c2d to do color conversion from UYVY to NV12
        // ret = c2d.Execute( pInput, 1, pBuffer );
        m_index++;
        if ( m_index > 4 )
        {
            m_index = 0;
        }
    }

    RideHalError_e Deinit( void )
    {
        RideHalError_e ret = RIDEHAL_ERROR_NONE;

        // release the 4 ping-pong buffers
        for ( int i = 0; ( i < 4 ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
        {
            ret = m_buffers.Free();
        }

        return ret;
    }

private:
    RideHal_SharedBuffer_t m_buffers[4];   // A case that want 4 ping-pong buffers.
    uint32_t m_index = 0;
}
```

But consideration of the life cycle manegement, the implementation will be totally different for the sharing between threads in the same process or between processes.

And the RideHal Sample [SharedBufferPool](../tests/sample/include/ridehal/sample/SharedBufferPool.hpp#L126) gives a demo that how to create a ping-pong buffer pool that the buffer can be shared between threads in the process, for more details, check [The RideHal Sample Buffer Life Cycle Management](./sample-buffer-life-cycle-management.md).

## 3.3 Allocate Tensor

The [SANITY_TensorAllocate](../tests/unit_test/buffer/gtest_Buffer.cpp#L237) demonstrate that how to allocate buffer for Tensor, it was through using API "[Allocate](../include/ridehal/common/SharedBuffer.hpp#L99)".


## 3.4 Convert Image to Tensor

Here for the component QnnRuntime, the inputs/outputs of this component must be Tensor not Image. So the shared buffer Image must be converted into Tensor.

### 3.4.1 Convert the RGB Image to the Tensor

For QnnRuntime with RGB or normalized RGB as input, here this API [ImageToTensor](../include/ridehal/common/SharedBuffer.hpp#L133) can be used to convert the RGB Image to a Tensor.

- Refer [SampleQnn ThreadMain](../tests/sample/source/SampleQnn.cpp#L224).
- Refer [gtest SANITY_ImageAllocateByWHF](../tests/unit_test/buffer/gtest_Buffer.cpp#L30).

### 3.4.2 Convert the NV12/P010 Image to the Luma and Chroma Tensor

For QnnRuntime with NV12 or P010 as input, here this API [ImageToTensor](../include/ridehal/common/SharedBuffer.hpp#L144) can be used to convert the NV12/P010 Image to the Luma and Chroma Tensor.

- Refer [gtest L2_Image2Tensor](../tests/unit_test/buffer/gtest_Buffer.cpp#L861).


