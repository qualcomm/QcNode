// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/sample/shared_ring/SharedMemory.hpp"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace QC
{
namespace sample
{
namespace shared_ring
{

SharedMemory::SharedMemory()
{
    QC_LOGGER_INIT( "SHM", LOGGER_LEVEL_ERROR );
}

SharedMemory::~SharedMemory()
{
    QC_LOGGER_DEINIT();
}

QCStatus_e SharedMemory::Create( std::string name, size_t size )
{
    QCStatus_e ret = QC_STATUS_OK;
    int fd;
    void *ptr;
    int rv;

    fd = shm_open( name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600 );
    if ( fd >= 0 )
    {
        rv = ftruncate( fd, size );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to reserve shm <%s> size to %" PRIu64 ": %d", name.c_str(), size,
                      rv );
            ret = QC_STATUS_NOMEM;
            shm_unlink( name.c_str() );
        }
        else
        {
            ptr = mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
            if ( nullptr != ptr )
            {
                m_name = name;
                m_fd = fd;
                m_ptr = ptr;
                m_size = size;
                (void) memset( ptr, 0, size );
            }
            else
            {
                QC_ERROR( "Failed to mmap shm <%s>", name.c_str() );
                ret = QC_STATUS_FAIL;
                shm_unlink( name.c_str() );
            }
        }
    }
    else
    {
        QC_ERROR( "Failed to create shm <%s>: %d", name.c_str(), fd );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}


QCStatus_e SharedMemory::Destroy()
{
    QCStatus_e ret = QC_STATUS_OK;

    int rv;
    if ( m_fd >= 0 )
    {
        rv = munmap( m_ptr, m_size );
        if ( 0 != rv )
        {
            QC_LOG_ERROR( "Failed to do munmap shm <%s>: %d", m_name.c_str(), rv );
            ret = QC_STATUS_FAIL;
        }

        rv = shm_unlink( m_name.c_str() );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to destroy shm <%s>: %d", m_name.c_str(), rv );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "shm not created" );
    }

    return ret;
}

QCStatus_e SharedMemory::Open( std::string name )
{
    QCStatus_e ret = QC_STATUS_OK;
    int fd;
    void *ptr;
    int rv;
    off_t off;

    fd = shm_open( name.c_str(), O_RDWR, 0600 );
    if ( fd >= 0 )
    {
        off = lseek( fd, 0, SEEK_END );
        if ( off <= 0 )
        {
            QC_ERROR( "Failed to get shm <%s> size: %d", name.c_str(), (int) off );
            ret = QC_STATUS_NOMEM;
            close( fd );
        }
        else
        {
            ptr = mmap( NULL, off, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );
            if ( nullptr != ptr )
            {
                m_name = name;
                m_fd = fd;
                m_ptr = ptr;
                m_size = off;
            }
            else
            {
                QC_ERROR( "Failed to mmap shm <%s>", name.c_str() );
                ret = QC_STATUS_FAIL;
                close( fd );
            }
        }
    }
    else
    {
        QC_ERROR( "Failed to open shm <%s>: %d", name.c_str(), fd );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e SharedMemory::Close()
{
    QCStatus_e ret = QC_STATUS_OK;

    int rv;
    if ( m_fd >= 0 )
    {
        rv = munmap( m_ptr, m_size );
        if ( 0 != rv )
        {
            QC_LOG_ERROR( "Failed to do munmap shm <%s>: %d", m_name.c_str(), rv );
            ret = QC_STATUS_FAIL;
        }

        rv = close( m_fd );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to close shm <%s>: %d", m_name.c_str(), rv );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "shm not opened" );
    }

    return ret;
}

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC
