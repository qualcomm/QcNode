// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_BUFFER_MANAGER_HPP
#define QC_SAMPLE_BUFFER_MANAGER_HPP

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/BufferDescriptor.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryManagerIfs.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Infras/Memory/UtilsBase.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"

namespace QC
{
namespace sample
{
using namespace QC::Memory;

class BufferManager
{
public:
    /**
     * @brief Constructor for BufferManager.
     * @param[in] nodeId The nodeId of the binary allocator.
     * @param[in] logLevel The log level of the binary allocator.
     * @return None.
     */
    BufferManager( const QCNodeID_t &nodeId, Logger_Level_e logLevel = LOGGER_LEVEL_ERROR );

    ~BufferManager();

    /**
     * @brief Allocates a memory buffer.
     *
     * Allocates a QCNode DMA buffer based on the specified properties and populates the response
     * with a descriptor representing the allocated shared DMA buffer.
     *
     * @param[in] request The QCNode DMA buffer properties, provided as a reference to
     * QCBufferPropBase_t.
     * @param[out] response The QCNode shared DMA buffer descriptor, provided as a reference to
     * QCBufferDescriptorBase_t.
     *
     * @return QC_STATUS_OK on success; otherwise, returns an appropriate error code indicating the
     * failure reason.
     */
    QCStatus_e Allocate( const QCBufferPropBase_t &request, QCBufferDescriptorBase_t &response );

    /**
     * @brief Frees the memory buffer.
     * @param[in] buffer The buffer descriptor.
     * @return QC_STATUS_OK on success, other status codes on failure.
     */
    QCStatus_e Free( const QCBufferDescriptorBase_t &buffer );

private:
    /**
     * @brief Allocates a binary raw memory buffer.
     * @param[in] request The QCNode DMA buffer properties, a referece to BufferProps_t.
     * @param[out] response The QCNode shared DMA buffer descriptor, a reference to
     * BufferDescriptor_t.
     * @return QC_STATUS_OK on success, other status codes on failure.
     * @example
     * @code
     * BufferProps_t request( 1024, QC_BUFFER_USAGE_DEFAULT, QC_CACHEABLE );
     * BufferDescriptor_t response;
     * QCStatus_e status = allocator.Allocate( request, response );
     * @endcode
     */
    QCStatus_e AllocateBinary( const BufferProps_t &request, BufferDescriptor_t &response );

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
    QCStatus_e AllocateBasicImage( const ImageBasicProps_t &request, ImageDescriptor_t &response );

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
    QCStatus_e AllocateImage( const ImageProps_t &request, ImageDescriptor_t &response );

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
    QCStatus_e AllocateTensor( const TensorProps_t &request, TensorDescriptor_t &response );

private:
    QC_DECLARE_LOGGER();

    QCNodeID_t m_nodeId;
    QCMemoryHandle_t m_memoryHandle;
    UtilsBase m_util;

    struct BufferManagerHolder
    {
        BufferManager *pBufMgr;
        uint32_t ref;
    };

    static std::mutex s_instanceMapMutex;
    static std::map<uint32_t, BufferManagerHolder> s_instanceMap;

public:
    static BufferManager *Get( const QCNodeID_t &nodeId,
                               Logger_Level_e logLevel = LOGGER_LEVEL_ERROR );
    static void Put( BufferManager *pBufMgr );
};

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_BUFFER_MANAGER_HPP
