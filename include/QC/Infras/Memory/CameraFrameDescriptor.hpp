// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_CAMERA_FRAME_DESCRIPTOR_HPP
#define QC_CAMERA_FRAME_DESCRIPTOR_HPP

#include "QC/Infras/Memory/ImageDescriptor.hpp"

namespace QC
{
namespace Memory
{

/**
 * @brief Descriptor for QCNode Shared Camera Frame Descriptor.
 * This structure represents the camera frame descriptor for QCNode. It
 * extends the ImageDescriptor and includes additional members specific
 * to camera frames.
 *
 * Inherited Members from ImageDescriptor:
 * @param name The name of the buffer.
 * @param pBuf The virtual address of the dma buffer.
 * @param size The dma size of the buffer.
 * @param type The type of the buffer.
 * @param alignment The alignment of the buffer.
 * @param cache The cache type of the buffer.
 * @param allocatorType The allocaor type used for allocation the buffer.
 * @param dmaHandle The dmaHandle of the buffer.
 * @param pid The process ID of the buffer.
 * @param validSize The size of valid data currently stored in the buffer.
 * @param offset The offset of the valid buffer within the shared buffer.
 * @param id The unique ID assigned by the buffer manager.
 * @param format The image format.
 * @param batchSize The image batch size.
 * @param width The image width in pixels.
 * @param height The image height in pixels.
 * @param stride The image stride along the width in bytes for each plane.
 * @param actualHeight The actual height of the image in scanlines for each plane.
 * @param planeBufSize The actual buffer size of the image for each plane, calculated as (stride *
 * actualHeight + padding size).
 * @param numPlanes The number of image planes.
 *
 * New Members:
 * @param timestamp The hardware timestamp of the frame in nanoseconds.
 * @param timestampQGPTP The Generic Precision Time Protocol (GPTP) timestamp in nanoseconds.
 * @param frameIdx The index of the camera frame.
 * @param flags Indicating the error state of the buffer.
 * @param streamId The identifier for the Qcarcam buffer list.
 */
typedef struct CameraFrameDescriptor : public ImageDescriptor
{
public:
    CameraFrameDescriptor() : ImageDescriptor() {}

    /**
     * @brief Sets up the camera frame descriptor from another buffer descriptor base object.
     * @param[in] other The camera frame descriptor object from which buffer members are copied.
     * @return The updated camera frame descriptor object.
     */
    CameraFrameDescriptor &operator=( const QCBufferDescriptorBase &other );

    uint64_t timestamp;
    uint64_t timestampQGPTP;
    uint32_t frameIdx;
    uint32_t flags;
    uint32_t streamId;
} CameraFrameDescriptor_t;
}   // namespace Memory
}   // namespace QC

#endif   // QC_CAMERA_FRAME_DESCRIPTOR_HPP

