*Menu*:
- [1. DepthFromStereo overview](#1-depthfromstereo-overview)
- [2. DepthFromStereo Data Structures](#2-depthfromstereo-data-structures)
- [3. DepthFromStereo APIs](#3-depthfromstereo-apis)
- [4. DepthFromStereo Examples](#4-depthfromstereo-examples)
  - [4.1 DepthFromStereo initialization](#41-depthfromstereo-initialization)
  - [4.2 DepthFromStereo Input/Output buffer allocation](#42-depthfromstereo-inputoutput-buffer-allocation)
  - [4.3 Register buffer to DepthFromStereo](#43-register-buffer-to-depthfromstereo)
  - [4.4 Start and Run](#44-start-and-run)
  - [4.5 Stop and Deinit](#45-stop-and-deinit)
  - [4.6 QC E2E sample](#46-qc-e2e-sample)

# 1. DepthFromStereo overview

Depth From Stereo (DFS) is a technique for estimating the depth of a scene by comparing two images of the scene taken from slightly different viewpoints. This is achieved by analyzing the disparities or differences in the positions of corresponding points in the two images, which are caused by the geometric relationship between the camera positions and the scene.

This DepthFromStereo component offers a user-friendly API to leverage the EVA hardware accelerator for computing DFS disparities using two viewpoint images.

# 2. DepthFromStereo Data Structures

- [DepthFromStereo_Config_t](../include/QC/component/DepthFromStereo.hpp#L55)

This configuration data structure comes with a default initializer that sets up its members with suggested default values. Refer to [DepthFromStereo_Config::DepthFromStereo_Config](../source/components/DepthFromStereo/DepthFromStereo.cpp#L41). For startup, you only need to update the relevant information for the input image.

```c
    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 720;
```

# 3. DepthFromStereo APIs

- [Init](../include/QC/component/DepthFromStereo.hpp#L74)
- [RegisterBuffers](../include/QC/component/DepthFromStereo.hpp#L85)
- [Start](../include/QC/component/DepthFromStereo.hpp#L91)
- [Execute](../include/QC/component/DepthFromStereo.hpp#L101)
- [Stop](../include/QC/component/DepthFromStereo.hpp#L110)
- [DeRegisterBuffers](../include/QC/component/DepthFromStereo.hpp#L120)
- [Deinit](../include/QC/component/DepthFromStereo.hpp#L126)


# 4. DepthFromStereo Examples

## 4.1 DepthFromStereo initialization

```c
    DepthFromStereo dfs;

    DepthFromStereo_Config_t config;
    config.width = 1280;
    config.height = 720;

    ret = dfs.Init( "DFS0", &config );
```

## 4.2 DepthFromStereo Input/Output buffer allocation

```c
    // below allocate a buffer to hold image
    QCSharedBuffer_t priImg;
    QCSharedBuffer_t auxImg;
    ret = refImg.Allocate( 1280, 720, QC_IMAGE_FORMAT_NV12 );
    ret = curImg.Allocate( 1280, 720, QC_IMAGE_FORMAT_NV12 );

    // define the output tensor properties that used to allocate the output tensor buffer
    QCTensorProps_t dispMapTsProp = {
            QC_TENSOR_TYPE_UINT_16,
            { 1, ALIGN_S( config.height, 2 ), ALIGN_S( config.width, 128 ), 1 },
            4 };
    QCTensorProps_t confMapTsProp = {
            QC_TENSOR_TYPE_UINT_8,
            { 1, ALIGN_S( config.height, 2 ), ALIGN_S( config.width, 128 ), 1 },
            4 };
    QCSharedBuffer_t dispMap;
    QCSharedBuffer_t confMap;
    // do allocate buffer to hold DFS output
    ret = dispMap.Allocate( &dispMapTsProp );
    ret = confMap.Allocate( &confMapTsProp );
```

## 4.3 Register buffer to DepthFromStereo

This step is optional, if it was not done, the Execute API will help to do the buffer register only once.

```c
    ret = dfs.RegisterBuffers( &priImg, 1 );
    ret = dfs.RegisterBuffers( &auxImg, 1 );
    ret = dfs.RegisterBuffers( &dispMap, 1 );
    ret = dfs.RegisterBuffers( &confMap, 1 );
```

## 4.4 Start and Run

```c
    ret = dfs.Start();
    ret = dfs.Execute( &priImg, &auxImg, &dispMap, &confMap );
```

## 4.5 Stop and Deinit

```c
    ret = dfs.Stop();

    ret = dfs.DeRegisterBuffers( &priImg, 1 );
    ret = dfs.DeRegisterBuffers( &auxImg, 1 );
    ret = dfs.DeRegisterBuffers( &dispMap, 1 );
    ret = dfs.DeRegisterBuffers( &confMap, 1 );

    ret = dfs.Deinit();
```

## 4.6 QC E2E sample

The [SampleDepthFromStereo](../tests/sample/source/SampleDepthFromStereo.cpp#L145) and [SampleDepthFromStereoViz](../tests/sample/source/SampleDepthFromStereoViz.cpp#L212) is an end-to-end pipeline demo that demonstrates how to use the DepthFromStereo component. And the [SampleDepthFromStereoViz](../tests/sample/source/SampleDepthFromStereoViz.cpp#L212) demonstrates how to decode the DFS output.
