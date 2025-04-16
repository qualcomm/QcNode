// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_TYPES_HPP
#define QC_TYPES_HPP

#include "QC/common/QCDefs.hpp"
#include <cinttypes>
#include <cstddef>

namespace QC
{

/** @brief The maximum number of a image planes. */
#define QC_NUM_IMAGE_PLANES ( 4 )

/** @brief The maximum number of a tensor planes. */
#define QC_NUM_TENSOR_DIMS ( 8 )

/** @brief The maximum number of inputs of the QC component. */
#ifndef QC_MAX_INPUTS
#define QC_MAX_INPUTS 32
#endif

/** @brief Allocate uncached memory (default). */
#define QC_BUFFER_FLAGS_CACHE_NONE (QCBufferFlags_t) 0x00000000U

/** @brief Allocate write-back, write-allocate memory, to be used if IP block is
 * coherent with CPU cache. */
#define QC_BUFFER_FLAGS_CACHE_WB_WA (QCBufferFlags_t) 0x0000001U

#define QC_BUFFER_FLAGS_CACHE_MASK (QCBufferFlags_t) 0x0000000FU


/** @brief QC Buffer Flags */
typedef uint32_t QCBufferFlags_t;

/** @brief QC Buffer Type */
typedef enum
{
    QC_BUFFER_TYPE_RAW = 0, /**< The buffer for raw data */
    QC_BUFFER_TYPE_IMAGE,   /**< The buffer for image */
    QC_BUFFER_TYPE_TENSOR   /**< The buffer for tensor */
} QCBufferType_e;

/** @brief QC Buffer Usage */
typedef enum
{
    QC_BUFFER_USAGE_DEFAULT = 0, /**< Default */
    QC_BUFFER_USAGE_CAMERA,      /**< Buffer used by camera */
    QC_BUFFER_USAGE_GPU,         /**< Buffer used by GPU */
    QC_BUFFER_USAGE_VPU,         /**< Buffer used by VPU */
    QC_BUFFER_USAGE_EVA,         /**< Buffer used by EVA */
    QC_BUFFER_USAGE_HTP,         /**< Buffer used by HTP */
    QC_BUFFER_USAGE_MAX
} QCBufferUsage_e;

/** @brief QC Computing Processor Type */
typedef enum
{
    QC_PROCESSOR_HTP0, /**< do computing on the processor HTP0 */
    QC_PROCESSOR_HTP1, /**< do computing on the processor HTP1 */
    QC_PROCESSOR_CPU,  /**< do computing on the processor CPU */
    QC_PROCESSOR_GPU,  /**< do computing on the processor GPU */
    QC_PROCESSOR_MAX
} QCProcessorType_e;

/** @brief The attributes of an allocated DMA memory. */
typedef struct
{
    void *pData;           /**< The buffer virtual address */
    uint64_t dmaHandle;    /**< The buffer DMA handle */
    size_t size;           /**< The buffer size */
    uint64_t id;           /**< The unique ID assigned by the buffer manager */
    uint64_t pid;          /**< The process id that allocated this buffer */
    QCBufferUsage_e usage; /**< The buffer usage */
    QCBufferFlags_t flags; /**< The buffer flags */
} QCBuffer_t;

/** @brief The image format. */
typedef enum
{
    /**< Below formats for an image without compression */
    QC_IMAGE_FORMAT_RGB888 = 0,
    QC_IMAGE_FORMAT_BGR888,
    QC_IMAGE_FORMAT_UYVY,
    QC_IMAGE_FORMAT_NV12,
    QC_IMAGE_FORMAT_P010,
    QC_IMAGE_FORMAT_NV12_UBWC,
    QC_IMAGE_FORMAT_TP10_UBWC,
    QC_IMAGE_FORMAT_MAX,
    /**< Below formats for an image with compression, such as by the Video Encoder */
    QC_IMAGE_FORMAT_COMPRESSED_MIN = 100,
    QC_IMAGE_FORMAT_COMPRESSED_H264 = 100,
    QC_IMAGE_FORMAT_COMPRESSED_H265,
    QC_IMAGE_FORMAT_COMPRESSED_MAX,
} QCImageFormat_e;

/** @brief The image properties. */
typedef struct
{
    QCImageFormat_e format; /**< The image format */
    uint32_t batchSize;     /**< The image batch size */
    uint32_t width;         /**< The image width in pixels */
    uint32_t height;        /**< The image height in pixels */
    uint32_t stride[QC_NUM_IMAGE_PLANES];
    /**< The image stride along width in bytes for each plane */

    uint32_t actualHeight[QC_NUM_IMAGE_PLANES];
    /**< The image actual height in scanlines for each plane */

    uint32_t planeBufSize[QC_NUM_IMAGE_PLANES];
    /**< The image actual buffer size for each plane.
     * This equals to (stride * actualHeight + padding size).
     */

    uint32_t numPlanes; /**< The number of the image planes */
} QCImageProps_t;

/** @brief The QC tensor data type. */
typedef enum
{

    QC_TENSOR_TYPE_INT_8,  /**< 8-bit integer type */
    QC_TENSOR_TYPE_INT_16, /**< 16-bit integer type */
    QC_TENSOR_TYPE_INT_32, /**< 32-bit integer type */
    QC_TENSOR_TYPE_INT_64, /** 64-bit integer type */

    QC_TENSOR_TYPE_UINT_8,  /**< 8-bit unsigned integer type */
    QC_TENSOR_TYPE_UINT_16, /**< 16-bit unsigned integer type */
    QC_TENSOR_TYPE_UINT_32, /**< 32-bit unsigned integer type */
    QC_TENSOR_TYPE_UINT_64, /**< 64-bit unsigned integer type */

    QC_TENSOR_TYPE_FLOAT_16, /**< 16-bit float point type */
    QC_TENSOR_TYPE_FLOAT_32, /**< 32-bit float point type */
    QC_TENSOR_TYPE_FLOAT_64, /**< 64-bit float point type */

    QC_TENSOR_TYPE_SFIXED_POINT_8,  /**< 8-bit singed fixed point type */
    QC_TENSOR_TYPE_SFIXED_POINT_16, /**< 16-bit singed fixed point type */
    QC_TENSOR_TYPE_SFIXED_POINT_32, /**< 32-bit singed fixed point type */

    QC_TENSOR_TYPE_UFIXED_POINT_8,  /**< 8-bit unsinged fixed point type */
    QC_TENSOR_TYPE_UFIXED_POINT_16, /**< 16-bit unsinged fixed point type */
    QC_TENSOR_TYPE_UFIXED_POINT_32, /**< 32-bit unsinged fixed point type */

    QC_TENSOR_TYPE_MAX,
} QCTensorType_e;

/** @brief The tensor properties. */
typedef struct
{
    QCTensorType_e type;               /**< The tensor type */
    uint32_t dims[QC_NUM_TENSOR_DIMS]; /**< The tensor dimensions */
    uint32_t numDims;                  /**< The number of dimensions */
} QCTensorProps_t;


}   // namespace QC

#endif   // QC_TYPES_HPP
