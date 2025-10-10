// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_MEMORY_DEFS_HPP
#define QC_MEMORY_DEFS_HPP

#include "QC/Common/QCDefs.hpp"

namespace QC
{
namespace Memory
{

/**
 * @defgroup MemoryDefinitions Memory Definitions
 * @{
 */

/**================================================================================================
** Typedefs & Enumerations
================================================================================================**/

/**
 * @enum QCMemoryAllocator_e
 * @brief Enumerates the types of memory allocators.
 *
 * This enumeration defines the different types of memory allocators that can be used.
 */
typedef enum
{
    /**
     * @brief Heap memory allocator.
     */
    QC_MEMORY_ALLOCATOR_HEAP = 0,
    /**
     * @brief DMA (Direct Memory Access) memory allocator.
     */
    QC_MEMORY_ALLOCATOR_DMA,
    /**
     * @brief DMA memory allocator for camera devices.
     */
    QC_MEMORY_ALLOCATOR_DMA_CAMERA,
    /**
     * @brief DMA memory allocator for GPU (Graphics Processing Unit) devices.
     */
    QC_MEMORY_ALLOCATOR_DMA_GPU,
    /**
     * @brief DMA memory allocator for VPU (Video Processing Unit) devices.
     */
    QC_MEMORY_ALLOCATOR_DMA_VPU,
    /**
     * @brief DMA memory allocator for EVA (Embedded Vision Accelerator) devices.
     */
    QC_MEMORY_ALLOCATOR_DMA_EVA,
    /**
     * @brief DMA memory allocator for HTP (Hexagon  Tensor Processor) devices.
     */
    QC_MEMORY_ALLOCATOR_DMA_HTP,
    /**
     * @brief Last memory allocator type.
     */
    QC_MEMORY_ALLOCATOR_LAST,
    /**
     * @brief Maximum memory allocator type value.
     */
    QC_MEMORY_ALLOCATOR_MAX = UINT32_MAX
} QCMemoryAllocator_e;

/**
 * @enum QCAllocationCache_e
 * @brief Enumerates the cache attributes for memory allocations.
 *
 * This enumeration defines the different cache attributes that can be used for memory allocations.
 */
typedef enum
{
    /**
     * @brief Non-cacheable memory.
     */
    QC_CACHEABLE_NON = 0,
    /**
     * @brief Cacheable memory.
     */
    QC_CACHEABLE,
    /**
     * @brief Write-through cacheable memory.
     */
    QC_CACHEABLE_WRITE_THROUGH,
    /**
     * @brief Write-back cacheable memory.
     */
    QC_CACHEABLE_WRITE_BACK,
    /**
     * @brief Last cache attribute.
     */
    QC_CACHEABLE_LAST,
    /**
     * @brief Maximum cache attribute value.
     */
    QC_CACHEABLE_MAX = UINT32_MAX
} QCAllocationCache_e;

/**
 * @typedef QCAlignment_t
 * @brief Type definition for memory alignment values.
 *
 * This type is used to represent memory alignment values.
 */
typedef size_t QCAlignment_t;

/**
 * @typedef QCCount_t
 * @brief Type definition for count values.
 *
 * This type is used to represent count values.
 */
typedef unsigned long QCCount_t;

/**
 * @def QC_MEMORY_DEFAULT_ALLIGNMENT
 * @brief Default memory alignment value.
 *
 * This constant specifies the default memory alignment value.
 */
static const QCAlignment_t QC_MEMORY_DEFAULT_ALLIGNMENT = 4 * 1024;

/**
 * @def QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES
 * @brief Default cache attributes for memory allocations.
 *
 * This constant specifies the default cache attributes for memory allocations.
 */
static const QCAllocationCache_e QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES = { QC_CACHEABLE };

/**
 * @struct QCBufferPropBase_t
 * @brief Structure for buffer properties.
 *
 * This structure provides a common set of properties that can be used to describe a buffer.
 */
typedef struct QCBufferPropBase
{
    /**
     * @brief Virtual destructor.
     *
     * This destructor is declared as virtual to ensure that the correct destructor is called
     * when an object of a derived class is deleted through a pointer to the base class.
     */
    virtual ~QCBufferPropBase() = default;

    /**
     * @brief Constructor for the QCBufferPropBase structure.
     *
     * This constructor initializes the QCBufferPropBase structure with default values.
     */
    QCBufferPropBase()
        : size( 0 ),
          alignment( QC_MEMORY_DEFAULT_ALLIGNMENT ),
          cache( QC_MEMORY_DEFAULT_CACHE_ATTRIBUTES ){};

    /**
     * @var size
     * @brief The size of the buffer.
     */
    size_t size;

    /**
     * @var alignment
     * @brief The alignment of the buffer.
     */
    QCAlignment_t alignment;

    /**
     * @var cache
     * @brief The cache attributes of the buffer.
     */
    QCAllocationCache_e cache;

} QCBufferPropBase_t;

/**
 * @def QC_CALC_ALIGN_SIZE
 * @brief Calculates the aligned size of a buffer.
 *
 * This macro calculates the aligned size of a buffer based on the provided size and alignment.
 *
 * @param size The size of the buffer.
 * @param ALLIGNMENT The alignment of the buffer.
 */
#define QC_CALC_ALIGN_SIZE( size, ALLIGNMENT )                                                     \
    ( {                                                                                            \
        size_t ret;                                                                                \
        ret = size + ALLIGNMENT - 1ul;                                                             \
        ret;                                                                                       \
    } )

/**
 * @def QC_ALIGN_POINTER
 * @brief Aligns a pointer to a buffer.
 *
 * This macro aligns a pointer to a buffer based on the provided alignment.
 *
 * @param pVoid The pointer to the buffer.
 * @param ALLIGNMENT The alignment of the buffer.
 */
#define QC_ALIGN_POINTER( pVoid, ALLIGNMENT )                                                      \
    ( void *ptr; ( ptr = (void *) ( (long) pVoid + ALLIGNMENT - 1ul ) & ~( ALLIGNMENT - 1ul ) );   \
      ptr )

/**
 * @def QC_ALIGN_SIZE
 * @brief Provide the aligned size of a buffer.
 *
 * This macro calculates the aligned size of a buffer based on the provided size and alignment.
 *
 * @param size The size of the buffer.
 * @param ALLIGNMENT The alignment of the buffer.
 */
#define QC_ALIGN_SIZE( size, ALLIGNMENT )                                                          \
    ( ( ( ( size ) + ( ALLIGNMENT ) - 1 ) / ( ALLIGNMENT ) ) * ( ALLIGNMENT ) )

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_DEFS_HPP
