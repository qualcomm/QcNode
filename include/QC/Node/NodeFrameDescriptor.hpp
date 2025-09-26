// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_NODE_FRAME_DESCRIPTOR_HPP
#define QC_NODE_FRAME_DESCRIPTOR_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "QC/Node/Ifs/QCFrameDescriptorNodeIfs.hpp"

namespace QC
{
namespace Node
{
using namespace QC::Memory;

/**
 * @brief QCNode Dummy Buffer Descriptor
 * This is used by QCSharedFrameDescriptorNode to indicate that the buffer identified by
 * globalBufferId is dummy and not provided by the user application or QCNode.
 */
typedef struct QCDummyBufferDescriptor : public QCBufferDescriptorBase_t
{
public:
    QCDummyBufferDescriptor() noexcept
    {
        name = "Dummy";
        type = QC_BUFFER_TYPE_MAX;
    }
    virtual ~QCDummyBufferDescriptor() = default;
} QCDummyBufferDescriptor_t;

/**
 * @brief QCNode Node Frame Descriptor
 * This will be used by QCNode API ProcessFrameDescriptor and QCNodeInit::callback to pass the
 * buffer information.
 */
class NodeFrameDescriptor : public QCFrameDescriptorNodeIfs
{
public:
    NodeFrameDescriptor() = delete;
    NodeFrameDescriptor( uint32_t numOfBuffers ) noexcept : m_buffers( numOfBuffers, Dummy() ) {}
    ~NodeFrameDescriptor() {}

    /**
     * @brief Copies buffer descriptors from another QCFrameDescriptorNodeIfs object.
     * @param[in] other The QCFrameDescriptorNodeIfs object from which buffer descriptors are
     * copied.
     * @return The updated QCFrameDescriptorNodeIfs object.
     */
    QCFrameDescriptorNodeIfs &operator=( QCFrameDescriptorNodeIfs &other )
    {
        if ( this != &other )
        {
            for ( size_t i = 0; i < m_buffers.size(); i++ )
            {
                QCBufferDescriptorBase_t &bufDesc = other.GetBuffer( static_cast<uint32_t>( i ) );
                m_buffers[i] = bufDesc;
            }
        }
        return *this;
    }

    NodeFrameDescriptor &operator=( NodeFrameDescriptor &other )
    {
        if ( this != &other )
        {
            for ( size_t i = 0; i < m_buffers.size(); i++ )
            {
                QCBufferDescriptorBase_t &bufDesc = other.GetBuffer( static_cast<uint32_t>( i ) );
                m_buffers[i] = bufDesc;
            }
        }
        return *this;
    }

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
    virtual QCBufferDescriptorBase_t &GetBuffer( uint32_t globalBufferId )
    {
        std::reference_wrapper<QCBufferDescriptorBase_t> buffer = Dummy();
        if ( globalBufferId < m_buffers.size() )
        {
            buffer = m_buffers[globalBufferId];
        }
        return buffer;
    }

    /**
     * @brief Set the buffer descriptor identified by globalBufferId.
     * @param[in] globalBufferId The global buffer index.
     * @param[in] buffer The buffer descriptor.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e SetBuffer( uint32_t globalBufferId, QCBufferDescriptorBase_t &buffer )
    {
        QCStatus_e status = QC_STATUS_OK;
        if ( globalBufferId < m_buffers.size() )
        {
            m_buffers[globalBufferId] = buffer;
        }
        else
        {
            status = QC_STATUS_OUT_OF_BOUND;
        }
        return status;
    }

    /**
     * @brief Clear all the buffer descriptor to dummy.
     * @return None.
     */
    virtual void Clear()
    {
        for ( auto &buffer : m_buffers )
        {
            buffer = Dummy();
        }
    }

private:
    QCDummyBufferDescriptor_t &Dummy() { return s_dummy; }

    std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> m_buffers;
    static QCDummyBufferDescriptor_t s_dummy;
};

// @deprecated QCSharedFrameDescriptorNode will be removed after phase 2
using QCSharedFrameDescriptorNode = NodeFrameDescriptor;

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_FRAME_DESCRIPTOR_HPP
