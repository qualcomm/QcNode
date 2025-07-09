// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

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
 * @param usage The intended usage of the buffer.
 */
typedef struct BufferProps : public QCBufferPropBase
{
public:
    /**
     * @brief Constructor for BufferProps.
     * @return None.
     */
    BufferProps() : QCBufferPropBase(), usage( QC_BUFFER_USAGE_DEFAULT ) {}


    /**
     * @brief Constructor for BufferProps.
     * @param size The total required buffer size.
     * @param usage The intended usage of the buffer, default is QC_BUFFER_USAGE_DEFAULT.
     * @param attr The attributes of the buffer, default is QC_CACHEABLE.
     * @return None.
     * @note This constructor sets up the properties of the buffer with the given parameters, while
     * most other properties are set to their default suggested values.
     */
    BufferProps( size_t size, QCBufferUsage_e usage = QC_BUFFER_USAGE_DEFAULT,
                 QCAllocationAttribute_e attr = QC_CACHEABLE )
        : QCBufferPropBase(),
          usage( usage )
    {
        this->size = size;
        this->attr = attr;
    }

    QCBufferUsage_e usage;
} BufferProps_t;


/**
 * @brief Descriptor for QCNode Shared DMA Buffer.
 *
 * This structure represents the shared DMA buffer descriptor for QCNode. It extends the
 * QCBufferDescriptorBase and includes additional members specific to DMA buffers.
 *
 * Inherited Members from QCBufferDescriptorBase:
 * @param name The name of the buffer.
 * @param pBuf The virtual address of the actual buffer.
 * @param size The actual size of the buffer.
 * @param type The type of the buffer.
 *
 * New Members:
 * @param pBufBase The base virtual address of the DMA buffer.
 * @param dmaHandle The DMA handle of the buffer.
 * @param dmaSize The size of the DMA buffer.
 * @param offset The offset of the valid buffer within the shared buffer.
 * @param id The unique ID assigned by the buffer manager.
 * @param pid The process ID that allocated this buffer.
 * @param usage The intended usage of the buffer.
 * @param attr The attributes associated with the buffer.
 *
 * @note The shared concept here means that pBufBase, dmaHandle, and dmaSize are not changeable and
 * always represent one large DMA buffer allocated through platform DMA buffer-related APIs. For
 * QNX, this is the PMEM API, and for Linux, it is the dma-buf API. The offset and size will be
 * adjusted to describe the memory portion of the large DMA buffer that should be actually used by
 * the QCNode. Generally:
 *      pBuf = pBufBase + offset;
 *      size + offset <= dmaSize;
 */
typedef struct BufferDescriptor : public QCBufferDescriptorBase
{
public:
    BufferDescriptor()
    {
        pBufBase = nullptr;
        pBuf = nullptr;
        size = 0;
    }
    /**
     * @brief Sets up the buffer descriptor from another buffer descriptor object.
     * @param[in] other The buffer descriptor object from which buffer members are copied.
     * @return The updated buffer descriptor object.
     */
    BufferDescriptor &operator=( BufferDescriptor &other );

    void *pBufBase;
    uint64_t dmaHandle;
    size_t dmaSize;
    size_t offset;
    uint64_t id;
    uint64_t pid;
    QCBufferUsage_e usage;
    QCAllocationAttribute_e attr;
} BufferDescriptor_t;

}   // namespace Memory
}   // namespace QC

#endif   // QC_BUFFER_DESCRIPTOR_HPP
