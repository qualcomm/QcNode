// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_MEMORY_PMEM_ALLOCATOR_HPP
#define QC_MEMORY_PMEM_ALLOCATOR_HPP

#include "QC/Common/Types.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryAllocatorIfs.hpp"

namespace QC
{
namespace Memory
{

/**
 * @class PMEMAllocator
 * @brief A concrete implementation of the QCMemoryAllocatorIfs interface for PMEM allocation.
 *
 * This class provides methods for allocating and freeing memory from the PMEM software package.
 */
class PMEMAllocator : public QCMemoryAllocatorIfs
{
public:
    /**
     * @brief Deleted default constructor.
     *
     * This constructor is deleted to prevent default initialization.
     */
    PMEMAllocator() = delete;

    /**
     * @brief Constructor for PMEMAllocator.
     * @return None.
     */
    PMEMAllocator( const QCMemoryAllocatorConfigInit_t &config,
                   const QCMemoryAllocator_e allocator );

    /**
     * @brief Destructor for the PMEMAllocator class.
     * This destructor releases any resources allocated by the PMEMAllocator object.
     */
    virtual ~PMEMAllocator();

    /**
     * @brief Allocate an memory buffer with the specified properties.
     * @param[in] request The buffer detailed properties, a reference to QCBufferPropBase_t.
     * @param[out] response The allocated buffer descriptor, a reference to
     * QCBufferDescriptorBase_t.
     * @return QC_STATUS_OK on success, other status codes on failure.
     */
    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response );

    /**
     * @brief Free a buffer with the specified descriptor.
     * @param BufferDescriptor The descriptor of the buffer to free.
     * @return The status of the free operation.
     */
    virtual QCStatus_e Free( const QCBufferDescriptorBase_t &BufferDescriptor );

private:
    inline QCStatus_e AllocaotrorToPMEM_ID( const QCMemoryAllocator_e &allocatorType,
                                            uint32_t &pmemId );
};


}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_PMEM_ALLOCATOR_HPP
