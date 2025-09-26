// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "QC/sample/shared_ring/SharedSemaphore.hpp"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>


namespace QC
{
namespace sample
{
namespace shared_ring
{

SharedSemaphore::SharedSemaphore()
{
    QC_LOGGER_INIT( "SEM", LOGGER_LEVEL_ERROR );
}

SharedSemaphore::~SharedSemaphore()
{
    QC_LOGGER_DEINIT();
}

QCStatus_e SharedSemaphore::Create( std::string name, int value )
{
    QCStatus_e ret = QC_STATUS_OK;
    sem_t *pSem;

    pSem = sem_open( name.c_str(), O_CREAT | O_EXCL, 0644, value );
    if ( nullptr != pSem )
    {
        m_name = name;
        m_pSem = pSem;
    }
    else
    {
        QC_ERROR( "Failed to create sem <%s>: %d", name.c_str(), errno );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e SharedSemaphore::Destroy()
{
    QCStatus_e ret = QC_STATUS_OK;
    int rv;

    if ( nullptr != m_pSem )
    {
        rv = sem_close( m_pSem );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to destroy/close sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = QC_STATUS_FAIL;
        }

        rv = sem_unlink( m_name.c_str() );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to destroy/unlink sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "sem not created" );
    }

    return ret;
}

QCStatus_e SharedSemaphore::Open( std::string name )
{
    QCStatus_e ret = QC_STATUS_OK;
    sem_t *pSem;

    pSem = sem_open( name.c_str(), 0 );
    if ( nullptr != pSem )
    {
        m_name = name;
        m_pSem = pSem;
    }
    else
    {
        QC_ERROR( "Failed to open sem <%s>: %d", name.c_str(), errno );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e SharedSemaphore::Close()
{
    QCStatus_e ret = QC_STATUS_OK;
    int rv;

    if ( nullptr != m_pSem )
    {
        rv = sem_close( m_pSem );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to close sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "sem not opened" );
    }

    return ret;
}

QCStatus_e SharedSemaphore::Wait( uint32_t timeoutMs )
{
    QCStatus_e ret = QC_STATUS_OK;
    int rv;
    struct timespec ts;

    if ( nullptr == m_pSem )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "sem not ready!" );
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
                QC_ERROR( "Failed to get clock realtime: %d", rv );
                ret = QC_STATUS_FAIL;
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
                ret = QC_STATUS_TIMEOUT;
            }
            else if ( EINVAL == errno )
            {
                QC_DEBUG( "Failed to wait on sem <%s>: %d %d", m_name.c_str(), rv, errno );
                ret = QC_STATUS_TIMEOUT;
            }
            else
            {
                QC_ERROR( "Failed to wait on sem <%s>: %d %d", m_name.c_str(), rv, errno );
                ret = QC_STATUS_FAIL;
            }
        }
    }

    return ret;
}

QCStatus_e SharedSemaphore::Post()
{
    QCStatus_e ret = QC_STATUS_OK;
    int rv;
    struct timespec ts;

    if ( nullptr == m_pSem )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "sem not ready!" );
    }
    else
    {
        rv = sem_post( m_pSem );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to post on sem <%s>: %d %d", m_name.c_str(), rv, errno );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e SharedSemaphore::Reset()
{
    QCStatus_e ret = QC_STATUS_OK;
    int rv;
    struct timespec ts;

    if ( nullptr == m_pSem )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "sem not ready!" );
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
}   // namespace QC
