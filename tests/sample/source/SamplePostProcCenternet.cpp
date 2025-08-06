// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SamplePostProcCenternet.hpp"
#include <algorithm>
#include <assert.h>

namespace QC
{
namespace sample
{

#define U2F( dname, index ) ( ( dname[index] + dname##Offset ) * dname##Scale )

#define KernelCode( ... ) #__VA_ARGS__

static const char *s_pSourceBboxDet = KernelCode( __kernel void BboxDet(
        __global uchar *hm, __global uchar *wh, __global uchar *reg, __global uint *objClsIds,
        __global float *objProbs, __global float *objCoords, uint clsNum, uint maxObjNum,
        float thresh, uint kernelSize, uint width, uint height, uint imgWidth, uint imgHeight,
        uint roiX, uint roiY, float hmScale, int hmOffset, float whScale, int whOffset,
        float regScale, int regOffset ) {
    int c = get_global_id( 0 );
    int y = get_global_id( 1 );
    int x = get_global_id( 2 );

    int idx = ( y * width + x ) * clsNum + c;
    float obj_hm = ( hm[idx] + hmOffset ) * hmScale;
    float obj_prob = native_recip( native_exp( -1.0 * obj_hm ) + 1.0 );
    uint padding = ( kernelSize - 1 ) / 2;
    int offset = -1 * padding;
    uint l = 0, m = 0;
    int cur_x = 0, cur_y = 0, cur_index = 0;
    float cur_hm = -1.0, cur_prob = -1.0, max_prob = -1.0;
    int max_index = 0;
    bool valid = false;
    float4 coords;

    if ( obj_prob > thresh )
    {
        for ( l = 0; l < kernelSize; ++l )
        {
            for ( m = 0; m < kernelSize; ++m )
            {
                cur_x = offset + l + x;
                cur_y = offset + m + y;
                valid = ( ( cur_x >= 0 ) && ( cur_x < width ) && ( cur_y >= 0 ) &&
                          ( cur_y < height ) );
                cur_prob = ( valid != false ) ? obj_prob : -1.0f;
                max_index = ( cur_prob > max_prob ) ? idx : max_index;
                max_prob = ( cur_prob > max_prob ) ? cur_prob : max_prob;
            }
        }

        if ( idx == max_index )
        {
            uint reg_idx = ( y * width + x );
            float2 xyReg = convert_float2( vload2( reg_idx, reg ) );
            float2 whReg = convert_float2( vload2( reg_idx, wh ) );
            float2 wh2 = (float2) ( width, height );
            xyReg = ( xyReg + regOffset ) * regScale + (float2) ( x, y );
            whReg = ( whReg + whOffset ) * whScale * 0.5f;

            float4 imgWH4 = (float4) ( imgWidth, imgHeight, imgWidth, imgHeight );
            float4 roiXY4 = (float4) ( roiX, roiY, roiX, roiY );

            coords.s01 = ( xyReg - whReg ) / wh2;
            coords.s23 = ( xyReg + whReg ) / wh2;
            coords = clamp( coords, 0.0f, 0.99f );
            coords = mad( coords, imgWH4, roiXY4 );
            if ( ( coords.s0 < coords.s2 ) && ( coords.s1 < coords.s3 ) )
            {
                uint obj_idx = atomic_inc( &objClsIds[maxObjNum] );
                if ( obj_idx < maxObjNum )
                {
                    objClsIds[obj_idx] = c;
                    objProbs[obj_idx] = obj_prob;
                    vstore4( coords, obj_idx, objCoords );
                }
            }
        }
    }
} );

static float fastPow( float p )
{
    float offset = ( p < 0 ) ? 1.0f : 0.0f;
    float clipp = ( p < -126 ) ? -126.0f : p;
    int32_t w = (int32_t) clipp;
    float z = clipp - w + offset;
    union
    {
        uint32_t i;
        float f;
    } v = { (uint32_t) ( ( 1 << 23 ) * ( clipp + 121.2740575f + 27.7280233f / ( 4.84252568f - z ) -
                                         1.49012907f * z ) ) };

    return v.f;
}

static float fastExp( float p )
{
    return fastPow( 1.442695040f * p );
}

static float sigmoid( float data )
{
    return 1. / ( 1. + fastExp( -data ) );
}

SamplePostProcCenternet::SamplePostProcCenternet() {}
SamplePostProcCenternet::~SamplePostProcCenternet() {}

QCStatus_e SamplePostProcCenternet::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_roiX = Get( config, "roi_x", 0 );
    m_roiY = Get( config, "roi_y", 0 );
    m_camWidth = Get( config, "width", 1920 );
    m_camHeight = Get( config, "height", 1024 );
    m_scoreThreshold = Get( config, "score_threshold", 0.6f );
    m_NMSThreshold = Get( config, "nms_threshold", 0.6f );
    m_processor = Get( config, "processor", QC_PROCESSOR_CPU );
    if ( ( QC_PROCESSOR_CPU != m_processor ) && ( QC_PROCESSOR_GPU != m_processor ) )
    {
        QC_ERROR( "invalid processor type" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e SamplePostProcCenternet::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( CPU );
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    // OpenCL Init
    if ( QC_STATUS_OK == ret )
    {
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            ret = m_OpenclSrvObj.Init( name.c_str(), LOGGER_LEVEL_ERROR );
            SetCLParams();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to initialize openclSrvObj" );
            }
            if ( QC_STATUS_OK == ret )
            {
                ret = m_OpenclSrvObj.LoadFromSource( s_pSourceBboxDet );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to load kernel source code" );
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                ret = m_OpenclSrvObj.CreateKernel( &m_kernel, "BboxDet" );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to create kernel" );
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                ret = RegisterOutputBuffers();
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to create output buffer" );
                }
            }
        }
    }

    return ret;
}

QCStatus_e SamplePostProcCenternet::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = false;
    m_thread = std::thread( &SamplePostProcCenternet::ThreadMain, this );

    return ret;
}

void SamplePostProcCenternet::ThreadMain()
{
    QCStatus_e ret;

    while ( false == m_stop )
    {
        DataFrames_t tensors;
        ret = m_sub.Receive( tensors );
        if ( QC_STATUS_OK == ret )
        {
            if ( QC_PROCESSOR_CPU == m_processor )
            {
                PROFILER_BEGIN();
                TRACE_BEGIN( tensors.FrameId( 0 ) );
                PostProcCPU( tensors );
                PROFILER_END();
                TRACE_END( tensors.FrameId( 0 ) );
            }
            else if ( QC_PROCESSOR_GPU == m_processor )
            {
                PROFILER_BEGIN();
                TRACE_BEGIN( tensors.FrameId( 0 ) );
                ret = RegisterInputBuffers( tensors );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to create input buffer" );
                }
                ret = PostProcCL( tensors );
                PROFILER_END();
                TRACE_END( tensors.FrameId( 0 ) );
            }
            else
            {
                QC_ERROR( "invalid processor type" );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }
}

QCStatus_e SamplePostProcCenternet::RegisterInputBuffers( DataFrames_t &tensors )
{
    QCStatus_e ret = QC_STATUS_OK;

    QCSharedBuffer_t hm = tensors.SharedBuffer( 0 );
    QCSharedBuffer_t wh = tensors.SharedBuffer( 1 );
    QCSharedBuffer_t reg = tensors.SharedBuffer( 2 );

    m_hmScale = tensors.QuantScale( 0 );
    m_whScale = tensors.QuantScale( 1 );
    m_regScale = tensors.QuantScale( 2 );
    m_hmOffset = tensors.QuantOffset( 0 );
    m_whOffset = tensors.QuantOffset( 1 );
    m_regOffset = tensors.QuantOffset( 2 );
    m_height = hm.tensorProps.dims[1];
    m_width = hm.tensorProps.dims[2];
    m_classNum = hm.tensorProps.dims[3];

    // Create hm data buffer
    ret = m_OpenclSrvObj.RegBuf( &( hm.buffer ), &m_clInputHMBuf );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to create hm data buffer" );
    }

    // Create wh data buffer
    ret = m_OpenclSrvObj.RegBuf( &( wh.buffer ), &m_clInputWHBuf );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to create wh data buffer" );
    }

    // Create reg data buffer
    ret = m_OpenclSrvObj.RegBuf( &( reg.buffer ), &m_clInputRegBuf );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to create reg data buffer" );
    }

    return ret;
}

QCStatus_e SamplePostProcCenternet::RegisterOutputBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t outputClsIdBufSize =
            ( MAX_OBJ_NUM + 1 ) * sizeof( uint32_t );   // the last element is object num
    size_t outputProbBufSize = MAX_OBJ_NUM * sizeof( float );
    size_t outputCoordsBufSize = MAX_OBJ_NUM * 4 * sizeof( float );
    uint64_t bufHandle = 0;

    // create output classId data buffer
    ret = m_outputClsIdBuf.Allocate( outputClsIdBufSize );
    bufHandle = m_outputClsIdBuf.buffer.dmaHandle;
    ret = m_OpenclSrvObj.RegBuf( &( m_outputClsIdBuf.buffer ), &m_clOutputClsIdBuf );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to create output classId data buffer" );
    }

    // create output classId prob buffer
    ret = m_outputProbBuf.Allocate( outputProbBufSize );
    bufHandle = m_outputProbBuf.buffer.dmaHandle;
    ret = m_OpenclSrvObj.RegBuf( &( m_outputProbBuf.buffer ), &m_clOutputProbBuf );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to create output prob data buffer" );
    }

    // create output coords data buffer
    ret = m_outputCoordsBuf.Allocate( outputCoordsBufSize );
    bufHandle = m_outputCoordsBuf.buffer.dmaHandle;
    ret = m_OpenclSrvObj.RegBuf( &( m_outputCoordsBuf.buffer ), &m_clOutputCoordsBuf );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to create output coords data buffer" );
    }

    return ret;
}

void SamplePostProcCenternet::SetCLParams()
{
    m_openclArgs[0].pArg = (void *) &m_clInputHMBuf;
    m_openclArgs[0].argSize = sizeof( cl_mem );
    m_openclArgs[1].pArg = (void *) &m_clInputWHBuf;
    m_openclArgs[1].argSize = sizeof( cl_mem );
    m_openclArgs[2].pArg = (void *) &m_clInputRegBuf;
    m_openclArgs[2].argSize = sizeof( cl_mem );
    m_openclArgs[3].pArg = (void *) &m_clOutputClsIdBuf;
    m_openclArgs[3].argSize = sizeof( cl_mem );
    m_openclArgs[4].pArg = (void *) &m_clOutputProbBuf;
    m_openclArgs[4].argSize = sizeof( cl_mem );
    m_openclArgs[5].pArg = (void *) &m_clOutputCoordsBuf;
    m_openclArgs[5].argSize = sizeof( cl_mem );
    m_openclArgs[6].pArg = (void *) &m_classNum;
    m_openclArgs[6].argSize = sizeof( cl_uint );
    m_openclArgs[7].pArg = (void *) &m_maxObjNum;
    m_openclArgs[7].argSize = sizeof( cl_uint );
    m_openclArgs[8].pArg = (void *) &m_scoreThreshold;
    m_openclArgs[8].argSize = sizeof( cl_float );
    m_openclArgs[9].pArg = (void *) &m_kernelSize;
    m_openclArgs[9].argSize = sizeof( cl_uint );
    m_openclArgs[10].pArg = (void *) &m_width;
    m_openclArgs[10].argSize = sizeof( cl_uint );
    m_openclArgs[11].pArg = (void *) &m_height;
    m_openclArgs[11].argSize = sizeof( cl_uint );
    m_openclArgs[12].pArg = (void *) &m_camWidth;
    m_openclArgs[12].argSize = sizeof( cl_uint );
    m_openclArgs[13].pArg = (void *) &m_camHeight;
    m_openclArgs[13].argSize = sizeof( cl_uint );
    m_openclArgs[14].pArg = (void *) &m_roiX;
    m_openclArgs[14].argSize = sizeof( cl_uint );
    m_openclArgs[15].pArg = (void *) &m_roiY;
    m_openclArgs[15].argSize = sizeof( cl_uint );
    m_openclArgs[16].pArg = (void *) &m_hmScale;
    m_openclArgs[16].argSize = sizeof( cl_float );
    m_openclArgs[17].pArg = (void *) &m_hmOffset;
    m_openclArgs[17].argSize = sizeof( cl_int );
    m_openclArgs[18].pArg = (void *) &m_whScale;
    m_openclArgs[18].argSize = sizeof( cl_float );
    m_openclArgs[19].pArg = (void *) &m_whOffset;
    m_openclArgs[19].argSize = sizeof( cl_int );
    m_openclArgs[20].pArg = (void *) &m_regScale;
    m_openclArgs[20].argSize = sizeof( cl_float );
    m_openclArgs[21].pArg = (void *) &m_regOffset;
    m_openclArgs[21].argSize = sizeof( cl_int );

    return;
}

void SamplePostProcCenternet::PostProcCPU( DataFrames_t &tensors )
{
    QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", tensors.FrameId( 0 ),
              tensors.Timestamp( 0 ) );

    Road2DObjects_t objs;

    QCBufferDescriptorBase_t &hmDesc = tensors.GetBuffer( 0 );
    QCBufferDescriptorBase_t &whDesc = tensors.GetBuffer( 1 );
    QCBufferDescriptorBase_t &regDesc = tensors.GetBuffer( 2 );

    TensorDescriptor_t *pTsHmDesc = dynamic_cast<TensorDescriptor_t *>( &hmDesc );

    int H = (int) pTsHmDesc->dims[1];
    int W = (int) pTsHmDesc->dims[2];
    int classNum = (int) pTsHmDesc->dims[3];

    uint8_t *hm = (uint8_t *) hmDesc.pBuf;
    float hmScale = tensors.QuantScale( 0 );
    int32_t hmOffset = tensors.QuantOffset( 0 );
    uint8_t *wh = (uint8_t *) whDesc.pBuf;
    float whScale = tensors.QuantScale( 1 );
    int32_t whOffset = tensors.QuantOffset( 1 );
    uint8_t *reg = (uint8_t *) regDesc.pBuf;
    float regScale = tensors.QuantScale( 2 );
    int32_t regOffset = tensors.QuantOffset( 2 );
    const int kernel_size = 7;

    std::vector<int> class_ids;
    if ( 80 == classNum )
    {
        // coco centernet, only care road object
        // https://github.com/amikelive/coco-labels/blob/master/coco-labels-paper.txt
        class_ids = std::vector<int>{ 0, 2, 5, 7 };
    }
    else
    {
        class_ids.resize( classNum );
        for ( int cls = 0; cls < classNum; cls++ )
        {
            class_ids[cls] = classNum;
        }
    }

    for ( int y = 0; y < H; y++ )
    {
        for ( int x = 0; x < W; x++ )
        {
            for ( int cls : class_ids )
            {
                int idx = ( y * W + x ) * classNum + cls;
                float objProb = sigmoid( U2F( hm, idx ) );
                if ( objProb > m_scoreThreshold )
                {
                    int padding = ( kernel_size - 1 ) / 2;
                    int offset = -padding;
                    int l, m;
                    float max = -1;
                    int max_index = 0;
                    for ( l = 0; l < kernel_size; ++l )
                    {
                        for ( m = 0; m < kernel_size; ++m )
                        {
                            int cur_x = offset + l + x;
                            int cur_y = offset + m + y;
                            int cur_index = ( y * W + x ) * classNum + cls;
                            int valid = ( ( cur_x >= 0 ) && ( cur_x < W ) && ( cur_y >= 0 ) &&
                                          ( cur_y < H ) );
                            float val = ( valid != 0 ) ? sigmoid( U2F( hm, cur_index ) ) : -1;
                            max_index = ( val > max ) ? cur_index : max_index;
                            max = ( val > max ) ? val : max;
                        }
                    }

                    if ( idx == max_index )
                    {
                        int reg_index = ( y * W + x ) * 2;
                        float c_x, c_y;
                        Road2DObject_t det;
                        c_x = x + U2F( reg, reg_index );
                        c_y = y + U2F( reg, reg_index + 1 );
                        float topX = ( c_x - U2F( wh, reg_index ) / 2 ) / W;
                        float topY = ( c_y - U2F( wh, reg_index + 1 ) / 2 ) / H;
                        float bottomX = ( c_x + U2F( wh, reg_index ) / 2 ) / W;
                        float bottomY = ( c_y + U2F( wh, reg_index + 1 ) / 2 ) / H;
                        topX = ( ( topX > 0 ) ? topX * m_camWidth : 0 ) + m_roiX;
                        topY = ( ( topY > 0 ) ? topY * m_camHeight : 0 ) + m_roiY;
                        bottomX = ( ( bottomX < 1 ) ? bottomX : 0.99 ) * m_camWidth + m_roiX;
                        bottomY = ( ( bottomY < 1 ) ? bottomY : 0.99 ) * m_camHeight + m_roiY;
                        det.classId = cls;
                        det.prob = objProb;
                        if ( ( topX < bottomX ) && ( topY < bottomY ) )
                        {
                            det.points[0] = Point2D_t{ topX, topY };
                            det.points[1] = Point2D_t{ bottomX, topY };
                            det.points[2] = Point2D_t{ bottomX, bottomY };
                            det.points[3] = Point2D_t{ topX, bottomY };
                            objs.objs.push_back( det );
                            QC_DEBUG( "[frame %" PRIu64 "- %" PRIu64
                                      "] class=%d score=%.3f points=[%.3f %.3f %.3f %.3f]",
                                      tensors.FrameId( 0 ), objs.objs.size() - 1, det.classId,
                                      det.prob, topX, topY, bottomX, bottomY );
                        }
                    }
                }
            }
        }
    }

    NMS( objs.objs, m_NMSThreshold );

    objs.frameId = tensors.FrameId( 0 );
    objs.timestamp = tensors.Timestamp( 0 );
    m_pub.Publish( objs );
    QC_DEBUG( "number of detections %" PRIu64 " for frame %" PRIu64, objs.objs.size(),
              tensors.FrameId( 0 ) );
}

QCStatus_e SamplePostProcCenternet::PostProcCL( DataFrames_t &tensors )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numArgs = 22;
    OpenclIface_WorkParams_t openclWorkParams;
    openclWorkParams.workDim = 3;
    size_t globalWorkSize[3] = { m_classNum, m_height, m_width };
    size_t globalWorkOffset[3] = { 0, 0, 0 };
    openclWorkParams.pGlobalWorkSize = globalWorkSize;
    openclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    openclWorkParams.pLocalWorkSize = NULL;

    uint32_t *pdetClsIds = (uint32_t *) m_outputClsIdBuf.data();
    *( pdetClsIds + MAX_OBJ_NUM ) = 0;

    ret = m_OpenclSrvObj.Execute( &m_kernel, m_openclArgs, numArgs, &openclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute BboxDet kernel" );
        ret = QC_STATUS_FAIL;
    }

    // non-maximum suppression
    Road2DObject_t detObj;
    Road2DObjects_t objs;
    uint32_t objNum = *( pdetClsIds + MAX_OBJ_NUM );
    float *pdetProbs = (float *) m_outputProbBuf.data();
    float *pdetCoords = (float *) m_outputCoordsBuf.data();
    for ( uint32_t i = 0; i < objNum; i++ )
    {
        uint32_t idx = i * 4;
        detObj.classId = *( pdetClsIds + i );
        detObj.prob = *( pdetProbs + i );
        detObj.points[0] = { *( pdetCoords + idx ), *( pdetCoords + idx + 1 ) };
        detObj.points[1] = { *( pdetCoords + idx + 2 ), *( pdetCoords + idx + 1 ) };
        detObj.points[2] = { *( pdetCoords + idx + 2 ), *( pdetCoords + idx + 3 ) };
        detObj.points[3] = { *( pdetCoords + idx ), *( pdetCoords + idx + 3 ) };
        objs.objs.push_back( detObj );
    }
    NMS( objs.objs, m_NMSThreshold );

    objs.frameId = tensors.FrameId( 0 );
    objs.timestamp = tensors.Timestamp( 0 );
    m_pub.Publish( objs );
    QC_DEBUG( "number of detections %" PRIu64 " for frame %" PRIu64, objs.objs.size(),
              tensors.FrameId( 0 ) );

    return ret;
}

void SamplePostProcCenternet::NMS( std::vector<Road2DObject_t> &boxes, float thres )
{
    std::sort( boxes.begin(), boxes.end(),
               []( const Road2DObject_t &a, const Road2DObject_t &b ) { return a.prob > b.prob; } );

    for ( auto it = boxes.begin(); it != boxes.end(); it++ )
    {
        Road2DObject_t &cand1 = *it;

        for ( auto jt = it + 1; jt != boxes.end(); )
        {
            Road2DObject_t &cand2 = *jt;
            if ( ComputeIou( cand1, cand2 ) >= thres )
                jt = boxes.erase( jt );   // Possible candidate for optimization
            else
                jt++;
        }
    }
}

float SamplePostProcCenternet::ComputeIou( const Road2DObject_t &box1, const Road2DObject_t &box2 )
{
    float box1_xmin = box1.points[0].x, box2_xmin = box2.points[0].x;
    float box1_ymin = box1.points[0].y, box2_ymin = box2.points[0].y;
    float box1_xmax = box1.points[2].x, box2_xmax = box2.points[2].x;
    float box1_ymax = box1.points[2].y, box2_ymax = box2.points[2].y;
    float ixmin = std::max( box1_xmin, box2_xmin );
    float iymin = std::max( box1_ymin, box2_ymin );
    float ixmax = std::min( box1_xmax, box2_xmax );
    float iymax = std::min( box1_ymax, box2_ymax );

    assert( ( box1_xmin <= box1_xmax ) && ( box1_ymin <= box1_ymax ) &&
            ( box2_xmin <= box2_xmax ) && ( box2_ymin <= box2_ymax ) );

    float iou = 0.0f;
    if ( ( ixmin < ixmax ) && ( iymin < iymax ) )
    {
        float intersection_area = ( ixmax - ixmin ) * ( iymax - iymin );
        // union = area1 + area2 - intersection
        float union_area = ( box1_xmax - box1_xmin ) * ( box1_ymax - box1_ymin ) +
                           ( box2_xmax - box2_xmin ) * ( box2_ymax - box2_ymin ) -
                           intersection_area;
        iou = ( union_area > 0.0f ) ? intersection_area / union_area : 0.0f;
    }
    return iou;
}

QCStatus_e SamplePostProcCenternet::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SamplePostProcCenternet::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_PROCESSOR_GPU == m_processor )
    {
        ret = m_OpenclSrvObj.Deinit();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Release CL resources failed!" );
            ret = QC_STATUS_FAIL;
        }
    }
    return ret;
}

REGISTER_SAMPLE( PostProcCenternet, SamplePostProcCenternet );

}   // namespace sample
}   // namespace QC
