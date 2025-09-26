// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_MEMORY_UTILS_DMABUFF_HPP
#define QC_MEMORY_UTILS_DMABUFF_HPP

#include "QC/Infras/Memory/UtilsBase.hpp"

namespace QC
{
namespace Memory
{

/**
 * @class DMABUFFUtils
 * @brief A concrete implementation of the QCMemoryUtilsIfs interface for DMABUFF allocation.
 *
 * This class provides methods for allocating and freeing memory from the DMABUFF software package.
 */
class DMABUFFUtils : public UtilsBase
{
public:
    /**
     * @brief Default constructor.
     * This constructor Intilizes members which do not require external parameters
     */
    DMABUFFUtils();
    /**
     * @brief Destructor for the DMABUFFUtils class.
     * This destructor releases any resources allocated by the ManagerLocal object.
     */
    ~DMABUFFUtils();

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

#endif   // QC_MEMORY_UTILS_DMABUFF_HPP