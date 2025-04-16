*Menu*:
- [1. PostCenterPoint overview](#1-postcenterpoint-overview)
- [2. PostCenterPoint Data Structures](#2-postcenterpoint-data-structures)
- [3. PostCenterPoint APIs](#3-postcenterpoint-apis)
- [4. PostCenterPoint Examples](#4-postcenterpoint-examples)
  - [4.1 PostCenterPoint initialization](#41-postcenterpoint-initialization)
    - [4.1.1 How to configure the `filterParams`](#411-how-to-configure-the-filterparams)
  - [4.2 PostCenterPoint execution](#42-postcenterpoint-execution)

# 1. PostCenterPoint overview

The Component PostCenterPoint is a postprocessing that extracts and filters bounding boxes from the center point network output according to the definition in paper [PointPillars: Fast Encoders for Object Detection from Point Clouds](https://arxiv.org/pdf/1812.05784).

And this Component PostCenterPoint is based on [FastADAS FadasVM library](https://developer.qualcomm.com/sites/default/files/docs/adas-sdk/api/group__vm__bb.html).

# 2. PostCenterPoint Data Structures

- [PostCenterPoint_3DBBoxFilterParams_t](../include/QC/component/PostCenterPoint.hpp#L34)
- [PostCenterPoint_Config_t](../include/QC/component/PostCenterPoint.hpp#L68)
- [PostCenterPoint_Object3D_t](../include/QC/component/PostCenterPoint.hpp#L86)

# 3. PostCenterPoint APIs

- [Init](../include/QC/component/PostCenterPoint.hpp#L108)
- [RegisterBuffers](../include/QC/component/PostCenterPoint.hpp#L122)
- [Start](../include/QC/component/PostCenterPoint.hpp#L129)
- [Execute](../include/QC/component/PostCenterPoint.hpp#L157)
- [Stop](../include/QC/component/PostCenterPoint.hpp#L168)
- [DeRegisterBuffers](../include/QC/component/PostCenterPoint.hpp#L180)
- [Deinit](../include/QC/component/PostCenterPoint.hpp#L186)

# 4. PostCenterPoint Examples

## 4.1 PostCenterPoint initialization

The [gtest plrPostConfig0](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L51) and [gtest plrPostConfig1](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L71) are 2 typical PostCenterPoint configuration examples according to the pointpilalr model configuration.

And the plrPostConfig0 is for the [OpenPCDet default pointpillar config](https://github.com/open-mmlab/OpenPCDet/blob/master/tools/cfgs/kitti_models/pointpillar.yaml).

```c++
static PostCenterPoint_Config_t plrPostConfig0 = {
        QC_PROCESSOR_HTP0,
        0.16,
        0.16, /* pillar size: x, y */
        0.0,
        -39.68, /* min Range, x, y */
        69.12,
        39.68,                                        /* max Range, x, y */
        3,                                            /* numClass */
        300000,                                       /* maxNumInPts */
        4,                                            /* numInFeatureDim*/
        500,                                          /* maxNumDetOut */
        2,                                            /* stride */
        0.1,                                          /* threshScore */
        0.1,                                          /* threshIOU */
        { 0, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, nullptr }, /* filterParams */
        true,                                         /* bMapPtsToBBox */
        false,                                        /* bBBoxFilter */
};

PostCenterPoint plrPost;
QCStatus_e ret;

ret = plrPost.Init( "PLRPOST0", &config, LOGGER_LEVEL_INFO );
ret = plrPost.Start();
```

### 4.1.1 How to configure the `filterParams`

The [filterParams](../include/QC/component/PostCenterPoint.hpp#L58) are only valid and will be used if both [bMapPtsToBBox](../include/QC/component/PostCenterPoint.hpp#L61) and [bBBoxFilter](../include/QC/component/PostCenterPoint.hpp#L67) are true.

When `bMapPtsToBBox` was true, the PostCenterPoint checks the input point cloud for each detected 3D bounding box. It calculates the mean values of x/y/z coordinates and intensities for all points inside this bounding box. However, this process can be time-consuming. In real-world scenarios, ADAS applications may only require this mean calculation for specific classes and limit it to the first several bounding boxes with high scores.

As an example configuration, consider using filterParams to optimize performance:

```c
PostCenterPoint_Config_t config = plrPostConfig0;
PostCenterPoint plrPost;
QCStatus_e ret;
bool labelSelect[3] = { true, false, false }; // only do the mean calcuation for the class: Car
// do the mean calcuation for the first 10 bounding box with high scores.
PostCenterPoint_3DBBoxFilterParams_t filterParams = { 10,
                                                      config.minXRange,
                                                      config.minYRange,
                                                      plrPreConfig0.minZRange,
                                                      config.maxXRange,
                                                      config.maxYRange,
                                                      plrPreConfig0.maxZRange,
                                                      labelSelect };
config.filterParams = filterParams;
config.bBBoxFilter = true;
```

## 4.2 PostCenterPoint execution

The [SANITY_PostCenterPoint](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L336) gtest code is a good example. It loads the related input and output buffers from a raw file by calling APIs [LoadRaw](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L121) or [LoadPoints](../tests/unit_test/components/PointPillar/gtest_PointPillar.cpp#L103). Additionally, this code demonstrates how to decode the detection output buffer. Below is a copy of it.

```c++
    PostCenterPoint_Object3D_t *pObj = (PostCenterPoint_Object3D_t *) det.data();
    for ( uint32_t i = 0; i < det.tensorProps.dims[0]; i++ )
    {
        printf( "[%d] class=%d score=%.3f bbox=[%.3f %.3f %.3f %.3f %.3f %.3f] "
                "theta=%.3f, mean=[%.3f %.3f %.3f %.3f]x%u\n",
                i, pObj->label, pObj->score, pObj->x, pObj->y, pObj->z, pObj->length, pObj->width,
                pObj->height, pObj->theta, pObj->meanPtX, pObj->meanPtY, pObj->meanPtZ,
                pObj->meanIntensity, pObj->numPts );
        pObj++;
    }
```

And the [SamplePlrPost](../tests/sample/source/SamplePlrPost.cpp#L228) provides an end-to-end pipeline demo that illustrates how to call the Execute API to extract bounding boxes.

