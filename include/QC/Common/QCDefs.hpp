// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_DEFS_HPP
#define QC_DEFS_HPP

#include <cstdint>

namespace QC
{

#ifndef QC_TARGET_SOC
#define QC_TARGET_SOC 8797
#warning "QC_TARGET_SOC is not defined. Default to 8797"
#endif

/** @brief QC Status */
typedef enum
{
    QC_STATUS_OK = 0,        /**< No error. */
    QC_STATUS_BAD_ARGUMENTS, /**< Bad arguments. */
    QC_STATUS_BAD_STATE,     /**< Bad state */
    QC_STATUS_NOMEM,         /**< No memory. */
    QC_STATUS_INVALID_BUF,   /**< Invalid buffer. */
    QC_STATUS_UNSUPPORTED,   /**< Unsupported. */
    QC_STATUS_ALREADY,       /**< The operation is already done */
    QC_STATUS_TIMEOUT,       /**< Timeout */
    QC_STATUS_OUT_OF_BOUND,  /**< Out of bound */
    QC_STATUS_FAIL,          /**< General failure. */
    QC_STATUS_NULL_PTR,      /**< nullptr */
    QC_STATUS_NO_RESOURCE,
    QC_STATUS_LAST = -1
} QCStatus_e;

/** @brief QC Object state */
typedef enum
{
    QC_OBJECT_STATE_INITIAL = 0,    /**< the initial state */
    QC_OBJECT_STATE_INITIALIZING,   /**< the state during initializing */
    QC_OBJECT_STATE_READY,          /**< the ready state */
    QC_OBJECT_STATE_STARTING,       /**< the state during starting */
    QC_OBJECT_STATE_RUNNING,        /**< the running state */
    QC_OBJECT_STATE_STOPING,        /**< the state during stopping */
    QC_OBJECT_STATE_ERROR,          /**< the error state */
    QC_OBJECT_STATE_PAUSING,        /**< the state during pausing */
    QC_OBJECT_STATE_PAUSE,          /**< the paused state */
    QC_OBJECT_STATE_RESUMING,       /**< the state during resuming */
    QC_OBJECT_STATE_DEINITIALIZING, /**< the state during deinitializing */
    QC_OBJECT_STATE_MAX = UINT32_MAX
} QCObjectState_e;

typedef enum
{
    QC_NODE_TYPE_RESERVED = 0,
    QC_NODE_TYPE_QNN,
    QC_NODE_TYPE_QCX,
    QC_NODE_TYPE_FADAS_REMAP,
    QC_NODE_TYPE_CL_2D_FLEX,
    QC_NODE_TYPE_GL_2D_FLEX,
    QC_NODE_TYPE_EVA_DFS,
    QC_NODE_TYPE_EVA_OPTICAL_FLOW,
    QC_NODE_TYPE_VENC,
    QC_NODE_TYPE_VDEC,
    QC_NODE_TYPE_RADAR,
    QC_NODE_TYPE_VOXEL,
    QC_NODE_TYPE_CUSTOM_0,
    QC_NODE_TYPE_CUSTOM_1,
    QC_NODE_TYPE_CUSTOM_2,
    QC_NODE_TYPE_CUSTOM_3,
    QC_NODE_TYPE_CUSTOM_4,
    QC_NODE_TYPE_LAST,
    QC_NODE_TYPE_MAX = UINT32_MAX
} QCNodeType_e;

/** @brief QC Buffer Type */
typedef enum
{
    QC_BUFFER_TYPE_TXT = 0,
    QC_BUFFER_TYPE_RAW,    /**< The buffer for raw data */
    QC_BUFFER_TYPE_IMAGE,  /**< The buffer for image */
    QC_BUFFER_TYPE_TENSOR, /**< The buffer for tensor */
    QC_BUFFER_TYPE_CUSTOM_0,
    QC_BUFFER_TYPE_CUSTOM_1,
    QC_BUFFER_TYPE_CUSTOM_2,
    QC_BUFFER_TYPE_CUSTOM_3,
    QC_BUFFER_TYPE_CUSTOM_4,
    QC_BUFFER_TYPE_LAST,
    QC_BUFFER_TYPE_MAX = UINT32_MAX
} QCBufferType_e;

}   // namespace QC

#endif   // QC_DEFS_HPP
