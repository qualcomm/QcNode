// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


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
#include "QC/component/VideoEncoder.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/BufferDescriptor.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryDefs.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/QCBufferDescriptorBase.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Node/Ifs/QCFrameDescriptorNodeIfs.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"
#include "QC/Node/Ifs/QCNodeIfs.hpp"

namespace QC
{
namespace Node
{

using namespace QC::component;
using namespace QC::Memory;
/**
 * @brief QCNode Dummy Buffer Descriptor
 * This is used by QCSharedFrameDescriptorNode to indicate that the buffer identified by
 * globalBufferId is dummy and not provided by the user application or QCNode.
 */
typedef struct QCDummyBufferDescriptor : public QCBufferDescriptorBase
{
public:
    QCDummyBufferDescriptor()
    {
        name = "Dummy";
        pBuf = nullptr;
        size = 0;
        type = QC_BUFFER_TYPE_MAX;
    }
    virtual ~QCDummyBufferDescriptor() = default;
} QCDummyBufferDescriptor_t;

/**
 * @brief QCNode Shared Buffer Descriptor
 * @param buffer The QC shared buffer.
 */
typedef struct QCSharedBufferDescriptor : public QCBufferDescriptorBase
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

/**
 * @brief QCNode Shared Frame Descriptor for Node
 * This will be used by QCNode API ProcessFrameDescriptor and QCNodeInit::callback to pass the
 * buffer information.
 */
class QCSharedFrameDescriptorNode : public QCFrameDescriptorNodeIfs
{
public:
    QCSharedFrameDescriptorNode() = delete;
    QCSharedFrameDescriptorNode( uint32_t numOfBuffers ) : m_buffers( numOfBuffers, Dummy() ) {}
    ~QCSharedFrameDescriptorNode() {}

    /**
     * @brief Copies buffer descriptors from another QCFrameDescriptorNodeIfs object.
     * @param[in] other The QCFrameDescriptorNodeIfs object from which buffer descriptors are
     * copied.
     * @return The updated QCFrameDescriptorNodeIfs object.
     */
    QCFrameDescriptorNodeIfs &operator=( QCFrameDescriptorNodeIfs &other )
    {
        if ( this != &other )
        {
            for ( size_t i = 0; i < m_buffers.size(); i++ )
            {
                QCBufferDescriptorBase_t &bufDesc = other.GetBuffer( i );
                m_buffers[i] = bufDesc;
            }
        }
        return *this;
    }

    QCSharedFrameDescriptorNode &operator=( QCSharedFrameDescriptorNode &other )
    {
        if ( this != &other )
        {
            for ( size_t i = 0; i < m_buffers.size(); i++ )
            {
                QCBufferDescriptorBase_t &bufDesc = other.GetBuffer( i );
                m_buffers[i] = bufDesc;
            }
        }
        return *this;
    }

    /**
     * @brief Get the buffer descriptor identified by globalBufferId.
     * @param[in] globalBufferId The global buffer index.
     * @return The buffer descriptor identified by globalBufferId.
     * @note Retrieves the specific buffer descriptor according to the enumeration provided as part
     * of the initialization structure. The mapping between the buffer ID from the general buffer
     * vector to the buffer ID in the context for a node will be shared as part of the node
     * configuration structure. Follow the usage of QCNodeBufferMapEntry_t in the QCNodeBase.hpp
     * file. If the globalBufferId is out of range, a dummy buffer descriptor is returned.
     */
    virtual QCBufferDescriptorBase_t &GetBuffer( uint32_t globalBufferId )
    {
        std::reference_wrapper<QCBufferDescriptorBase_t> buffer = Dummy();
        if ( globalBufferId < m_buffers.size() )
        {
            buffer = m_buffers[globalBufferId];
        }
        return buffer;
    }

    /**
     * @brief Set the buffer descriptor identified by globalBufferId.
     * @param[in] globalBufferId The global buffer index.
     * @param[in] buffer The buffer descriptor.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e SetBuffer( uint32_t globalBufferId, QCBufferDescriptorBase_t &buffer )
    {
        QCStatus_e status = QC_STATUS_OK;
        if ( globalBufferId < m_buffers.size() )
        {
            m_buffers[globalBufferId] = buffer;
        }
        else
        {
            status = QC_STATUS_OUT_OF_BOUND;
        }
        return status;
    }

    /**
     * @brief Clear all the buffer descriptor to dummy.
     * @return None.
     */
    virtual void Clear()
    {
        for ( auto &buffer : m_buffers )
        {
            buffer = Dummy();
        }
    }

private:
    QCDummyBufferDescriptor_t &Dummy() { return *( (QCDummyBufferDescriptor_t *) &s_dummy ); }

private:
    std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> m_buffers;
    static const QCDummyBufferDescriptor_t s_dummy;
};

/**
 * @brief Represents the return object for QCNode API calls.
 * @param status The status of the API call.
 * @param obj A reference to the object returned by the API.
 *
 * @note This template struct encapsulates the status of the API call and a reference
 * to the object returned by the API. It provides a standardized way to handle
 * API responses, ensuring that both the status and the returned object are
 * easily accessible.
 */
template<typename T>
struct QCReturn
{
    QCStatus_e status;
    std::reference_wrapper<T> obj;
};

/**
 * @brief QCNode Shared Frame Descriptor Pool
 * This pool can be utilized by QCNode itself or by user applications to obtain a
 * QCSharedFrameDescriptorNode object for use.
 */
class QCSharedFrameDescriptorNodePool
{
public:
    /**
     * @brief Constructor for QCSharedFrameDescriptorNodePool.
     * @param[in] numOfFrameDesc The total number of QCSharedFrameDescriptorNode objects to create.
     * @param[in] numOfBuffers The total number of buffers each QCSharedFrameDescriptorNode object
     * holds.
     * @note This constructor initializes a pool of QCSharedFrameDescriptorNode objects, each
     * holding a specified number of buffers. It also populates a queue with these objects for easy
     * access.
     */
    QCSharedFrameDescriptorNodePool( uint32_t numOfFrameDesc, uint32_t numOfBuffers )
        : m_numOfBuffers( numOfBuffers ),
          m_frameDescs( numOfFrameDesc, QCSharedFrameDescriptorNode( numOfBuffers ) )
    {
        for ( auto &frameDesc : m_frameDescs )
        {
            m_queue.push( frameDesc );
        }
    }

    /**
     * @brief QCSharedFrameDescriptorNodePool Destructor
     * @return None
     */
    ~QCSharedFrameDescriptorNodePool() {}

    /**
     * @brief Retrieves a QCFrameDescriptorNodeIfs object from the pool.
     * @return A QCReturn object containing the status and the retrieved QCFrameDescriptorNodeIfs
     * object.
     * @note This method attempts to retrieve a QCFrameDescriptorNodeIfs object from the pool. If
     * the pool is not empty, it returns the front object from the queue, clears its contents, and
     * updates the status to indicate success. If the pool is empty, it updates the status to
     * indicate an out-of-bound error.
     */
    QCReturn<QCFrameDescriptorNodeIfs> Get()
    {
        QCReturn<QCFrameDescriptorNodeIfs> ret = { QC_STATUS_OK, Dummy() };
        std::lock_guard<std::mutex> l( m_lock );
        if ( false == m_queue.empty() )
        {
            ret.obj = m_queue.front().get();
            m_queue.pop();
            QCFrameDescriptorNodeIfs &frameDesc = ret.obj;
            frameDesc.Clear();
        }
        else
        {
            ret.status = QC_STATUS_OUT_OF_BOUND;
        }
        return ret;
    }

    /**
     * @brief Adds a QCFrameDescriptorNodeIfs object back to the pool.
     * @param[in] frameDesc The QCFrameDescriptorNodeIfs object to be added back to the pool.
     * @note This method pushes a QCFrameDescriptorNodeIfs object onto the queue, making it
     * available for future retrieval.
     */
    void Put( QCFrameDescriptorNodeIfs &frameDesc ) { m_queue.push( frameDesc ); }

private:
    QCSharedFrameDescriptorNode &Dummy() { return *( (QCSharedFrameDescriptorNode *) &s_dummy ); }
    std::vector<QCSharedFrameDescriptorNode> m_frameDescs;
    std::queue<std::reference_wrapper<QCFrameDescriptorNodeIfs>> m_queue;
    std::mutex m_lock;
    uint32_t m_numOfBuffers;
    static const QCSharedFrameDescriptorNode s_dummy;
};

class NodeConfigIfs : public QCNodeConfigIfs
{
public:
    /**
     * @brief NodeConfigIfs Constructor.
     * @param[in] logger A reference to a QC logger shared to be used by NodeConfigIfs.
     * @return None.
     */
    NodeConfigIfs( Logger &logger ) : m_logger( logger ) {}

    /**
     * @brief NodeConfigIfs Destructor
     * @return None
     */
    ~NodeConfigIfs() {}

    /**
     * @brief Verify and Load the json string
     *
     * @param[in] config the json configuration string
     * @param[out] errors the error string to be used to return readable error information
     *
     * @return QC_STATUS_OK on success, others on failure
     *
     * @note This API will also initialize the logger only once.
     * And this API can be called multiple times to apply dynamic parameter settings during runtime
     * after initialization.
     */
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get Configuration Options
     * @return A reference string to the JSON configuration options.
     */
    virtual const std::string &GetOptions() = 0;

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    virtual const QCNodeConfigBase_t &Get() = 0;

protected:
    bool m_bLoggerInit = false;
    DataTree m_dataTree;
    Logger &m_logger;
    static const std::string s_QC_STATUS_UNSUPPORTED;   //= "QC_STATUS_UNSUPPORTED";
};

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
