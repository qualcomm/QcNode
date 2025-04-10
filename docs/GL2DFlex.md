*Menu*:
- [1. Introduction](#1-introduction)
- [2. RideHal GL2DFlex Data Structures](#2-ridehal-gl2dflex-data-structures)
  - [2.1 The details of GL2DFlex_Config_t](#21-the-details-of-gl2dflex_config_t)
  - [2.2 The details of GL2DFlex_InputConfig_t](#22-the-details-of-gl2dflex_inputconfig_t)
  - [2.3 The details of GL2DFlex_ROIConfig_t](#23-the-details-of-gl2dflex_roiconfig_t)
  - [2.4 The details of GL2DFlex_ImageResolution_t](#24-the-details-of-gl2dflex_imageresolution_t)
- [3. RideHal GL2DFlex APIs](#3-ridehal-gl2dflex-apis)
- [4. Typical Use Case](#4-typical-use-case)

# 1. Introduction
The Ridehal GL2DFlex component offers a range of camera image processing functions, such as cropping, downscaling, upscaling, and color conversion. It works based on OpenGL ES and is compatible with the HGY platform. Additionally, it provides a user-friendly API and a high-performance function library.

# 2. RideHal GL2DFlex Data Structures

- [GL2DFlex_Config_t](../include/ridehal/component/GL2DFlex.hpp#L68)
- [GL2DFlex_InputConfig_t](../include/ridehal/component/GL2DFlex.hpp#L59)
- [GL2DFlex_ROIConfig_t](../include/ridehal/component/GL2DFlex.hpp#L51)
- [GL2DFlex_ImageResolution_t](../include/ridehal/component/GL2DFlex.hpp#L42)

## 2.1 The details of GL2DFlex_Config_t
GL2DFlex_Config_t is the data structure that used to initialize GL2DFlex component. It contains input and output config parameters that need to be set by the user:
- numOfInputs: Number of Input Images in each processing
- inputConfigs: Array of Input Configurations
- outputResolution: Image Resolution of Output frame
- outputFormat: Image format of Output frame (Supported format: NV12, UYVY, RGB888)

## 2.2 The details of GL2DFlex_InputConfig_t
GL2DFlex_InputConfig_t contains all input parameters:
- inputFormat: Image format of Input frame (Supported format: NV12, UYVY, RGB888)
- inputResolution: Image Resolution of Input frame
- ROI: Reigion of Interest in Input frame

## 2.3 The details of GL2DFlex_ROIConfig_t
GL2DFlex_ROIConfig_t describes ROI config of input frame:
- topX: X coordinate of upper left point
- topY: Y coordinate of upper left point
- width: ROI width
- height: ROI height

## 2.4 The details of GL2DFlex_ImageResolution_t
GL2DFlex_ImageResolution_t is the data structure that contains image resolution information:
- width: Image width
- height: Image height

# 3. RideHal GL2DFlex APIs
- [Initialize the GL2DFlex component](../include/ridehal/component/GL2DFlex.hpp#L94)
- [Start the GL2DFlex pipeline](../include/ridehal/component/GL2DFlex.hpp#L102)
- [Stop the GL2DFlex pipeline](../include/ridehal/component/GL2DFlex.hpp#L109)
- [Deinitialize the GL2DFlex component](../include/ridehal/component/GL2DFlex.hpp#L116)
- [Execute the GL2DFlex pipeline](../include/ridehal/component/GL2DFlex.hpp#L126)
- [Register shared buffers for each input](../include/ridehal/component/GL2DFlex.hpp#L136)
- [Register shared buffers for output](../include/ridehal/component/GL2DFlex.hpp#L147)
- [Deregister shared buffers for each input](../include/ridehal/component/GL2DFlex.hpp#L158)
- [Deregister shared buffers for output](../include/ridehal/component/GL2DFlex.hpp#L168)

# 4. Typical Use Case
The Ridehal GL2DFlex component is used for image cropping, color conversion and downscaling/upscaling. User just needs to set input/output configuration parameters, then call the RideHal Init/Start API to launch this component. Here is an example use case that converts UYVY to NV12 and do cropping:

- Step 1: Configure and Initialize input/output parameters
```c++
    GL2DFlex GL2DFlexObj;
    char pName[12] = "GL2DFlex";
    GL2DFlex_Config_t GL2DFlexConfig;
    GL2DFlex_Config_t *pConfig = &GL2DFlexConfig;
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    GL2DFlexConfig.numOfInputs = 1;
    for ( size_t i = 0; i < GL2DFlexConfig.numOfInputs; i++ )
    {
        GL2DFlexConfig.inputConfigs[i].inputFormat = RIDEHAL_IMAGE_FORMAT_UYVY;
        GL2DFlexConfig.inputConfigs[i].inputResolution.width = 1920;
        GL2DFlexConfig.inputConfigs[i].inputResolution.height = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.topX = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.topY = 100;
        GL2DFlexConfig.inputConfigs[i].ROI.width = 1080;
        GL2DFlexConfig.inputConfigs[i].ROI.height = 720;
    }
    RideHal_ImageFormat_e outputFormat = RIDEHAL_IMAGE_FORMAT_NV12;
    uint32_t outputWidth = 1080;
    uint32_t outputHeight = 720;

    GL2DFlexConfig.outputFormat = outputFormat;
    GL2DFlexConfig.outputResolution.width = outputWidth;
    GL2DFlexConfig.outputResolution.height = outputHeight;

    ret = GL2DFlexObj.Init( pName, pConfig );
```

- Step 2: Allocate and register input/output buffers
```c++
    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = inputs[i].Allocate( GL2DFlexConfig.inputConfigs[i].inputResolution.width,
                                  GL2DFlexConfig.inputConfigs[i].inputResolution.height,
                                  GL2DFlexConfig.inputConfigs[i].inputFormat );
    }

    ret = output.Allocate( outputWidth, outputHeight, GL2DFlexConfig.outputFormat );

    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = GL2DFlexObj.RegisterInputBuffers( &inputs[i], 1 );
    }

    ret = GL2DFlexObj.RegisterOutputBuffers( &output, 1 );
```

- Step 3: Launch the pipeline
```c++
    ret = GL2DFlexObj.Start();
    ret = GL2DFlexObj.Execute( inputs, GL2DFlexConfig.numOfInputs, &output );
```

- Step 4: Stop and deinitialize the pipeline
```c++
    ret = GL2DFlexObj.Stop();
    for ( size_t i = 0; i < numInputs; i++ )
    {
        ret = GL2DFlexObj.DeregisterInputBuffers( &inputs[i], 1 );
    }

    ret = GL2DFlexObj.DeregisterOutputBuffers( &output, 1 );
    ret = GL2DFlexObj.Deinit();
```

Reference: 
- [gtest_GL2DFlex](../tests/unit_test/components/GL2DFlex/gtest_GL2DFlex.cpp)
- [SampleGL2DFlex](../tests/sample/source/SampleGL2DFlex.cpp)


