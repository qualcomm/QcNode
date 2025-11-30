*Menu*:
- [1. QC VideoEncoder Data Structures](#1-qc-videoencoder-data-structures)
  - [1.1 The details of image properties.](#11-the-details-of-videoencoder_config_t)
- [2. Video Encoder Configuraion](#2-video-encoder-configuraion)
  - [2.1 Video Encoder Node Configuraion](#21-video-encoder-node-configuraion)
- [3. QC buffer APIs](#2-qc-videoencoder-apis)
- [4. Typical VideoEncoder Use Case](#3-typical-videoencoder-use-cases)
  - [4.1 Dynamic input/output buffer](#31-dynamic-inputoutput-buffer)
  - [4.2 Non-Dynamic input/output buffer](#32-non-dynamic-inputoutput-buffer)
    - [4.2.1 not set input/output buffer by config](#321-not-set-inputoutput-buffer-by-config)
    - [4.2.2 set input/output buffer by config](#322-set-inputoutput-buffer-by-config)

# 1. QC VideoEncoder Data Structures

- [VideoFrameDescriptor_t](../include/QC/Infras/Memory/VideoFrameDescriptor.hpp#L50)

## 2.1 The details of VideoFrameDescriptor_t
VideoFrameDescriptor_t contains all the parameters of input and output video frame:
- ImageDescriptor: Image buffer of input video frame
- timestampNs: Timestamp in nanoseconds of frame data
- appMarkData: Mark data of frame data
- frameType: Indication of I/P/B/IDR frame, used by encoder
- frameFlag: Indication of whether some error occurred during decoding this frame

# 2. Video Encoder Configuraion

## 2.1 Video Encoder Node Configuraion
| Parameter            | Required  | Type        | Default | Description            |
|----------------------|-----------|-------------|------------------------|
| `name`               | true      | string      |         | The Node unique name.  |
| `id`                 | true      | uint32_t    |         | The Node unique ID.    |
| `width`              | true      | uint32_t    |         | Video frame width.     |
| `height`             | true      | uint32_t    |         | Video frame height.    |
| `frameRate`          | true      | uint32_t    |         | Frames per second.     |
| `bitrate`            | false     | uint32_t    | 8000000 | The encoding bitrate |
| `gop`                | false     | uint32_t    | 0       |
| `rateControlMode`    | false     | string      | CBR_CFR | Bit rate control profile
| `format`             | false     | string      | nv12    | The image format, options from [nv12, nv12_ubwc] |
| `output_format`      | false     | string      | h265    | The output image format, options from [h264, h265] |
| `bInputDynamicMode`  | false     | bool        | true    | Input frame dynamic mode |
| `bOutputDynamicMode` | false     | bool        | false   | Output frame dynamic mode |
| `numInputBufferReq`  | false     | uint32_t    | 4       | Number of input buffers |
| `numOutputBufferReq` | false     | uint32_t    | 4       | Number of output buffers |

- Example Configurations
```json
{
    "static": {
        "name": "VENC0",
        "id": 0,
        "width": 1920,
        "height": 1024,
        "rateControlMode": "CBR_CFR",
        "gop": 20,
        "bitrate": 64000,
        "frameRate": 30,
        "format": "nv12",
        "output_format": "h265",
        "bInputDynamicMode": true,
        "bOutputDynamicMode": false,
        "numOutputBufferReq": 4,
        "numInputBufferReq": 4,
    }
}

# 3. QC VideoEncoder APIs

- [Init the video encoder](../include/QC/Node/VideoEncoder.hpp#L275)
- [Start the video encoder](../include/QC/Node/VideoEncoder.hpp#L316)
- [Stop the video encoder](../include/QC/Node/VideoEncoder.hpp#L317)
- [Submit Input and Output Frame](../include/QC/Node/VideoEncoder.hpp#L308)

# 4. Typical VideoEncoder Use Cases

- [gtest_VideoEncoder](../tests/unit_test/Node/VideoEncoder/gtest_VideoEncoder.cpp)
- [SampleVideoEncoder](../tests/sample/source/SampleVideoEncoder.cpp)

## 4.1 Dynamic input/output buffer
```c++
//...
    QCStatus_e ret;
    std::string errors;
    BufferManager bufMgr = BufferManager( { "VENC", QC_NODE_TYPE_VENC, 0 } );
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "name", "SANITY_VideoEncoder_Dynamic" );
    dt.Set<uint32_t>( "id", g_nodeId = 1 );
    dt.Set<std::string>( "logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", 176 );
    dt.Set<uint32_t>( "height", 144 );
    dt.Set<uint32_t>( "bitrate", 64000 );
    dt.Set<uint32_t>( "gop", 20 );
    dt.Set<bool>( "bInputDynamicMode", true );
    dt.Set<bool>( "bOutputDynamicMode", true );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 30 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

    DataTree dataTree;
    dataTree.Set( "static", dt );

    QCNodeInit_t config = { .config = dataTree.Dump(), .callback = OnDoneCb };

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    QCNodeConfigIfs &cfgIfs = pNodeVide->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    DataTree optionsDt;
    ret = optionsDt.Load( options, errors );
    ASSERT_EQ( QC_STATUS_OK, ret );

    uint32_t width = dt.Get( "width", 0 );
    uint32_t height = dt.Get( "height", 0 );
    uint32_t numInputBufferReq = dt.Get( "numInputBufferReq", 1 );
    uint32_t numOutputBufferReq = dt.Get( "numOutputBufferReq", 1 );
    QCImageFormat_e inFormat = dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );
    QCImageFormat_e outFormat = dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H264 );
//... Init and Start
    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_EVENT_BUFF_ID + 1 );

    std::vector<VideoFrameDescriptor_t> inputs;
    std::vector<VideoFrameDescriptor_t> outputs;

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( ImageBasicProps( width, height, inFormat ), bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        inputs.push_back( bufDesc );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = width;
        imgProps.height = height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 118784;
        imgProps.format = outFormat;
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( imgProps, bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        outputs.push_back( bufDesc );
    }

    frameDesc.Clear();
    
    uint32_t nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    for ( uint32_t i = 0; i < nr; i++ )
    {
        //  Set up an input buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID, inputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Set up an output buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, outputs[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the frame
        ret = pNodeVide->ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

//...
```
## 4.2 Non-Dynamic input/output buffer
```c++
//...
    QCStatus_e ret;
    std::string errors;
    BufferManager bufMgr = BufferManager( { "VENC", QC_NODE_TYPE_VENC, 0 } );
    QCNodeIfs *pNodeVide = new QC::Node::VideoEncoder();
    DataTree dt;
    dt.Set<std::string>( "name", "SANITY_VideoEncoder_NonDynamic" );
    dt.Set<uint32_t>( "id", g_nodeId = 2 );
    dt.Set<std::string>( "logLevel", "ERROR" );
    dt.Set<uint32_t>( "width", 1920 );
    dt.Set<uint32_t>( "height", 1080 );
    dt.Set<uint32_t>( "bitrate", 20000000 );
    dt.Set<uint32_t>( "gop", 0 );
    dt.Set<bool>( "bInputDynamicMode", false );
    dt.Set<bool>( "bOutputDynamicMode", false );
    dt.Set<uint32_t>( "numInputBufferReq", 4 );
    dt.Set<uint32_t>( "numOutputBufferReq", 4 );
    dt.Set<uint32_t>( "frameRate", 60 );
    dt.Set<std::string>( "profile", "H264_MAIN" );
    dt.Set<std::string>( "rateControlMode", "CBR_CFR" );
    dt.Set<std::string>( "inputImageFormat", "nv12" );
    dt.Set<std::string>( "outputImageFormat", "h264" );

    DataTree dataTree;
    dataTree.Set( "static", dt );

    uint32_t width = dt.Get( "width", 0 );
    uint32_t height = dt.Get( "height", 0 );
    uint32_t numInputBufferReq = dt.Get( "numInputBufferReq", 1 );
    uint32_t numOutputBufferReq = dt.Get( "numOutputBufferReq", 1 );
    QCImageFormat_e inFormat = dt.GetImageFormat( "inputImageFormat", QC_IMAGE_FORMAT_NV12 );
    QCImageFormat_e outFormat = dt.GetImageFormat( "outputImageFormat", QC_IMAGE_FORMAT_COMPRESSED_H264 );
    
    std::vector<VideoFrameDescriptor_t> buffers;

    for ( uint32_t i = 0; i < numInputBufferReq; i++ )
    {
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( ImageBasicProps( width, height, inFormat,
                                                QC_MEMORY_ALLOCATOR_DMA_VPU, QC_CACHEABLE ),
                               bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        buffers.push_back( bufDesc );
    }

    for ( uint32_t i = 0; i < numOutputBufferReq; i++ )
    {
        ImageProps_t imgProps;
        imgProps.batchSize = 1;
        imgProps.width = width;
        imgProps.height = height;
        imgProps.numPlanes = 1;
        imgProps.planeBufSize[0] = 2 * 1024 * 1024;
        imgProps.format = outFormat;
        imgProps.allocatorType = QC_MEMORY_ALLOCATOR_DMA_VPU;
        imgProps.cache = QC_CACHEABLE;
        VideoFrameDescriptor_t bufDesc;
        ret = bufMgr.Allocate( imgProps, bufDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
        buffers.push_back( bufDesc );
    }

    std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> bufferRefs;

    for ( VideoFrameDescriptor_t &frameDesc : buffers )
    {
        bufferRefs.push_back(frameDesc);
    }

    QCNodeInit_t config = { .config = dataTree.Dump(), .callback = OnDoneCb, .buffers = bufferRefs };

    ret = pNodeVide->Initialize( config );
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    QCNodeConfigIfs &cfgIfs = pNodeVide->GetConfigurationIfs();
    const std::string &options = cfgIfs.GetOptions();

    ASSERT_EQ( QC_OBJECT_STATE_READY, pNodeVide->GetState() );

    ret = pNodeVide->Start();
    ASSERT_EQ( QC_STATUS_OK, ret );
    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    uint32_t nr = std::min( numInputBufferReq, numOutputBufferReq );

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

    NodeFrameDescriptor frameDesc( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID + 1 );
    frameDesc.Clear();

    for ( uint32_t i = 0; i < nr; i++ )
    {
        //  Set up an input buffer for the frame
        ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_INPUT_BUFF_ID, buffers[i] );
        ASSERT_EQ( QC_STATUS_OK, ret );
        //  Process the input frame
        ret = pNodeVide->ProcessFrameDescriptor( frameDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    ASSERT_EQ( QC_OBJECT_STATE_RUNNING, pNodeVide->GetState() );

### 4.2.1 not set input/output buffer by config
//...
```
### 4.2.2 set input/output buffer by config
```c++
//...
    frameDesc.Clear();
    ret = frameDesc.SetBuffer( QC_NODE_VIDEO_ENCODER_OUTPUT_BUFF_ID, buffers[numInputBufferReq] );
    ASSERT_EQ( QC_STATUS_OK, ret );
    //  Process the output frame
    ret = pNodeVide->ProcessFrameDescriptor( frameDesc );
//...
```

