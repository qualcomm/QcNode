*Menu*:
- [1. Overview](#1-overview)
- [2. Feature Changes](#2-feature-changes)
  - [2.1 Version 1.9.0](#21-version-190)
  - [2.2 Version 1.8.0](#22-version-180)
  - [2.3 Version 1.7.1](#23-version-171)
- [3. Quality Improvements](#3-quality-improvements)
  - [3.1 Version 1.9.0](#31-version-190)
  - [3.2 Version 1.8.0](#32-version-180)
  - [3.3 Version 1.7.1](#33-version-171)
- [4. Verified Platforms](#4-verified-platforms)

# 1. Overview
The QC is Qualcomm ADAS hardware abstraction layer that provides user friendly APIs to access the Qualcomm ADAS hardware accelerator. It is ADAS service oriented to provide user friendly component and utils to cover the perception and compute vision related task.
- The QC Component with high quality that can be directly used by customer, and it is easy to be used.
- The QC Sample is a very good demo about how to use the QC Component and it clearly demonstrates about how to do memory zero copy and do the buffer life cycle management.
- The QC Sample with multi-threading to demonstrate that how to improve the FPS throughputs.
- The QC Component each provides necessary logs(especially for ERROR) for easy issue debug.
- The QC Logger can be customized and easily integrated with customer ADAS application log system.

# 2. Feature Changes
## 2.1 Version 1.9.0
- DFS: EVA memory register with bCache setup according to buffer flags
- VideoEncoder: Add sync frame sequence header option
- Sample: Enable rsm for HGY Linux
- FadasIface: Add fadas skel lib for variant v68/v73/v75
- SampleCamera: Use thread/queue to control frame request/release
## 2.2 Version 1.8.0
- CL2DFlex: Add perf priority to OpenCL Iface
- CL2DFlex: Add resize from NV12 to NV12 function to CL2DFlex component
- CL2DFlex: Add remap from NV12 to BGR pipeline and test cases for CL2DFlex
- CL2DFlex: Add remap from NV12 to RGB pipeline for CL2DFlex component
- Camera: Add session recovery support
- Sample: re-implement the DFS method to convert disparity to depth heatmap
## 2.3 Version 1.7.1
- QnnRuntime: Support QNN Async Execute feature
- Camera: HGY multi-client buffer import support
- CL2DFlex: Add convert NV12 UBWC to NV12 pipeline
- Camera: Add support of format TP10 UBWC
- Sample: Add support for image format NV12/P010
- Sample: Add DepthFromStereo Sample

# 3. Quality Improvements
## 3.1 Version 1.9.0
- Camera: add xml2 as a depend library
- TinyViz: unset DISPLAY to avoid SDL from using X11
- Build: Fix QCNode package not found warnings
- Sample: Fix QNN udo path string that use after free issue
## 3.2 Version 1.8.0
- CL2DFlex: Fix priority setting parameter issue for OpenCL Iface
- CL2DFlex: Modify OpenCL priority setting method to fit QNX platform
- CL2DFlex: Add CL2DFlex resize from NV12 to NV12 gtest
- QnnRuntime: Modify cmake files to fit QNN version 2.32.0
- VideoDecoder: Fix VideoDecoder double init issue
- VideoDecoder: Create gtest cases for VideoDecoder
- VideoDecoder: Add document for VideoDecoder
- VideoDemuxer: Add building option for Video Demuxer
- Camera: Fix issues for multiple streams more than 4
- DFS: Add full function and coverage gtest
- Sample: Fix SampleVideoDemuxer building issue with CMake option
- Build: Update building scripts to adapt HGY cortexa7x toolchain
- Build: Add static scan to CI
## 3.3 Version 1.7.1
- Remap: Modify DSP mapping table parse method from whole buffer to fd zero copy
- Build: Update the toolchain create script for HQX CS2.1
- Camera: Fix coredump issue if no callback registerred
- CL2DFlex: Reconstruct CL2DFlex component with plymorphism
- QnnRuntime: Fix smmu crash when 2 qnn instance use the same buffer and 1 exit
- CL2DFlex: Split kernel cl files of CL2DFlex component
- QnnRuntime: Default QNN libs to oe instead of rh variant
- CL2DFlex: Add supported pipelines table to CL2DFlex document
- QnnRuntime: Enable gtest for HGY Linux and update QNN to 2.28
- Build: Add performance evaluation for CI pipeline
- Build: Update building scripts for qclinux Ubuntu ES19
- Sample: Remove the default value for path of uscenes2dr
- CL2DFlex: Enhance NV12 UBWC convert to NV12 CL2DFlex pipeline
- Sample: Add component based on EVA DFS
- CL2DFlex: Enhance CL2DFLex buffer register logic
- Build: Fix some issue for manually building with scripts
- Sample: Add sample to visualize the DFS output
- Sample: Add accuracy test for DepthFromStereo
- Build: Fix CI QnnRuntime not build issue
- Build: Change CI perf test to manual mode
- Sample: Add tool to convert kitti stereo 2015 to inputs of SampleDataReader
- CL2DFlex: Add UBWC test cases for CL2DFlex gtest
- Sample: Video Demuxer to support read frames from video file
- Sample: Add simple user guide for DepthFromStereo
- Sample: Update docs to enable QC building with Video Demuxer

# 4. Verified Platforms
QC is verified on Lemans device with the following meta build:
- HQX QNX Snapdragon_Auto.HQX.4.5.6.0.1.r1
- HGY Ubuntu Snapdragon_Auto.HGY.4.1.6.0.r1
- HGY Linux Snapdragon_Auto.HGY.4.1.6.0.1.r1


