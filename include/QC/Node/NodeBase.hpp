// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_NODE_BASE_HPP
#define QC_NODE_BASE_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "QC/Common/DataTree.hpp"
#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/BufferDescriptor.hpp"
#include "QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp"
#include "QC/Infras/Memory/CameraFrameDescriptor.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryDefs.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Node/Ifs/QCFrameDescriptorNodeIfs.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"
#include "QC/Node/Ifs/QCNodeIfs.hpp"
#include "QC/Node/NodeConfigBase.hpp"
#include "QC/Node/NodeFrameDescriptor.hpp"
#include "QC/Node/NodeFrameDescriptorPool.hpp"
#include "QC/component/VideoEncoder.hpp"

namespace QC
{
namespace Node
{

using namespace QC::component;
using namespace QC::Memory;

/**
 * @brief QCNode Shared Buffer Descriptor
 * @param buffer The QC shared buffer.
 */
typedef struct QCSharedBufferDescriptor : public QCBufferDescriptorBase_t
{

public:
    QCSharedBufferDescriptor() = default;
    virtual ~QCSharedBufferDescriptor() = default;

    QCSharedBuffer_t buffer;
} QCSharedBufferDescriptor_t;

/**
 * @brief Descriptor for QCNode Shared Video Buffer.
 *
 * This structure represents the buffer for the QCNode VideoEncoder or VideoDecoder. It extends the
 * QCSharedBufferDescriptor and includes additional members specific to video encoding/decoding.
 *
 * @param[inout] timestampNs The timestamp of the frame data in nanoseconds.
 * @param[inout] appMarkData The mark data for the frame, which will be copied to the corresponding
 * output frame. The API will not modify this data, only copy it.
 * @param[out] frameFlag Indicates whether any error occurred during the encoding/decoding of this
 * frame. This is only used for the output frame.
 * @param[inout] frameType Indicates the type of frame (I, P, B, or IDR). This is used only for
 * H.264 or H.265 frames.
 *
 * @note This is a proposal, and this type needs to be moved to the Node Video related header file.
 *
 * @note For phase 2: Update to inherit QCSharedImageDescriptor.
 */
typedef struct QCSharedVideoFrameDescriptor : public QCSharedBufferDescriptor
{
public:
    QCSharedVideoFrameDescriptor() = default;
    virtual ~QCSharedVideoFrameDescriptor() = default;
    uint64_t timestampNs;
    uint64_t appMarkData;
    uint32_t frameFlag;
    VideoEncoder_FrameType_e frameType;
} QCSharedVideoFrameDescriptor_t;

class NodeBase : public QCNodeIfs
{
public:
    /**
     * @brief NodeBase Constructor.
     * @return None.
     */
    NodeBase() = default;

    /**
     * @brief NodeBase Destructor
     * @return None
     */
    ~NodeBase() = default;

    /**
     * @brief Initializes Node.
     * @param[in] config The Node configuration.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config ) = 0;

    /**
     * @brief Get the Node configuration interface.
     * @return A reference to the Node configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() = 0;

    /**
     * @brief Get the Node monitoring interface.
     * @return A reference to the Node monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() = 0;

    /**
     * @brief Start the Node
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start() = 0;

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc ) = 0;

    /**
     * @brief Stop the Node
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop() = 0;

    /**
     * @brief De-initialize Node.
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize() = 0;

    /**
     * @brief Get the current state of the Node
     * @return The current state of the Node
     */
    virtual QCObjectState_e GetState() = 0;


protected:
    /**
     * @brief Initialize the Node
     * @param[in] nodeId the Node unique ID
     * @param[in] level the logger message level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( QCNodeID_t nodeId, Logger_Level_e level = LOGGER_LEVEL_ERROR );

protected:
    QCNodeID_t m_nodeId;
    Logger m_logger;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_BASE_HPP
