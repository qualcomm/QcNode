// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_DEFS_HPP
#define QC_NODE_DEFS_HPP

#include "QC/common/QCDefs.hpp"

namespace QC
{

typedef struct QCNodeID
{
    constexpr bool operator==( const QCNodeID &nodeId )
    {
        return ( ( id == nodeId.id ) && ( name.compare( nodeId.name ) == 0 ) );
    };
    constexpr bool operator!=( const QCNodeID &nodeId )
    {
        return ( ( id != nodeId.id ) || ( name.compare( nodeId.name ) != 0 ) );
    };
    std::string name;
    uint32_t id;
} QCNodeID_t;

/**
 * @brief QCNode Buffer Index Map Entry used for accessing QCFrameDescriptorNodeIfs.
 * @param name The buffer name.
 * @param globalBufferId The global buffer index.
 */
typedef struct QCNodeBufferMapEntry_t
{
    std::string name;
    uint32_t globalBufferId;
} QCNodeBufferMapEntry_t;

}   // namespace QC

#endif   // QC_NODE_DEFS_HPP