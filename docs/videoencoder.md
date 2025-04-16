*Menu*:
- [1. QC VideoEncoder Data Structures](#1-qc-videoencoder-data-structures)
  - [1.1 The details of image properties.](#11-the-details-of-videoencoder_config_t)
- [2. QC buffer APIs](#2-qc-videoencoder-apis)
- [3. Typical VideoEncoder Use Case](#3-typical-videoencoder-use-cases)
  - [3.1 Dynamic input/output buffer](#31-dynamic-inputoutput-buffer)
  - [3.2 Non-Dynamic input/output buffer](#32-non-dynamic-inputoutput-buffer)
    - [3.2.1 not set input/output buffer by config](#321-not-set-inputoutput-buffer-by-config)
    - [3.2.2 set input/output buffer by config](#322-set-inputoutput-buffer-by-config)

# 1. QC VideoEncoder Data Structures

- [VideoEncoder_Config_t](../include/QC/component/VideoEncoder.hpp#L80)
- [VideoEncoder_OnTheFlyCmd_t](../include/QC/component/VideoEncoder.hpp#L101)
- [VideoEncoder_InputFrame_t](../include/QC/component/VideoEncoder.hpp#L108)
- [VideoEncoder_OutputFrame_t](../include/QC/component/VideoEncoder.hpp#L122)

## 1.1 The details of VideoEncoder_Config_t.

VideoEncoder_Config_t is a data structure that contains the configuration that should be set in Initialize.

The configs must set by customer [range]:
width [128, 8192]
height [128, 8192]
numInputBufferReq [4, 64]
numOutputBufferReq [4, 64]
bInputDynamicMode
bOutputDynamicMode
rateControlMode [choose from VideoEncoder_RateControlMode_e]
profile [choose from VideoEncoder_Profile_e]
inFormat [choose from QCImageFormat_e that without compression]
outFormat [choose from QCImageFormat_e that with compression]

The configs that have default values (default):
bitRate (64000)
gop (30)
frameRate (30)

# 2. QC VideoEncoder APIs

- [Init the video encoder](../include/QC/component/VideoEncoder.hpp#L182)
- [Start the video encoder](../include/QC/component/VideoEncoder.hpp#L189)
- [Stop the video encoder](../include/QC/component/VideoEncoder.hpp#L195)
- [Deinit the video encoder (release all resources)](../include/QC/component/VideoEncoder.hpp#L201)
- [SubmitInputFrame](../include/QC/component/VideoEncoder.hpp#L208)
- [SubmitOutputFrame](../include/QC/component/VideoEncoder.hpp#L215)
- [GetInputBuffers that allocated by video encoder](../include/QC/component/VideoEncoder.hpp#L223)
- [GetOutputBuffers that allocated by video encoder](../include/QC/component/VideoEncoder.hpp#L231)
- [Configure](../include/QC/component/VideoEncoder.hpp#L238)
- [RegisterCallback](../include/QC/component/VideoEncoder.hpp#L248)

# 3. Typical VideoEncoder Use Cases

- [gtest_VideoEncoder](../tests/unit_test/components/VideoEncoder/gtest_VideoEncoder.cpp)
- [SampleVideoEncoder](../tests/sample/source/SampleVideoEncoder.cpp)

## 3.1 Dynamic input/output buffer
```c++
//...
    config.width = 176;
    config.height = 144;
    config.bitRate = 64000;
    config.gop = 20;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 30;
    config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H264;
    config.bInputDynamicMode = true;
    config.bOutputDynamicMode = true;
    config.pInputBufferList = nullptr;
    config.pOutputBufferList = nullptr;
//... Init and Start
    VideoEncoder_InputFrame_t inputFrame;
    sharedBuffer = &inputFrame.sharedBuffer;
    ret = sharedBuffer->Allocate( config.width, config.height, config.inFormat );
    //... set other info
    ret = xxx.SubmitInputFrame( &inputFrame );
//...
```
## 3.2 Non-Dynamic input/output buffer

### 3.2.1 not set input/output buffer by config
```c++
//...
    config.width = 1920;
    config.height = 1080;
    config.bitRate = 20000000;
    config.gop = 0;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 60;
    config.profile = VIDEO_ENCODER_PROFILE_H264_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H264;
    config.bInputDynamicMode = false;
    config.bOutputDynamicMode = false;
    config.pInputBufferList = nullptr; // buffer will allocated inside video encoder
    config.pOutputBufferList = nullptr; // buffer will allocated inside video encoder
//... Init and Start
    QCSharedBuffer_t *inputList = new QCSharedBuffer_t[config.numInputBufferReq];
    ret = xxx.GetInputBuffers( inputList, config.numInputBufferReq );

    for ( i = 0; i < config.numInputBufferReq; i++ )
    {
        VideoEncoder_InputFrame_t inputFrame;
        inputFrame.sharedBuffer = inputList[i];
        //... set other info
        ret = xxx.SubmitInputFrame( &inputFrame );
    }
//...
```
### 3.2.2 set input/output buffer by config
```c++
//...
    config.width = 176;
    config.height = 144;
    config.bitRate = 64000;
    config.gop = 0;
    config.numInputBufferReq = 4;
    config.numOutputBufferReq = 4;
    config.frameRate = 30;
    config.profile = VIDEO_ENCODER_PROFILE_HEVC_MAIN;
    config.rateControlMode = VIDEO_ENCODER_RCM_CBR_CFR;
    config.inFormat = QC_IMAGE_FORMAT_NV12;
    config.outFormat = QC_IMAGE_FORMAT_COMPRESSED_H265;
    config.bInputDynamicMode = false;
    config.bOutputDynamicMode = false;
//... allocate input and output buffer
    config.pInputBufferList = inBufferList; // buffer allocated by customer
    config.pOutputBufferList = outBufferList; // buffer allocated by customer
//... Init and Start
    for ( i = 0; i < config.numInputBufferReq; i++ )
    {
        VideoEncoder_InputFrame_t inputFrame;
        inputFrame.sharedBuffer = inBufferList[i];
        //... set other info
        ret = xxx.SubmitInputFrame( &inputFrame );
    }
//...
```

