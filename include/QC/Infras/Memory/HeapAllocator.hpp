// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_MEMORY_HEAP_ALLOCATOR_HPP
#define QC_MEMORY_HEAP_ALLOCATOR_HPP

#include "QC/Common/Types.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryAllocatorIfs.hpp"

namespace QC
{
namespace Memory
{

/**
 * @class HeapAllocator
 * @brief A concrete implementation of the QCMemoryAllocatorIfs interface for heap allocation.
 *
 * This class provides methods for allocating and freeing memory from the heap.
 */
class HeapAllocator : public QCMemoryAllocatorIfs
{
public:
    /**
     * @brief Constructor for the HeapAllocator class.
     * This constructor initializes the HeapAllocator object with a default configuration.
     */
    HeapAllocator();

    /**
     * @brief Destructor for the HeapAllocator class.
     *
     * This destructor releases any resources allocated by the HeapAllocator object.
     */
    ~HeapAllocator();

    /**
     * @brief Allocate a buffer with the specified properties.
     * This method allocates a buffer with the specified properties from the heap.
     * @param request The properties of the buffer to allocate.
     * @param response The descriptor of the allocated buffer.
     * @return The status of the allocation operation.
     */
    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response );

    /**
     * @brief Free a buffer with the specified descriptor.
     *
     * This method frees a buffer with the specified descriptor that was allocated from the heap.
     *
     * @param buff The descriptor of the buffer to free.
     * @return The status of the free operation.
     */
    virtual QCStatus_e Free( const QCBufferDescriptorBase_t &buff );
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_HEAP_ALLOCATOR_HPP
