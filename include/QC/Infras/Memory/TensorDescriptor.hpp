// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_TENSOR_DESCRIPTOR_HPP
#define QC_TENSOR_DESCRIPTOR_HPP

#include "QC/Infras/Memory/BufferDescriptor.hpp"

#include <string>
#include <vector>

namespace QC
{
namespace Memory
{

/**
 * @brief QCNode Tensor Buffer Properties.
 *
 * Inherited Members from BufferProps:
 * @param size The total required buffer size.
 * @param alignment The alignment requirement of the buffer.
 * @param attr The attributes of the buffer.
 * @param usage The intended usage of the buffer.
 *
 * New Members:
 * @param type The tensor type.
 * @param dims The tensor dimensions.
 * @param numDims The number of dimensions.
 *
 * @note When using this struct to request tensor buffer allocation, the member "size" will not be
 * used. The size is determined by the other new members.
 */
typedef struct TensorProps : public BufferProps
{
public:
    /**
     * @brief Constructor for TensorProps.
     * @return None.
     */
    TensorProps() : BufferProps() {}

    /**
     * @brief Constructor for TensorProps.
     * @param tensorType The tensor type.
     * @param dims The tensor dimensions.
     * @param usage The intended usage of the buffer, default is QC_BUFFER_USAGE_CAMERA.
     * @param attr The attributes of the buffer, default is QC_CACHEABLE.
     * @return None.
     * @note This constructor sets up the properties of the tensor with the given parameters, while
     * most other properties are set to their default suggested values.
     * @note The user must ensure the sizes of the vector dims is less than or equal to
     * QC_NUM_TENSOR_DIMS.
     */
    TensorProps( QCTensorType_e tensorType, std::vector<uint32_t> dims,
                 QCBufferUsage_e usage = QC_BUFFER_USAGE_CAMERA,
                 QCAllocationAttribute_e attr = QC_CACHEABLE )
        : BufferProps( 0, usage, attr ),
          tensorType( tensorType )
    {
        numDims = static_cast<uint32_t>( dims.size() );
        std::copy( dims.begin(), dims.end(), this->dims );
    }
    QCTensorType_e tensorType;
    uint32_t dims[QC_NUM_TENSOR_DIMS];
    uint32_t numDims;
} TensorProps_t;

/**
 * @brief Descriptor for QCNode Shared Tensor Buffer.
 * This structure represents the shared tensor buffer for QCNode. It extends the BufferDescriptor
 * and includes additional members specific to tensor properties.
 *
 * Inherited Members from BufferDescriptor:
 * @param name The name of the buffer.
 * @param pBuf The virtual address of the actual buffer.
 * @param size The actual size of the buffer.
 * @param type The type of the buffer.
 * @param pBufBase The base virtual address of the DMA buffer.
 * @param dmaHandle The DMA handle of the buffer.
 * @param dmaSize The size of the DMA buffer.
 * @param offset The offset of the valid buffer within the shared buffer.
 * @param id The unique ID assigned by the buffer manager.
 * @param pid The process ID that allocated this buffer.
 * @param usage The intended usage of the buffer.
 * @param attr The attributes associated with the buffer.
 *
 * New Members:
 * @param tensorType The tensor type.
 * @param dims The tensor dimensions.
 * @param numDims The number of dimensions.
 */
typedef struct TensorDescriptor : public BufferDescriptor
{
public:
    TensorDescriptor() : BufferDescriptor() {}
    /**
     * @brief Sets up the tensor descriptor from another buffer descriptor object.
     * @param[in] other The buffer descriptor object from which buffer members are copied.
     * @return The updated tensor descriptor object.
     */
    TensorDescriptor &operator=( const BufferDescriptor &other );


    /**
     * @brief Sets up the tensor descriptor using another shared buffer object.
     * @param[in] other The shared buffer object from which buffer members are copied.
     * @return The updated tensor descriptor.
     * @note This is a temporary workaround API to support smoother development during phase 2.
     *       It will be removed once phase 2 is complete.
     */
    TensorDescriptor &operator=( const QCSharedBuffer_t &other );

    QCTensorType_e tensorType;
    uint32_t dims[QC_NUM_TENSOR_DIMS];
    uint32_t numDims;
} TensorDescriptor_t;

}   // namespace Memory
}   // namespace QC

#endif   // QC_TENSOR_DESCRIPTOR_HPP
