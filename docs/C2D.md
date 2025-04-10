*Menu*:
- [1. Introduction](#1-introduction)
- [2. RideHal C2D Data Structures](#2-ridehal-c2d-data-structures)
  - [2.1 The details of C2D_Config_t](#21-the-details-of-c2d_config_t)
  - [2.2 The details of C2D_InputConfig_t](#22-the-details-of-c2d_inputconfig_t)
  - [2.3 The details of C2D_ROIConfig_t](#23-the-details-of-c2d_roiconfig_t)
  - [2.4 The details of C2D_ImageResolution_t](#24-the-details-of-c2d_imageresolution_t)
- [3. RideHal C2D APIs](#3-ridehal-c2d-apis)
- [4. Typical Use Case](#4-typical-use-case)

# 1. Introduction
The RideHal C2D component is a system-level programming interface designed to accelerate 2D bit manipulation. It supports camera image processing functions for RideHal, including cropping, downscaling, upscaling, and color conversion. This component is compatible with the QNX platform and offers a user-friendly API along with a high-performance function library.

# 2. RideHal C2D Data Structures

- [C2D_Config_t](../include/ridehal/component/C2D.hpp#L53)
- [C2D_InputConfig_t](../include/ridehal/component/C2D.hpp#L46)
- [C2D_ROIConfig_t](../include/ridehal/component/C2D.hpp#L38)
- [C2D_ImageResolution_t](../include/ridehal/component/C2D.hpp#L29)

## 2.1 The details of C2D_Config_t
C2D_Config_t is the data structure that used to initialize C2D component. It contains C2D input configurations, all the parameters needs to be set by user.
The parameter numOfInputs must be an positive integer in the range of [1, 32]. The inputConfigs is an array which contains 32 elements, each element contains image format, image resolution and ROI as input parameters of each input stream. This version of C2D supports NV12 as input image format. For input ROI, the topX and topY must be less than image width and height, and ROI width less than `image width - topX`, ROI height less than `image height - topY`.
- numOfInputs: Number of Input Images
- inputConfigs: Array of Input Configurations

## 2.2 The details of C2D_InputConfig_t
C2D_InputConfig_t contains all input parameters:
- inputFormat: Image format of Input frame (Supported format: NV12, UYVY, RGB888)
- inputResolution: Image Resolution of Input frame
- ROI: Reigion of Interest in Input frame

## 2.3 The details of C2D_ROIConfig_t
C2D_ROIConfig_t describes ROI config of input frame:
- topX: X coordinate of upper left point
- topY: Y coordinate of upper left point
- width: ROI width
- height: ROI height

## 2.4 The details of C2D_ImageResolution_t
C2D_ImageResolution_t is the data structure that contains image resolution information:
- width: Image width
- height: Image height

# 3. RideHal C2D APIs

- [Initialize the C2D component](../include/ridehal/component/C2D.hpp#L79)
- [Start the C2D pipeline](../include/ridehal/component/C2D.hpp#L87)
- [Stop the C2D pipeline](../include/ridehal/component/C2D.hpp#L94)
- [Deinitialize the C2D component](../include/ridehal/component/C2D.hpp#L101)
- [Execute the C2D pipeline](../include/ridehal/component/C2D.hpp#L111)
- [Register shared buffers for each input](../include/ridehal/component/C2D.hpp#L121)
- [Register shared buffers for output](../include/ridehal/component/C2D.hpp#L131)
- [Deregister shared buffers for each input](../include/ridehal/component/C2D.hpp#L141)
- [Deregister shared buffers for output](../include/ridehal/component/C2D.hpp#L151)

# 4. Typical Use Case

It needs just several steps to configure and launch C2D component. User needs to set input and output parameters, the input format supports NV12, and output format supports UYVY. 

- Step 1: Configure and Initialize input/output parameters
```c++
    C2D C2DObj;
    C2D_Config_t C2DConfig;
    C2D_Config_t *pC2DConfig = &C2DConfig;
    char pName[5] = "C2D";
    C2DConfig.numOfInputs = 1;

    for ( size_t i = 0; i < C2DConfig.numOfInputs; i++ )
    {
        C2DConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
        C2DConfig.inputConfigs[i].inputResolution.width = 1920;
        C2DConfig.inputConfigs[i].inputResolution.height = 1080;
        C2DConfig.inputConfigs[i].ROI.topX = 100;
        C2DConfig.inputConfigs[i].ROI.topY = 100;
        C2DConfig.inputConfigs[i].ROI.width = 600;
        C2DConfig.inputConfigs[i].ROI.height = 600;
    }

    RideHal_ImageFormat_e C2DOutputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
    uint32_t C2DOutputWidth = 1080;
    uint32_t C2DOutputHeight = 720;

    ret = C2DObj.Init( pName, pC2DConfig );
```

- Step 2: Allocate and register input/output buffers
```c++
    RideHal_SharedBuffer_t inputs[C2DConfig.numOfInputs];
    ret = inputs[0].Allocate( C2DConfig.inputConfigs[0].inputResolution.width,
                                C2DConfig.inputConfigs[0].inputResolution.height,
                                C2DConfig.inputConfigs[0].inputFormat );

    RideHal_SharedBuffer_t output;
    ret = output.Allocate( C2DOutputWidth, C2DOutputHeight, C2DOutputFormat );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.RegisterInputBuffers( &inputs[i], 1 );
    }

    ret = C2DObj.RegisterOutputBuffers( &output, 1 );
```

- Step 3: Launch the pipeline
```c++

ret = C2DObj.Start();
ret = C2DObj.Execute( inputs, C2DConfig.numOfInputs, &output );

```

- Step 4: Stop and deinitialize the pipeline
```c++
    ret = C2DObj.Stop();
    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = C2DObj.DeregisterInputBuffers( &inputs[i], 1 );
    }
    ret = C2DObj.DeregisterOutputBuffers( &output, 1 );
    ret = C2DObj.Deinit();
```

Reference: 
- [gtest_ComponentC2D](../tests/unit_test/components/C2D/gtest_C2D.cpp)
- [SampleC2D](../tests/sample/source/SampleC2D.cpp)


