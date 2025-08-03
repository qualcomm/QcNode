// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_MANAGER_IFS_HPP
#define QC_MEMORY_MANAGER_IFS_HPP

#include "QC/Common/QCDefs.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"
#include "QCMemoryAllocatorIfs.hpp"
#include "QCMemoryPoolIfs.hpp"
#include <array>
#include <functional>

namespace QC
{
namespace Memory
{

/**
 * @struct QCMemoryHandle_t
 * @brief Structure for memory handles.
 *
 * This structure provides a unique identifier for a memory allocation.
 */
typedef struct QCMemoryHandle
{

    /**
     * @brief Less-than operator for comparing memory handles.
     *
     * This operator compares two memory handles based on their internal handle values.
     *
     * @param other The other memory handle to compare with.
     * @return True if this handle is less than the other handle, false otherwise.
     */
    bool operator<( const QCMemoryHandle &other ) const { return handle < other.handle; }

// Handle internal structure will be as follows:
// MSB 63<->48 QCNodeType_e (16 bits)
// MSB 47<->32 Instance count (16 bits)
// MSB 31<->0  random number (32 bits)

/**
 * @def QC_MEMORY_HANDLE_NODE_ID_ENUM_SHIFT
 * @brief Shift value for extracting the node ID from the handle.
 */
#define QC_MEMORY_HANDLE_NODE_ID_ENUM_SHIFT 48

/**
 * @def QC_MEMORY_HANDLE_NODE_COUNT_SHIFT
 * @brief Shift value for extracting the instance count from the handle.
 */
#define QC_MEMORY_HANDLE_NODE_COUNT_SHIFT 32

/**
 * @def QC_MEMORY_HANDLE_NODE_ID_ENUM_MASK
 * @brief Mask for extracting the node ID from the handle.
 */
#define QC_MEMORY_HANDLE_NODE_ID_ENUM_MASK 0xFFFF000000000000

/**
 * @def QC_MEMORY_HANDLE_NODE_COUNT_MASK
 * @brief Mask for extracting the instance count from the handle.
 */
#define QC_MEMORY_HANDLE_NODE_COUNT_MASK 0x0000FFFF00000000

/**
 * @def QC_MEMORY_HANDLE_RANDOM_MASK
 * @brief Mask for extracting the random number from the handle.
 */
#define QC_MEMORY_HANDLE_RANDOM_MASK 0x00000000FFFFFFFF

/**
 * @def QC_MEMORY_HANDLE_RANDOM_INV_MASK
 * @brief Mask for zeroing the random number bits of the handle.
 */
#define QC_MEMORY_HANDLE_RANDOM_INV_MASK 0xFFFFFFFF00000000

    /**
     * @brief Gets the node ID from the handle.
     * This method extracts the node ID from the handle using the
     * QC_MEMORY_HANDLE_NODE_ID_ENUM_MASK.
     * @return The node type as a QCNodeType_e enum value.
     */
    QCNodeType_e GetNodeType() const
    {
        return static_cast<QCNodeType_e>( ( QC_MEMORY_HANDLE_NODE_ID_ENUM_MASK & handle ) >>
                                          QC_MEMORY_HANDLE_NODE_ID_ENUM_SHIFT );
    };

    /**
     * @brief Gets the instance count from the handle.
     * This method extracts the instance count from the handle using the
     * QC_MEMORY_HANDLE_NODE_COUNT_MASK.
     * @return The instance count as a 16-bit unsigned integer.
     */
    uint16_t GetNodeCount() const
    {
        return static_cast<uint16_t>( ( QC_MEMORY_HANDLE_NODE_COUNT_MASK & handle ) >>
                                      QC_MEMORY_HANDLE_NODE_COUNT_SHIFT );
    };

    /**
     * @brief Gets the random number from the handle.
     * This method extracts the random number from the handle using the
     * QC_MEMORY_HANDLE_RANDOM_MASK.
     * @return The random number as a 32-bit unsigned integer.
     */
    uint32_t GetRandomNumber() const
    {
        return static_cast<uint32_t>( QC_MEMORY_HANDLE_RANDOM_MASK & handle );
    };

    /**
     * @brief Sets the node ID in the handle.
     * This method sets the node ID in the handle using the QC_MEMORY_HANDLE_NODE_ID_ENUM_MASK.
     * @param nodeID The new node ID to set.
     * @return The updated handle value.
     */
    uint64_t SetNodeType( const QCNodeType_e nodeID )
    {
        handle &= ~QC_MEMORY_HANDLE_NODE_ID_ENUM_MASK;
        handle |= ( ( ( static_cast<uint64_t>( nodeID ) ) << QC_MEMORY_HANDLE_NODE_ID_ENUM_SHIFT ) &
                    QC_MEMORY_HANDLE_NODE_ID_ENUM_MASK );
        return handle;
    };

    /**
     * @brief Sets the instance count in the handle.
     * This method sets the instance count in the handle using the QC_MEMORY_HANDLE_NODE_COUNT_MASK.
     * @param count The new instance count to set.
     * @return The updated handle value.
     */
    uint64_t SetNodeCount( const uint16_t count )
    {
        handle &= ~QC_MEMORY_HANDLE_NODE_COUNT_MASK;
        handle |= ( ( ( static_cast<uint64_t>( count ) ) << QC_MEMORY_HANDLE_NODE_COUNT_SHIFT ) &
                    QC_MEMORY_HANDLE_NODE_COUNT_MASK );
        return handle;
    };

    /**
     * @brief Sets the random number in the handle.
     * This method sets the random number in the handle using the QC_MEMORY_HANDLE_RANDOM_MASK.
     * @param randomNumber The new random number to set.
     * @return The updated handle value.
     */
    uint64_t SetRandomNumber( const uint32_t randomNumber )
    {
        handle &= QC_MEMORY_HANDLE_RANDOM_INV_MASK;
        handle |= ( ( static_cast<uint64_t>( randomNumber ) ) & QC_MEMORY_HANDLE_RANDOM_MASK );
        return handle;
    };

    /**
     * @brief Gets the handle value.
     * This method gets the internal handle value.
     * @return The updated handle value.
     */
    uint64_t GetHandle() const
    {
        return handle;
    };

    /**
     * @brief Sets the handle value.
     * This method sets the internal handle value.
     * @param handleValue The new handle value to set.
     */
    void SetHandle( const uint64_t handleValue )
    {
        handle = handleValue;
    };


private:
    /**
     * @var handle
     * @brief The internal handle value.
     */
    uint64_t handle;
} QCMemoryHandle_t;

/**
 * @struct QCMemoryPoolHandle_t
 * @brief Structure for memory pool handles.
 *
 * This structure provides a unique identifier for a memory pool.
 */
typedef struct QCMemoryPoolHandle
{

    /**
     * @brief Less-than operator for comparing memory pool handles.
     * This operator compares two memory pool handles based on their internal handle values.
     * @param other The other memory pool handle to compare with.
     * @return True if this handle is less than the other handle, false otherwise.
     */
    bool operator<( const QCMemoryPoolHandle &other ) const { return handle < other.handle; }

// Handle internal structure will be as follows:
// MSB 63<->48 QCNodeType_e (16 bits)
// MSB 47<->32 Pool count in a node (16 bits)
// MSB 31<->0  random number (32 bits)

/**
 * @def QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_SHIFT
 * @brief Shift value for extracting the node ID from the handle.
 */
#define QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_SHIFT 48

/**
 * @def QC_MEMORY_POOL_HANDLE_NODE_COUNT_SHIFT
 * @brief Shift value for extracting the pool count from the handle.
 */
#define QC_MEMORY_POOL_HANDLE_NODE_COUNT_SHIFT 32

/**
 * @def QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_MASK
 * @brief Mask for extracting the node ID from the handle.
 */
#define QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_MASK 0xFFFF000000000000

/**
 * @def QC_MEMORY_POOL_HANDLE_POOL_COUNT_MASK
 * @brief Mask for extracting the pool count from the handle.
 */
#define QC_MEMORY_POOL_HANDLE_POOL_COUNT_MASK 0x0000FFFF00000000

/**
 * @def QC_MEMORY_POOL_HANDLE_RANDOM_MASK
 * @brief Mask for extracting the random number from the handle.
 */
#define QC_MEMORY_POOL_HANDLE_RANDOM_MASK 0x00000000FFFFFFFF

/**
 * @def QC_MEMORY_POOL_HANDLE_RANDOM_INV_MASK
 * @brief Mask for zeroing the random number bits of the handle.
 */
#define QC_MEMORY_POOL_HANDLE_RANDOM_INV_MASK 0xFFFFFFFF00000000

    /**
     * @brief Gets the node ID from the handle.
     * This method extracts the node ID from the handle using the
     * QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_MASK.
     * @return The node ID as a QCNodeType_e enum value.
     */
    QCNodeType_e GetNodeType() const
    {
        return static_cast<QCNodeType_e>( ( QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_MASK & handle ) >>
                                          QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_SHIFT );
    };

    /**
     * @brief Gets the pool count from the handle.
     * This method extracts the pool count from the handle using the
     * QC_MEMORY_POOL_HANDLE_POOL_COUNT_MASK.
     * @return The pool count as a 16-bit unsigned integer.
     */
    uint16_t GetPoolCount() const
    {
        return static_cast<uint16_t>( ( QC_MEMORY_POOL_HANDLE_POOL_COUNT_MASK & handle ) >>
                                      QC_MEMORY_POOL_HANDLE_NODE_COUNT_SHIFT );
    };

    /**
     * @brief Gets the random number from the handle.
     * This method extracts the random number from the handle using the
     * QC_MEMORY_POOL_HANDLE_RANDOM_MASK.
     * @return The random number as a 32-bit unsigned integer.
     */
    uint32_t GetRandomNumber() const
    {
        return static_cast<uint32_t>( QC_MEMORY_POOL_HANDLE_RANDOM_MASK & handle );
    };

    /**
     * @brief Sets the node ID in the handle.
     * This method sets the node ID in the handle using the QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_MASK.
     * @param nodeID The new node ID to set.
     * @return The updated handle value.
     */
    uint64_t SetNodeType( const QCNodeType_e nodeID )
    {
        handle &= ~QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_MASK;
        handle |= ( ( ( static_cast<uint64_t>( nodeID ) )
                      << QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_SHIFT ) &
                    QC_MEMORY_POOL_HANDLE_NODE_ID_ENUM_MASK );
        return handle;
    };

    /**
     * @brief Sets the pool count in the handle.
     * This method sets the pool count in the handle using the
     * QC_MEMORY_POOL_HANDLE_POOL_COUNT_MASK.
     * @param count The new pool count to set.
     * @return The updated handle value.
     */
    uint64_t SetPoolCount( const uint16_t count )
    {
        handle &= ~QC_MEMORY_POOL_HANDLE_POOL_COUNT_MASK;
        handle |=
                ( ( ( static_cast<uint64_t>( count ) ) << QC_MEMORY_POOL_HANDLE_NODE_COUNT_SHIFT ) &
                  QC_MEMORY_POOL_HANDLE_POOL_COUNT_MASK );
        return handle;
    };

    /**
     * @brief Sets the random number in the handle.
     * This method sets the random number in the handle using the QC_MEMORY_POOL_HANDLE_RANDOM_MASK.
     * @param randomNumber The new random number to set.
     * @return The updated handle value.
     */
    uint64_t SetRandomNumber( const uint32_t randomNumber )
    {
        handle &= QC_MEMORY_POOL_HANDLE_RANDOM_INV_MASK;
        handle |= ( ( static_cast<uint64_t>( randomNumber ) ) & QC_MEMORY_POOL_HANDLE_RANDOM_MASK );
        return handle;
    };

    /**
     * @brief Gets the handle value.
     * This method gets the internal handle value.
     * @return The updated handle value.
     */
    uint64_t GetHandle() const
    {
        return handle;
    };

    /**
     * @brief Sets the handle value.
     * This method sets the internal handle value.
     * @param handleValue The new handle value to set.
     */
    void SetHandle( const uint64_t handleValue )
    {
        handle = handleValue;
    };

private:
    /**
     * @var handle
     * @brief The internal handle value.
     */
    uint64_t handle;
} QCMemoryPoolHandle_t;

/**
 * @struct QCMemoryManagerInit_t
 * @brief Structure for initializing a memory manager.
 *
 * This structure provides the necessary information to initialize a memory manager,
 * including the number of nodes and an array of memory allocators.
 */
typedef struct QCMemoryManagerInit
{

    /**
     * @brief Deleted default constructor.
     * This constructor is deleted to prevent default initialization of the QCMemoryManagerInit
     * structure.
     */
    QCMemoryManagerInit() = delete;

    /**
     * @brief Constructor for the QCMemoryManagerInit structure.
     * This constructor initializes the QCMemoryManagerInit structure with the provided number of
     * nodes and array of memory allocators.
     * @param numOfNodes The number of nodes in the system.
     * @param allocators The array of memory allocators.
     */
    QCMemoryManagerInit(
            uint32_t numOfNodes,
            std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                    allocators )
        : numOfNodes( numOfNodes ),
          allocators( allocators )
    {}

    /**
     * @var numOfNodes
     * @brief The number of nodes in the system.
     */
    uint32_t numOfNodes;

    /**
     * @var allocators
     * @brief The array of memory allocators.
     */
    std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST> allocators;

} QCMemoryManagerInit_t;

/**
 * @class QCMemoryManagerIfs
 * @brief Interface for memory managers.
 *
 * This class provides a common interface for memory managers,
 * including methods for registration, unregistration, pool creation and destruction, buffer
 * allocation and deallocation, and resource reclamation.
 */
class QCMemoryManagerIfs
{

public:
    /**
     * @brief Constructor.
     * This Constructor is declared as virtual to ensure that the correct destructor is called
     * when an object of a derived class is deleted through a pointer to the base class.
     */
    // QCMemoryManagerIfs(){};

    /**
     * @brief Virtual destructor.
     * This destructor is declared as virtual to ensure that the correct destructor is called
     * when an object of a derived class is deleted through a pointer to the base class.
     */
    virtual ~QCMemoryManagerIfs(){};

    /**
     * @brief Initializes internal data bases.
     * This Function allocates heap memory for internal data base structures.
     * Validates the allocation process success.
     * @param init The initialization parameters for the memory manager.
     * @return The status of the initialization operation.
     */
    virtual QCStatus_e Initialize( const QCMemoryManagerInit_t &init ) = 0;

    /**
     * @brief DeInitializes internal data bases and allocated resources.
     * This Function Deinitializes all allocated resoces and
     * heap memory for internal data base structures.
     * Validates the deallocation process success.
     * @return The status of the deinitialization operation.
     */
    virtual QCStatus_e DeInitialize() = 0;

    /**
     * @brief Registers a node with the memory manager.
     * This method registers a node with the memory manager and returns a handle that can be used to
     * identify the node.
     * @param node The unique ID of the node to register.
     * @param handle The unique handle that will be used to identify the node.
     * @return The status of the registration operation.
     */
    virtual QCStatus_e Register( const QCNodeID_t &node, QCMemoryHandle_t &handle ) = 0;

    /**
     * @brief Unregisters a node from the memory manager.
     * This method unregisters a node from the memory manager using the provided handle.
     * @param memHandle The handle of the node to unregister.
     * @return The status of the unregistration operation.
     */
    virtual QCStatus_e UnRegister( const QCMemoryHandle_t &memHandle ) = 0;

    /**
     * @brief Creates a memory pool.
     * This method creates a memory pool using the provided handle, pool configuration, and returns
     * a handle that can be used to identify the pool.
     * @param handle The unique handle of the node that owns the pool.
     * @param poolCfg The configuration of the pool to create.
     * @param poolHandle The unique handle that will be used to identify the pool.
     * @return The status of the pool creation operation.
     */
    virtual QCStatus_e CreatePool( const QCMemoryHandle_t &handle,
                                   const QCMemoryPoolConfig &poolCfg,
                                   QCMemoryPoolHandle_t &poolHandle ) = 0;

    /**
     * @brief Destroys a memory pool.
     * This method destroys a memory pool using the provided unique handle and pool handle.
     * @param handle The handle of the node that owns the pool.
     * @param poolHandle The handle of the pool to destroy.
     * @return The status of the pool destruction operation.
     */
    virtual QCStatus_e DestroyPool( const QCMemoryHandle_t &handle,
                                    const QCMemoryPoolHandle_t &poolHandle ) = 0;

    /**
     * @brief Allocates a buffer from a memory pool.
     * This method allocates a buffer from a memory pool using the provided memory handle, pool
     * handle, and returns a buffer descriptor.
     * @param memoryHandle The unique handle of the node that owns the pool.
     * @param poolHandle The unique handle of the pool to allocate from.
     * @param buff The buffer descriptor that will be used to store the allocated buffer.
     * @return The status of the buffer allocation operation.
     */
    virtual QCStatus_e AllocateBufferFromPool( const QCMemoryHandle_t &memoryHandle,
                                               const QCMemoryPoolHandle_t &poolHandle,
                                               QCBufferDescriptorBase_t &buff ) = 0;

    /**
     * @brief Puts a buffer back into a memory pool.
     * This method puts a buffer back into a memory pool using the provided memory handle, pool
     * handle, and buffer descriptor.
     * @param memoryHandle The handle of the node that owns the pool.
     * @param poolHandle The handle of the pool to put the buffer back into.
     * @param buff The buffer descriptor of the buffer to put back into the pool.
     * @return The status of the buffer put operation.
     */
    virtual QCStatus_e PutBufferToPool( const QCMemoryHandle_t &memoryHandle,
                                        const QCMemoryPoolHandle_t &poolHandle,
                                        const QCBufferDescriptorBase_t &buff ) = 0;

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
                                       QCBufferDescriptorBase_t &buff ) = 0;

    /**
     * @brief Frees a buffer.
     * This method frees a buffer using the provided memory handle and buffer descriptor.
     * @param handle The unique handle of the node that owns the buffer.
     * @param buff The buffer descriptor of the buffer to free.
     * @return The status of the buffer free operation.
     */
    virtual QCStatus_e FreeBuffer( const QCMemoryHandle_t handle,
                                   const QCBufferDescriptorBase_t &buff ) = 0;

    /**
     * @brief Reclaims resources.
     * This method reclaims resources using the provided memory handle.
     * @param handle The handle of the node that owns the resources to reclaim.
     * @return The status of the resource reclamation operation.
     */
    virtual QCStatus_e ReclaimResources( const QCMemoryHandle_t &handle ) = 0;

    /**
     * @brief Returns object state.
     * @return The state of the  Memory manager object
     */
    virtual QCObjectState_e GetState() { return m_state; };

protected:
    /**
     * @var m_state
     * @brief Dsecribes the state of the memory manager.
     */
    QCObjectState_e m_state;
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_MANAGER_IFS_HPP
