// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/shared_ring/SharedSemaphore.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>


namespace ridehal
{
namespace sample
{
namespace shared_ring
{

SharedSemaphore::SharedSemaphore()
{
    RIDEHAL_LOGGER_INIT( "SEM", LOGGER_LEVEL_ERROR );
}

SharedSemaphore::~SharedSemaphore()
{
    RIDEHAL_LOGGER_DEINIT();
}

RideHalError_e SharedSemaphore::Create( std::string name, int value )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    sem_t *pSem;

    pSem = sem_open( name.c_str(), O_CREAT | O_EXCL, 0644, value );
    if ( nullptr != pSem )
    {
        m_name = name;
        m_pSem = pSem;
    }
    else
    {
        RIDEHAL_ERROR( "Failed to create sem <%s>: %d", name.c_str(), errno );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e SharedSemaphore::Destroy()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rv;

    if ( nullptr != m_pSem )
    {
        rv = sem_close( m_pSem );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to destroy/close sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = RIDEHAL_ERROR_FAIL;
        }

        rv = sem_unlink( m_name.c_str() );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to destroy/unlink sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "sem not created" );
    }

    return ret;
}

RideHalError_e SharedSemaphore::Open( std::string name )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    sem_t *pSem;

    pSem = sem_open( name.c_str(), 0 );
    if ( nullptr != pSem )
    {
        m_name = name;
        m_pSem = pSem;
    }
    else
    {
        RIDEHAL_ERROR( "Failed to open sem <%s>: %d", name.c_str(), errno );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e SharedSemaphore::Close()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rv;

    if ( nullptr != m_pSem )
    {
        rv = sem_close( m_pSem );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to close sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "sem not opened" );
    }

    return ret;
}

RideHalError_e SharedSemaphore::Wait( uint32_t timeoutMs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rv;
    struct timespec ts;

    if ( nullptr == m_pSem )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "sem not ready!" );
    }
    else
    {
        if ( 0 == timeoutMs )
        {
            rv = sem_trywait( m_pSem );
        }
        else if ( SEMAPHORE_WAIT_INFINITE == timeoutMs )
        {
            rv = sem_wait( m_pSem );
        }
        else
        {
            rv = clock_gettime( CLOCK_REALTIME, &ts );
            if ( 0 != rv )
            {
                RIDEHAL_ERROR( "Failed to get clock realtime: %d", rv );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                ts.tv_sec += timeoutMs / 1000;
                ts.tv_nsec += ( timeoutMs % 1000 ) * 1000000;
                rv = sem_timedwait( m_pSem, &ts );
            }
        }

        if ( 0 != rv )
        {
            if ( ETIMEDOUT == errno )
            {
                ret = RIDEHAL_ERROR_TIMEOUT;
            }
            else if ( EINVAL == errno )
            {
                RIDEHAL_DEBUG( "Failed to wait on sem <%s>: %d %d", m_name.c_str(), rv, errno );
                ret = RIDEHAL_ERROR_TIMEOUT;
            }
            else
            {
                RIDEHAL_ERROR( "Failed to wait on sem <%s>: %d %d", m_name.c_str(), rv, errno );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

RideHalError_e SharedSemaphore::Post()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rv;
    struct timespec ts;

    if ( nullptr == m_pSem )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "sem not ready!" );
    }
    else
    {
        rv = sem_post( m_pSem );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to post on sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e SharedSemaphore::Reset()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rv;
    struct timespec ts;

    if ( nullptr == m_pSem )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
        RIDEHAL_ERROR( "sem not ready!" );
    }
    else
    {
        do
        {
            rv = sem_trywait( m_pSem );
        } while ( 0 == rv );
    }

    return ret;
}


}   // namespace shared_ring
}   // namespace sample
}   // namespace ridehal
