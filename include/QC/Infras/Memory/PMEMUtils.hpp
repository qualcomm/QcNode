// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_UTILS_PMEM_HPP
#define QC_MEMORY_UTILS_PMEM_HPP

#include "QC/Infras/Memory/UtilsBase.hpp"

namespace QC
{
namespace Memory
{

/**
 * @class PMEMUtils
 * @brief A concrete implementation of the QCMemoryUtilsIfs interface for PMEM allocation.
 *
 * This class provides methods for allocating and freeing memory from the PMEM software package.
 */
class PMEMUtils : public UtilsBase
{
public:
    /**
     * @brief Default constructor.
     * This constructor Intilizes members which do not require external parameters
     */
    PMEMUtils();

    /**
     * @brief Destructor for the PMEMUtils class.
     * This destructor releases any resources allocated by the ManagerLocal object.
     */
    virtual ~PMEMUtils();

    /**
     * @brief Maps a buffer into memory.
     * This method maps a buffer into memory and returns a status code indicating success or
     * failure.
     * @param orig The buffer descriptor to map into memory.
     * @param mapped The buffer descriptor mapped into memory.
     * @return The status of the memory mapping operation.
     */
    virtual QCStatus_e MemoryMap( const QCBufferDescriptorBase_t &orig,
                                  QCBufferDescriptorBase_t &mapped );

    /**
     * @brief Unmaps a buffer from memory.
     * This method unmaps a buffer from memory and returns a status code indicating success or
     * failure.
     * @param buff The buffer descriptor to unmap from memory.
     * @return The status of the memory unmapping operation.
     */
    virtual QCStatus_e MemoryUnMap( const QCBufferDescriptorBase_t &buff );

private:
    /**
     * @brief Declare the logger for this class.
     */
    QC_DECLARE_LOGGER();
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_UTILS_PMEM_HPP