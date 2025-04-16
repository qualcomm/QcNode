// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef _QC_SAMPLE_DATA_TYPES_HPP_
#define _QC_SAMPLE_DATA_TYPES_HPP_

#include <vector>

#include "QC/sample/SharedBufferPool.hpp"

using namespace QC::common;

namespace QC
{
namespace sample
{

#define ROAD_2D_OBJECT_NUM_POINTS 4

typedef struct
{
    std::shared_ptr<SharedBuffer_t> buffer;
    /* Extra DataFrame information for Image and Tensor */
    uint64_t frameId;
    uint64_t timestamp;

    /* Extra DataFrame information for Tensor */
    std::string name;
    float quantScale;
    int32_t quantOffset;

public:
    QCBufferType_e BufferType() { return buffer->sharedBuffer.type; }
    QCSharedBuffer_t &SharedBuffer() { return buffer->sharedBuffer; }
    void *data() { return buffer->sharedBuffer.data(); }
    uint32_t size() { return buffer->sharedBuffer.size; }
} DataFrame_t;

typedef struct
{
    std::vector<DataFrame_t> frames;

public:
    QCBufferType_e BufferType( int index ) { return frames[index].BufferType(); }
    QCSharedBuffer_t &SharedBuffer( int index ) { return frames[index].SharedBuffer(); }
    void *data( int index ) { return frames[index].data(); }
    uint32_t size( int index ) { return frames[index].size(); }

    uint64_t FrameId( int index ) { return frames[index].frameId; };
    uint64_t Timestamp( int index ) { return frames[index].timestamp; };

    std::string Name( int index ) { return frames[index].name; };
    float QuantScale( int index ) { return frames[index].quantScale; };
    int32_t QuantOffset( int index ) { return frames[index].quantOffset; };

    void Add( DataFrame_t &frame ) { frames.push_back( frame ); }
} DataFrames_t;

typedef struct
{
    float x;
    float y;
} Point2D_t;

/** @brief 2D road object represent with 4 cornor points
 *  A bbox with yaw = 0, looks like:
 *   pt0                pt1
 *   +-------------------+
 *   |                   |
 *   |                   |
 *   |                   |
 *   +-------------------+
 *  pt3                pt2
 */
typedef struct
{
    int classId;
    float prob;
    Point2D_t points[ROAD_2D_OBJECT_NUM_POINTS];
} Road2DObject_t;

typedef struct
{
    std::vector<Road2DObject_t> objs;
    uint64_t frameId;
    uint64_t timestamp;
} Road2DObjects_t;

typedef struct
{
    std::string name;           /**< The name of tensor */
    QCTensorProps_t properties; /**< The property of tensor */
    float quantScale;           /**< The value of quantization scale */
    int32_t quantOffset;        /**< The value of quantization offset */
} TensorInfo_t;

typedef struct
{
    std::vector<TensorInfo_t> inputs;
    std::vector<TensorInfo_t> outputs;
} ModelInOutInfo_t;

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_DATA_TYPES_HPP_
