// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_BUFFER_DESCRIPTOR_HPP
#define QC_BUFFER_DESCRIPTOR_HPP

#include "QC/Common/QCDefs.hpp"
#include "QC/Common/Types.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryDefs.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"

#include <string>
#include <vector>

namespace QC
{
namespace Memory
{

/**
 * @brief QCNode DMA Buffer Properties.
 *
 * Inherited Members from QCBufferPropBase:
 * @param size The total required buffer size.
 * @param alignment The alignment requirement of the buffer.
 * @param attr The attributes of the buffer.
 *
 * New Members:
 * @param allocatorType The allocator type of the buffer.
 */
typedef struct BufferProps : public QCBufferPropBase
{
public:
    /**
     * @brief Constructor for BufferProps.
     * @return None.
     */
    BufferProps() : QCBufferPropBase(), allocatorType( QC_MEMORY_ALLOCATOR_DMA ) {}


    /**
     * @brief Constructor for BufferProps.
     * @param size The total required buffer size.
     * @param allocatorType The allocator type of the buffer, default is QC_MEMORY_ALLOCATOR_DMA.
     * @param cache The cache attributes of the buffer, default is QC_CACHEABLE.
     * @return None.
     * @note This constructor sets up the properties of the buffer with the given parameters, while
     * most other properties are set to their default suggested values.
     */
    BufferProps( size_t size, QCMemoryAllocator_e allocatorType = QC_MEMORY_ALLOCATOR_DMA,
                 QCAllocationCache_e cache = QC_CACHEABLE )
        : QCBufferPropBase(),
          allocatorType( allocatorType )
    {
        this->size = size;
        this->cache = cache;
    }

    QCMemoryAllocator_e allocatorType;
} BufferProps_t;


/**
 * @brief Descriptor for QCNode Shared DMA Buffer.
 *
 * This structure represents the shared DMA buffer descriptor for QCNode. It extends the
 * QCBufferDescriptorBase and includes additional members specific to DMA buffers.
 *
 * Inherited Members from QCBufferDescriptorBase:
 * @param name The name of the buffer.
 * @param pBuf The virtual address of the dma buffer.
 * @param size The dma size of the buffer.
 * @param type The type of the buffer.
 * @param alignment The alignment of the buffer.
 * @param cache The cache type of the buffer.
 * @param allocatorType The allocaor type used for allocation the buffer.
 * @param dmaHandle The dmaHandle of the buffer.
 * @param pid The process ID of the buffer.
 *
 * New Members:
 * @param validSize The size of valid data currently stored in the buffer.
 * @param offset The offset of the valid buffer within the shared buffer.
 * @param id The unique ID assigned by the buffer manager.
 *
 * @note The shared concept here means that pBuf, dmaHandle, and size are not changeable and
 * always represent one large DMA buffer allocated through platform DMA buffer-related APIs. For
 * QNX, this is the PMEM API, and for Linux, it is the dma-buf API. The offset and validSize will
 * be adjusted to describe the memory portion of the large DMA buffer that should be actually used
 * by the QCNode. Generally:
 *    validSize + offset <= size;
 */
typedef struct BufferDescriptor : public QCBufferDescriptorBase_t
{
public:
    BufferDescriptor() : QCBufferDescriptorBase_t(), validSize( 0 ), offset( 0 ), id( 0 ) {}

    /**
     * @brief Sets up the buffer descriptor from another buffer descriptor object.
     * @param[in] other The buffer descriptor object from which buffer members are copied.
     * @return The updated buffer descriptor object.
     */
    BufferDescriptor &operator=( BufferDescriptor &other );

    size_t validSize;
    size_t offset;
    uint64_t id;

    /**
     * @brief Returns a pointer to the valid data in the buffer.
     *
     * This method returns a pointer to the start of the valid data within the buffer,
     * accounting for any internal offset. The returned pointer may not point to the
     * beginning of the allocated memory, but rather to the portion containing meaningful data.
     *
     * @return void* Pointer to the valid data in the buffer.
     */
    virtual void *GetDataPtr() const { return (void *) ( (uint8_t *) pBuf + offset ); }

    /**
     * @brief Returns the size of valid data in the buffer.
     *
     * This method indicates the number of bytes of meaningful data currently stored in the buffer.
     * It does not reflect the total allocated size, only the portion considered valid for
     * processing.
     *
     * @return size_t Size of valid data in bytes.
     */
    virtual size_t GetDataSize() const { return validSize; }

} BufferDescriptor_t;

}   // namespace Memory
}   // namespace QC

#endif   // QC_BUFFER_DESCRIPTOR_HPP
