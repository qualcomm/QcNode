*Menu*:
- [1. Introduction](#1-introduction)
- [2. Video Decoder Configuraion](#2-video-decoder-configuraion)
  - [2.1 Video Decoder Node Configuraion](#21-video-decoder-node-configuraion)
- [3. QC VideoDecoder Data Structures](#2-qc-videodecoder-data-structures)
  - [3.1 The details of VideoDecoder_Config_t](#21-the-details-of-videodecoder_config_t)
  - [3.2 The details of VideoDecoder_InputFrame_t](#22-the-details-of-videodecoder_inputframe_t)
  - [3.3 The details of VideoDecoder_OutputFrame_t](#23-the-details-of-videodecoder_outputframe_t)
- [4. QC VideoDecoder APIs](#3-qc-videodecoder-apis)
- [5. Typical Use Case](#4-typical-use-case)
  - [5.1 Typical use case of dynamic mode](#41-typical-use-case-of-dynamic-mode)

# 1. Introduction
The QC VideoDecoder node provides APIs for video decoding. It calls vidc library to process video frames based on video hardware. 
This node supports QNX and HGY Linux/Ubuntu platforms.

# 2. Video Decoder Configuraion

## 2.1 Video Decoder Node Configuraion
| Parameter            | Required  | Type        | Default | Description            |
|----------------------|-----------|-------------|------------------------|
| `name`               | true      | string      |         | The Node unique name.  |
| `id`                 | true      | uint32_t    |         | The Node unique ID.    |
| `width`              | true      | uint32_t    |         | Video frame width.     |
| `height`             | true      | uint32_t    |         | Video frame height.    |
| `frameRate`          | true      | uint32_t    |         | Frames per second.     |
| `format`             | false     | string      | h265    | The image format, options from [nv12, nv12_ubwc] |
| `output_format`      | false     | string      | nv12    | The output image format, options from [h264, h265] |
| `bInputDynamicMode`  | false     | bool        | true    | Input frame dynamic mode |
| `bOutputDynamicMode` | false     | bool        | false   | Output frame dynamic mode |
| `numInputBufferReq`  | false     | uint32_t    | 4      | Number of input buffers |
| `numOutputBufferReq` | false     | uint32_t    | 4      | Number of output buffers |

- Example Configurations
```json
{
    "static": {
        "name": "VDEC0",
        "id": 0,
        "width": 1920,
        "height": 1024,
        "frameRate": 30,
        "format": "h265",
        "output_format": "nv12",
        "bInputDynamicMode": true,
        "bOutputDynamicMode": false,
        "numOutputBufferReq": 4,
        "numInputBufferReq": 4,
    }
}

# 3. QC VideoDecoder Data Structures
- [VideoFrameDescriptor_t](../include/QC/Infras/Memory/VideoFrameDescriptor.hpp#L50)

## 3.1 The details of VideoFrameDescriptor_t
VideoFrameDescriptor_t contains all the parameters of input and output video frame:
- ImageDescriptor: Image buffer of input video frame
- timestampNs: Timestamp in nanoseconds of frame data
- appMarkData: Mark data of frame data
- frameType: Indication of I/P/B/IDR frame, used by encoder
- frameFlag: Indication of whether some error occurred during decoding this frame

# 4. QC VideoDecoder APIs
- [Initialize the VideoDecoder node](../include/QC/Node/VideoDecoder.hpp#L209)
- [Start the VideoDecoder pipeline](../include/QC/Node/VideoDecoder.hpp#L250)
- [Stop the VideoDecoder pipeline](../include/QC/Node/VideoDecoder.hpp#L251)
- [Submit Input and Output Frames to VideoDecoder](../include/QC/Node/VideoDecoder.hpp#L242)

# 5. Typical Use Case

## 5.1 Typical use case of dynamic mode

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
void VdInputDoneCb( VideoFrameDescriptor_t &inFrameDesc, void *pPrivData ) {}
void VdOutputDoneCb( VideoFrameDescriptor_t &inFrameDesc, void *pPrivData ) {}
void VdEventCb( conVideoCodec_EventType_e eventId, const void *pEvent, void *pPrivData ) {}
```

- Step 2: Allocate buffers
```c++
    ret = m_imagePool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_numOutputBufferReq, m_width,
                            m_height, m_outFormat, QC_MEMORY_ALLOCATOR_DMA_GPU );

    if ( QC_STATUS_OK == ret )
    {
        ret = m_imagePool.GetBuffers( m_nodeCfg.buffers );
    }

```

- Step 3: Init and Register callback
```c++

using std::placeholders::_1;
m_nodeCfg.config = m_dataTree.Dump();
m_nodeCfg.callback = std::bind( &SampleVideoDecoder::OnDoneCb, this, _1 );

ret = m_decoder.Initialize( m_nodeCfg );
```

- Step 4: Submit input/output frame
```c++
for ( uint32_t i = 0; i < bufferNum; i++ )
{
    inputs[bufferIdx].timestampNs = frameInfo.startTime;
    inputs[bufferIdx].appMarkData = i;

    ret = inFrameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_INPUT_BUFF_ID, inputs[bufferIdx] );
    ASSERT_EQ( QC_STATUS_OK, ret );
    //  Process the frame
    ret = pNodeVide->ProcessFrameDescriptor( inFrameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );

    std::unique_lock<std::mutex> outLock( g_OutMutex );
    g_OutCondVar.wait( outLock );

    //  Set up an output buffer for the frame
    ret = outFrameDesc.SetBuffer( QC_NODE_VIDEO_DECODER_OUTPUT_BUFF_ID, outputs[bufferIdx] );
    ASSERT_EQ( QC_STATUS_OK, ret );
    //  Process the frame
    ret = pNodeVide->ProcessFrameDescriptor( outFrameDesc );
    ASSERT_EQ( QC_STATUS_OK, ret );
}
```

- Step 5: Stop and deinitialize the pipeline
```c++
ret = vidcDecoder.Stop();
ret = vidcDecoder.Deinit();

for ( auto &output : outputs )
{
    ret = bufMgr.Free( output );
    ASSERT_EQ( QC_STATUS_OK, ret );
}

for ( auto &input : inputs )
{
    ret = bufMgr.Free( input );
    ASSERT_EQ( QC_STATUS_OK, ret );
}
```

Reference: 
- [gtest_VideoDecoder](../tests/unit_test/Node/VideoDecoder/gtest_VideoDecoder.cpp)
- [SampleVideoDecoder](../tests/sample/source/SampleVideoDecoder.cpp)
