// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_BINARY_ALLOCATOR_HPP
#define QC_BINARY_ALLOCATOR_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/BufferDescriptor.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryManagerIfs.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"

namespace QC
{
namespace Memory
{

class BinaryAllocator
{
public:
    /**
     * @brief Constructor for BinaryAllocator.
     * @param[in] name The name of the binary allocator.
     * @return None.
     * @deprecated Will be removed
     */
    BinaryAllocator( const std::string &name );

    /**
     * @brief Constructor for BinaryAllocator.
     * @param[in] nodeId The nodeId of the binary allocator.
     * @param[in] logLevel The log level of the binary allocator.
     * @return None.
     */
    BinaryAllocator( const QCNodeID_t &nodeId, Logger_Level_e logLevel = LOGGER_LEVEL_ERROR );

    virtual ~BinaryAllocator();

    /**
     * @brief Allocates a binary raw memory buffer.
     * @param[in] request The QCNode DMA buffer properties, a referece to BufferProps_t.
     * @param[out] response The QCNode shared DMA buffer descriptor, a reference to
     * BufferDescriptor_t.
     * @return QC_STATUS_OK on success, other status codes on failure.
     * @example
     * @code
     * BufferProps_t request( 1024, QC_BUFFER_USAGE_DEFAULT, QC_CACHEABLE );
     * BufferDescriptor_t response;
     * QCStatus_e status = allocator.Allocate( request, response );
     * @endcode
     */
    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response );

    /**
     * @brief Frees the memory buffer.
     * @param[in] buffer The buffer descriptor.
     * @return QC_STATUS_OK on success, other status codes on failure.
     */
    virtual QCStatus_e Free( const QCBufferDescriptorBase_t &buffer );

protected:
    QC_DECLARE_LOGGER();

    QCNodeID_t m_nodeId;
    QCMemoryHandle_t m_memoryHandle;
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_BINARY_ALLOCATOR_HPP
