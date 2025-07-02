// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_TENSOR_ALLOCATOR_HPP
#define QC_TENSOR_ALLOCATOR_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Infras/Memory/Utils/BinaryAllocator.hpp"

namespace QC
{
namespace Memory
{

class TensorAllocator : public BinaryAllocator
{
public:
    /**
     * @brief Constructor for TensorAllocator.
     * @param[in] name The name of the tensor allocator.
     * @return None.
     */
    TensorAllocator( const std::string &name );

    virtual ~TensorAllocator();

    /**
     * @brief Allocates a tensor memory buffer with the specified tensor properties.
     * @param[in] request The QCNode tensor properties, a reference to TensorProps_t.
     * @param[out] response The QCNode shared tensor buffer descriptor, a reference to
     * TensorDescriptor_t.
     * @return QC_STATUS_OK on success, other status codes on failure.
     * @note use the Free API inherited from BinaryAllocator to free the tensor buffer.
     * @example
     * @code
     * TensorProps_t request;
     * request.tensorType = QC_TENSOR_TYPE_UFIXED_POINT_8;
     * request.dims[0] = 1;
     * request.dims[1] = 224;
     * request.dims[2] = 224;
     * request.dims[3] = 3;
     * request.numDims = 4;
     * TensorDescriptor_t response;
     * QCStatus_e status = tensorAllocator.Allocate( request, response );
     * @endcode
     */
    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response );

private:
    QC_DECLARE_LOGGER();
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_TENSOR_ALLOCATOR_HPP
