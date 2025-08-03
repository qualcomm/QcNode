// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_UTILS_BASE_HPP
#define QC_MEMORY_UTILS_BASE_HPP

#include "QC/Infras/Memory/Ifs/QCMemoryUtilsIfs.hpp"

namespace QC
{
namespace Memory
{

/**
 * @class UtilsBase
 * @brief A concrete implementation of the QCMemoryUtilsIfs interface for PMEM allocation.
 *
 * This class provides methods for allocating and freeing memory from the PMEM software package.
 */
class UtilsBase : public QCMemoryUtilsIfs
{
public:
    /**
     * @brief Default constructor.
     * This constructor Intilizes members which do not require external parameters
     */
    UtilsBase();
    
    /**
     * @brief Destructor for the UtilsBase class.
     * This destructor releases any resources allocated by the ManagerLocal object.
     */
    virtual ~UtilsBase();

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

    /**
     * @brief Calculates the buffer size for a tensor.
     * This method calculates the buffer size for a tensor based on its properties and returns a
     * status code indicating success or failure.
     * @param prop The tensor properties.
     * @return The status of the buffer size calculation operation.
     */
    virtual QCStatus_e SetTensorBuffSizeFromTensorProp( TensorProps_t &prop );

    /**
     * @brief Calculates the buffer size for an image.
     * This method calculates the buffer size for an image based on its properties and returns a
     * status code indicating success or failure.
     * @param prop The image properties.
     * @return The status of the buffer size calculation operation.
     */
    virtual QCStatus_e SetImageBuffSizeFromImageProp( ImageProps_t &prop );

private:
    /**
     * @var s_qcTensorTypeToDataSize
     * @brief Array mapping tensor types to data sizes.
     * This array maps each tensor type to its corresponding data size in bytes.
     */
    static const uint32_t s_qcTensorTypeToDataSize[QC_TENSOR_TYPE_MAX];

    /**
     * @var s_qcFormatToBytesPerPixel
     * @brief Array mapping image formats to bytes per pixel.
     * This array maps each image format to its corresponding bytes per pixel.
     */
    static const uint32_t s_qcFormatToBytesPerPixel[QC_IMAGE_FORMAT_MAX];

    /**
     * @var s_qcFormatToNumPlanes
     * @brief Array mapping image formats to number of planes.
     * This array maps each image format to its corresponding number of planes.
     */
    static const uint32_t s_qcFormatToNumPlanes[QC_IMAGE_FORMAT_MAX];

    /**
     * @var s_qcFormatToHeightDividerPerPlanes
     * @brief Array mapping image formats to height dividers per plane.
     * This array maps each image format to its corresponding height dividers per plane.
     */
    static const uint32_t s_qcFormatToHeightDividerPerPlanes[QC_IMAGE_FORMAT_MAX][QC_NUM_IMAGE_PLANES];

    /**
     * @brief Declare the logger for this class.
     */
    QC_DECLARE_LOGGER();

};

}   // namespace Memory
}   // namespace QC

#endif   // BASE