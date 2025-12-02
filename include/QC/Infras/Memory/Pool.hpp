// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_MEMORY_POOL_HPP
#define QC_MEMORY_POOL_HPP

#include <algorithm>
#include <functional>
#include <list>
#include <string>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryAllocatorIfs.hpp"
#include "QC/Infras/Memory/Ifs/QCMemoryPoolIfs.hpp"

namespace QC
{
namespace Memory
{

class Pool : public QCMemoryPoolIfs
{
public:
    /**
     * @brief Constructor for Pool.
     * @return None.
     */
    Pool() = delete;
    Pool( const QCMemoryPoolConfig_t &poolCfg ) : QCMemoryPoolIfs( poolCfg )
    {
        (void) QC_LOGGER_INIT( GetConfiguration().name.c_str(), LOGGER_LEVEL_ERROR );
    };

    virtual ~Pool()
    {
        std::lock_guard<std::mutex> lk( m_lock );

        for ( auto it = m_freeObjects.begin(); it != m_freeObjects.end(); ++it )
        {
            QCBufferDescriptorBase_t descriptor;
            descriptor.pBuf = *it;
            descriptor.allocatorType = GetConfiguration().allocator.GetConfiguration().type;
            GetConfiguration().allocator.Free( descriptor );
        }

        for ( auto it = m_allocatedObjects.begin(); it != m_allocatedObjects.end(); ++it )
        {
            QCBufferDescriptorBase_t descriptor;
            descriptor.pBuf = *it;
            descriptor.allocatorType = GetConfiguration().allocator.GetConfiguration().type;
            GetConfiguration().allocator.Free( descriptor );
        }
        QC_LOGGER_DEINIT();
    };

    virtual QCStatus_e Init()
    {
        QCStatus_e status = QC_STATUS_OK;

        if ( 0 == GetConfiguration().maxElements )
        {
            QC_ERROR( "0 == m_config.m_maxElements" );
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            QCBufferPropBase_t request;
            request.alignment = GetConfiguration().buff.alignment;
            request.cache = GetConfiguration().buff.cache;
            request.size = GetConfiguration().buff.size;
            // the below can reduce effectiveness of pool creations
            // in case of multiple creations
            std::lock_guard<std::mutex> lk( m_lock );
            for ( auto _ = GetConfiguration().maxElements; _--; )
            {
                QCBufferDescriptorBase_t response;
                QCStatus_e loopStatus = GetConfiguration().allocator.Allocate( request, response );
                if ( QC_STATUS_OK != loopStatus )
                {
                    status = loopStatus;
                    QC_ERROR( "Allocation failed with status %d", loopStatus );
                    break;
                }
                else
                {
                    m_freeObjects.push_back( response.pBuf );
                    // verify insertion DB correctness
                    if ( m_freeObjects.back() != response.pBuf )
                    {
                        status = QC_STATUS_FAIL;
                        QC_ERROR( "m_freeObjects.back() != response.pBuf %p", response.pBuf );
                        break;
                    }
                }
            }
        }
        return status;
    }

    virtual QCStatus_e GetElement( QCBufferDescriptorBase_t &buffer )
    {
        QCStatus_e status = QC_STATUS_OK;

        std::lock_guard<std::mutex> lk( m_lock );
        if ( !m_freeObjects.empty() )
        {
            buffer.pBuf = m_freeObjects.front();
            if ( nullptr == buffer.pBuf )
            {
                status = QC_STATUS_NULL_PTR;
                QC_ERROR( "nullptr == buffer.pBuf" );
            }
            else
            {
                m_freeObjects.pop_front();
                QC_DEBUG( "extracted %p", buffer.pBuf );
                m_allocatedObjects.push_back( buffer.pBuf );
                buffer.alignment = GetConfiguration().buff.alignment;
                buffer.cache = GetConfiguration().buff.cache;
                buffer.size = GetConfiguration().buff.size;
                buffer.name = GetConfiguration().name;   // pool name
                buffer.allocatorType = GetConfiguration().allocator.GetConfiguration().type;

                auto findIt = std::find( m_freeObjects.begin(), m_freeObjects.end(), buffer.pBuf );
                // removal from free objects data base verification
                if ( findIt != m_freeObjects.end() )
                {
                    status = QC_STATUS_FAIL;
                    QC_ERROR( "m_freeObjects.find(%p) != m_freeObjects.end()", buffer.pBuf );
                }
                // addition to allocated objects data base verification
                else if ( m_allocatedObjects.back() != buffer.pBuf )
                {
                    status = QC_STATUS_FAIL;
                    QC_ERROR( "m_allocatedObjects.back()!= buffer.pBuf %p", buffer.pBuf );
                }
                else
                {}
            }
        }
        else
        {
            status = QC_STATUS_NO_RESOURCE;
            QC_ERROR( "No resources in pool" );
        }

        return status;
    }

    virtual QCStatus_e PutElement( const QCBufferDescriptorBase_t &buffer )
    {
        QCStatus_e status = QC_STATUS_BAD_ARGUMENTS;
        bool inDataBase = false;
        // check the match from allocator perspective
        if ( GetConfiguration().allocator.GetConfiguration().type != buffer.allocatorType )
        {
            QC_ERROR( "Allocator type mismatch expected %d recieved",
                      GetConfiguration().allocator.GetConfiguration().type, buffer.allocatorType );
        }
        else if ( nullptr == buffer.pBuf )
        {
            QC_ERROR( "nullptr == buffer.pBuf" );
            status = QC_STATUS_NULL_PTR;
        }
        else
        {
            std::lock_guard<std::mutex> lk( m_lock );
            for ( auto it = m_allocatedObjects.begin(); it != m_allocatedObjects.end(); ++it )
            {
                if ( *it == buffer.pBuf )
                {
                    inDataBase = true;
                    m_allocatedObjects.erase( it );
                    m_freeObjects.push_back( buffer.pBuf );
                    status = QC_STATUS_OK;
                    // removal from allocated objects data base verification
                    auto findIt = std::find( m_allocatedObjects.begin(), m_allocatedObjects.end(),
                                             buffer.pBuf );
                    if ( findIt != m_allocatedObjects.end() )
                    {
                        status = QC_STATUS_FAIL;
                        QC_ERROR( "m_allocatedObjects.find(%p) != m_allocatedObjects.end()",
                                  buffer.pBuf );
                    }
                    // addition to allocated objects data base verification
                    else if ( m_freeObjects.back() != buffer.pBuf )
                    {
                        status = QC_STATUS_FAIL;
                        QC_ERROR( "m_freeObjects.back() != buffer.pBuf %p", buffer.pBuf );
                    }
                    else
                    {}

                    break;
                }
            }
            if ( false == inDataBase )
            {
                QC_ERROR( "the pointer %p was not allocated from this pool", buffer.pBuf );
            }
        }

        return status;
    }

private:
    /* Associative array of Alignment_t queues to pool's elements */
    std::list<void *> m_freeObjects;
    std::list<void *> m_allocatedObjects;
    QC_DECLARE_LOGGER();
};


}   // namespace Memory
}   // namespace QC

#endif   // QC_MEMORY_HEAP_ALLOCATOR_HPP
