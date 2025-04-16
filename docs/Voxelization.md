*Menu*:
- [1. Voxelization overview](#1-voxelization-overview)
- [2. Voxelization Data Structures](#2-voxelization-data-structures)
- [3. Voxelization APIs](#3-voxelization-apis)
- [4. Voxelization Examples](#4-voxelization-examples)
  - [4.1 Voxelization initialization](#41-voxelization-initialization)
  - [4.2 Voxelization execution](#42-voxelization-execution)

# 1. Voxelization overview

The Component Voxelization is a preprocessing that convert pointcloud to pillars according to the definition in paper [PointPillars: Fast Encoders for Object Detection from Point Clouds](https://arxiv.org/pdf/1812.05784). The raw point cloud is transformed into a stacked pillar tensor and a pillar index tensor.

And this Component Voxelization is based on [FastADAS FadasVM library](https://developer.qualcomm.com/sites/default/files/docs/adas-sdk/api/group__vm__pp.html).

# 2. Voxelization Data Structures

- [Voxelization_Config_t](../include/QC/component/Voxelization.hpp#L43)

# 3. Voxelization APIs

- [Init](../include/QC/component/Voxelization.hpp#L65)
- [RegisterBuffers](../include/QC/component/Voxelization.hpp#L79)
- [Start](../include/QC/component/Voxelization.hpp#L86)
- [Execute](../include/QC/component/Voxelization.hpp#L99)
- [Stop](../include/QC/component/Voxelization.hpp#L107)
- [DeRegisterBuffers](../include/QC/component/Voxelization.hpp#L119)
- [Deinit](../include/QC/component/Voxelization.hpp#L125)

# 4. Voxelization Examples

## 4.1 Voxelization initialization

The [gtest plrPreConfig0](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L15) and [gtest plrPreConfig1](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L33) are 2 typical Voxelization configuration examples according to the pointpilalr model configuration.

And the plrPreConfig0 is for the [OpenPCDet default pointpillar config](https://github.com/open-mmlab/OpenPCDet/blob/master/tools/cfgs/kitti_models/pointpillar.yaml).

```yaml
CLASS_NAMES: ['Car', 'Pedestrian', 'Cyclist']

DATA_CONFIG: 
    _BASE_CONFIG_: cfgs/dataset_configs/kitti_dataset.yaml
    POINT_CLOUD_RANGE: [0, -39.68, -3, 69.12, 39.68, 1]
    DATA_PROCESSOR:
        - NAME: mask_points_and_boxes_outside_range
          REMOVE_OUTSIDE_BOXES: True

        - NAME: shuffle_points
          SHUFFLE_ENABLED: {
            'train': True,
            'test': False
          }

        - NAME: transform_points_to_voxels
          VOXEL_SIZE: [0.16, 0.16, 4]
          MAX_POINTS_PER_VOXEL: 32
...
```

```c++
static Voxelization_Config_t plrPreConfig0 = {
        QC_PROCESSOR_HTP0,
        0.16,
        0.16,
        4.0, /* pillar size: x, y, z */
        0.0,
        -39.68,
        -3.0, /* min Range, x, y, z */
        69.12,
        39.68,
        1.0,    /* max Range, x, y, z */
        300000, /* maxNumInPts */
        4,      /* numInFeatureDim*/
        12000,  /* maxNumPlrs */
        32,     /* maxNumPtsPerPlr */
        10,     /* numOutFeatureDim */
};

Voxelization plrPre;
QCStatus_e ret;

ret = plrPre.Init( "PLRPRE0", &plrPreConfig0, LOGGER_LEVEL_INFO );

ret = plrPre.Start();
```

## 4.2 Voxelization execution

The [SANITY_Voxelization](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L203) gtest code is a good example. Below is a copy of it.

After initializing the component voxelization, allocate memory for the input and output tensor buffers.

```c++
    QCTensorProps_t inPtsTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { plrPreConfig0.maxNumInPts, plrPreConfig0.numInFeatureDim, 0 },
            2,
    };
    QCTensorProps_t outPlrsTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { plrPreConfig0.maxNumPlrs, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
            2,
    };
    QCTensorProps_t outFeatureTsProp = {
            QC_TENSOR_TYPE_FLOAT_32,
            { plrPreConfig0.maxNumPlrs, plrPreConfig0.maxNumPtsPerPlr, plrPreConfig0.numOutFeatureDim, 0 },
            3,
    };

    QCSharedBuffer_t inPts;
    QCSharedBuffer_t outPlrs;
    QCSharedBuffer_t outFeature;

    /* Allocate input and output tensors */
    ret = inPts.Allocate( &inPtsTsProp );
    /* Fill in the inPts with pointcloud data */
    ret = outPlrs.Allocate( &outPlrsTsProp );
    ret = outFeature.Allocate( &outFeatureTsProp );

    /* do executre */
    ret = plrPre.Execute( &inPts, &outPlrs, &outFeature );
```

The [SamplePlrPre](../tests/sample/source/SamplePlrPre.cpp#L158) is an end-to-end pipeline demo that demonstrates how to call the Execute API to create pillars from input point clouds