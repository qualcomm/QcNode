// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/Infras/Log/Logger.hpp"
#include <syslog.h>

namespace QC
{

typedef struct
{
    std::string name;
} Logger_HandleContext_t;

static int s_qcLoggerLevelToJournalPriotity[] = {
        LOG_DEBUG,   /* LOGGER_LEVEL_VERBOSE */
        LOG_INFO,    /* LOGGER_LEVEL_DEBUG */
        LOG_NOTICE,  /* LOGGER_LEVEL_INFO */
        LOG_WARNING, /* LOGGER_LEVEL_WARN */
        LOG_ERR      /* LOGGER_LEVEL_ERROR */
};

void Logger::DefaultLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                         va_list args )
{
    std::string strFmt;
    int priority = s_qcLoggerLevelToJournalPriotity[level];
    Logger_HandleContext_t *pContext = (Logger_HandleContext_t *) hHandle;
    strFmt = pContext->name + " : " + std::string( pFormat );
    vsyslog( priority, strFmt.c_str(), args );
}

QCStatus_e Logger::DefaultCreate( const char *pName, Logger_Level_e level,
                                  Logger_Handle_t *pHandle )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( nullptr == pName ) || ( nullptr == pHandle ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        Logger_HandleContext_t *pContext = new Logger_HandleContext_t;
        if ( nullptr != pContext )
        {
            pContext->name = pName;
            (void) level;
            *pHandle = (Logger_Handle_t) pContext;
        }
        else
        {
            ret = QC_STATUS_NOMEM;
        }
    }

    return ret;
}

void Logger::DefaultDestory( Logger_Handle_t hHandle )
{
    Logger_HandleContext_t *pContext = (Logger_HandleContext_t *) hHandle;
    if ( nullptr != pContext )
    {
        delete pContext;
    }
}

}   // namespace QC