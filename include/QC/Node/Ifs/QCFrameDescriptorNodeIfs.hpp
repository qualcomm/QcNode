// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_FRAME_DESCRIPTOR_NODE_BASES_HPP
#define QC_FRAME_DESCRIPTOR_NODE_BASES_HPP

#include "QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"
#include <chrono>

namespace QC
{

using namespace QC::Memory;

/**
 * @brief QCNode Frame Descriptor for Node
 */
class QCFrameDescriptorNodeIfs
{
public:
    /**
     * @brief Get the buffer descriptor identified by globalBufferId.
     * @param[in] globalBufferId The global buffer index.
     * @return The buffer descriptor identified by globalBufferId.
     * @note Retrieves the specific buffer descriptor according to the enumeration provided as part
     * of the initialization structure. The mapping between the buffer ID from the general buffer
     * vector to the buffer ID in the context for a node will be shared as part of the node
     * configuration structure. Follow the usage of QCNodeBufferMapEntry_t in the QCNodeBase.hpp
     * file. If the globalBufferId is out of range, a dummy buffer descriptor is returned.
     */
    virtual QCBufferDescriptorBase_t &GetBuffer( uint32_t globalBufferId ) = 0;

    /**
     * @brief Set the buffer descriptor identified by globalBufferId.
     * @param[in] globalBufferId The global buffer index.
     * @param[in] buffer The buffer descriptor.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e SetBuffer( uint32_t globalBufferId, QCBufferDescriptorBase_t &buffer ) = 0;

    /**
     * @brief Clear all the buffer descriptor to dummy.
     * @return None.
     */
    virtual void Clear() = 0;

    /**
     * @brief Copies buffer descriptors from another QCFrameDescriptorNodeIfs object.
     * @param[in] other The QCFrameDescriptorNodeIfs object from which buffer descriptors are
     * copied.
     * @return The updated QCFrameDescriptorNodeIfs object.
     */
    virtual QCFrameDescriptorNodeIfs &operator=( QCFrameDescriptorNodeIfs &other ) = 0;
};

}   // namespace QC
#endif   // QC_FRAME_DESCRIPTOR_NODE_BASES_HPP
