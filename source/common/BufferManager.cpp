// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/infras/memory/BufferManager.hpp"
#include <sstream>
#include <stdio.h>

namespace QC
{
namespace common
{

std::mutex BufferManager::s_lock;
static BufferManager s_dftBufMgr;
BufferManager *BufferManager::s_pDefaultBufferManager = nullptr;

static std::string GetBufferTextInfo( const QCSharedBuffer_t *pSharedBuffer )
{
    std::string str = "";
    std::stringstream ss;

    if ( QC_BUFFER_TYPE_RAW == pSharedBuffer->type )
    {
        str = "Raw";
    }
    else if ( QC_BUFFER_TYPE_IMAGE == pSharedBuffer->type )
    {
        ss << "Image format=" << pSharedBuffer->imgProps.format
           << " batch=" << pSharedBuffer->imgProps.batchSize
           << " resolution=" << pSharedBuffer->imgProps.width << "x"
           << pSharedBuffer->imgProps.height;
        if ( pSharedBuffer->imgProps.format < QC_IMAGE_FORMAT_MAX )
        {
            ss << " stride=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.stride[i] << ", ";
            }
            ss << "] actual height=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.actualHeight[i] << ", ";
            }
            ss << "] plane size=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        else
        {
            ss << " plane size=[";
            for ( uint32_t i = 0; i < pSharedBuffer->imgProps.numPlanes; i++ )
            {
                ss << pSharedBuffer->imgProps.planeBufSize[i] << ", ";
            }
            ss << "]";
        }
        str = ss.str();
    }
    else if ( QC_BUFFER_TYPE_TENSOR == pSharedBuffer->type )
    {
        ss << "Tensor type=" << pSharedBuffer->tensorProps.type << " dims=[";
        for ( uint32_t i = 0; i < pSharedBuffer->tensorProps.numDims; i++ )
        {
            ss << pSharedBuffer->tensorProps.dims[i] << ", ";
        }
        ss << "]";
        str = ss.str();
    }
    else
    {
        /* Invalid type, impossible case */
    }

    return str;
}

BufferManager::BufferManager() {}

BufferManager::~BufferManager() {}

QCStatus_e BufferManager::Init( const char *pName, Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = QC_LOGGER_INIT( pName, level );
    if ( QC_STATUS_OK != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to init logger for BUFMGR %s: ret = %d\n",
                        ( pName != nullptr ) ? pName : "null", ret );
    }
    ret = QC_STATUS_OK; /* ignore logger init error */

    return ret;
}

BufferManager *BufferManager::GetDefaultBufferManager()
{
    std::lock_guard<std::mutex> l( s_lock );
    if ( nullptr == s_pDefaultBufferManager )
    {
        s_pDefaultBufferManager = &s_dftBufMgr;
        (void) s_dftBufMgr.Init( "BUFMGR" );
    }

    return s_pDefaultBufferManager;
}

QCStatus_e BufferManager::Register( QCSharedBuffer_t *pSharedBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSharedBuffer )
    {
        QC_ERROR( "buffer is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    std::lock_guard<std::mutex> l( m_lock );

    if ( QC_STATUS_OK == ret )
    {
        m_IDAllocator++;
        pSharedBuffer->buffer.id = m_IDAllocator;
        m_bufferMap[pSharedBuffer->buffer.id] = *pSharedBuffer;
        QC_INFO( "register %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 ": %s\n",
                 pSharedBuffer->buffer.pData, pSharedBuffer->buffer.dmaHandle,
                 pSharedBuffer->buffer.size, pSharedBuffer->buffer.id,
                 GetBufferTextInfo( pSharedBuffer ).c_str() );
    }

    return ret;
}

QCStatus_e BufferManager::Deregister( uint64_t id )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::lock_guard<std::mutex> l( m_lock );

    auto it = m_bufferMap.find( id );
    if ( it != m_bufferMap.end() )
    {
        QCSharedBuffer_t &sharedBuffer = it->second;
        QC_INFO( "deregister %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 ": %s\n",
                 sharedBuffer.buffer.pData, sharedBuffer.buffer.dmaHandle, sharedBuffer.buffer.size,
                 sharedBuffer.buffer.id, GetBufferTextInfo( &sharedBuffer ).c_str() );
        (void) m_bufferMap.erase( it );
    }
    else
    {
        QC_ERROR( "buffer %" PRIu64 " not existed", id );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e BufferManager::GetSharedBuffer( uint64_t id, QCSharedBuffer_t *pSharedBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pSharedBuffer )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        std::lock_guard<std::mutex> l( m_lock );
        auto it = m_bufferMap.find( id );
        if ( it != m_bufferMap.end() )
        {
            *pSharedBuffer = it->second;
        }
        else
        {
            QC_ERROR( "buffer %" PRIu64 " not found", id );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    return ret;
}
}   // namespace common
}   // namespace QC
