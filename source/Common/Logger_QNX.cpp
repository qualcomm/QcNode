// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#if defined( __QNXNTO__ )
#include "QC/Infras/Log/Logger.hpp"
#include <map>
#include <mutex>
#include <sys/slog2.h>

namespace QC
{

extern "C" char *__progname;

static uint8_t s_qcLoggerLevelToSlog2Level[] = {
        SLOG2_DEBUG2,  /* LOGGER_LEVEL_VERBOSE */
        SLOG2_DEBUG1,  /* LOGGER_LEVEL_DEBUG */
        SLOG2_INFO,    /* LOGGER_LEVEL_INFO */
        SLOG2_WARNING, /* LOGGER_LEVEL_WARN */
        SLOG2_ERROR    /* LOGGER_LEVEL_ERROR */
};

static std::mutex s_slog2MapLock;
static std::map<std::string, Logger_Handle_t> s_slog2Map;

void Logger::DefaultLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                         va_list args )
{
    slog2_buffer_t hBuffer = static_cast<slog2_buffer_t>( hHandle );
    int rc;
    // NOTE: always use SLOG2_INFO level as observed that the DEBUG/VERBOSE message are lost,
    // and this is a workaround
    // rc = vslog2f( hBuffer, 0, s_qcLoggerLevelToSlog2Level[level], pFormat, args );
    (void) s_qcLoggerLevelToSlog2Level;
    rc = vslog2f( hBuffer, 0, SLOG2_INFO, pFormat, args );
    if ( 0 != rc )
    {
        (void) fprintf( stderr, "slog2f error=%d: ", rc );
        (void) vfprintf( stderr, pFormat, args );
    }
}

QCStatus_e Logger::DefaultCreate( const char *pName, Logger_Level_e level,
                                  Logger_Handle_t *pHandle )
{
    QCStatus_e ret = QC_STATUS_OK;
    slog2_buffer_t hBuffer = nullptr;
    slog2_buffer_set_config_t bufferConfig;
    (void) level;

    if ( ( nullptr == pName ) || ( nullptr == pHandle ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        std::lock_guard<std::mutex> l( s_slog2MapLock );
        auto it = s_slog2Map.find( pName );
        if ( it == s_slog2Map.end() )
        { /* register slog2 buffer if the area name not exists */
            bufferConfig.num_buffers = 1;
            bufferConfig.verbosity_level = SLOG2_DEBUG2;

            bufferConfig.buffer_config[0].buffer_name = pName;
            bufferConfig.buffer_set_name = __progname;
            bufferConfig.buffer_config[0].num_pages = 32;

            int rv = slog2_register( &bufferConfig, &hBuffer, SLOG2_TRY_REUSE_BUFFER_SET );
            if ( ( 0 == rv ) && ( nullptr != hBuffer ) )
            {
                *pHandle = static_cast<Logger_Handle_t>( hBuffer );
                s_slog2Map[pName] = static_cast<Logger_Handle_t>( hBuffer );
            }
            else
            {
                (void) fprintf( stderr, "ERROR: failed to create slog2 %s: %d\n", pName, rv );
                ret = QC_STATUS_FAIL;
            }
        }
        else
        {
            *pHandle = it->second;
        }
    }

    return ret;
}

void Logger::DefaultDestory( Logger_Handle_t hHandle ) {}


}   // namespace QC

#endif /*  __QNXNTO__ */
