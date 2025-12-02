// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/BufferManager.hpp"
#include "QC/Infras/Memory/HeapAllocator.hpp"
#include "QC/Infras/Memory/ManagerLocal.hpp"
#if defined( __QNXNTO__ )
#include "QC/Infras/Memory/PMEMAllocator.hpp"
#else
#include "QC/Infras/Memory/DMABUFFAllocator.hpp"
#endif
#include <functional>
#include <memory>
#include <unistd.h>

namespace QC
{
namespace sample
{
std::mutex BufferManager::s_instanceMapMutex;
std::map<uint32_t, BufferManager::BufferManagerHolder> BufferManager::s_instanceMap;

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
        uint32_t numOfNodes = QC_BUFMGR_MAX_NODE;
        const char *envValue = getenv( "QC_NUM_NODE" );
        if ( nullptr != envValue )
        {
            numOfNodes = (uint32_t) atoi( envValue );
        }
        QCMemoryManagerInit_t memorymanagerInit( numOfNodes, defaultAllocators );
        QCStatus_e status = m_defaultMemoryMgr.Initialize( memorymanagerInit );
        if ( QC_STATUS_OK != status )
        {
            QC_LOG_ERROR( "Default Memory Manager initialize failed." );
        }
    }

    ~DefaultMemoryManagerInstance() { (void) m_defaultMemoryMgr.DeInitialize(); }

    QCMemoryManagerIfs &GetMemoryManager() { return m_defaultMemoryMgr; }

    static std::shared_ptr<DefaultMemoryManagerInstance> GetInstance()
    {
        std::lock_guard<std::mutex> lock( s_instanceMutex );
        if ( nullptr == s_defaultMemoryManagerInstance )
        {
            s_defaultMemoryManagerInstance = std::make_shared<DefaultMemoryManagerInstance>();
        }
        return s_defaultMemoryManagerInstance;
    }

private:
    HeapAllocator m_heapAllocator;
    AllocatorType m_dmaAllocator = AllocatorType( { "DMA" }, QC_MEMORY_ALLOCATOR_DMA );
    AllocatorType m_dmaCameraAllocator =
            AllocatorType( { "DMA_CAMERA" }, QC_MEMORY_ALLOCATOR_DMA_CAMERA );
    AllocatorType m_dmaGpuAllocator = AllocatorType( { "DMA_GPU" }, QC_MEMORY_ALLOCATOR_DMA_GPU );
    AllocatorType m_dmaVpuAllocator = AllocatorType( { "DMA_VPU" }, QC_MEMORY_ALLOCATOR_DMA_VPU );
    AllocatorType m_dmaEvaAllocator = AllocatorType( { "DMA_EVA" }, QC_MEMORY_ALLOCATOR_DMA_EVA );
    AllocatorType m_dmaHtpAllocator = AllocatorType( { "DMA_HTP" }, QC_MEMORY_ALLOCATOR_DMA_HTP );
    ManagerLocal m_defaultMemoryMgr;

    static std::mutex s_instanceMutex;
    static std::shared_ptr<DefaultMemoryManagerInstance> s_defaultMemoryManagerInstance;
};

std::mutex DefaultMemoryManagerInstance::s_instanceMutex;
std::shared_ptr<DefaultMemoryManagerInstance>
        DefaultMemoryManagerInstance::s_defaultMemoryManagerInstance = nullptr;

BufferManager::BufferManager( const QCNodeID_t &nodeId, Logger_Level_e logLevel )
{
    m_nodeId = nodeId;
    (void) QC_LOGGER_INIT( m_nodeId.name.c_str(), logLevel );
    QCMemoryManagerIfs &memMgrIfs = DefaultMemoryManagerInstance::GetInstance()->GetMemoryManager();
    QCStatus_e status = memMgrIfs.Register( m_nodeId, m_memoryHandle );
    if ( QC_STATUS_OK != status )
    {
        QC_ERROR( "Failed to register memory handle for %s.", m_nodeId.name.c_str() );
    }
}

BufferManager::~BufferManager()
{
    QCMemoryManagerIfs &memMgrIfs = DefaultMemoryManagerInstance::GetInstance()->GetMemoryManager();
    (void) memMgrIfs.UnRegister( m_memoryHandle );
    (void) QC_LOGGER_DEINIT();
};

QCStatus_e BufferManager::Allocate( const QCBufferPropBase_t &request,
                                    QCBufferDescriptorBase_t &response )
{
    QCStatus_e status = QC_STATUS_OK;

    const BufferProps_t *pBufferProps = dynamic_cast<const BufferProps_t *>( &request );
    BufferDescriptor_t *pBufferDescriptor = dynamic_cast<BufferDescriptor_t *>( &response );

    const ImageBasicProps_t *pImageBasicProps = dynamic_cast<const ImageBasicProps_t *>( &request );
    const ImageProps_t *pImageProps = dynamic_cast<const ImageProps_t *>( &request );
    ImageDescriptor_t *pImageDescriptor = dynamic_cast<ImageDescriptor_t *>( &response );

    const TensorProps_t *pTensorProps = dynamic_cast<const TensorProps_t *>( &request );
    TensorDescriptor_t *pTensorDescriptor = dynamic_cast<TensorDescriptor_t *>( &response );

    if ( ( nullptr != pTensorProps ) && ( nullptr != pTensorDescriptor ) )
    {
        status = AllocateTensor( *pTensorProps, *pTensorDescriptor );
    }
    else if ( ( nullptr != pImageProps ) && ( nullptr != pImageDescriptor ) )
    {
        status = AllocateImage( *pImageProps, *pImageDescriptor );
    }
    else if ( ( nullptr != pImageBasicProps ) && ( nullptr != pImageDescriptor ) )
    {
        status = AllocateBasicImage( *pImageBasicProps, *pImageDescriptor );
    }
    else if ( ( nullptr != pBufferProps ) && ( nullptr != pBufferDescriptor ) )
    {
        response.type = QC_BUFFER_TYPE_RAW;
        status = AllocateBinary( *pBufferProps, *pBufferDescriptor );
    }
    else
    {
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}


QCStatus_e BufferManager ::AllocateBinary( const BufferProps_t &request,
                                           BufferDescriptor_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    QCMemoryManagerIfs &memMgrIfs = DefaultMemoryManagerInstance::GetInstance()->GetMemoryManager();

    status = memMgrIfs.AllocateBuffer( m_memoryHandle, request.allocatorType, request, response );
    if ( QC_STATUS_OK == status )
    {
        response.validSize = request.size;
        response.offset = 0;
    }

    return status;
}

QCStatus_e BufferManager::AllocateBasicImage( const ImageBasicProps_t &request,
                                              ImageDescriptor_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    ImageBasicProps_t imgProps = request;

    status = m_util.SetImageDescFromImageBasicProp( imgProps, response );
    if ( QC_STATUS_OK == status )
    {
        status = AllocateBinary( imgProps, response );
    }

    return status;
}

QCStatus_e BufferManager::AllocateImage( const ImageProps_t &request, ImageDescriptor_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    ImageProps_t imgProps = request;

    status = m_util.SetImageDescFromImageProp( imgProps, response );
    if ( QC_STATUS_OK == status )
    {
        status = AllocateBinary( imgProps, response );
    }

    return status;
}

QCStatus_e BufferManager::AllocateTensor( const TensorProps_t &request,
                                          TensorDescriptor_t &response )
{
    QCStatus_e status = QC_STATUS_OK;
    TensorProps_t tsProps = request;
    status = m_util.SetTensorDescFromTensorProp( tsProps, response );

    if ( QC_STATUS_OK == status )
    {
        status = AllocateBinary( tsProps, response );
    }

    return status;
}

QCStatus_e BufferManager::Free( const QCBufferDescriptorBase_t &buffer )
{
    QCStatus_e status = QC_STATUS_OK;
    const BufferDescriptor_t *pBufferDescriptor =
            dynamic_cast<const BufferDescriptor_t *>( &buffer );
    pid_t pid = getpid();
    QCMemoryManagerIfs &memMgrIfs = DefaultMemoryManagerInstance::GetInstance()->GetMemoryManager();
    if ( nullptr == pBufferDescriptor )
    {
        QC_ERROR( "buffer is invalid" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == buffer.pBuf )
    {
        QC_ERROR( "buffer not allocated" );
        status = QC_STATUS_INVALID_BUF;
    }
    else if ( static_cast<uint64_t>( pid ) != buffer.pid )
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


BufferManager *BufferManager::Get( const QCNodeID_t &nodeId, Logger_Level_e logLevel )
{
    BufferManager *pBufMgr = nullptr;
    std::lock_guard<std::mutex> lock( s_instanceMapMutex );
    auto it = s_instanceMap.find( nodeId.id );
    if ( it == s_instanceMap.end() )
    {
        BufferManager *pBufMgrNew = new BufferManager( nodeId, logLevel );
        if ( nullptr != pBufMgrNew )
        {
            if ( pBufMgrNew->m_memoryHandle.GetHandle() != 0u )
            {
                pBufMgr = pBufMgrNew;
                s_instanceMap[nodeId.id] = { pBufMgrNew, 1 };
            }
            else
            {
                delete pBufMgrNew;
                QC_LOG_ERROR( "Failed to create buffer manager for %s %u", nodeId.name.c_str(),
                              nodeId.id );
            }
        }
        else
        {
            QC_LOG_ERROR( "No memory for buffer manager %s %u", nodeId.name.c_str(), nodeId.id );
        }
    }
    else
    {
        it->second.ref++;
        pBufMgr = it->second.pBufMgr;
    }

    return pBufMgr;
}

void BufferManager::Put( BufferManager *pBufMgr )
{
    std::lock_guard<std::mutex> lock( s_instanceMapMutex );
    if ( nullptr != pBufMgr )
    {
        auto it = s_instanceMap.find( pBufMgr->m_nodeId.id );
        if ( it != s_instanceMap.end() )
        {
            if ( it->second.ref > 0 )
            {
                it->second.ref--;
                if ( 0 == it->second.ref )
                {
                    s_instanceMap.erase( it );
                    delete pBufMgr;
                }
            }
            else
            {
                QC_LOG_ERROR( "Wrong buffer manager reference counter for %s %u",
                              pBufMgr->m_nodeId.name.c_str(), pBufMgr->m_nodeId.id );
            }
        }
        else
        {
            QC_LOG_ERROR( "No buffer manager found for %s %u", pBufMgr->m_nodeId.name.c_str(),
                          pBufMgr->m_nodeId.id );
        }
    }
    else
    {
        QC_LOG_ERROR( "Invalid buffer manager" );
    }
}

}   // namespace sample
}   // namespace QC
