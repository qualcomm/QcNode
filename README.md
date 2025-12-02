# Overview

The [QCNode(Qualcomm Compute Node)](./docs/QCNode.md) is Qualcomm ADAS hardware abstraction layer that provides user friendly APIs to access the Qualcomm ADAS hardware accelerator. It is ADAS service oriented to provide user friendly node and utils to cover the perception and compute vision related task.

# Documents

Refer to the [INDEX](./docs/index.md) for full documentation.

Below is a summary information of the current implemented nodes.

| QCNode |  QNX PVM  | HGY PVM  | Library  |   Processor     |  Features |
|-----------|------------|-------------|----------|-----------------|---------------------------------------|
| [Camera](./docs/Camera.md) | YES| YES | qcarcam | Camera | camera streaming |
| [Remap](./docs/remap.md)| YES | YES | FastADAS | CPU, HTP0, HTP1, GPU | Color Conversion, ROI Crop, Downscaling, Undistortion |
| [QNN](./docs/QNN.md) | YES| YES | QNN | CPU, GPU, HTP0, HTP1 | AI model inference |
| [VideoEncoder](./docs/videoencoder.md) | YES| YES | vidc | VPU | Encode image to video frame |
| [VideoDecoder](./docs/VideoDecoder.md) | YES| YES | vidc | VPU | Decode video frame to image |
| [Voxelization](./docs/Voxelization.md) | YES| YES | FastADAS, OpenCL| CPU, HTP0, HTP1, GPU | Generate pillars from point clouds |
| [CL2DFlex](./docs/CL2DFlex.md) | YES| Yes| OpenCL | GPU | Color Conversion, Resize, ROI Crop, Undistortion |
| [OpticalFlow](./docs/OpticalFlow.md) | YES | YES| SV | EVA | Optical flow computation |
| [DepthFromStereo](./docs/DepthFromStereo.md) | YES | YES| SV | EVA | Depth estimation from stereo images |

# How to build

For how to build the QCNode package manually, check this:
- HGY Linux: [How to build for HGY Linux](./docs/build-hgy-linux.md)
- HGY Ubuntu: [How to build for HGY Ubuntu](./docs/build-hgy-ubuntu.md)

# How to run

For how to run the QCNode package, check this [README](./scripts/launch/README.md).

For how to run the QCNode E2E perception pipeline sample, check this [README](./tests/sample/README.md).

# License

QCNode is licensed under the [BSD-3-clause License](https://spdx.org/licenses/BSD-3-Clause.html). See [LICENSE.txt](LICENSE.txt) for the full license text.