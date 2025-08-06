// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_IMAGE_ALLOCATOR_HPP
#define QC_IMAGE_ALLOCATOR_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/Utils/BasicImageAllocator.hpp"

namespace QC
{
namespace Memory
{

class ImageAllocator : public BinaryAllocator
{
public:
    /**
     * @brief Constructor for ImageAllocator.
     * @param[in] name The name of the image allocator.
     * @return None.
     * @deprecated Will be removed
     */
    ImageAllocator( const std::string &name );

    /**
     * @brief Constructor for BasicImageAllocator.
     * @param[in] nodeId The nodeId of the binary allocator.
     * @param[in] logLevel The log level of the binary allocator.
     * @return None.
     */
    ImageAllocator( const QCNodeID_t &nodeId, Logger_Level_e logLevel = LOGGER_LEVEL_ERROR );

    virtual ~ImageAllocator();

    /**
     * @brief Allocate an image memory buffer with the specified image properties.
     * @param[in] request The QCNode detailed image properties, a reference to ImageProps_t.
     * @param[out] response The QCNode shared image buffer descriptor, a reference to
     * ImageDescriptor_t.
     * @return QC_STATUS_OK on success, other status codes on failure.
     * @note use the Free API inherited from BinaryAllocator to free the image buffer.
     * @example
     * @code
     * ImageProps_t request;
     * ImageDescriptor_t response;
     * request.width = 640;
     * request.height = 480;
     * request.format = QC_IMAGE_FORMAT_NV12;
     * request.usage = QC_BUFFER_USAGE_CAMERA;
     * request.attr = QC_CACHEABLE;
     * request.batchSize = 1;
     * request.stride[0] = 640;
     * request.stride[1] = 640;
     * request.actualHeight[0] = 480;
     * request.actualHeight[1] = 480;
     * request.planeBufSize[0] = 640 * 480;
     * request.planeBufSize[1] = 640 * 480 / 2;
     * request.numPlanes = 2;
     * QCStatus_e status = imageAllocator.Allocate( request, response );
     * @endcode
     */
    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response );
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_HPP
