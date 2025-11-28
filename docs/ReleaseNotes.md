*Menu*:
- [1. Overview](#1-overview)
- [2. Feature Changes](#2-feature-changes)
  - [2.1 Version 2.0.0](#21-version-200)
- [3. Quality Improvements](#3-quality-improvements)
- [4. Verified Platforms](#4-verified-platforms)

# 1. Overview
The **QCNode** is the Qualcomm ADAS hardware abstraction layer that provides user‑friendly APIs to access the Qualcomm ADAS hardware accelerator. It is service‑oriented and designed to deliver easy‑to‑use components and utilities for perception and computer vision tasks.

- **QCNode Nodes** are high‑quality modules that can be directly used by customers with minimal effort.
- **QCNode Samples** demonstrate how to use QC components effectively, including memory zero‑copy and buffer lifecycle management.
- **Multi‑threaded Samples** showcase how to improve FPS throughput by leveraging parallel execution.
- Each **QCNode Node** provides essential logging (especially for errors) to simplify debugging.
- The **QCNode Logger** is customizable and can be easily integrated with customer ADAS application logging systems.

# 2. Feature Changes
## 2.1 Version 2.0.0
- Initial release of **QCNode v2.0.0**, introducing the upgrade of RideHal to the new unified Node API.
  - Included nodes:
    - **Camera**: Built on *qcarcam* to support camera streaming
    - **Remap**: Powered by *FastADAS* for color conversion, ROI cropping, downscaling, and undistortion
    - **QNN**: Based on *QNN SDK* for AI model inference
    - **Video Encoder**: Built on *vidc* to encode images into video frames
    - **Video Decoder**: Built on *vidc* to decode video frames into images
    - **Voxelization**: Uses *FastADAS* and *OpenCL* to generate pillars from point clouds
    - **CL2DFlex**: Powered by *OpenCL* for color conversion, ROI cropping, downscaling, and undistortion
    - **Optical Flow**: Based on *SV* to compute optical flow
    - **Depth From Stereo**: Based on *SV* to compute depth from stereo images


# 3. Quality Improvements

# 4. Verified Platforms
- QCNode is verified on Lemans device with the following meta build:
  - HQX QNX Snapdragon_Auto.HQX.4.5.6.0.1.r1
  - HGY Ubuntu Snapdragon_Auto.HGY.4.1.6.0.r1
  - HGY Linux Snapdragon_Auto.HGY.4.1.6.0.1.r1

- QCNode is verified on Nordy device with the following meta build:
  - HGY Linux SA8797P.HGY.5.1.7.0.r1
  - HQX QNX SA8797P.HQX.5.7.7.0.r1


