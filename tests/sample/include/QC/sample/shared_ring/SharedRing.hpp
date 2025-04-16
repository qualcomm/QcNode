// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_SHARED_RING_HPP
#define QC_SAMPLE_SHARED_RING_HPP

#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <errno.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "QC/common/Types.hpp"
#include "QC/infras/logger/Logger.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"

#include "QC/sample/shared_ring/SpinLock.hpp"

using namespace QC::common;

namespace QC
{
namespace sample
{
namespace shared_ring
{

#ifndef SHARED_RING_NUM_DATA_FRAMES
#define SHARED_RING_NUM_DATA_FRAMES 32
#endif

#ifndef SHARED_RING_NUM_SUBSCRIBERS
#define SHARED_RING_NUM_SUBSCRIBERS 32
#endif

#ifndef SHARED_RING_NUM_DESC
/* @note this value must be power of 2 */
#define SHARED_RING_NUM_DESC 32
#endif

#ifndef SHARED_RING_NAME_MAX
#define SHARED_RING_NAME_MAX 256
#endif

#define SHARED_RING_MEMORY_UNINITIALIZED 0
#define SHARED_RING_MEMORY_INITIALIZED 1
#define SHARED_RING_MEMORY_DESTROYED 2
#define SHARED_RING_MEMORY_CORRUPTED 3

#define SHARED_RING_USED_UNINITIALIZED 0
#define SHARED_RING_USED_INITIALIZED 1
#define SHARED_RING_USED_DESTROYED 2
#define SHARED_RING_USED_CORRUPTED 3

typedef struct SharedRing_DataFrame
{
    QCSharedBuffer_t buf;

    /* Extra DataFrame information for Image and Tensor */
    uint64_t frameId;
    uint64_t timestamp;

    /* Extra DataFrame information for Tensor */
    float quantScale;
    int32_t quantOffset;
    char name[SHARED_RING_NAME_MAX];
    uint8_t reserved[1024 - ( sizeof( QCSharedBuffer_t ) + sizeof( uint64_t ) * 2 +
                              sizeof( float ) + sizeof( int32_t ) + SHARED_RING_NAME_MAX )];

public:
    void SetName( std::string name );
} SharedRing_DataFrame_t;

typedef struct
{
    SharedRing_DataFrame_t dataFrames[SHARED_RING_NUM_DATA_FRAMES];
    uint64_t timestamp;
    uint32_t numDataFrames;
    int32_t ref; /**< atomic referece counter, used to do life cycle management */
    uint8_t reserved[1024 - ( sizeof( uint64_t ) + sizeof( uint32_t ) + sizeof( int32_t ) )];
} SharedRing_Desc_t;

typedef struct SharedRing_Ring
{
    QCSpinLock_t spinlock;
    int32_t reserved; /**< if true, this ring was in used status */
    int32_t status;   /**< status of this ring message queue */
    uint32_t queueDepth;
    uint16_t readIdx;
    uint16_t writeIdx;
    uint16_t ring[SHARED_RING_NUM_DESC];
    char name[SHARED_RING_NAME_MAX];
    uint8_t reserved0[4096 -
                      ( sizeof( QCSpinLock_t ) + sizeof( int32_t ) * 2 + sizeof( uint32_t ) +
                        sizeof( uint16_t ) * ( SHARED_RING_NUM_DESC + 2 ) + SHARED_RING_NAME_MAX )];

public:
    QCStatus_e Push( uint16_t idx );
    QCStatus_e Pop( uint16_t &idx );
    uint32_t Size();
    void SetName( std::string name );
} SharedRing_Ring_t;

typedef struct SharedRing_Memory
{
    /* status of this shared ring memory */
    int32_t status;

    uint8_t reserved0[4096 - sizeof( int32_t )];

    /**< the descriptors that used to hold the shared buffer information */
    SharedRing_Desc_t descs[SHARED_RING_NUM_DESC];

    /**< ring that hold all the desc that can be used to publish message */
    SharedRing_Ring_t avail;

    /**< ring that hold all the desc that released by consumers */
    SharedRing_Ring_t free;

    /**< dedicated used ring for each consumer */
    SharedRing_Ring_t used[SHARED_RING_NUM_SUBSCRIBERS];

    char name[SHARED_RING_NAME_MAX];

    uint8_t reserved1[4096 - SHARED_RING_NAME_MAX];

public:
    void SetName( std::string name );
} SharedRing_Memory_t;

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_SHARED_RING_HPP
