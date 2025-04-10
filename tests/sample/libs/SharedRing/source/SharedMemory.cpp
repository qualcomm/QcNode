// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/shared_ring/SharedMemory.hpp"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace ridehal
{
namespace sample
{
namespace shared_ring
{

SharedMemory::SharedMemory()
{
    RIDEHAL_LOGGER_INIT( "SHM", LOGGER_LEVEL_ERROR );
}

SharedMemory::~SharedMemory()
{
    RIDEHAL_LOGGER_DEINIT();
}

RideHalError_e SharedMemory::Create( std::string name, size_t size )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int fd;
    void *ptr;
    int rv;

    fd = shm_open( name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600 );
    if ( fd >= 0 )
    {
        rv = ftruncate( fd, size );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to reserve shm <%s> size to %" PRIu64 ": %d", name.c_str(), size,
                           rv );
            ret = RIDEHAL_ERROR_NOMEM;
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
                RIDEHAL_ERROR( "Failed to mmap shm <%s>", name.c_str() );
                ret = RIDEHAL_ERROR_FAIL;
                shm_unlink( name.c_str() );
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "Failed to create shm <%s>: %d", name.c_str(), fd );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}


RideHalError_e SharedMemory::Destroy()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    int rv;
    if ( m_fd >= 0 )
    {
        rv = munmap( m_ptr, m_size );
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "Failed to do munmap shm <%s>: %d", m_name.c_str(), rv );
            ret = RIDEHAL_ERROR_FAIL;
        }

        rv = shm_unlink( m_name.c_str() );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to destroy shm <%s>: %d", m_name.c_str(), rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "shm not created" );
    }

    return ret;
}

RideHalError_e SharedMemory::Open( std::string name )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
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
            RIDEHAL_ERROR( "Failed to get shm <%s> size: %d", name.c_str(), (int) off );
            ret = RIDEHAL_ERROR_NOMEM;
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
                RIDEHAL_ERROR( "Failed to mmap shm <%s>", name.c_str() );
                ret = RIDEHAL_ERROR_FAIL;
                close( fd );
            }
        }
    }
    else
    {
        RIDEHAL_ERROR( "Failed to open shm <%s>: %d", name.c_str(), fd );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e SharedMemory::Close()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    int rv;
    if ( m_fd >= 0 )
    {
        rv = munmap( m_ptr, m_size );
        if ( 0 != rv )
        {
            RIDEHAL_LOG_ERROR( "Failed to do munmap shm <%s>: %d", m_name.c_str(), rv );
            ret = RIDEHAL_ERROR_FAIL;
        }

        rv = close( m_fd );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to close shm <%s>: %d", m_name.c_str(), rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "shm not opened" );
    }

    return ret;
}

}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal
