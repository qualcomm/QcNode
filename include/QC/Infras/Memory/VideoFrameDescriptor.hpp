// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_VIDEO_FRAME_DESCRIPTOR_HPP
#define QC_VIDEO_FRAME_DESCRIPTOR_HPP

#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "vidc_types.h"
#include <string>
#include <vector>
#include <iomanip>

namespace QC
{
namespace Memory
{
/**
 * @brief Descriptor for QCNode Shared Video Buffer.
 *
 * This structure represents the shared video buffer descriptor for QCNode. It extends the
 * BufferDescriptor and includes additional members specific to video properties.
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
 * Inherited Members from ImageDescriptor:
 * @param format The video format.
 * @param batchSize The video batch size.
 * @param width The video width in pixels.
 * @param height The video height in pixels.
 * @param stride The video stride along the width in bytes for each plane.
 * @param actualHeight The actual height of the video in scanlines for each plane.
 * @param planeBufSize The actual buffer size of the video for each plane, calculated as (stride *
 * actualHeight + padding size).
 * @param numPlanes The number of video planes.
 */
typedef struct VideoFrameDescriptor : public ImageDescriptor
{
    uint64_t timestampNs = 0; /**< frame data's timestamp. */
    uint64_t appMarkData = 0; /**< frame data's mark data, this data copied from corresponding
                               input buffer. API won't touch this data, only copy it. */
    vidc_frame_type frameType = VIDC_FRAME_NOTCODED; /**< indicate I/P/B/IDR frame, used by encoder. */
    uint32_t frameFlag = 0; /**< indicate whether some error occurred during decoding this frame. */

public:
    VideoFrameDescriptor() : ImageDescriptor(),
        timestampNs(0), appMarkData(0), frameType(VIDC_FRAME_NOTCODED), frameFlag(0) {}

    /**
     * @brief Sets up the video frame descriptor from another buffer descriptor base object.
     * @param[in] other The video frame descriptor object from which buffer members are copied.
     * @return The updated video frame descriptor object.
     */
    VideoFrameDescriptor &operator=( const QCBufferDescriptorBase &other );

    /**
     * @brief Initializes the image descriptor using another shared buffer object.
     * @param[in] other The shared buffer object from which buffer data is copied.
     * @return The updated image descriptor.
     * @note This is a temporary workaround API introduced to facilitate development during phase 2.
     *       It will be removed once phase 2 development is complete.
     */
    VideoFrameDescriptor &operator=( const QCSharedBuffer_t &other );

} VideoFrameDescriptor_t;

}   // namespace Memory
}   // namespace QC

#endif   // QC_VIDEO_FRAME_DESCRIPTOR_HPP
