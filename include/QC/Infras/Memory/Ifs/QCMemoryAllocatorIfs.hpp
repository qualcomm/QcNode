// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_MEMORY_ALLOCATOR_IFS_HPP
#define QC_MEMORY_ALLOCATOR_IFS_HPP

#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryDefs.hpp"

namespace QC
{
namespace Memory
{

/**
 * @struct QCMemoryAllocatorConfigInit_t
 * @brief Structure for initializing a memory allocator.
 *
 * This structure provides the necessary information to initialize a memory allocator,
 * including the name of the allocator.
 */
typedef struct QCMemoryAllocatorConfigInit
{
    /**
     * @var name
     * @brief The name of the allocator.
     */
    std::string name;
} QCMemoryAllocatorConfigInit_t;

/**
 * @struct QCMemoryAllocatorConfig_t
 * @brief Structure for configuring a memory allocator.
 *
 * This structure provides the necessary information to configure a memory allocator,
 * including the name and type of the allocator.
 */
typedef struct QCMemoryAllocatorConfig
{
    /**
     * @brief Deleted default constructor.
     *
     * This constructor is deleted to prevent default initialization of the QCMemoryAllocatorConfig
     * structure.
     */
    QCMemoryAllocatorConfig() = delete;

    /**
     * @brief Constructor for the QCMemoryAllocatorConfig structure.
     *
     * This constructor initializes the QCMemoryAllocatorConfig structure with the provided
     * configuration and type.
     *
     * @param config The configuration of the allocator.
     * @param type The type of the allocator.
     */
    QCMemoryAllocatorConfig( const QCMemoryAllocatorConfigInit_t &config,
                             const QCMemoryAllocator_e &type )
        : name( config.name ),
          type( type ){};

    /**
     * @var name
     * @brief The name of the allocator.
     */
    const std::string name;

    /**
     * @var type
     * @brief The type of the allocator.
     */
    const QCMemoryAllocator_e type;
} QCMemoryAllocatorConfig_t;

/**
 * @class QCMemoryAllocatorIfs
 * @brief Interface for memory allocators.
 *
 * This class provides a common interface for memory allocators,
 * including methods for allocation, deallocation, and configuration retrieval.
 */
class QCMemoryAllocatorIfs
{
public:
    /**
     * @brief Deleted default constructor.
     *
     * This constructor is deleted to prevent default initialization of the QCMemoryAllocatorIfs
     * class.
     */
    QCMemoryAllocatorIfs() = delete;

    /**
     * @brief Constructor for the QCMemoryAllocatorIfs class.
     *
     * This constructor initializes the QCMemoryAllocatorIfs object with the provided configuration
     * and type.
     *
     * @param config The configuration of the allocator.
     * @param allocator The type of the allocator.
     */
    QCMemoryAllocatorIfs( const QCMemoryAllocatorConfigInit_t &config,
                          QCMemoryAllocator_e allocator )
        : m_configuration( config, allocator ){};

    /**
     * @brief Assignment operator.
     *
     * This operator is not implemented and should not be used.
     *
     * @param t The object to assign from.
     * @return A reference to the current object.
     */
    QCMemoryAllocatorIfs &operator=( const QCMemoryAllocatorIfs &t ) { return *this; }

    /**
     * @brief Virtual destructor.
     *
     * This destructor is declared as virtual to ensure that the correct destructor is called
     * when an object of a derived class is deleted through a pointer to the base class.
     */
    virtual ~QCMemoryAllocatorIfs(){};

    /**
     * @brief Allocate a buffer with the specified properties.
     *
     * This method allocates a buffer with the specified properties.
     *
     * @param request The properties of the buffer to allocate.
     * @param response The descriptor of the allocated buffer.
     * @return The status of the allocation operation.
     */
    virtual QCStatus_e Allocate( const QCBufferPropBase_t &request,
                                 QCBufferDescriptorBase_t &response )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    /**
     * @brief Free a buffer with the specified descriptor.
     *
     * This method frees a buffer with the specified descriptor.
     *
     * @param buff The descriptor of the buffer to free.
     * @return The status of the free operation.
     */
    virtual QCStatus_e Free( const QCBufferDescriptorBase_t &buff )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    /**
     * @brief Get the configuration of the allocator.
     *
     * This method returns a reference to the configuration of the allocator.
     *
     * @return A constant reference to the configuration of the allocator.
     */
    virtual const QCMemoryAllocatorConfig_t &GetConfiguration() { return m_configuration; };

protected:
    /**
     * @brief Declare the logger for this class.
     */
    QC_DECLARE_LOGGER();

private:
    /**
     * @var m_configuration
     * @brief The configuration of the allocator.
     *
     * This member variable stores the configuration of the allocator,
     * which is set during construction.
     */
    const QCMemoryAllocatorConfig_t m_configuration;
};

}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_ALLOCATOR_IFS_HPP
