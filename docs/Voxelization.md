*Menu*:
- [1. Voxelization overview](#1-voxelization-overview)
- [2. Voxelization Configuraion](#2-voxelization-configuraion)
  - [2.1 Voxelization Static JSON Configuration](#21-voxelization-static-json-configuration)
- [3. Voxelization APIs](#3-voxelization-apis)
  - [3.1 QCNode Voxelization APIS](#31-qcnode-voxelization-apis)
  - [3.2 QCNode Configuration Interfaces](#32-qcnode-configuration-interfaces)
- [4. Voxelization Examples](#4-voxelization-examples)
  - [4.1 Voxelization Config initialization](#41-voxelization-config-initialization)
  - [4.2 Buffer allocation and data loading](#42-buffer-allocation-and-data-loading)
  - [4.3 Voxelization execution](#43-voxelization-execution)
  - [4.4 Buffer Free](#44-buffer-free)
- [5. References](#5-references)

# 1. Voxelization overview

The Component Voxelization is a preprocessing that convert pointcloud to pillars according to the definition in paper [PointPillars: Fast Encoders for Object Detection from Point Clouds](https://arxiv.org/pdf/1812.05784). The raw point cloud is transformed into a stacked pillar tensor and a pillar index tensor.

And this Component Voxelization is based on [FastADAS FadasVM library](https://developer.qualcomm.com/sites/default/files/docs/adas-sdk/api/group__vm__pp.html).

# 2. Voxelization Configuraion

## 2.1 Voxelization Static JSON Configuration

| Parameter  | Required  | Type        | Description            |
|------------|-----------|-------------|------------------------|
| `name`     | true      | string      | The Node unique name.  |
| `id`       | true      | uint32_t    | The Node unique ID.    |
| `processorType` | false | string     | The processor type. <br> Options: `htp0`, `htp1`, `cpu`, `gpu` <br> Default: `htp0` |
| `Xsize`    | true      | float       | Pillar size in X direction in meters.    |
| `Ysize`    | true      | float       | Pillar size in Y direction in meters.    |
| `Zsize`    | true      | float       | Pillar size in Z direction in meters.    |
| `Xmin`     | true      | float       | Minimum range value in X direction.      |
| `Ymin`     | true      | float       | Minimum range value in Y direction.      |
| `Zmin`     | true      | float       | Minimum range value in Z direction.      |
| `Xmax`     | true      | float       | Maximum range value in X direction.      |
| `Ymax`     | true      | float       | Maximum range value in Y direction.      |
| `Zmax`     | true      | float       | Maximum range value in Z direction.      |
| `maxPointNum`           | true      | uint32_t       | Maximum number of point pillars that can be created.      |
| `maxPointNumPerPlr`     | true      | uint32_t       | Maximum number of points to map to each pillar.           |
| `inputMode`             | true      | string         | Voxelization input pointclouds type. <br> Options: `xyzr`, `xyzrt` <br> Default: `xyzr`       |
| `outputFeatureDimNum`   | true      | uint32_t       | Number of features for each point in output point pillars.           |
| `outputPlrBufferIds`        | true      | uint32_t[]     | A list of uint32_t values representing the indices of output pillar buffers in QCNodeInit::buffers.          |
| `outputFeatureBufferIds`    | true      | uint32_t[]     | A list of uint32_t values representing the indices of output pillar buffers in QCNodeInit::buffers.      |
| `plrPointsBufferId`         | true      | uint32_t       | The index of buffer for maximal pillar point number in QCNodeInit::buffers.      |
| `coordToPlrIdxBufferId`     | true      | uint32_t       | The index of buffer to store coordinate to pillar point transform indices in QCNodeInit::buffers.      |
| `globalBufferIdMap`     | false | object[] | Mapping of buffer names to buffer indices in `QCFrameDescriptorNodeIfs`. <br>Each object contains:<br> - `name` (string)<br> - `id` (uint32_t)   |
| `deRegisterAllBuffersWhenStop` | false | bool     | Flag to deregister all buffers when stopped      <br>Default: `false` |

- Example Configurations
  - XYZR mode 
    ```json
    {
      "static": 
          {
              "name": "voxelization",
              "id": 0,
              "processorType": "cpu",
              "Xsize": 0.16,
              "Ysize": 0.16,
              "Zsize": 4.0,
              "Xmin": 0.0,
              "Ymin": -39.68,
              "Zmin": -3.0,
              "Xmax": 69.12,
              "Ymax": 39.68,
              "Zmax": 1.0,
              "maxPointNum": 300000,
              "maxPlrNum": 12000,
              "maxPointNumPerPlr": 32,
              "inputMode": "xyzr",
              "outputFeatureDimNum": 10,
              "outputPlrBufferIds": [0, 1, 2, 3],
              "outputFeatureBufferIds": [4, 5, 6, 7],
              "plrPointsBufferId": 8,
              "coordToPlrIdxBufferId": 9
          }
    }
    ```

# 3. Voxelization APIs

## 3.1 QCNode Voxelization APIS

- [Voxelization::Initialize](../include/QC/Node/Voxelization.hpp#L236)
- [Voxelization::Start](../include/QC/Node/Voxelization.hpp#L254)
- [Voxelization::ProcessFrameDescriptor](../include/QC/Node/Voxelization.hpp#L266)
- [Voxelization::Stop](../include/QC/Node/Voxelization.hpp#L272)
- [Voxelization::DeInitialize](../include/QC/Node/Voxelization.hpp#L278)
- [Voxelization::DeInitialize](../include/QC/Node/Voxelization.hpp#L278)
- [Voxelization::GetConfigurationIfs](../include/QC/Node/Voxelization.hpp#L242)
- [Voxelization::GetMonitoringIfs](../include/QC/Node/Voxelization.hpp#L248)

## 3.2 QCNode Configuration Interfaces

- [VoxelizationConfig::GetOptions](../include/QC/Node/Voxelization.hpp#L116) Get Configuration Options
  - Use this API to get the configuration options.
    - Below was a example output for Voxelization XYZR mode:
      ```json
      {
        "static":
        {
          "name":"voxelization",
          "id":0,
          "processorType":"gpu",
          "Xmax":69.12,
          "Xmin":0.0,
          "Xsize":0.16,
          "Ymax":39.68,
          "Ymin":-39.68,
          "Ysize":0.16,
          "Zmax":1.0,"
          Zmin":-3.0,
          "Zsize":4.0,
          "inputMode":"xyzr",
          "maxPlrNum":12000,
          "maxPointNum":300000,
          "maxPointNumPerPlr":32,
          "outputFeatureDimNum":10,
          "outputPlrBufferIds":[0,1,2,3],
          "outputFeatureBufferIds":[4,5,6,7],
          "plrPointsBufferId":8,
          "coordToPlrIdxBufferId":9
        }
      }
      ```

# 4. Voxelization Examples

## 4.1 Voxelization Config initialization
```c++
#include "QC/Node/Voxelization.hpp"
#include "QC/sample/BufferManager.hpp"
#include "gtest/gtest.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

using namespace QC;
using namespace QC::Node;
using namespace QC::sample;

#define EXPAND_JSON( ... ) #__VA_ARGS__

extern const size_t VOXELIZATION_PILLAR_COORDS_DIM;

std::string g_Config_XYZR = EXPAND_JSON( {
    "static": {
        "name": "voxelization",
        "id": 0,
        "processorType": "cpu",
        "Xsize": 0.16,
        "Ysize": 0.16,
        "Zsize": 4.0,
        "Xmin": 0.0,
        "Ymin": -39.68,
        "Zmin": -3.0,
        "Xmax": 69.12,
        "Ymax": 39.68,
        "Zmax": 1.0,
        "maxPointNum": 300000,
        "maxPlrNum": 12000,
        "maxPointNumPerPlr": 32,
        "inputMode": "xyzr",
        "outputFeatureDimNum": 10,
        "outputPlrBufferIds": [0, 1, 2, 3],
        "outputFeatureBufferIds": [4, 5, 6, 7],
        "plrPointsBufferId": 8,
        "coordToPlrIdxBufferId": 9
    }
} );

void Init_VoxelizationConfig( std::string &jsonStr, std::string &processorType,
                        std::string &inputMode, const char *pcdFile = nullptr )
{
    QCStatus_e ret;
    DataTree dt;
    DataTree staticCfg;
    QCNodeInit_t config;
    std::string errors;
    uint32_t globalIdx = 0;

    float Xsize = 0;
    float Ysize = 0;
    float Zsize = 0;
    float Xmin = 0;
    float Ymin = 0;
    float Zmin = 0;
    float Xmax = 0;
    float Ymax = 0;
    float Zmax = 0;
    uint32_t maxPointNum = 0;
    uint32_t maxPlrNum = 0;
    uint32_t maxPointNumPerPlr = 0;
    uint32_t inputFeatureDimNum = 0;
    uint32_t outputFeatureDimNum = 0;
    uint32_t plrPointsBufferId = 0;
    uint32_t coordToPlrIdxBufferId = 0;
    uint32_t gridXSize = 0;
    uint32_t gridYSize = 0;
    QCProcessorType_e processor;

    std::vector<uint32_t> outputPlrBufferIds;
    std::vector<uint32_t> outputFeatureBufferIds;

    QC::Node::Voxelization voxel;

    ret = dt.Load( jsonStr, errors );
    if ( QC_STATUS_OK == ret )
    {
        dt.Set<std::string>( "static.processorType", processorType );
        dt.Set<std::string>( "static.inputMode", inputMode );
        ret = dt.Get( "static", staticCfg );
    }
    else
    {
        std::cout << "Get config error: " << errors << std::endl;
    }

    if ( QC_STATUS_OK == ret )
    {
        Xsize = staticCfg.Get<float>( "Xsize", 0 );
        Ysize = staticCfg.Get<float>( "Ysize", 0 );
        Zsize = staticCfg.Get<float>( "Zsize", 0 );
        Xmin = staticCfg.Get<float>( "Xmin", 0 );
        Ymin = staticCfg.Get<float>( "Ymin", 0 );
        Zmin = staticCfg.Get<float>( "Zmin", 0 );
        Xmax = staticCfg.Get<float>( "Xmax", 0 );
        Ymax = staticCfg.Get<float>( "Ymax", 0 );
        Zmax = staticCfg.Get<float>( "Zmax", 0 );
        maxPointNum = staticCfg.Get<uint32_t>( "maxPointNum", 0 );
        maxPlrNum = staticCfg.Get<uint32_t>( "maxPlrNum", 0 );
        maxPointNumPerPlr = staticCfg.Get<uint32_t>( "maxPointNumPerPlr", 0 );
        outputFeatureDimNum = staticCfg.Get<uint32_t>( "outputFeatureDimNum", 0 );
        outputPlrBufferIds =
                staticCfg.Get<uint32_t>( "outputPlrBufferIds", std::vector<uint32_t>{} );
        outputFeatureBufferIds =
                staticCfg.Get<uint32_t>( "outputFeatureBufferIds", std::vector<uint32_t>{} );
        plrPointsBufferId = staticCfg.Get<uint32_t>( "plrPointsBufferId", 0 );
        coordToPlrIdxBufferId = staticCfg.Get<uint32_t>( "coordToPlrIdxBufferId", 0 );
        processor = staticCfg.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );
    }

    if ( QC_STATUS_OK == ret )
    {
        config = { dt.Dump() };
        std::cout << "config: " << config.config << std::endl;
    }
}
```

## 4.2 Buffer allocation and data loading
```c++
static void LoadPoints( void *pData, uint32_t size, uint32_t &numPts, const char *pcdFile )
{
    FILE *pFile = fopen( pcdFile, "rb" );

    fseek( pFile, 0, SEEK_END );
    int length = ftell( pFile );
    numPts = length / 16;
    fseek( pFile, 0, SEEK_SET );
    int r = fread( pData, 1, numPts * 16, pFile );
    printf( "load %u points from %s\n", numPts, pcdFile );
    fclose( pFile );
}

TensorDescriptor_t inputTensor;
TensorDescriptor_t outputPlrTensor;
TensorDescriptor_t outputFeatureTensor;
TensorDescriptor_t plrPointsTensor;
TensorDescriptor_t coordToPlrIdxTensor;

TensorProps_t inputTensorProp;
TensorProps_t outputPlrTensorProp;
TensorProps_t outputFeatureTensorProp;
TensorProps_t plrPointsTensorProp;
TensorProps_t coordToPlrIdxTensorProp;

BufferManager bufMgr = BufferManager( { "VOXEL", QC_NODE_TYPE_VOXEL, 0 } );

if ( inputMode == "xyzr" )
{
    inputFeatureDimNum = 4;
}
else if ( inputMode == "xyzrt" )
{
    inputFeatureDimNum = 5;
}

if ( ( maxPointNum > 0 ) && ( inputFeatureDimNum > 0 ) )
{
    inputTensorProp.tensorType = QC_TENSOR_TYPE_FLOAT_32;
    inputTensorProp.dims[0] = maxPointNum;
    inputTensorProp.dims[1] = inputFeatureDimNum;
    inputTensorProp.dims[2] = 0;
    inputTensorProp.numDims = 2;
}

if ( ( maxPlrNum > 0 ) && ( maxPointNumPerPlr > 0 ) && ( outputFeatureDimNum > 0 ) )
{
    if ( inputMode == "xyzrt" )
    {
        outputPlrTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
        outputPlrTensorProp.dims[0] = maxPlrNum;
        outputPlrTensorProp.dims[1] = 2;
        outputPlrTensorProp.dims[2] = 0;
        outputPlrTensorProp.numDims = 2;
    }
    else
    {
        outputPlrTensorProp.tensorType = QC_TENSOR_TYPE_FLOAT_32;
        outputPlrTensorProp.dims[0] = maxPlrNum;
        outputPlrTensorProp.dims[1] = VOXELIZATION_PILLAR_COORDS_DIM;
        outputPlrTensorProp.dims[2] = 0;
        outputPlrTensorProp.numDims = 2;
    }

    outputFeatureTensorProp.tensorType = QC_TENSOR_TYPE_FLOAT_32;
    outputFeatureTensorProp.dims[0] = maxPlrNum;
    outputFeatureTensorProp.dims[1] = maxPointNumPerPlr;
    outputFeatureTensorProp.dims[2] = outputFeatureDimNum;
    outputFeatureTensorProp.dims[3] = 0;
    outputFeatureTensorProp.numDims = 3;

    size_t gridXSize = ceil( ( Xmax - Xmin ) / Xsize );
    size_t gridYSize = ceil( ( Ymax - Ymin ) / Ysize );

    plrPointsTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
    plrPointsTensorProp.dims[0] = maxPlrNum + 1;
    plrPointsTensorProp.dims[1] = 0;
    plrPointsTensorProp.numDims = 1;

    coordToPlrIdxTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
    coordToPlrIdxTensorProp.dims[0] = (uint32_t) ( gridXSize * gridYSize * 2 );
    coordToPlrIdxTensorProp.dims[1] = 0;
    coordToPlrIdxTensorProp.numDims = 1;
}

const uint32_t inputBufferNum = 4;
const uint32_t outputPlrBufferNum = outputPlrBufferIds.size();
const uint32_t outputFeatureBufferNum = outputFeatureBufferIds.size();

TensorDescriptor_t inputTensors[inputBufferNum];
TensorDescriptor_t outputPlrTensors[outputPlrBufferNum];
TensorDescriptor_t outputFeatureTensors[outputFeatureBufferNum];

 QCSharedFrameDescriptorNode frameDesc( 3 );

for ( uint32_t i = 0; i < inputBufferNum; i++ )
{
    ret = bufMgr.Allocate( inputTensorProp, inputTensors[i] );

    uint32_t numPts = 0;
    if ( nullptr == pcdFile )
    {
        if ( nullptr == pcdFile )
        {
            std::cout << "point cloud file is not provided " << std::endl;
        }
    }
    else
    {
        LoadPoints( inputTensors[i].pBuf, inputTensors[i].size, numPts, pcdFile );
        std::cout << "using point cloud file: " << pcdFile << std::endl;
    }
    inputTensors[i].dims[0] = numPts;
}

for ( uint32_t i = 0; i < outputPlrBufferNum; i++ )
{
    ret = bufMgr.Allocate( outputPlrTensorProp, outputPlrTensors[i] );
    config.buffers.push_back( outputPlrTensors[i] );
}

for ( uint32_t i = 0; i < outputFeatureBufferNum; i++ )
{
    ret = bufMgr.Allocate( outputFeatureTensorProp, outputFeatureTensors[i] );
    config.buffers.push_back( outputFeatureTensors[i] );
}

if ( QC_PROCESSOR_GPU == processor )
{
    ret = bufMgr.Allocate( plrPointsTensorProp, plrPointsTensor );
    ret = bufMgr.Allocate( coordToPlrIdxTensorProp, coordToPlrIdxTensor );

    config.buffers.push_back( plrPointsTensor );
    config.buffers.push_back( coordToPlrIdxTensor );
}
```

## 4.3 Voxelization execution

```c++
ret = frameDesc.SetBuffer( 0, inputTensors[0] );
    
ret = frameDesc.SetBuffer( 1, outputPlrTensors[0] );

ret = frameDesc.SetBuffer( 2, outputFeatureTensors[0] );

ret = voxel.Initialize( config );

ret = voxel.Start();

ret = voxel.ProcessFrameDescriptor( frameDesc );

ret = voxel.Stop();

ret = voxel.DeInitialize();
```

## 4.4 Buffer Free
```c++
for ( uint32_t i = 0; i < inputBufferNum; i++ )
{
    ret = bufMgr.Free( inputTensors[i] );
}

for ( uint32_t i = 0; i < outputPlrBufferNum; i++ )
{
    ret = bufMgr.Free( outputPlrTensors[i] );
}

for ( uint32_t i = 0; i < outputFeatureBufferNum; i++ )
{
    ret = bufMgr.Free( outputFeatureTensors[i] );
}

if ( QC_PROCESSOR_GPU == processor )
{
    ret = bufMgr.Free( coordToPlrIdxTensor );
    ret = bufMgr.Free( plrPointsTensor );
}
```

# 5. References
- [gtest Voxelization](../tests/unit_test/Node/Voxelization/gtest_NodeVoxelization.cpp).
- [Sample Voxelization](../tests/sample/source/SamplePlrPre.cpp).
