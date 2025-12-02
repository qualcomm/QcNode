*Menu*:
- [1. Remap Overview](#1-remap-overview)
    - [Key Features](#key-features)
- [2. Remap Configuraion](#2-remap-configuraion)
  - [2.1 Remap Static JSON Configuration](#21-remap-static-json-configuration)
- [3. Remap APIs](#3-remap-apis)
- [4. Typical Remap API Usage Examples](#4-typical-remap-api-usage-examples)
  - [4.1 Set Configurations](#41-set-configurations)
  - [4.2 API Call Flow](#42-api-call-flow)
  - [4.3 Supported Pipelines](#43-supported-pipelines)
- [5. References](#5-references)

# 1. Remap Overview

**QCNode Remap**  is based on [FastADAS Remap APIs](https://docs.qualcomm.com/bundle/publicresource/topics/80-63309-1/remap.html). It can do undistortion, downscaling, color conversion, normalization and ROI scaling in one singel API calling on specific processor(CPU, GPU or DSP). 

### Key Features

- **Single API Processing**
  All operations are performed in a single API call for efficiency.

- **Multi-Processor Support**
  Supports CPU, GPU, and DSP processors for optimized performance.

- **Flexible Input/Output Formats**
  Accepts various input/output formats including NV12, RGB, UYVY.

- **Remap from mapping table**
  Supports remap operation using a mapping table from file or directly coding for flexible pixel reordering.

- **Efficient Memory Management**
  Optimized memory handling for high-performance processing of image data.

- **Scalable Architecture**
  Designed to handle both single and batched image processing scenarios efficiently.

# 2. Remap Configuraion

## 2.1 Remap Static JSON Configuration

| Parameter  | Required  | Type        | Description            |
|------------|-----------|-------------|------------------------|
| `name`     | true      | string      | The Node unique name.  |
| `id`       | true      | uint32_t    | The Node unique ID.    |
| `logLevel` | false     | string      | The message log level. <br> Options: `VERBOSE`, `DEBUG`, `INFO`, `WARN`, `ERROR` <br> Default: `ERROR`   |
| `processorType` | true | string      | The processor type, type: string. <br> Options: `cpu`, `gpu`, `htp0`, `htp1` |
| `outputWidth`  | true  | uint32_t    | The output width.      |
| `outputHeight` | true  | uint32_t    | The output height.     |
| `outputFormat` | true  | string      | The output format. <br> Options: `rgb`, `bgr` <br> Default: `rgb` |
| `bEnableUndistortion` | false  | bool | Enable undistortion or not. <br> Default: `false`    |
| `bEnableNormalize`    | false  | bool | Enable normalization or not. <br> Default: `false`   |
| `RAdd` | false | float    | The normalize parameter for R channel add.     |
| `RMul` | false | float    | The normalize parameter for R channel mul.     |
| `RSub` | false | float    | The normalize parameter for R channel sub.     |
| `GAdd` | false | float    | The normalize parameter for G channel add.     |
| `GMul` | false | float    | The normalize parameter for G channel mul.     |
| `GSub` | false | float    | The normalize parameter for G channel sub.     |
| `BAdd` | false | float    | The normalize parameter for B channel add.     |
| `BMul` | false | float    | The normalize parameter for B channel mul.     |
| `BSub` | false | float    | The normalize parameter for B channel sub.     |
| `inputs`       | true  | object[]    | List of input configurations. <br>Each object contains: <br> - `inputWidth` (uint32_t) <br> - `inputHeight` (uint32_t) <br> - `inputFormat` (string) <br> - `roiX` (uint32_t) <br> - `roiY` (uint32_t) <br> - `roiWidth` (uint32_t) <br> - `roiHeight` (uint32_t) <br> - `workMode` (string) <br> - `mapXBufferId` (uint32_t) <br> - `mapYBufferId` (uint32_t)  |
| `inputWidth`   | true  | uint32_t    | The input width.       |
| `inputHeight`  | true  | uint32_t    | The input height.      |
| `inputFormat`  | true  | string      | The input format. <br> Options: `rgb`, `nv12`, `uyvy`, `nv12_ubwc` |
| `mapWidth`     | false | uint32_t    | The map roiWidth. <br> Default: `outputWidth`    |
| `mapHeight`    | false | uint32_t    | The map roiHeight. <br> Default: `outputHeight`  |
| `roiX`         | false | uint32_t    | The input roiX. <br> Default: `0`  |
| `roiY`         | false | uint32_t    | The input roiY. <br> Default: `0`  |
| `roiWidth`     | false | uint32_t    | The input roiWidth. <br> Default: `inputWidth`    |
| `roiHeight`    | false | uint32_t    | The input roiHeight. <br> Default: `inputHeight`  |
| `mapXBufferId` | false | uint32_t    | The buffer id of X direction map table  |
| `mapYBufferId` | false | uint32_t    | The buffer id of Y direction map table  |
| `nodeId`    | true     | uint32_t    | The Node unique ID.    |
| `bufferIds` | false    | uint32_t[]  | List of buffer indices in `QCNodeInit::buffers`  |
| `globalBufferIdMap` | false | object[] | Mapping of buffer names to buffer indices in `QCFrameDescriptorNodeIfs`. <br>Each object contains:<br> - `name` (string)<br> - `id` (uint32_t)   |
| `deRegisterAllBuffersWhenStop` | false | bool     | Flag to deregister all buffers when stopped      <br>Default: `false` |

- Example Configurations

  - two UYV2 inputs remap to RGB output with undistortion CPU pipeline
    ```json
    {
        "static":
        {
            "bufferIds":[0,1,2,3,4],
            "id":0,
            "inputs":
            [
                {
                    "inputFormat":"uyvy",
                    "inputHeight":1024,
                    "inputWidth":1920,
                    "roiHeight":1024,
                    "roiWidth":1920,
                    "roiX":0,
                    "roiY":0,
                    "mapHeight":800,
                    "mapWidth":1152,
                    "mapXBufferId":2,
                    "mapYBufferId":3
                },
                {
                    "inputFormat":"uyvy",
                    "inputHeight":1024,
                    "inputWidth":1920,
                    "roiHeight":1024,
                    "roiWidth":1920,
                    "roiX":0,
                    "roiY":0,
                    "mapHeight":800,
                    "mapWidth":1152,
                    "mapXBufferId":3,
                    "mapYBufferId":4
                }
            ],
            "name":"REMAP",
            "outputFormat":"rgb",
            "outputHeight":800,
            "outputWidth":1152,
            "processorType":"cpu",
            "bEnableUndistortion":true,
            "bEnableNormalize":false
        }
    }
    ```

    - two UYV2 inputs remap to RGB output with normalization HTP pipeline
    ```json
    {
        "static":
        {
            "bufferIds":[0,1,2],
            "id":0,
            "inputs":
            [
                {
                    "inputFormat":"uyvy",
                    "inputHeight":1024,
                    "inputWidth":1920,
                    "roiHeight":1024,
                    "roiWidth":1920,
                    "roiX":0,
                    "roiY":0,
                    "mapHeight":800,
                    "mapWidth":1152
                },
                {
                    "inputFormat":"uyvy",
                    "inputHeight":1024,
                    "inputWidth":1920,
                    "roiHeight":1024,
                    "roiWidth":1920,
                    "roiX":0,
                    "roiY":0,
                    "mapHeight":800,
                    "mapWidth":1152
                }
            ],
            "name":"REMAP",
            "outputFormat":"rgb",
            "outputHeight":800,
            "outputWidth":1152,
            "processorType":"htp0",
            "bEnableUndistortion":false,
            "bEnableNormalize":true,
            "RSub":123.675,
            "RMul":1.0/58.395,
            "RAdd":0.0,
            "GSub":116.28,
            "GMul":1.0/57.12,
            "GAdd":0.0,
            "BSub":103.53,
            "BMul":1.0/57.375,
            "BAdd":0.0
        }
    }
    ```

Refer to [Remap gtest](../tests/unit_test/Node/Remap/gtest_NodeRemap.cpp) for more details.

# 3. Remap APIs 

- [Remap::Initialize](../include/QC/Node/Remap.hpp#L249) Initialize Remap node

- [Remap::GetConfigurationIfs](../include/QC/Node/Remap.hpp#L255) Get Remap configuration interfaces

- [Remap::GetMonitoringIfs](../include/QC/Node/Remap.hpp#L261) Get Remap monitoring interfaces

- [Remap::Start](../include/QC/Node/Remap.hpp#L267) Start the Remap node

- [Remap::ProcessFrameDescriptor](../include/QC/Node/Remap.hpp#L282) Execute Remap node with input and output buffers

- [Remap::Stop](../include/QC/Node/Remap.hpp#L288) Stop the Remap node

- [Remap::DeInitialize](../include/QC/Node/Remap.hpp#L294) Deinit the Remap node

# 4. Typical Remap API Usage Examples

## 4.1 Set Configurations

The remap configuration parameters of 2 different input images can be set as following example:
```c++
    DataTree dt;
    dt.Set<std::string>( "static.name", "REMAP" );
    dt.Set<uint32_t>( "static.id", 0 );
    dt.Set<uint32_t>( "static.outputWidth", 64 );
    dt.Set<uint32_t>( "static.outputHeight", 64 );
    dt.SetImageFormat( "static.outputFormat", QC_IMAGE_FORMAT_RGB888 );
    dt.Set<bool>( "static.bEnableUndistortion", false );
    dt.Set<bool>( "static.bEnableUndistortion", false );
    dt.SetProcessorType( "static.processorType", QC_PROCESSOR_HTP0 );
    std::vector<DataTree> inputDts;
    for ( int i = 0; i < 2; i++ )
    {
        DataTree inputDt;
        inputDt.Set<uint32_t>( "inputWidth", 128 );
        inputDt.Set<uint32_t>( "inputHeight", 128 );
        inputDt.SetImageFormat( "inputFormat", QC_IMAGE_FORMAT_UYVY );
        inputDt.Set<uint32_t>( "roiX", 0 );
        inputDt.Set<uint32_t>( "roiY", 0 );
        inputDt.Set<uint32_t>( "roiWidth", 64 );
        inputDt.Set<uint32_t>( "roiHeight", 64 );
        inputDt.Set<uint32_t>( "mapWidth", 64 );
        inputDt.Set<uint32_t>( "mapHeight", 64 );
        inputDts.push_back( inputDt );
    }
    dt.Set( "static.inputs", inputDts );
    QCNodeInit_t config = { dt.Dump() };
```
The relationship of input, map, ROI, output scales are showed in following picture. The mapWidth must not be larger than inputWidth and the mapHeight must not be larger than inputHeight. The ROI.width+ROI.x must not be larger than mapWidth and the ROI.height+ROI.y must not be larger than mapHeight. The ROI.width must be equal to outputWidth and the ROI.height must be equal to output.height.
![remap-image](./images/remap-image.jpg)

If bEnableUndistortion is set to true, user can do undistortion or lens distortion correction for fisheye type camera by using the calibrated mapping table mapX and mapY. The mapping table mapX and mapY are floating point matrixs, each element is the column/row coordinate of the mapped location in the source image. The following example show how to set a map table with linear resize. 

## 4.2 API Call Flow

The typical call flow of a QC Remap pipeline is showed as following example:
```c++
    QCStatus_e ret;
    QCNodeIfs *pRemap = new QC::Node::Remap();
    BufferManager bufMgr( { "MANAGER", QC_NODE_TYPE_FADAS_REMAP, 0 } );
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

    ret = pRemap->Initialize( config );

    ret = pRemap->Start();

    ret = pRemap->ProcessFrameDescriptor( frameDesc );

    ret = pRemap->Stop();

    ret = pRemap->DeInitialize();
```
Generally, user should call Init API once at the beginning of the pipeline and call Deinit API once at the ending of the pipeline.

## 4.3 Supported pipelines

The supported remap pipelines for different input/output image format on each processor are listed below. In which Y means supported, N means unsupported. And norm means pipeline with normalization, corresponding to bEnableNormalize = true in the configuration parameters. Note that the NV12 input pipelines of each processor are only supported with internal engineering fadas libraries, and only verified in specific QNX meta build(Snapdragon_Auto.HQX.4.5.6.0.1.r1-00010).

| Pipeline         | DSP processor | CPU processor | GPU processor |
|------------------|---------------|---------------|---------------|
| RGB  to RGB      |     Y         |     Y         |     Y         |
| RGB  to RGB norm |     N         |     N         |     N         |
| UYVY to RGB      |     Y         |     Y         |     Y         |
| UYVY to RGB norm |     Y         |     Y         |     Y         |
| UYVY to BGR      |     Y         |     N         |     Y         |
| UYVY to BGR norm |     N         |     N         |     N         |
| NV12 to RGB      |     N         |     Y         |     Y         |
| NV12 to RGB norm |     N         |     Y         |     Y         |
| NV12 to BGR      |     Y         |     N         |     Y         |
| NV12 to BGR norm |     N         |     N         |     N         |
| NV12 UBWC to BGR |     N         |     N         |     Y         |

# 5. References

- [gtest_Remap](../tests/unit_test/Node/Remap/gtest_Remap.cpp)
- [SampleRemap](../tests/sample/source/SampleRemap.cpp)
- [FastADAS Remap](https://docs.qualcomm.com/bundle/publicresource/topics/80-63309-1/remap.html)