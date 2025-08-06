// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/Infras/Log/Logger.hpp"
#include <stdlib.h>

namespace QC
{

Logger_Log_t Logger::s_logFnc = Logger::DefaultLog;
Logger_Create_t Logger::s_createFnc = Logger::DefaultCreate;
Logger_Destroy_t Logger::s_destroyFnc = Logger::DefaultDestory;

std::mutex Logger::s_lock;
Logger Logger::s_defaultLogger;

Logger::Logger() : m_hHandle( nullptr ) {}

Logger::~Logger() {}

QCStatus_e Logger::Setup( Logger_Log_t logFnc, Logger_Create_t createFnc,
                          Logger_Destroy_t destoryFnc )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( DefaultLog == s_logFnc ) && ( DefaultCreate == s_createFnc ) &&
         ( DefaultDestory == s_destroyFnc ) )
    {
        // only alow to setup once when the default function pointers are used
        s_logFnc = logFnc;
        s_createFnc = createFnc;
        s_destroyFnc = destoryFnc;

        ret = s_defaultLogger.Init( "QCNODE" );
    }
    else
    {
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

Logger &Logger::GetDefault()
{
    if ( nullptr == s_defaultLogger.m_hHandle )
    {
        std::lock_guard<std::mutex> l( s_lock );
        // the mutext lock is need to ensure the 2 more threads reach here at the same time, thus
        // the first thread that call this API do the default logger initialization. The second
        // thread Init call will get error QC_STATUS_BAD_STATE as it was already initialized.
        (void) s_defaultLogger.Init( "QCNODE" );
    }

    return s_defaultLogger;
}

QCStatus_e Logger::Init( const char *pName, Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pName )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( level >= LOGGER_LEVEL_MAX ) || ( level < LOGGER_LEVEL_VERBOSE ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( m_hHandle != nullptr )
    {
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        m_createFnc = s_createFnc;
        m_logFnc = s_logFnc;
        m_destroyFnc = s_destroyFnc;
        m_level = DecideLoggerLevel( pName, level );
        ret = m_createFnc( pName, m_level, &m_hHandle );
    }

    return ret;
}

void Logger::Log( Logger_Level_e level, const char *pFormat, ... )
{
    va_list args;

    if ( ( level >= m_level ) && ( nullptr != m_hHandle ) )
    {
        va_start( args, pFormat );
        m_logFnc( m_hHandle, level, pFormat, args );
        va_end( args );
    }
}

void Logger::Log( Logger_Level_e level, const char *pFormat, va_list args )
{
    if ( ( level >= m_level ) && ( nullptr != m_hHandle ) )
    {
        m_logFnc( m_hHandle, level, pFormat, args );
    }
}

QCStatus_e Logger::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( m_hHandle == nullptr )
    {
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        m_destroyFnc( m_hHandle );
        m_hHandle = nullptr;
    }

    return ret;
}

Logger_Level_e Logger::DecideLoggerLevel( std::string name, Logger_Level_e level )
{
    Logger_Level_e loggerLevel = level;
    std::string envName = name + "_QC_LOG_LEVEL";
    const char *envValue = getenv( envName.c_str() );
    if ( nullptr == envValue )
    {
        envName = "QC_LOG_LEVEL";
        envValue = getenv( envName.c_str() );
    }

    if ( nullptr != envValue )
    {
        std::string strEnv = envValue;
        if ( strEnv == "VERBOSE" )
        {
            loggerLevel = LOGGER_LEVEL_VERBOSE;
        }
        else if ( strEnv == "DEBUG" )
        {
            loggerLevel = LOGGER_LEVEL_DEBUG;
        }
        else if ( strEnv == "INFO" )
        {
            loggerLevel = LOGGER_LEVEL_INFO;
        }
        else if ( strEnv == "WARN" )
        {
            loggerLevel = LOGGER_LEVEL_WARN;
        }
        else if ( strEnv == "ERROR" )
        {
            loggerLevel = LOGGER_LEVEL_ERROR;
        }
        else
        {
            (void) fprintf( stderr, "ERROR: invalid env %s=%s\n", envName.c_str(), envValue );
        }
    }

    return loggerLevel;
}

}   // namespace QC