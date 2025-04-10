*Menu*:
- [1. Optical flow overview](#1-optical-flow-overview)
- [2. OpticalFlow Data Structures](#2-opticalflow-data-structures)
- [3. OpticalFlow APIs](#3-opticalflow-apis)
- [4. OpticalFlow Examples](#4-opticalflow-examples)
  - [4.1 OpticalFlow initialization](#41-opticalflow-initialization)
  - [4.2 OpticalFlow Input/Output buffer allocation](#42-opticalflow-inputoutput-buffer-allocation)
  - [4.3 Register buffer to optical flow](#43-register-buffer-to-optical-flow)
  - [4.4 Start and Run](#44-start-and-run)
  - [4.5 Stop and Deinit](#45-stop-and-deinit)
  - [4.6 RideHal E2E sample](#46-ridehal-e2e-sample)

# 1. Optical flow overview

Optical flow is the pattern of apparent motion of image objects between two consecutive frames caused by the movement of object or camera. It is 2D vector field where each vector is a displacement vector showing the movement of points from first frame to second. Consider the image below (Image Courtesy: [Wikipedia article on Optical Flow](https://en.wikipedia.org/wiki/Optical_flow)).

![Optical Flow](https://docs.opencv.org/3.4/optical_flow_basic1.jpg)

This OpticalFlow component provides a user-friendly API to utilize the hardware accelerator EVA for computing the optical flow between two consecutive frames.

# 2. OpticalFlow Data Structures

- [OpticalFlow_Config_t](../include/ridehal/component/OpticalFlow.hpp#L60)

This configuration data structure comes with a default initializer that sets up its members with suggested default values. Refer to [OpticalFlow_Config::OpticalFlow_Config](../source/components/OpticalFlow/OpticalFlow.cpp#L41). For startup, you only need to update the relevant information for the input image.

```c
    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
```

# 3. OpticalFlow APIs

- [Init](../include/ridehal/component/OpticalFlow.hpp#L87)
- [RegisterBuffers](../include/ridehal/component/OpticalFlow.hpp#L90)
- [Start](../include/ridehal/component/OpticalFlow.hpp#L96)
- [Execute](../include/ridehal/component/OpticalFlow.hpp#L106)
- [Stop](../include/ridehal/component/OpticalFlow.hpp#L115)
- [DeRegisterBuffers](../include/ridehal/component/OpticalFlow.hpp#L125)
- [Deinit](../include/ridehal/component/OpticalFlow.hpp#L131)


# 4. OpticalFlow Examples

## 4.1 OpticalFlow initialization

```c
    OpticalFlow ofl;

    OpticalFlow_Config_t config;
    config.width = 1920;
    config.height = 1024;
    config.filterOperationMode = EVA_OF_MODE_DSP;

    ret = ofl.Init( "OFL0", &config );
```

## 4.2 OpticalFlow Input/Output buffer allocation

```c
    // below allocate a buffer to hold image
    RideHal_SharedBuffer_t refImg;
    RideHal_SharedBuffer_t curImg;
    ret = refImg.Allocate( 1920, 1024, RIDEHAL_IMAGE_FORMAT_NV12 );
    ret = curImg.Allocate( 1920, 1024, RIDEHAL_IMAGE_FORMAT_NV12 );

    // below calculated the output motion vector width/hegith
    uint32_t width = ( config.width >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;
    uint32_t height = ( config.height >> config.amFilter.nStepSize ) << config.amFilter.nUpScale;

    // define the output tensor properties that used to allocate the output tensor buffer
    RideHal_TensorProps_t mvFwdMapTsProp = {
            RIDEHAL_TENSOR_TYPE_UINT_16,
            { 1, ALIGN_S( height, 8 ), ALIGN_S( width * 2, 128 ), 1 },
            4 };
    RideHal_TensorProps_t mvConfTsProp = { RIDEHAL_TENSOR_TYPE_UINT_8,
                                           { 1, ALIGN_S( height, 8 ), ALIGN_S( width, 128 ), 1 },
                                           4 };
    // do allocate buffer to hold optical flow output
    ret = mvFwdMap.Allocate( &mvFwdMapTsProp );
    ret = mvConf.Allocate( &mvConfTsProp );
```

## 4.3 Register buffer to optical flow

This step is optional, if it was not done, the Execute API will help to do the buffer register only once.

```c
    ret = ofl.RegisterBuffers( &refImg, 1 );
    ret = ofl.RegisterBuffers( &curImg, 1 );
    ret = ofl.RegisterBuffers( &mvFwdMap, 1 );
    ret = ofl.RegisterBuffers( &mvConf, 1 );
```

## 4.4 Start and Run

```c
    ret = ofl.Start();
    ret = ofl.Execute( &curImg, &refImg, &mvFwdMap, &mvConf );
```

## 4.5 Stop and Deinit

```c
    ret = ofl.Stop();

    ret = ofl.DeRegisterBuffers( &refImg, 1 );
    ret = ofl.DeRegisterBuffers( &curImg, 1 );
    ret = ofl.DeRegisterBuffers( &mvFwdMap, 1 );
    ret = ofl.DeRegisterBuffers( &mvConf, 1 );

    ret = ofl.Deinit();
```

## 4.6 RideHal E2E sample

The [SampleOpticalFlow](../tests/sample/source/SampleOpticalFlow.cpp#L186) and [SampleOpticalFlowViz](../tests/sample/source/SampleOpticalFlowViz.cpp#L328) is an end-to-end pipeline demo that demonstrates how to use the opticalflow component. And the [SampleOpticalFlowViz](../tests/sample/source/SampleOpticalFlowViz.cpp#L328) demonstrates how to decode the motion vector output.
