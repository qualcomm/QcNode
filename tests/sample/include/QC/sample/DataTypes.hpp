// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef _QC_SAMPLE_DATA_TYPES_HPP_
#define _QC_SAMPLE_DATA_TYPES_HPP_

#include <vector>

#include "QC/sample/SharedBufferPool.hpp"

using namespace QC;

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
    /**
     * @brief Returns the buffer descriptor associated with the data frame.
     * @return A reference to the buffer descriptor.
     * @note The SharedBuffer_t structure now includes a member that references a
     * QCBufferDescriptorBase_t. As a result, DataFrame_t holds both the legacy QCSharedBuffer_t
     * descriptor and the new QCBufferDescriptorBase_t. Both descriptors point to the same buffer
     * data. This design allows for a smoother transition in phase 2 when updating QCNode, as it
     * maintains compatibility between the old and new buffer descriptor formats.
     */
    QCBufferDescriptorBase_t &GetBuffer() { return buffer->buffer; }
    void Workaound()
    {
        /* workaround to update the RideHal buffer tensor descriptor dims[0] */
        QCBufferDescriptorBase_t &bufDesc = GetBuffer();
        const TensorDescriptor_t *pTensor = dynamic_cast<const TensorDescriptor_t *>( &bufDesc );
        if ( nullptr != pTensor )
        {
            buffer->sharedBuffer.tensorProps.dims[0] = pTensor->dims[0];
        }

        /* workaround for case that buffer not allocated by sample app */
        if ( &bufDesc == &buffer->dummy )
        {
            if ( QC_BUFFER_TYPE_IMAGE == buffer->sharedBuffer.type )
            {
                buffer->imgDesc = buffer->sharedBuffer;
                buffer->buffer = buffer->imgDesc;
            }
            else
            {
                buffer->tensorDesc = buffer->sharedBuffer;
                buffer->buffer = buffer->tensorDesc;
            }
        }
        if ( nullptr == buffer->sharedBuffer.data() )
        {
            buffer->sharedBuffer = bufDesc;
        }
    }

    QCBufferType_e GetBufferType() { return buffer->GetBufferType(); }
    QCSharedBuffer_t &SharedBuffer() { return buffer->sharedBuffer; }
    void *GetDataPtr() { return buffer->GetDataPtr(); }
    uint32_t GetDataSize() { return buffer->GetDataSize(); }
    QCImageProps_t GetImageProps() { return buffer->GetImageProps(); };
    QCTensorProps_t GetTensorProps() { return buffer->GetTensorProps(); };
} DataFrame_t;

typedef struct
{
    std::vector<DataFrame_t> frames;

public:
    QCBufferDescriptorBase_t &GetBuffer( int index ) { return frames[index].GetBuffer(); }

    QCBufferType_e GetBufferType( int index ) { return frames[index].GetBufferType(); }
    QCSharedBuffer_t &SharedBuffer( int index ) { return frames[index].SharedBuffer(); }
    void *GetDataPtr( int index ) { return frames[index].GetDataPtr(); }
    uint32_t GetDataSize( int index ) { return frames[index].GetDataSize(); }

    QCImageProps_t GetImageProps( int index ) { return frames[index].GetImageProps(); };
    QCTensorProps_t GetTensorProps( int index ) { return frames[index].GetTensorProps(); };

    uint64_t FrameId( int index ) { return frames[index].frameId; };
    uint64_t Timestamp( int index ) { return frames[index].timestamp; };

    std::string Name( int index ) { return frames[index].name; };
    float QuantScale( int index ) { return frames[index].quantScale; };
    int32_t QuantOffset( int index ) { return frames[index].quantOffset; };

    void Add( DataFrame_t &frame )
    {
        frame.Workaound();
        frames.push_back( frame );
    }
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
