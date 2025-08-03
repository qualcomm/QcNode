// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_POOL_IFS_HPP
#define QC_MEMORY_POOL_IFS_HPP

#include "QCMemoryAllocatorIfs.hpp"
#include "QCMemoryDefs.hpp"
#include <mutex>

namespace QC
{

namespace Memory
{

/**
 * @struct QCMemoryPoolConfig_t
 * @brief Structure for configuring a memory pool.
 * This structure provides the necessary information to configure a memory pool,
 * including the properties of individual buffers in the pool, the maximum number of elements,
 * the name of the pool, and a reference to the allocator used by the pool.
 */
typedef struct QCMemoryPoolConfig
{
    /**
     * @brief Deleted default constructor.
     * This constructor is deleted to prevent default initialization of the QCMemoryPoolConfig
     * structure.
     */
    QCMemoryPoolConfig() = delete;

    /**
     * @brief Constructor for the QCMemoryPoolConfig structure.
     * This constructor initializes the QCMemoryPoolConfig structure with the provided allocator.
     * @param allocator The allocator to be used by the pool.
     */
    QCMemoryPoolConfig( QCMemoryAllocatorIfs &allocator ) : maxElements( 0 ), allocator( allocator )
    {}

    /**
     * @var buff
     * @brief Properties of individual buffers in the pool.
     * This structure specifies the properties of each buffer in the pool,
     * such as size, alignment, and cache attributes.
     */
    QCBufferPropBase_t buff;

    /**
     * @var maxElements
     * @brief Maximum number of elements in the pool.
     * This value specifies the maximum number of buffers that can be allocated from the pool.
     * A value of 0 indicates that the pool has no limit on the number of elements.
     */
    QCCount_t maxElements;

    /**
     * @var name
     * @brief Name of the pool.
     * This string specifies the name of the pool, which can be used for identification and
     * debugging purposes.
     */
    std::string name;

    /**
     * @var allocator
     * @brief Reference to the allocator used by the pool.
     * This reference specifies the allocator that will be used to allocate and release memory for
     * the pool.
     */
    QCMemoryAllocatorIfs &allocator;
} QCMemoryPoolConfig_t;

/**
 * @class QCMemoryPoolIfs
 * @brief Interface for memory pool classes.
 * This class provides a common interface for memory pools,
 * including methods for initialization, getting and putting elements, and accessing the pool
 * configuration.
 */
class QCMemoryPoolIfs
{
public:
    /**
     * @brief Deleted default constructor.
     * This constructor is deleted to prevent default initialization of the QCMemoryPoolIfs class.
     */
    QCMemoryPoolIfs() = delete;

    /**
     * @brief Constructor for the QCMemoryPoolIfs class.
     * This constructor initializes the QCMemoryPoolIfs object with the provided pool configuration.
     * @param poolCfg The configuration of the pool.
     */
    QCMemoryPoolIfs( const QCMemoryPoolConfig_t &poolCfg ) : m_config( poolCfg ){};

    /**
     * @brief Virtual destructor.
     * This destructor is declared as virtual to ensure that the correct destructor is called
     * when an object of a derived class is deleted through a pointer to the base class.
     */
    virtual ~QCMemoryPoolIfs(){};

    /**
     * @brief Initializes the pool.
     * This method initializes the pool and prepares it for use.
     * @return The status of the initialization operation.
     */
    virtual QCStatus_e Init() = 0;

    /**
     * @brief Gets the configuration of the pool.
     * This method returns a reference to the pool configuration.
     * @return A constant reference to the pool configuration.
     */
    const QCMemoryPoolConfig_t &GetConfiguration() { return m_config; };

    /**
     * @brief Gets an element from the pool.
     * This method retrieves an element from the pool and returns it in the provided buffer
     * descriptor.
     * @param buffer The buffer descriptor that will be used to store the retrieved element.
     * @return The status of the get operation.
     */
    virtual QCStatus_e GetElement( QCBufferDescriptorBase_t &buffer ) = 0;

    /**
     * @brief Puts an element back into the pool.
     * This method returns an element to the pool using the provided buffer descriptor.
     * @param buffer The buffer descriptor of the element to put back into the pool.
     * @return The status of the put operation.
     */
    virtual QCStatus_e PutElement( const QCBufferDescriptorBase_t &buffer ) = 0;

private:
    /**
     * @var m_config
     * @brief The configuration of the pool.
     * This member variable stores the configuration of the pool, which is set during construction.
     */
    const QCMemoryPoolConfig_t m_config;

protected:
    /**
     * @var m_lock
     * @brief Mutex for synchronizing access to the pool.
     * This mutex is used to protect the pool from concurrent access and ensure thread safety.
     */
    std::mutex m_lock;
};


}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_POOL_IFS_HPP
