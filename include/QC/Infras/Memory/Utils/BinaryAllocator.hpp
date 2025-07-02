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
#include "QC/Infras/Memory/Ifs/QCMemoryAllocatorIfs.hpp"

namespace QC
{
namespace Memory
{

class BinaryAllocator : public QCMemoryAllocatorIfs
{
public:
    /**
     * @brief Constructor for BinaryAllocator.
     * @param[in] name The name of the binary allocator.
     * @return None.
     */
    BinaryAllocator( const std::string &name );

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

private:
    QC_DECLARE_LOGGER();
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_BINARY_ALLOCATOR_HPP
