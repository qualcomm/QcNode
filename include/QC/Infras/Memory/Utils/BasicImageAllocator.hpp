// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_BASIC_IMAGE_ALLOCATOR_HPP
#define QC_BASIC_IMAGE_ALLOCATOR_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/Utils/BinaryAllocator.hpp"
#include "QC/Infras/Memory/Utils/TensorAllocator.hpp"

namespace QC
{
namespace Memory
{

class BasicImageAllocator : public BinaryAllocator
{
public:
    /**
     * @brief Constructor for BasicImageAllocator.
     * @param[in] name The name of the image basic allocator.
     * @return None.
     */
    BasicImageAllocator( const std::string &name );

    virtual ~BasicImageAllocator();

    /**
     * @brief Allocate an image memory buffer with the best strides/paddings
     * that can be shared among CPU/GPU/VPU/HTP, etc.
     * @param[in] request The QCNode image basic properties, a reference to ImageBasicProps_t.
     * @param[out] response The QCNode shared image buffer descriptor, a reference to
     * ImageDescriptor_t.
     * @return QC_STATUS_OK on success, other status codes on failure.
     * @note use the Free API inherited from BinaryAllocator to free the image buffer.
     * @example
     * @code
     * ImageBasicProps_t request( 3840, 2160, QC_IMAGE_FORMAT_NV12 );
     * ImageDescriptor_t response;
     * QCStatus_e status = basicImageAllocator.Allocate( request, response );
     * @endcode
     */
    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response );

private:
    QC_DECLARE_LOGGER();
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_BASIC_IMAGE_ALLOCATOR_HPP
