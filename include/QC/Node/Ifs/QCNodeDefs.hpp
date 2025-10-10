// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_NODE_DEFS_HPP
#define QC_NODE_DEFS_HPP

#include "QC/Common/QCDefs.hpp"
#include <string>

namespace QC
{

/**
 * @struct QCNodeID_t
 * @brief Structure representing a QCNode ID.
 * This structure contains a name and an ID that uniquely identify a QCNode. */
typedef struct QCNodeID
{
    constexpr bool operator==( const QCNodeID &nodeId )
    {
        return ( ( type == nodeId.type ) && ( 0 == name.compare( nodeId.name ) == 0 ) &&
                 ( id == nodeId.id ) );
    };
    constexpr bool operator!=( const QCNodeID &nodeId )
    {
        return ( ( type != nodeId.type ) || ( name.compare( nodeId.name ) != 0 ) ||
                 ( id != nodeId.id ) );
    };

    /**
     * @var name
     * @brief The unique name of the QCNode. */
    std::string name;

    /**
     * @var type
     * @brief The ID of the QCNode. */
    QCNodeType_e type;

    /**
     * @var id
     * @brief The unique id of the QCNode.
     * The id values are zero based countinuous value*/
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
