// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_BUFFER_DESCRIPTOR_BASE_HPP
#define QC_BUFFER_DESCRIPTOR_BASE_HPP

#include "QC/Common/QCDefs.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryDefs.hpp"
#include <string>

namespace QC
{
namespace Memory
{

/**
 * @brief QCNode Buffer Descriptor Base
 * @param name The buffer name.
 * @param pBuf The buffer virtual address.
 * @param size The buffer size.
 * @param alignment The buffer alignment.
 * @param cache The buffer cache attributes.
 * @param allocatorType The buffer allocator type.
 * @param type The buffer type.
 * @param dmaHandle The buffer DMA handle.
 * @param pid the process id of the buffer allocator.
 */

/**
 * @struct QCBufferDescriptorBase_t
 * @brief Base structure for buffer descriptors.
 * This structure provides a common set of attributes that describe a buffer,
 * including its name, pointer, size, alignment, cache attributes, allocator type, and buffer type.
 */
typedef struct QCBufferDescriptorBase
{
public:
    /*
     * @brief Default constructor for the QCBufferDescriptorBase structure.
     * This constructor initializes the buffer descriptor with default values:
     * pBuf: nullptr
     * size: 0
     * alignment: QC_MEMORY_DEFAULT_ALIGNMENT (4KB)
     * cache: QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES (QC_CACHEABLE)
     * allocatorType: QC_MEMORY_ALLOCATOR_LAST
     * type: QC_BUFFER_TYPE_LAST
     * dmaHandle: 0 */
    QCBufferDescriptorBase()
        : pBuf( nullptr ),
          size( 0 ),
          alignment( QC_MEMORY_DEFAULT_ALLIGNMENT ),
          cache( QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES ),
          allocatorType( QC_MEMORY_ALLOCATOR_LAST ),
          type( QC_BUFFER_TYPE_LAST ),
          dmaHandle( 0 ),
          pid( 0 )
    {}

    /**
     * @brief Virtual destructor for the QCBufferDescriptorBase structure.
     * This destructor is declared as virtual to ensure that the correct destructor is called
     * when an object of a derived class is deleted through a pointer to the base class. */
    virtual ~QCBufferDescriptorBase() = default;

    // Overload operator< for MyKey
    bool operator<( const QCBufferDescriptorBase &other ) const
    {
        bool result = false;
        if ( pBuf != other.pBuf )
        {
            result = pBuf < other.pBuf;
        }
        else if ( size != other.size )
        {
            result = size < other.size;
        }
        else if ( dmaHandle != other.dmaHandle )
        {
            result = dmaHandle < other.dmaHandle;
        }
        else if ( pid != other.pid )
        {
            result = pid < other.pid;
        }
        else if ( alignment != other.alignment )
        {
            result = alignment < other.alignment;
        }
        else if ( cache != other.cache )
        {
            result = cache < other.cache;
        }
        else if ( allocatorType != other.allocatorType )
        {
            result = allocatorType < other.allocatorType;
        }
        else
        {
        }
        return result;
    }

    /**
     * @var name
     * @brief The name of the buffer.
     * This string specifies the name of the buffer, which can be used for identification
     * and debugging purposes. */
    std::string name;

    /**
     * @var pBuf
     * @brief The pointer to the buffer.
     * This pointer specifies the location of the buffer in memory. */
    void *pBuf;

    /**
     * @var size
     * @brief The size of the buffer in bytes.
     * This value specifies the total size of the buffer. */
    size_t size;

    /**
     * @var alignment
     * @brief The alignment of the buffer in bytes.
     * This value specifies the boundary at which the buffer is aligned. */
    QCAlignment_t alignment;

    /**
     * @var cache
     * @brief The cache attributes of the buffer.
     * This value specifies the cache behavior of the buffer, such as whether it is cacheable or
     * not.*/
    QCAllocationCache_e cache;

    /**
     * @var allocatorType
     * @brief The type of allocator used to allocate the buffer.
     * This value specifies the type of allocator that was used to allocate the buffer. */
    QCMemoryAllocator_e allocatorType;

    /**
     * @var type
     * @brief The type of buffer.
     * This value specifies the type of buffer, such as a tensor or image buffer. */
    QCBufferType_e type;

    /**
     * @var dmaHandle
     * @brief The DMA handle associated with the buffer.
     * This value specifies the DMA handle that is associated with the buffer. */
    uint64_t dmaHandle;

    /**
     * @var pid
     * @brief The Process identifier number.
     * This value specifies the identification number given to a process. */
    pid_t pid;

    /**
     * @brief Returns a pointer to the buffer's data.
     *
     * This method provides access to the raw memory where the buffer's content is stored.
     * The returned pointer may be used for reading or writing, depending on the buffer's usage
     * context.
     *
     * @return void* Pointer to the beginning of the buffer's data.
     */
    virtual void *GetDataPtr() const { return pBuf; }

    /**
     * @brief Returns the size of valid data in the buffer.
     *
     * This method indicates how many bytes of meaningful data are currently stored in the buffer.
     * It does not necessarily reflect the total allocated capacity.
     *
     * @return size_t Number of bytes of valid data in the buffer.
     */
    virtual size_t GetDataSize() const { return size; }

} QCBufferDescriptorBase_t;

}   // namespace Memory
}   // namespace QC

#endif   // QC_BUFFER_DESCRIPTOR_BASE_HPP
