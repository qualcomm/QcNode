// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/Utils/BinaryAllocator.hpp"
#include "QC/Infras/Memory/HeapAllocator.hpp"
#include "QC/Infras/Memory/ManagerLocal.hpp"
#if defined( __QNXNTO__ )
#include "QC/Infras/Memory/PMEMAllocator.hpp"
#else
#include "QC/Infras/Memory/DMABUFFAllocator.hpp"
#endif
#include "QC/Infras/Memory/SharedBuffer.hpp"
#include <unistd.h>

namespace QC
{
namespace Memory
{
#ifndef QC_BUFMGR_MAX_NODE
#define QC_BUFMGR_MAX_NODE 1024
#endif

#if defined( __QNXNTO__ )
using AllocatorType = PMEMAllocator;
#else
using AllocatorType = DMABUFFAllocator;
#endif

// #define USE_MEMORY_MANAGER

#ifdef USE_MEMORY_MANAGER
class DefaultMemoryManagerInstance
{
public:
    DefaultMemoryManagerInstance()
    {
        std::array<std::reference_wrapper<QCMemoryAllocatorIfs>, QC_MEMORY_ALLOCATOR_LAST>
                defaultAllocators = { m_heapAllocator,   m_dmaAllocator,    m_dmaCameraAllocator,
                                      m_dmaGpuAllocator, m_dmaVpuAllocator, m_dmaEvaAllocator,
                                      m_dmaHtpAllocator };
        QCMemoryManagerInit_t memorymanagerInit( QC_BUFMGR_MAX_NODE, defaultAllocators );

        QCStatus_e status = m_defaultMemoryMgr.Initialize( memorymanagerInit );
        if ( QC_STATUS_OK == status )
        {
            m_bReady = true;
        }
        else
        {
            QC_LOG_ERROR( "Default Memory Manager initialize failed." );
        }
    }

    bool IsReady() const { return m_bReady; }

    ~DefaultMemoryManagerInstance() { /*(void) m_defaultMemoryMgr.DeInitialize(); */ }

    QCMemoryManagerIfs &GetMemoryManager() { return m_defaultMemoryMgr; }

private:
    bool m_bReady = false;

    HeapAllocator m_heapAllocator;
    AllocatorType m_dmaAllocator = AllocatorType( { "DMA" }, QC_MEMORY_ALLOCATOR_DMA );
    AllocatorType m_dmaCameraAllocator =
            AllocatorType( { "DMA_CAMERA" }, QC_MEMORY_ALLOCATOR_DMA_CAMERA );
    AllocatorType m_dmaGpuAllocator = AllocatorType( { "DMA_GPU" }, QC_MEMORY_ALLOCATOR_DMA_GPU );
    AllocatorType m_dmaVpuAllocator = AllocatorType( { "DMA_VPU" }, QC_MEMORY_ALLOCATOR_DMA_VPU );
    AllocatorType m_dmaEvaAllocator = AllocatorType( { "DMA_EVA" }, QC_MEMORY_ALLOCATOR_DMA_EVA );
    AllocatorType m_dmaHtpAllocator = AllocatorType( { "DMA_HTP" }, QC_MEMORY_ALLOCATOR_DMA_HTP );
    ManagerLocal m_defaultMemoryMgr;
};

static DefaultMemoryManagerInstance s_defaultMemoryManagerInstance;
#endif

BinaryAllocator::BinaryAllocator( const std::string &name )
    : BinaryAllocator( { name, QC_NODE_TYPE_CUSTOM_0, 0 }, LOGGER_LEVEL_ERROR )
{}


BinaryAllocator::BinaryAllocator( const QCNodeID_t &nodeId, Logger_Level_e logLevel )
{
    m_nodeId = nodeId;
    (void) QC_LOGGER_INIT( m_nodeId.name.c_str(), logLevel );
#ifdef USE_MEMORY_MANAGER
    QCMemoryManagerIfs &memMgrIfs = s_defaultMemoryManagerInstance.GetMemoryManager();
    QCStatus_e status = memMgrIfs.Register( m_nodeId, m_memoryHandle );
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to register memory handle for %s.", m_nodeId.name.c_str() );
    }
#endif
}

BinaryAllocator::~BinaryAllocator()
{
#ifdef USE_MEMORY_MANAGER
    QCMemoryManagerIfs &memMgrIfs = s_defaultMemoryManagerInstance.GetMemoryManager();
    (void) memMgrIfs.UnRegister( m_memoryHandle );
#endif
    (void) QC_LOGGER_DEINIT();
};

#ifndef USE_MEMORY_MANAGER
QCStatus_e BinaryAllocator::Allocate( const QCBufferPropBase_t &request,
                                      QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    void *pData;
    uint64_t dmaHandle;
    QCBufferFlags_t flags = 0;
    const BufferProps_t *pBufferProps = dynamic_cast<const BufferProps_t *>( &request );
    BufferDescriptor_t *pBufferDescriptor = dynamic_cast<BufferDescriptor_t *>( &response );
    static const QCBufferUsage_e s_Allocator2Usage[] = {
            QC_BUFFER_USAGE_MAX,     /* QC_MEMORY_ALLOCATOR_HEAP */
            QC_BUFFER_USAGE_DEFAULT, /* QC_MEMORY_ALLOCATOR_DMA */
            QC_BUFFER_USAGE_CAMERA,  /* QC_MEMORY_ALLOCATOR_DMA_CAMERA */
            QC_BUFFER_USAGE_GPU,     /* QC_MEMORY_ALLOCATOR_DMA_GPU */
            QC_BUFFER_USAGE_VPU,     /* QC_MEMORY_ALLOCATOR_DMA_VPU */
            QC_BUFFER_USAGE_EVA,     /* QC_MEMORY_ALLOCATOR_DMA_EVA */
            QC_BUFFER_USAGE_HTP,     /* QC_MEMORY_ALLOCATOR_DMA_HTP */
            QC_BUFFER_USAGE_MAX,     /* QC_MEMORY_ALLOCATOR_LAST */
    };

    QCBufferUsage_e usage = QC_BUFFER_USAGE_MAX;

    if ( ( nullptr != pBufferProps ) && ( nullptr != pBufferDescriptor ) )
    {
        if ( pBufferProps->allocatorType <
             sizeof( s_Allocator2Usage ) / sizeof( s_Allocator2Usage[0] ) )
        {
            usage = s_Allocator2Usage[pBufferProps->allocatorType];
        }
        if ( usage >= QC_BUFFER_USAGE_MAX )
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        if ( QC_CACHEABLE_NON != request.cache )
        {
            flags |= QC_BUFFER_FLAGS_CACHE_WB_WA;
        }
        if ( QC_STATUS_OK == status )
        {
            status = QCDmaAllocate( &pData, &dmaHandle, pBufferProps->size, flags, usage );
        }
        if ( QC_STATUS_OK == status )
        {
            pBufferDescriptor->name = m_nodeId.name + "-" + std::to_string( dmaHandle );
            pBufferDescriptor->pBuf = pData;
            pBufferDescriptor->size = pBufferProps->size;
            pBufferDescriptor->type = QC_BUFFER_TYPE_RAW;
            pBufferDescriptor->dmaHandle = dmaHandle;
            pBufferDescriptor->validSize = pBufferProps->size;
            pBufferDescriptor->offset = 0;
            pBufferDescriptor->id = 0; /* TODO: talk with Vadiam about the new buffer manager */
            pBufferDescriptor->pid = getpid();
            pBufferDescriptor->allocatorType = pBufferProps->allocatorType;
            pBufferDescriptor->cache = pBufferProps->cache;
        }
    }
    else
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }
    return status;
}
QCStatus_e BinaryAllocator::Free( const QCBufferDescriptorBase_t &buffer )
{
    QCStatus_e status = QC_STATUS_OK;
    const BufferDescriptor_t *pBufferDescriptor =
            dynamic_cast<const BufferDescriptor_t *>( &buffer );
    pid_t pid = getpid();
    if ( nullptr == pBufferDescriptor )
    {
        QC_ERROR( "buffer is invalid" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pBufferDescriptor->pBuf )
    {
        QC_ERROR( "buffer not allocated" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( static_cast<uint64_t>( pid ) != pBufferDescriptor->pid )
    {
        QC_ERROR( "buffer not allocated by self, can't do free" );
        status = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }
    if ( QC_STATUS_OK == status )
    {
        status = QCDmaFree( pBufferDescriptor->pBuf, pBufferDescriptor->dmaHandle,
                            pBufferDescriptor->size );
    }
    return status;
}
#else

QCStatus_e BinaryAllocator::Allocate( const QCBufferPropBase_t &request,
                                      QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    const BufferProps_t *pBufferProps = dynamic_cast<const BufferProps_t *>( &request );
    BufferDescriptor_t *pBufferDescriptor = dynamic_cast<BufferDescriptor_t *>( &response );
    QCMemoryManagerIfs &memMgrIfs = s_defaultMemoryManagerInstance.GetMemoryManager();

    if ( ( nullptr != pBufferProps ) && ( nullptr != pBufferDescriptor ) )
    {
        status = memMgrIfs.AllocateBuffer( m_memoryHandle, pBufferProps->allocatorType, request,
                                           response );
        if ( QC_STATUS_OK == status )
        {
            pBufferDescriptor->name = m_nodeId.name + "-" + std::to_string( response.dmaHandle );
            pBufferDescriptor->size = pBufferProps->size;
            pBufferDescriptor->type = QC_BUFFER_TYPE_RAW;
            pBufferDescriptor->pBuf = response.pBuf;
            pBufferDescriptor->dmaHandle = response.dmaHandle;
            pBufferDescriptor->validSize = pBufferProps->size;
            pBufferDescriptor->offset = 0;
            pBufferDescriptor->id = 0; /* TODO: talk with Vadiam about the new buffer manager */
            pBufferDescriptor->pid = getpid();
            pBufferDescriptor->allocatorType = pBufferProps->allocatorType;
            pBufferDescriptor->cache = pBufferProps->cache;
        }
    }
    else
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

QCStatus_e BinaryAllocator::Free( const QCBufferDescriptorBase_t &buffer )
{
    QCStatus_e status = QC_STATUS_OK;
    const BufferDescriptor_t *pBufferDescriptor =
            dynamic_cast<const BufferDescriptor_t *>( &buffer );
    pid_t pid = getpid();
    QCMemoryManagerIfs &memMgrIfs = s_defaultMemoryManagerInstance.GetMemoryManager();
    if ( nullptr == pBufferDescriptor )
    {
        QC_ERROR( "buffer is invalid" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pBufferDescriptor->pBuf )
    {
        QC_ERROR( "buffer not allocated" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( static_cast<uint64_t>( pid ) != pBufferDescriptor->pid )
    {
        QC_ERROR( "buffer not allocated by self, can't do free" );
        status = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == status )
    {
        QC_DEBUG( "FREE BEGIN" );
        status = memMgrIfs.FreeBuffer( m_memoryHandle, buffer );
        QC_DEBUG( "FREE END: %d", status );
    }

    return status;
}
#endif
}   // namespace Memory
}   // namespace QC
