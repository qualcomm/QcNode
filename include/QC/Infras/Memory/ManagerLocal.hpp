// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_MEMORY_MANAGER_LOCAL_HPP
#define QC_MEMORY_MANAGER_LOCAL_HPP

#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/HeapAllocator.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryManagerIfs.hpp"
#include <functional>
#include <map>
#include <vector>

namespace QC
{
namespace Memory
{

/**
 * @class ManagerLocal
 * @brief A concrete implementation of the QCMemoryManagerIfs interface for local memory management.
 *
 * This class provides methods for managing memory locally, including registration, unregistration,
 * pool creation and destruction, buffer allocation and deallocation, and resource reclamation.
 */
class ManagerLocal : public QCMemoryManagerIfs
{

public:
    /**
     * @brief Default constructor.
     * This constructor Intilizes members which do not require external parameters
     */
    ManagerLocal();

    /**
     * @brief Initializer the ManagerLocal class.
     * This function initializes the ManagerLocal object with the provided initialization
     * parameters, allocates memories for the internal database.
     * @param init The initialization parameters for the memory manager.
     * @return The status of the registration operation.
     */
    virtual QCStatus_e Initialize( const QCMemoryManagerInit_t &init );

    /**
     * @brief DeInitializes internal data bases and allocated resources.
     * This Function DEinitializes all allocated resoces and
     * heap memory for internal data base structures.
     * Validates the deallocation process success.
     * @return The status of the deinitialization operation.
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Deleted assignment operator.
     * This operator is deleted to prevent assignment of ManagerLocal objects.
     * @param other The object to assign from.
     * @return A reference to the current object.
     */
    ManagerLocal &operator=( const ManagerLocal & ) = delete;

    /**
     * @brief Destructor for the ManagerLocal class.
     * This destructor releases any resources allocated by the ManagerLocal object.
     */
    virtual ~ManagerLocal();

    /**
     * @brief Registers a node with the memory manager.
     * This method registers a node with the memory manager and returns a handle that can be used to
     * identify the node.
     * @param node The ID of the node to register.
     * @param handle The handle that will be used to identify the node.
     * @return The status of the registration operation.
     */
    virtual QCStatus_e Register( const QCNodeID_t &node, QCMemoryHandle_t &handle );

    /**
     * @brief Unregisters a node from the memory manager.
     * This method unregisters a node from the memory manager using the provided handle.
     * @param memHandle The handle of the node to unregister.
     * @return The status of the unregistration operation.
     */
    virtual QCStatus_e UnRegister( const QCMemoryHandle_t &memHandle );

    /**
     * @brief Creates a memory pool.
     * This method creates a memory pool using the provided handle, pool configuration, and returns
     * a handle that can be used to identify the pool.
     * @param handle The handle of the node that owns the pool.
     * @param poolCfg The configuration of the pool to create.
     * @param poolHandle The handle that will be used to identify the pool.
     * @return The status of the pool creation operation.
     */
    virtual QCStatus_e CreatePool( const QCMemoryHandle_t &handle,
                                   const QCMemoryPoolConfig &poolCfg,
                                   QCMemoryPoolHandle_t &poolHandle );

    /**
     * @brief Destroys a memory pool.
     * This method destroys a memory pool using the provided handle and pool handle.
     * @param handle The handle of the node that owns the pool.
     * @param poolHandle The handle of the pool to destroy.
     * @return The status of the pool destruction operation.
     */
    virtual QCStatus_e DestroyPool( const QCMemoryHandle_t &handle,
                                    const QCMemoryPoolHandle_t &poolHandle );

    /**
     * @brief Allocates a buffer from a memory pool.
     * This method allocates a buffer from a memory pool using the provided memory handle, pool
     * handle, and returns a buffer descriptor.
     * @param memoryHandle The handle of the node that owns the pool.
     * @param poolHandle The handle of the pool to allocate from.
     * @param buff The buffer descriptor that will be used to store the allocated buffer.
     * @return The status of the buffer allocation operation.
     */
    virtual QCStatus_e AllocateBufferFromPool( const QCMemoryHandle_t &memoryHandle,
                                               const QCMemoryPoolHandle_t &poolHandle,
                                               QCBufferDescriptorBase_t &buff );

    /**
     * @brief Puts a buffer back into a memory pool.
     * This method returns a buffer to a memory pool using the provided memory handle, pool handle,
     * and buffer descriptor.
     * @param memoryHandle The handle of the node that owns the pool.
     * @param poolHandle The handle of the pool to put the buffer back into.
     * @param buff The buffer descriptor of the buffer to put back into the pool.
     * @return The status of the buffer put operation.
     */
    virtual QCStatus_e PutBufferToPool( const QCMemoryHandle_t &memoryHandle,
                                        const QCMemoryPoolHandle_t &poolHandle,
                                        const QCBufferDescriptorBase_t &buff );

    /**
     * @brief Allocates a buffer.
     * This method allocates a buffer using the provided memory handle, allocator, buffer
     * properties, and returns a buffer descriptor.
     * @param handle The handle of the node that owns the buffer.
     * @param allocator The type of allocator to use.
     * @param request The properties of the buffer to allocate.
     * @param buff The buffer descriptor that will be used to store the allocated buffer.
     * @return The status of the buffer allocation operation.
     */
    virtual QCStatus_e AllocateBuffer( const QCMemoryHandle_t handle,
                                       const QCMemoryAllocator_e allocator,
                                       const QCBufferPropBase_t &request,
                                       QCBufferDescriptorBase_t &buff );

    /**
     * @brief Frees a buffer.
     * This method frees a buffer using the provided memory handle and buffer descriptor.
     * @param handle The handle of the node that owns the buffer.
     * @param buff The buffer descriptor of the buffer to free.
     * @return The status of the buffer free operation.
     */
    virtual QCStatus_e FreeBuffer( const QCMemoryHandle_t handle,
                                   const QCBufferDescriptorBase_t &buff );

    /**
     * @brief Reclaims resources.
     * This method reclaims resources using the provided memory handle.
     * @param handle The handle of the node that owns the resources to reclaim.
     * @return The status of the resource reclamation operation.
     */
    virtual QCStatus_e ReclaimResources( const QCMemoryHandle_t &handle );

private:
    /**
     * @var m_handleToNodeIdInVector
     * @brief A mapping between memory handles and node IDs.
     */
    std::map<QCMemoryHandle_t, uint32_t> m_handleToNodeIdInVector;

    /**
     * @var m_pools
     * @brief A vector of nodes with mapping between pool handles and pool instances.
     */
    std::vector<std::map<QCMemoryPoolHandle_t, std::reference_wrapper<QCMemoryPoolIfs>>> m_pools;

    /**
     * @var m_allocations
     * @brief A vector of length of nodes containing map of allocated buffers and allocators.
     */
    std::vector<std::map<QCBufferDescriptorBase_t, QCMemoryAllocator_e>> m_allocations;

    /**
     * @var m_allocators
     * @brief An array of allocators.
     */
    // std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>*
    // m_allocators;

    /**
     * @brief Checks if a handle is legal.
     * This method checks if a handle is legal by verifying that it is present in the
     * handle-to-node-ID mapping.
     * @param handle The handle to check.
     * @param it An iterator to the handle in the mapping.
     * @return True if the handle is legal, false otherwise.
     */
    inline bool IsMemoryHandleRegistered( const QCMemoryHandle_t &handle,
                                          std::map<QCMemoryHandle_t, uint32_t>::iterator &it );

    /**
     * @brief Gets the number of registered nodes of the same type.
     * This method gets the number of registered nodes of the same type as the provided node.
     * @param node The node to get the number of registered nodes for.
     * @return The number of registered nodes of the same type.
     */
    inline uint64_t GetRegisteredNodesFromTheSameType( const QCNodeID_t &node );

    /**
     * @brief Gets the number of registered pools for a node.
     * This method gets the number of registered pools for a node using the provided handle.
     * @param handle The handle of the node to get the number of registered pools for.
     * @return The number of registered pools for the node.
     */
    inline uint64_t GetRegisteredPoolsCountForANode( const QCMemoryHandle_t &handle );

    /**
     * @brief Checks if a allocator is legal.
     * This method checks if allocatoe enum is legal.
     * @param allocator The allocator enaum to check.
     * @return True if the allocator is legal, false otherwise
     */
    inline bool IsAllocatorLeagal( const QCMemoryAllocator_e allocator );

    /**
     * @brief Checks if a node index is unique.
     * This method checks if node index is allready registered in m_handleToNodeIdInVector.
     * @param node The node structure content to check.
     * @return True if the node index is unique, false otherwise
     */
    inline bool IsNodeIdUnique( const QCNodeID_t &node );

    /**
     * @brief Declare the logger for this class.
     */
    QC_DECLARE_LOGGER();

    /**
     * @var m_handle2NodeIdLock
     * @brief A mutex for synchronizing access to the handle-to-node-ID mapping.
     */
    std::mutex m_handle2NodeIdLock;

    /**
     * @var m_poolsLock
     * @brief A mutex for synchronizing access to the pools.
     */
    std::mutex m_poolsLock;

    /**
     * @var m_allocationsLock
     * @brief A mutex for synchronizing access to the allocations.
     */
    std::mutex m_allocationsLock;

    /**
     * @var m_config
     * @brief Initial configuration passed by user.
     */
    QCMemoryManagerInit_t *m_config;
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_MANAGER_LOCAL_HPP
