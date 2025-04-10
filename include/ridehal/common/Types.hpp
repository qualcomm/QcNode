// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_TYPES_HPP
#define RIDEHAL_TYPES_HPP

#include <cinttypes>
#include <cstddef>

namespace ridehal
{
namespace common
{

/** @brief The maximum number of a image planes. */
#define RIDEHAL_NUM_IMAGE_PLANES ( 4 )

/** @brief The maximum number of a tensor planes. */
#define RIDEHAL_NUM_TENSOR_DIMS ( 8 )

/** @brief The maximum number of inputs of the RideHal component. */
#ifndef RIDEHAL_MAX_INPUTS
#define RIDEHAL_MAX_INPUTS 32
#endif

/** @brief Allocate uncached memory (default). */
#define RIDEHAL_BUFFER_FLAGS_CACHE_NONE (RideHal_BufferFlags_t) 0x00000000U

/** @brief Allocate write-back, write-allocate memory, to be used if IP block is
 * coherent with CPU cache. */
#define RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA (RideHal_BufferFlags_t) 0x0000001U

#define RIDEHAL_BUFFER_FLAGS_CACHE_MASK (RideHal_BufferFlags_t) 0x0000000FU

/** @brief RideHal Errors
 *  Errors for Component API call */
typedef enum
{
    RIDEHAL_ERROR_NONE = 0,        /**< No error. */
    RIDEHAL_ERROR_BAD_ARGUMENTS,   /**< Bad arguments. */
    RIDEHAL_ERROR_BAD_STATE,       /**< Bad state */
    RIDEHAL_ERROR_NOMEM,           /**< No memory. */
    RIDEHAL_ERROR_INVALID_BUF,     /**< Invalid buffer. */
    RIDEHAL_ERROR_UNSUPPORTED,     /**< Unsupported. */
    RIDEHAL_ERROR_ALREADY,         /**< The operation is already done */
    RIDEHAL_ERROR_TIMEOUT,         /**< Timeout */
    RIDEHAL_ERROR_OUT_OF_BOUND,    /**< Out of bound */
    RIDEHAL_ERROR_FAIL,            /**< General failure. */
    RIDEHAL_ERROR_MAX = 0x7FFFFFFF /**< Do not use. */
} RideHalError_e;

/** @brief RideHal Buffer Flags */
typedef uint32_t RideHal_BufferFlags_t;

/** @brief RideHal Buffer Type */
typedef enum
{
    RIDEHAL_BUFFER_TYPE_RAW = 0, /**< The buffer for raw data */
    RIDEHAL_BUFFER_TYPE_IMAGE,   /**< The buffer for image */
    RIDEHAL_BUFFER_TYPE_TENSOR   /**< The buffer for tensor */
} RideHal_BufferType_e;

/** @brief RideHal Buffer Usage */
typedef enum
{
    RIDEHAL_BUFFER_USAGE_DEFAULT = 0, /**< Default */
    RIDEHAL_BUFFER_USAGE_CAMERA,      /**< Buffer used by camera */
    RIDEHAL_BUFFER_USAGE_GPU,         /**< Buffer used by GPU */
    RIDEHAL_BUFFER_USAGE_VPU,         /**< Buffer used by VPU */
    RIDEHAL_BUFFER_USAGE_EVA,         /**< Buffer used by EVA */
    RIDEHAL_BUFFER_USAGE_HTP,         /**< Buffer used by HTP */
    RIDEHAL_BUFFER_USAGE_MAX
} RideHal_BufferUsage_e;

/** @brief RideHal Computing Processor Type */
typedef enum
{
    RIDEHAL_PROCESSOR_HTP0, /**< do computing on the processor HTP0 */
    RIDEHAL_PROCESSOR_HTP1, /**< do computing on the processor HTP1 */
    RIDEHAL_PROCESSOR_CPU,  /**< do computing on the processor CPU */
    RIDEHAL_PROCESSOR_GPU,  /**< do computing on the processor GPU */
    RIDEHAL_PROCESSOR_MAX
} RideHal_ProcessorType_e;

/** @brief The attributes of an allocated DMA memory. */
typedef struct
{
    void *pData;                 /**< The buffer virtual address */
    uint64_t dmaHandle;          /**< The buffer DMA handle */
    size_t size;                 /**< The buffer size */
    uint64_t id;                 /**< The unique ID assigned by the buffer manager */
    uint64_t pid;                /**< The process id that allocated this buffer */
    RideHal_BufferUsage_e usage; /**< The buffer usage */
    RideHal_BufferFlags_t flags; /**< The buffer flags */
} RideHal_Buffer_t;

/** @brief The image format. */
typedef enum
{
    /**< Below formats for an image without compression */
    RIDEHAL_IMAGE_FORMAT_RGB888 = 0,
    RIDEHAL_IMAGE_FORMAT_BGR888,
    RIDEHAL_IMAGE_FORMAT_UYVY,
    RIDEHAL_IMAGE_FORMAT_NV12,
    RIDEHAL_IMAGE_FORMAT_P010,
    RIDEHAL_IMAGE_FORMAT_NV12_UBWC,
    RIDEHAL_IMAGE_FORMAT_TP10_UBWC,
    RIDEHAL_IMAGE_FORMAT_MAX,
    /**< Below formats for an image with compression, such as by the Video Encoder */
    RIDEHAL_IMAGE_FORMAT_COMPRESSED_MIN = 100,
    RIDEHAL_IMAGE_FORMAT_COMPRESSED_H264 = 100,
    RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265,
    RIDEHAL_IMAGE_FORMAT_COMPRESSED_MAX,
} RideHal_ImageFormat_e;

/** @brief The image properties. */
typedef struct
{
    RideHal_ImageFormat_e format; /**< The image format */
    uint32_t batchSize;           /**< The image batch size */
    uint32_t width;               /**< The image width in pixels */
    uint32_t height;              /**< The image height in pixels */
    uint32_t stride[RIDEHAL_NUM_IMAGE_PLANES];
    /**< The image stride along width in bytes for each plane */

    uint32_t actualHeight[RIDEHAL_NUM_IMAGE_PLANES];
    /**< The image actual height in scanlines for each plane */

    uint32_t planeBufSize[RIDEHAL_NUM_IMAGE_PLANES];
    /**< The image actual buffer size for each plane.
     * This equals to (stride * actualHeight + padding size).
     */

    uint32_t numPlanes; /**< The number of the image planes */
} RideHal_ImageProps_t;

/** @brief The RideHal tensor data type. */
typedef enum
{

    RIDEHAL_TENSOR_TYPE_INT_8,  /**< 8-bit integer type */
    RIDEHAL_TENSOR_TYPE_INT_16, /**< 16-bit integer type */
    RIDEHAL_TENSOR_TYPE_INT_32, /**< 32-bit integer type */
    RIDEHAL_TENSOR_TYPE_INT_64, /** 64-bit integer type */

    RIDEHAL_TENSOR_TYPE_UINT_8,  /**< 8-bit unsigned integer type */
    RIDEHAL_TENSOR_TYPE_UINT_16, /**< 16-bit unsigned integer type */
    RIDEHAL_TENSOR_TYPE_UINT_32, /**< 32-bit unsigned integer type */
    RIDEHAL_TENSOR_TYPE_UINT_64, /**< 64-bit unsigned integer type */

    RIDEHAL_TENSOR_TYPE_FLOAT_16, /**< 16-bit float point type */
    RIDEHAL_TENSOR_TYPE_FLOAT_32, /**< 32-bit float point type */
    RIDEHAL_TENSOR_TYPE_FLOAT_64, /**< 64-bit float point type */

    RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8,  /**< 8-bit singed fixed point type */
    RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16, /**< 16-bit singed fixed point type */
    RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32, /**< 32-bit singed fixed point type */

    RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8,  /**< 8-bit unsinged fixed point type */
    RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16, /**< 16-bit unsinged fixed point type */
    RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32, /**< 32-bit unsinged fixed point type */

    RIDEHAL_TENSOR_TYPE_MAX,
} RideHal_TensorType_e;

/** @brief The tensor properties. */
typedef struct
{
    RideHal_TensorType_e type;              /**< The tensor type */
    uint32_t dims[RIDEHAL_NUM_TENSOR_DIMS]; /**< The tensor dimensions */
    uint32_t numDims;                       /**< The number of dimensions */
} RideHal_TensorProps_t;

/** @brief RideHal Component state */
typedef enum
{
    RIDEHAL_COMPONENT_STATE_INITIAL = 0,    /**< the initial state */
    RIDEHAL_COMPONENT_STATE_INITIALIZING,   /**< the state during initializing */
    RIDEHAL_COMPONENT_STATE_READY,          /**< the ready state */
    RIDEHAL_COMPONENT_STATE_STARTING,       /**< the state during starting */
    RIDEHAL_COMPONENT_STATE_RUNNING,        /**< the running state */
    RIDEHAL_COMPONENT_STATE_STOPING,        /**< the state during stopping */
    RIDEHAL_COMPONENT_STATE_ERROR,          /**< the error state */
    RIDEHAL_COMPONENT_STATE_PAUSING,        /**< the state during pausing */
    RIDEHAL_COMPONENT_STATE_PAUSE,          /**< the paused state */
    RIDEHAL_COMPONENT_STATE_RESUMING,       /**< the state during resuming */
    RIDEHAL_COMPONENT_STATE_DEINITIALIZING, /**< the state during deinitializing */
    RIDEHAL_COMPONENT_STATE_MAX
} RideHal_ComponentState_t;

}   // namespace common
}   // namespace ridehal

#endif   // RIDEHAL_TYPES_HPP
