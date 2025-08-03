
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_UTILS_IFS_HPP
#define QC_MEMORY_UTILS_IFS_HPP

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"

namespace QC
{
namespace Memory
{

/**
 * @class QCMemoryUtilsIfs
 * @brief Interface for memory utility classes.
 * This class provides a common interface for memory utilities,
 * including methods for memory mapping, unmapping, and calculating buffer sizes for tensors and
 * images.
 */
class QCMemoryUtilsIfs
{
public:
    /**
     * @brief Maps a buffer into memory.
     * This method maps a buffer into memory and returns a status code indicating success or
     * failure.
     * @param orig The buffer descriptor to map into memory.
     * @param mapped The buffer descriptor mapped into memory.
     * @return The status of the memory mapping operation.
     */
    virtual QCStatus_e MemoryMap( const QCBufferDescriptorBase_t &orig,
                                  QCBufferDescriptorBase_t &mapped ) = 0;

    /**
     * @brief Unmaps a buffer from memory.
     * This method unmaps a buffer from memory and returns a status code indicating success or
     * failure.
     * @param buff The buffer descriptor to unmap from memory.
     * @return The status of the memory unmapping operation.
     */
    virtual QCStatus_e MemoryUnMap( const QCBufferDescriptorBase_t &buff ) = 0;

    /**
     * @brief Calculates the buffer size for a tensor.
     * This method calculates the buffer size for a tensor based on its properties and returns a
     * status code indicating success or failure.
     * @param prop The tensor properties.
     * @return The status of the buffer size calculation operation.
     */
    virtual QCStatus_e SetTensorBuffSizeFromTensorProp( TensorProps_t &prop ) = 0;

    /**
     * @brief Calculates the buffer size for an image.
     * This method calculates the buffer size for an image based on its properties and returns a
     * status code indicating success or failure.
     * @param prop The image properties.
     * @return The status of the buffer size calculation operation.
     */
    virtual QCStatus_e SetImageBuffSizeFromImageProp( ImageProps_t &prop ) = 0;

};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_UTILS_IFS_HPP