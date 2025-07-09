// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_DEFS_HPP
#define QC_MEMORY_DEFS_HPP

#include "QC/Common/QCDefs.hpp"
#include "QC/Infras/Memory/QCBufferDescriptorBase.hpp"

namespace QC
{

/**================================================================================================
** Typedefs
================================================================================================**/

typedef enum
{
    QC_NON_CACHEABLE,
    QC_CACHEABLE, /* impl. depended whether writethrough or writeback */
    QC_CACHEABLE_WRITE_THROUGH,
    QC_CACHEABLE_WRITE_BACK,
} QCAllocationAttribute_e;

typedef size_t QCAlignment_t;
typedef unsigned long QCCount_t;

static const QCAlignment_t QC_MEMORY_DEFAULT_ALLIGNMENT = 4 * 1024;
static const QCAllocationAttribute_e QC_MEMORY_DEFAULT_ATTRIBUTES = { QC_CACHEABLE };

typedef struct QCBufferPropBase
{
    virtual ~QCBufferPropBase() = default;
    QCBufferPropBase()
        : size( 0 ),
          alignment( QC_MEMORY_DEFAULT_ALLIGNMENT ),
          attr( QC_MEMORY_DEFAULT_ATTRIBUTES ) {};

    size_t size;
    QCAlignment_t alignment;
    QCAllocationAttribute_e attr;
} QCBufferPropBase_t;

#define QC_CALC_ALIGN_SIZE( size, ALLIGNMENT )                                                     \
    ( {                                                                                            \
        size_t ret;                                                                                \
        ret = size + ALLIGNMENT - 1ul;                                                             \
        ret;                                                                                       \
    } )

#define QC_ALIGN_POINTER( pVoid, ALLIGNMENT )                                                      \
    ( void *ptr; ( ptr = (void *) ( (long) pVoid + ALLIGNMENT - 1ul ) & ~( ALLIGNMENT - 1ul ) );   \
      ptr )

}   // namespace QC

#endif   // QC_MEMORY_DEFS_HPP
