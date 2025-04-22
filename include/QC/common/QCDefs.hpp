// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_DEFS_HPP
#define QC_DEFS_HPP

#include <cstdint>

namespace QC
{

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
    QC_NODE_ID_RESERVED = 0,
    QC_NODE_ID_QNN,
    QC_NODE_ID_QCX,
    QC_NODE_ID_FADAS_REMAP,
    QC_NODE_ID_FADAS_POST_PROC_CENTER_POINT,
    QC_NODE_ID_FADAS_PRE_PROC_VOXELIZATION,
    QC_NODE_ID_CL_2D_FLEX,
    QC_NODE_ID_CL_VOXELIZATION,
    QC_NODE_ID_CL_POST_PROC_CENTER_NET,
    QC_NODE_ID_CL_POST_PROC_BEV_DET,
    QC_NODE_ID_GL_2D_FLEX,
    QC_NODE_ID_EVA_DFS,
    QC_NODE_ID_EVA_OPTICAL_FLOW,
    QC_NODE_ID_EVA_PIRAMID_SCALING,
    QC_NODE_ID_VENC,
    QC_NODE_ID_VDEC,
    QC_NODE_ID_DISP,
    QC_NODE_ID_CUSTOM_0,
    QC_NODE_ID_CUSTOM_1,
    QC_NODE_ID_CUSTOM_2,
    QC_NODE_ID_CUSTOM_3,
    QC_NODE_ID_CUSTOM_4,
    QC_NODE_ID_LAST,
    QC_NODE_ID_MAX = UINT32_MAX
} QCNodeID_e;

typedef enum
{
    QC_BUF_TXT = 0,
    QC_BUF_BINARY,
    QC_BUF_TENSOR,
    QC_BUF_IMAGE,
    QC_BUF_CUSTOM_0,
    QC_BUF_CUSTOM_1,
    QC_BUF_CUSTOM_2,
    QC_BUF_CUSTOM_3,
    QC_BUF_CUSTOM_4,
    QC_BUF_LAST,
    QC_BUF_MAX = UINT32_MAX
} QCBufferDataFormat_e;

}   // namespace QC

#endif   // QC_DEFS_HPP
