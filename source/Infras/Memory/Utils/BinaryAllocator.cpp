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

    ~DefaultMemoryManagerInstance() { (void) m_defaultMemoryMgr.DeInitialize(); }

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

static uint32_t s_dummyNodeId = 0;

BinaryAllocator::BinaryAllocator( const std::string &name )
    : BinaryAllocator( { name, QC_NODE_TYPE_CUSTOM_0, s_dummyNodeId++ }, LOGGER_LEVEL_ERROR )
{}


BinaryAllocator::BinaryAllocator( const QCNodeID_t &nodeId, Logger_Level_e logLevel )
{
    m_nodeId = nodeId;
    (void) QC_LOGGER_INIT( m_nodeId.name.c_str(), logLevel );
    QCMemoryManagerIfs &memMgrIfs = s_defaultMemoryManagerInstance.GetMemoryManager();
    QCStatus_e status = memMgrIfs.Register( m_nodeId, m_memoryHandle );
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to register memory handle for %s.", m_nodeId.name.c_str() );
    }
}

BinaryAllocator::~BinaryAllocator()
{
    QCMemoryManagerIfs &memMgrIfs = s_defaultMemoryManagerInstance.GetMemoryManager();
    (void) memMgrIfs.UnRegister( m_memoryHandle );
    (void) QC_LOGGER_DEINIT();
};

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
            pBufferDescriptor->type = QC_BUFFER_TYPE_RAW;
            pBufferDescriptor->validSize = pBufferProps->size;
            pBufferDescriptor->offset = 0;
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
}   // namespace Memory
}   // namespace QC
