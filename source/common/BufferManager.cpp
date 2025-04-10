// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/common/BufferManager.hpp"
#include <sstream>
#include <stdio.h>

namespace ridehal
{
namespace common
{

std::mutex BufferManager::s_lock;
static BufferManager s_dftBufMgr;
BufferManager *BufferManager::s_pDefaultBufferManager = nullptr;

static std::string GetBufferTextInfo( const RideHal_SharedBuffer_t *pSharedBuffer )
{
    std::string str = "";
    std::stringstream ss;

    if ( RIDEHAL_BUFFER_TYPE_RAW == pSharedBuffer->type )
    {
        str = "Raw";
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE == pSharedBuffer->type )
    {
        ss << "Image format=" << pSharedBuffer->imgProps.format
           << " batch=" << pSharedBuffer->imgProps.batchSize
           << " resolution=" << pSharedBuffer->imgProps.width << "x"
           << pSharedBuffer->imgProps.height;
        if ( pSharedBuffer->imgProps.format < RIDEHAL_IMAGE_FORMAT_MAX )
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
    else if ( RIDEHAL_BUFFER_TYPE_TENSOR == pSharedBuffer->type )
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

RideHalError_e BufferManager::Init( const char *pName, Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = RIDEHAL_LOGGER_INIT( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to init logger for BUFMGR %s: ret = %d\n",
                        ( pName != nullptr ) ? pName : "null", ret );
    }
    ret = RIDEHAL_ERROR_NONE; /* ignore logger init error */

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

RideHalError_e BufferManager::Register( RideHal_SharedBuffer_t *pSharedBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSharedBuffer )
    {
        RIDEHAL_ERROR( "buffer is nullptr" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    std::lock_guard<std::mutex> l( m_lock );

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_IDAllocator++;
        pSharedBuffer->buffer.id = m_IDAllocator;
        m_bufferMap[pSharedBuffer->buffer.id] = *pSharedBuffer;
        RIDEHAL_INFO( "register %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 ": %s\n",
                      pSharedBuffer->buffer.pData, pSharedBuffer->buffer.dmaHandle,
                      pSharedBuffer->buffer.size, pSharedBuffer->buffer.id,
                      GetBufferTextInfo( pSharedBuffer ).c_str() );
    }

    return ret;
}

RideHalError_e BufferManager::Deregister( uint64_t id )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    std::lock_guard<std::mutex> l( m_lock );

    auto it = m_bufferMap.find( id );
    if ( it != m_bufferMap.end() )
    {
        RideHal_SharedBuffer_t &sharedBuffer = it->second;
        RIDEHAL_INFO( "deregister %p(%" PRIu64 ", %" PRIu64 ") as %" PRIu64 ": %s\n",
                      sharedBuffer.buffer.pData, sharedBuffer.buffer.dmaHandle,
                      sharedBuffer.buffer.size, sharedBuffer.buffer.id,
                      GetBufferTextInfo( &sharedBuffer ).c_str() );
        (void) m_bufferMap.erase( it );
    }
    else
    {
        RIDEHAL_ERROR( "buffer %" PRIu64 " not existed", id );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e BufferManager::GetSharedBuffer( uint64_t id, RideHal_SharedBuffer_t *pSharedBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pSharedBuffer )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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
            RIDEHAL_ERROR( "buffer %" PRIu64 " not found", id );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
    }

    return ret;
}
}   // namespace common
}   // namespace ridehal
