// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_MEMORY_DMABUFF_ALLOCATOR_HPP
#define QC_MEMORY_DMABUFF_ALLOCATOR_HPP

#include "QC/Common/Types.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryAllocatorIfs.hpp"
#include <fcntl.h>
#include <linux/dma-heap.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace QC
{
namespace Memory
{

/**
 * @class DMABUFFAllocator
 * @brief A concrete implementation of the QCMemoryAllocatorIfs interface for DMA buffer allocation.
 * This class provides methods for allocating and freeing DMA buffers.
 */
class DMABUFFAllocator : public QCMemoryAllocatorIfs
{
public:
    /**
     * @brief Deleted default constructor.
     *
     * This constructor is deleted to prevent default initialization.
     */
    DMABUFFAllocator() = delete;
    /**
     * @brief Constructor for the DMABUFFAllocator class.
     * This constructor initializes the DMABUFFAllocator object with the provided
     * configuration and allocator type.
     * @param config The configuration for the allocator.
     * @param allocator The type of allocator.
     */
    DMABUFFAllocator( const QCMemoryAllocatorConfigInit_t &config,
                      const QCMemoryAllocator_e allocator );

    /**
     * @brief Destructor for the DMABUFFAllocator class.
     * This destructor releases any resources allocated by the DMABUFFAllocator object.
     */
    ~DMABUFFAllocator();

    /**
     * @brief Allocate a buffer with the specified properties.
     * @param request The properties of the buffer to allocate.
     * @param response The descriptor of the allocated buffer.
     * @return The status of the allocation operation.
     */
    QCStatus_e Allocate( const QCBufferPropBase_t &request, QCBufferDescriptorBase_t &response );

    /**
     * @brief Free a buffer with the specified descriptor.
     * @param BufferDescriptor The descriptor of the buffer to free.
     * @return The status of the free operation.
     */
    virtual QCStatus_e Free( const QCBufferDescriptorBase_t &BufferDescriptor );

private:
    /**
     * @var dmaBufDevFdCached
     * @brief The DMA BUFF device file descriptor for cached memory.
     */
    int32_t m_dmaBufDevFdCached;
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_DMABUFF_ALLOCATOR_HPP
