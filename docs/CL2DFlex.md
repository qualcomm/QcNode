*Menu*:
- [1. CL2DFlex Overview](#1-cl2dflex-overview)
    - [Key Features](#key-features)
- [2. CL2DFlex Configuraion](#2-cl2dflex-configuraion)
  - [2.1 CL2DFlex Static JSON Configuration](#21-cl2dflex-static-json-configuration)
- [3. CL2DFlex APIs](#3-cl2dflex-apis)
- [4. Typical CL2DFlex API Usage Examples](#4-typical-cl2dflex-api-usage-examples)
  - [4.1 Set Configurations](#41-set-configurations)
  - [4.2 API Call Flow](#42-api-call-flow)
  - [4.3 Supported Pipelines](#43-supported-pipelines)
- [5. References](#5-references)

# 1. CL2DFlex Overview

**QCNode CL2DFlex** is based on OpenCL library, it provides user-friendly APIs and visible CL kernels to do color conversion and resize on single image input. Currently support color conversion and resize of multiple image inputs to single output between different formats. It also supports remap from mapping table. 

### Key Features

- **Multiple Color Conversion Pipelines**
  Supports various color conversion operations between NV12, RGB, UYVY, NV12 UBWC.

- **Remap from mapping table**
  Supports remap operation using a mapping table from file or directly coding for flexible pixel reordering.

- **Efficient Memory Management**
  Optimized memory handling for high-performance processing of image data.

- **Scalable Architecture**
  Designed to handle both single and batched image processing scenarios efficiently.

- **OpenCL Integration**
  Built upon OpenCL for leveraging GPU acceleration capabilities with configurable priority and device ID.


# 2. CL2DFlex Configuraion

## 2.1 CL2DFlex Static JSON Configuration

| Parameter  | Required  | Type        | Description            |
|------------|-----------|-------------|------------------------|
| `name`     | true      | string      | The Node unique name.  |
| `id`       | true      | uint32_t    | The Node unique ID.    |
| `logLevel` | false     | string      | The message log level. <br> Options: `VERBOSE`, `DEBUG`, `INFO`, `WARN`, `ERROR` <br> Default: `ERROR`   |
| `priority` | false     | string      | The performance priority level. <br> Options: `low`, `normal`, `high` <br> Default: `normal` |
| `deviceId` | false     | uint32_t    | The device index to use. <br> Default: `0` |
| `outputWidth`  | true  | uint32_t    | The output width.      |
| `outputHeight` | true  | uint32_t    | The output height.     |
| `outputFormat` | true  | string      | The output format. <br> Options: `rgb`, `bgr`, `nv12`, `uyvy` <br> Default: `rgb` |
| `inputs`       | true  | object[]    | List of input configurations. <br>Each object contains: <br> - `inputWidth` (uint32_t) <br> - `inputHeight` (uint32_t) <br> - `inputFormat` (string) <br> - `roiX` (uint32_t) <br> - `roiY` (uint32_t) <br> - `roiWidth` (uint32_t) <br> - `roiHeight` (uint32_t) <br> - `workMode` (string) <br> - `mapXBufferId` (uint32_t) <br> - `mapYBufferId` (uint32_t)  |
| `inputWidth`   | true  | uint32_t    | The input width.       |
| `inputHeight`  | true  | uint32_t    | The input height.      |
| `inputFormat`  | true  | string      | The input format. <br> Options: `rgb`, `nv12`, `uyvy`, `nv12_ubwc` |
| `roiX`         | false | uint32_t    | The input roiX. <br> Default: `0`  |
| `roiY`         | false | uint32_t    | The input roiY. <br> Default: `0`  |
| `roiWidth`     | false | uint32_t    | The input roiWidth. <br> Default: `inputWidth`    |
| `roiHeight`    | false | uint32_t    | The input roiHeight. <br> Default: `inputHeight`  |
| `workMode`     | true  | string      | The input format. <br> Options: `convert`, `resize_nearest`, `letterbox_nearest`, `convert_ubwc`, `letterbox_nearest_multiple`, `resize_nearest_multiple`, `remap_nearest` |
| `mapXBufferId` | false | uint32_t    | The buffer id of X direction map table  |
| `mapYBufferId` | false | uint32_t    | The buffer id of Y direction map table  |
| `numOfROIs`    | false | uint32_t    | The number of ROIs for multiple ROIs work mode.  |
| `ROIsBufferId` | false | uint32_t    | TThe ROIs buffer ID for multiple ROIs work mode.  |
| `nodeId`    | true     | uint32_t    | The Node unique ID.    |
| `bufferIds` | false    | uint32_t[]  | List of buffer indices in `QCNodeInit::buffers`  |
| `globalBufferIdMap` | false | object[] | Mapping of buffer names to buffer indices in `QCFrameDescriptorNodeIfs`. <br>Each object contains:<br> - `name` (string)<br> - `id` (uint32_t)   |
| `deRegisterAllBuffersWhenStop` | false | bool     | Flag to deregister all buffers when stopped      <br>Default: `false` |

- Example Configurations

  - NV12 resize to RGB pipeline
    ```json
    {
        "static":
        {
            "bufferIds":[0,1],
            "id":0,
            "inputs":
            [
                {
                    "inputFormat":"nv12",
                    "inputHeight":1024,
                    "inputWidth":1920,
                    "roiHeight":1024,
                    "roiWidth":1920,
                    "roiX":0,
                    "roiY":0,
                    "workMode":"resize_nearest"
                }
            ],
            "name":"CL2D",
            "outputFormat":"rgb",
            "outputHeight":800,
            "outputWidth":1152
        }
    }
    ```

    - NV12 remap to RGB pipeline
    ```json
    {
        "static":
        {
            "bufferIds":[0,1,2,3],
            "id":0,
            "inputs":
            [
                {
                    "inputFormat":"nv12",
                    "inputHeight":1024,
                    "inputWidth":1920,
                    "mapXBufferId":2,
                    "mapYBufferId":3,
                    "roiHeight":1024,
                    "roiWidth":1920,
                    "roiX":0,
                    "roiY":0,
                    "workMode":"remap_nearest"
                }
            ],
            "name":"CL2D",
            "outputFormat":"rgb",
            "outputHeight":800,
            "outputWidth":1152
        }
    }
    ```

Refer to [CL2DFlex gtest](../tests/unit_test/Node/CL2DFlex/gtest_NodeCL2DFlex.cpp) for more details.

# 3. CL2DFlex APIs 

- [CL2DFlex::Initialize](../include/QC/Node/CL2DFlex.hpp#L273) Initialize CL2DFlex node

- [CL2DFlex::GetConfigurationIfs](../include/QC/Node/CL2DFlex.hpp#L279) Get CL2DFlex configuration interfaces

- [CL2DFlex::GetMonitoringIfs](../include/QC/Node/CL2DFlex.hpp#L285) Get CL2DFlex monitoring interfaces

- [CL2DFlex::Start](../include/QC/Node/CL2DFlex.hpp#L291) Start the CL2DFlex node

- [CL2DFlex::ProcessFrameDescriptor](../include/QC/Node/CL2DFlex.hpp#L306) Execute CL2DFlex node with input and output buffers

- [CL2DFlex::Stop](../include/QC/Node/CL2DFlex.hpp#L312) Stop the CL2DFlex node

- [CL2DFlex::DeInitialize](../include/QC/Node/CL2DFlex.hpp#L318) Deinit the CL2DFlex node

# 4. Typical CL2DFlex API Usage Examples

## 4.1 Set Configurations

QC CL2DFlex node can do image color conversion, resize and ROI scaling for input images. Take a 2 input images with NV12 format resize to output image with RGB format pipeline as example, the configuration parameters can be set as:
```c++
    DataTree dt;
    dt.Set<std::string>( "static.name", "CL2D" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<uint32_t>( "static.outputWidth", 64 );
    dt.Set<uint32_t>( "static.outputHeight", 64 );
    dt.SetImageFormat( "static.outputFormat", QC_IMAGE_FORMAT_RGB888 );
    std::vector<DataTree> inputDts;
    for ( int i = 0; i < 2; i++ )
    {
        DataTree inputDt;
        inputDt.Set<uint32_t>( "inputWidth", 128 );
        inputDt.Set<uint32_t>( "inputHeight", 128 );
        inputDt.SetImageFormat( "inputFormat", QC_IMAGE_FORMAT_NV12 );
        inputDt.Set<uint32_t>( "roiX", 0 );
        inputDt.Set<uint32_t>( "roiY", 0 );
        inputDt.Set<uint32_t>( "roiWidth", 64 );
        inputDt.Set<uint32_t>( "roiHeight", 64 );
        inputDt.Set<std::string>( "workMode", "resize_nearest" );
        inputDts.push_back( inputDt );
    }
    dt.Set( "static.inputs", inputDts );
    QCNodeInit_t config = { dt.Dump() };
```
Note that the ROI.width+ROI.x must not be larger than inputWidth and the ROI.height+ROI.y must not be larger than inputHeight.

## 4.2 API Call Flow

The typical call flow of a QC CL2DFlex pipeline with certain configurations is showed in following codes as an example:
```c++
    QCStatus_e ret;
    QCNodeIfs *pCL2DFlex = new QC::Node::CL2DFlex();
    BufferManager bufMgr( { "MANAGER", QC_NODE_TYPE_CL_2D_FLEX, 0 } );
    QCSharedFrameDescriptorNode frameDesc( 3 );
    uint32_t globalIdx = 0;

    ImageProps_t imgPropInputs[2];
    std::vector<ImageDescriptor_t> inputs;
    inputs.reserve( 2 );
    for ( int i = 0; i < 2; i++ )
    {
        ImageDescriptor_t imageDesc;
        ret = bufMgr.Allocate( imgPropInputs[i], imageDesc );
        inputs.push_back( imageDesc );
        ret = frameDesc.SetBuffer( globalIdx, inputs.back() );
        globalIdx++;
    }
    
    ImageProps_t imgPropOutput;
    std::vector<ImageDescriptor_t> outputs;
    outputs.reserve( 1 );
    ImageDescriptor_t imageDesc;
    ret = bufMgr.Allocate( imgPropOutput, imageDesc );
    outputs.push_back( imageDesc );
    ret = frameDesc.SetBuffer( globalIdx, outputs.back() );
    globalIdx++;

    ret = pCL2DFlex->Initialize( config );

    ret = pCL2DFlex->Start();

    ret = pCL2DFlex->ProcessFrameDescriptor( frameDesc );

    ret = pCL2DFlex->Stop();

    ret = pCL2DFlex->DeInitialize();
```
Generally, user should call Init API once at the beginning of the pipeline and call Deinit API once at the ending of the pipeline.

## 4.3 Supported Pipelines

The currently supported input/output image format for each work mode of CL2DFLex pipelines are listed below.

| Work Mode | Input Format | Output Format | 
|-----------|--------------|---------------|
| Convert | NV12 | RGB |
| Convert | UYVY | RGB |
| Convert | UYVY | NV12 |
| Convert UBWC | NV12 UBWC | NV12 |
| Resize nearest | NV12 | RGB |
| Resize nearest | UYVY | RGB |
| Resize nearest | UYVY | NV12 |
| Resize nearest | RGB | RGB |
| Resize nearest | NV12 | NV12 |
| Letterbox nearest | NV12 | RGB |
| Resize nearest multiple | NV12 | RGB |
| Letterbox nearest multiple | NV12 | RGB |
| Remap | NV12 | RGB |
| Remap | NV12 | BGR |

 In the work mode name column, multiple means execute with single input image and multiple output images using different ROI parameters. Letterbox means resize with fixed height/width ratio and add padding to the right or bottom side, so the height/width ratio of output image is the same as ROI box. Nearest means use the nearest point as interpolation algorithm. UBWC means use uncompressed bandwidth compression format image as input.

# 5. References

- [gtest_CL2DFlex](../tests/unit_test/Node/CL2DFlex/gtest_CL2DFlex.cpp)
- [SampleCL2DFlex](../tests/sample/source/SampleCL2DFlex.cpp)
