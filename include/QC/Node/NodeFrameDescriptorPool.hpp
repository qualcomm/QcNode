// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_NODE_FRAME_DESCRIPTOR_POOL_HPP
#define QC_NODE_FRAME_DESCRIPTOR_POOL_HPP

#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include "QC/Node/NodeFrameDescriptor.hpp"

namespace QC
{
namespace Node
{

using namespace QC::Memory;

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
 * NodeFrameDescriptor object for use.
 */
class NodeFrameDescriptorPool
{
public:
    /**
     * @brief Constructor for NodeFrameDescriptorPool.
     * @param[in] numOfFrameDesc The total number of NodeFrameDescriptor objects to create.
     * @param[in] numOfBuffers The total number of buffers each NodeFrameDescriptor object
     * holds.
     * @note This constructor initializes a pool of NodeFrameDescriptor objects, each
     * holding a specified number of buffers. It also populates a queue with these objects for easy
     * access.
     */
    NodeFrameDescriptorPool( uint32_t numOfFrameDesc, uint32_t numOfBuffers )
        : m_frameDescs( numOfFrameDesc, NodeFrameDescriptor( numOfBuffers ) )
    {
        for ( auto &frameDesc : m_frameDescs )
        {
            m_queue.push( frameDesc );
        }
    }

    /**
     * @brief NodeFrameDescriptorPool Destructor
     * @return None
     */
    ~NodeFrameDescriptorPool() {}

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
    NodeFrameDescriptor &Dummy() { return s_dummy; }
    std::vector<NodeFrameDescriptor> m_frameDescs;
    std::queue<std::reference_wrapper<QCFrameDescriptorNodeIfs>> m_queue;
    std::mutex m_lock;
    static NodeFrameDescriptor s_dummy;
};


}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_FRAME_DESCRIPTOR_POOL_HPP
