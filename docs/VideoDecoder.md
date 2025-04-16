*Menu*:
- [1. Introduction](#1-introduction)
- [2. QC VideoDecoder Data Structures](#2-qc-videodecoder-data-structures)
  - [2.1 The details of VideoDecoder_Config_t](#21-the-details-of-videodecoder_config_t)
  - [2.2 The details of VideoDecoder_InputFrame_t](#22-the-details-of-videodecoder_inputframe_t)
  - [2.3 The details of VideoDecoder_OutputFrame_t](#23-the-details-of-videodecoder_outputframe_t)
- [3. QC VideoDecoder APIs](#3-qc-videodecoder-apis)
- [4. Typical Use Case](#4-typical-use-case)
  - [4.1 Typical use case of dynamic mode](#41-typical-use-case-of-dynamic-mode)

# 1. Introduction
The QC VideoDecoder component provides APIs for video decoding. It calls vidc library to process video frames based on video hardware. This component supports QNX and HGY Linux/Ubuntu platforms.

# 2. QC VideoDecoder Data Structures
- [VideoDecoder_Config_t](../include/QC/component/VideoDecoder.hpp#L53)
- [VideoDecoder_InputFrame_t](../include/QC/component/VideoDecoder.hpp#L62)
- [VideoDecoder_OutputFrame_t](../include/QC/component/VideoDecoder.hpp#L72)

## 2.1 The details of VideoDecoder_Config_t
VideoDecoder_Config_t is the data structure that used to initialize VideoDecoder component. It contains video frame parameters, buffer number, buffer lists, working mode and image format. All the config parameters need to be set by the user:
- width: width of input frame and output image
- height: height of input frame and output image
- frameRate: video frames per second
- numInputBuffer: bufferList number for input buffer
- numOutputBuffer: bufferList number for output buffer
- bInputDynamicMode: is dynamic mode or not, for input buffer
- bOutputDynamicMode: is dynamic mode or not, for output buffer
- pInputBufferList: bufferList ptr for input buffer
- pOutputBufferList: bufferList ptr for output buffer
- inFormat: input video frame format, support h264 and h265
- outFormat: decoded image format, support NV12, NV12_UBWC, P010

## 2.2 The details of VideoDecoder_InputFrame_t
VideoDecoder_InputFrame_t contains all the parameters of input video frame:
- sharedBuffer: buffer of input video frame
- timestampNs: timestamp of frame data
- appMarkData: mark data of frame data

## 2.3 The details of VideoDecoder_OutputFrame_t
VideoDecoder_OutputFrame_t contains all the parameters of decoded output image:
- sharedBuffer: buffer of decoded image frame
- timestampNs: timestamp of frame data
- appMarkData: mark data of frame data
- frameFlag: indicate whether some error occurred during deccoding this frame

# 3. QC VideoDecoder APIs
- [Initialize the VideoDecoder component](../include/QC/component/VideoDecoder.hpp#L103)
- [Start the VideoDecoder pipeline](../include/QC/component/VideoDecoder.hpp#L110)
- [Stop the VideoDecoder pipeline](../include/QC/component/VideoDecoder.hpp#L116)
- [Deinitialize the VideoDecoder component](../include/QC/component/VideoDecoder.hpp#L122)
- [Submit Input Frame to VideoDecoder](../include/QC/component/VideoDecoder.hpp#L129)
- [Submit Input Frame from VideoDecoder](../include/QC/component/VideoDecoder.hpp#L136)
- [Register callback for VideoDecoder](../include/QC/component/VideoDecoder.hpp#L145)

# 4. Typical Use Case

## 4.1 Typical use case of dynamic mode

- Step 1: Configure parameters
```c++
QCStatus_e ret;
char pName[20] = "VidcDecoder";
uint32_t bufferNum = 4;
uint32_t frameNum = 10;

VideoDecoder vidcDecoder;
VideoDecoder_Config_t vidcDecoderConfig;
vidcDecoderConfig.bInputDynamicMode = true;
vidcDecoderConfig.bOutputDynamicMode = true;
vidcDecoderConfig.numInputBuffer = bufferNum;
vidcDecoderConfig.numOutputBuffer = bufferNum;

QCSharedBuffer_t inputBuffers[bufferNum];
QCSharedBuffer_t outputBuffers[bufferNum];

QCImageProps_t inputImgProps;
inputImgProps.batchSize = 1;
inputImgProps.width = 1920;
inputImgProps.height = 1024;
inputImgProps.numPlanes = 1;
inputImgProps.planeBufSize[0] = 2961408;
inputImgProps.format = QC_IMAGE_FORMAT_COMPRESSED_H265;

vidcDecoderConfig.width = 1920;
vidcDecoderConfig.height = 1024;
vidcDecoderConfig.inFormat = QC_IMAGE_FORMAT_COMPRESSED_H265;
vidcDecoderConfig.outFormat = QC_IMAGE_FORMAT_NV12;
vidcDecoderConfig.frameRate = 30;
vidcDecoderConfig.pInputBufferList = nullptr;
vidcDecoderConfig.pOutputBufferList = nullptr;

// Define callback:
void VdInputDoneCb( const VideoDecoder_InputFrame_t *pInputFrame, void *pPrivData ) {}
void VdOutputDoneCb( const VideoDecoder_OutputFrame_t *pOutputFrame, void *pPrivData ) {}
void VdEventCb( const VideoDecoder_EventType_e eventId, const void *pEvent, void *pPrivData ) {}
```

- Step 2: Allocate buffers
```c++
for ( uint32_t i = 0; i < bufferNum; i++ )
{
    ret = inputBuffers[i].Allocate( &inputImgProps );
    ASSERT_EQ( QC_STATUS_OK, ret );
}

for ( uint32_t i = 0; i < bufferNum; i++ )
{
    ret = outputBuffers[i].Allocate( videoInfo.frameWidth, videoInfo.frameHeight, outFormat );
    ASSERT_EQ( QC_STATUS_OK, ret );
}
```

- Step 3: Init and Register callback
```c++
ret = vidcDecoder.Init( pName, &vidcDecoderConfig );
ret = vidcDecoder.RegisterCallback( VdInputDoneCb, VdOutputDoneCb, VdEventCb,
                                    (void *) &vidcDecoder );
```

- Step 4: Submit input/output frame
```c++
for ( uint32_t i = 0; i < bufferNum; i++ )
{
    inputFrame.sharedBuffer = inputBuffers[i];
    ret = vidcDemuxer.GetFrame( &inputBuffers[i], frameInfo );
    ASSERT_EQ( QC_STATUS_OK, ret );

    inputFrame.timestampNs = frameInfo.startTime;
    inputFrame.appMarkData = i;

    ret = vidcDecoder.SubmitInputFrame( &inputFrame );
    ASSERT_EQ( QC_STATUS_OK, ret );
}

for ( uint32_t i = 0; i < bufferNum; i++ )
{
    outputFrame.sharedBuffer = outputBuffers[i];

    std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
    ret = vidcDecoder.SubmitOutputFrame( &outputFrame );
    ASSERT_EQ( QC_STATUS_OK, ret );
}
```

- Step 5: Stop and deinitialize the pipeline
```c++
ret = vidcDecoder.Stop();
ret = vidcDecoder.Deinit();

for ( uint32_t i = 0; i < bufferNum; i++ )
{
    ret = inputBuffers[i].Free();
    ret = outputBuffers[i].Free();
}
```

Reference: 
- [gtest_VideoDecoder](../tests/unit_test/components/VideoDecoder/gtest_VideoDecoder.cpp)
- [SampleVideoDecoder](../tests/sample/source/SampleVideoDecoder.cpp)
