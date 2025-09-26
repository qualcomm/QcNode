
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_MEMORY_UTILS_IFS_HPP
#define QC_MEMORY_UTILS_IFS_HPP

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
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
     * @brief Sets tensor descriptorvalues from tensor properties.
     * @param prop The tensor properties.
     * @param desc The tensor descriptor.
     * @return The status of the buffer size calculation operation.
     */
    virtual QCStatus_e SetTensorDescFromTensorProp( TensorProps_t &prop,
                                                    TensorDescriptor_t &desc ) = 0;

    /**
     * @brief Sets the buffer descriptor values for an image.
     * @param prop The image basic properties.
     * @param desc The image descriptor.
     * @return The status of the buffer descriptor setting operation.
     */
    virtual QCStatus_e SetImageDescFromImageBasicProp( ImageBasicProps_t &prop,
                                                       ImageDescriptor_t &desc ) = 0;

    /**
     * @brief Sets the buffer descriptor values for an image.
     * @param prop The image properties.
     * @param desc The image descriptor.
     * @return The status of the buffer descriptor setting operation.
     */
    virtual QCStatus_e SetImageDescFromImageProp( ImageProps_t &prop, ImageDescriptor_t &desc ) = 0;
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_UTILS_IFS_HPP