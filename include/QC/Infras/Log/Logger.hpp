// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_LOGGER_HPP
#define QC_LOGGER_HPP

#include "QC/Common/Types.hpp"
#include <mutex>
#include <stdarg.h>
#include <stdio.h>
#include <string>

namespace QC
{

#ifndef DISABLE_QC_LOG

/** @brief A set of macros to be used by the QC components or utils.
 *
 * static void NonClassAPI(void) {
 *    QC_LOG_DEBUG( "NonClassAPI called" );
 * }
 *
 * class ComponentA {
 *   QCStatus_e Init( const char *pName, const char *pName, Logger_Level_e level ) {
 *     QCStatus_e ret = QC_LOGGER_INIT(); # do logger Init at the begin
 *     ... # those the QC_VERBOSE|DEBUG|INFO|WARN|ERROR related macros can be used.
 *     QC_INFO( "componentA: Init" );
 *  }
 *
 *  QCStatus_e Deinit( ) {
 *     QCStatus_e ret;
 *     ...
 *     QC_INFO( "componentA: Deinit" );
 *     ret = QC_LOGGER_DEINIT(); # Do logger Deinit at the end
 *  }
 *
 *  private:
 *    QC_DECLARE_LOGGER();
 * }
 */

/** @brief Define a logger that to be used by a class. */
#define QC_DECLARE_LOGGER() Logger m_logger

/** @brief Do initialization of the logger. */
#define QC_LOGGER_INIT( pName, level ) m_logger.Init( ( pName ), ( level ) )

/** @brief Do deinitialization of the logger. */
#define QC_LOGGER_DEINIT() m_logger.Deinit()


/**
 * @brief Do log a message
 * @param[in] logger the logger object use to do logger
 * @param[in] level the message log level
 * @param[in] pFormat the message format
 * @param[in] args variable arguments
 * @return void
 * @detdesc
 * Do log a message by calling the API Log of the logger.
 */
#define QC_LOGGER_LOG( logger, level, format, ... )                                                \
    ( logger ).Log( ( level ), "%s:%d " format, __FILE__, __LINE__, ##__VA_ARGS__ )


/** @brief Do log a verbose level message within a class API.
 * Note: the class must has a logger member by using QC_DECLARE_LOGGER, the same for
 * QC_DEBUG|INFO|WARN|ERROR. */
#define QC_VERBOSE( format, ... )                                                                  \
    QC_LOGGER_LOG( m_logger, LOGGER_LEVEL_VERBOSE, "VERBOSE: " format, ##__VA_ARGS__ )

/** @brief Do log a debug level message within a class API. */
#define QC_DEBUG( format, ... )                                                                    \
    QC_LOGGER_LOG( m_logger, LOGGER_LEVEL_DEBUG, "DEBUG: " format, ##__VA_ARGS__ )

/** @brief Do log a information level message within a class API. */
#define QC_INFO( format, ... )                                                                     \
    QC_LOGGER_LOG( m_logger, LOGGER_LEVEL_INFO, "INFO: " format, ##__VA_ARGS__ )

/** @brief Do log a warning level message within a class API. */
#define QC_WARN( format, ... )                                                                     \
    QC_LOGGER_LOG( m_logger, LOGGER_LEVEL_WARN, "WARN: " format, ##__VA_ARGS__ )

/** @brief Do log a error level message within a class API. */
#define QC_ERROR( format, ... )                                                                    \
    QC_LOGGER_LOG( m_logger, LOGGER_LEVEL_ERROR, "ERROR: " format, ##__VA_ARGS__ )


/** @brief Do log a verbose level message in non-class API. */
#define QC_LOG_VERBOSE( format, ... )                                                              \
    QC_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_VERBOSE, "VERBOSE: " format, ##__VA_ARGS__ )

/** @brief Do log a debug level message in non-class API. */
#define QC_LOG_DEBUG( format, ... )                                                                \
    QC_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_DEBUG, "DEBUG: " format, ##__VA_ARGS__ )

/** @brief Do log a information level message in non-class API. */
#define QC_LOG_INFO( format, ... )                                                                 \
    QC_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_INFO, "INFO: " format, ##__VA_ARGS__ )

/** @brief Do log a warning level message in non-class API. */
#define QC_LOG_WARN( format, ... )                                                                 \
    QC_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_WARN, "WARN: " format, ##__VA_ARGS__ )

/** @brief Do log a error level message in non-class API. */
#define QC_LOG_ERROR( format, ... )                                                                \
    QC_LOGGER_LOG( Logger::GetDefault(), LOGGER_LEVEL_ERROR, "ERROR: " format, ##__VA_ARGS__ )
#else
#define QC_DECLARE_LOGGER()

#define QC_LOGGER_INIT( pName, level ) QC_STATUS_OK
#define QC_LOGGER_DEINIT() QC_STATUS_OK

#define QC_LOGGER_LOG( logger, level, format, ... )

#define QC_VERBOSE( format, ... )
#define QC_DEBUG( format, ... )
#define QC_INFO( format, ... )
#define QC_WARN( format, ... )
#define QC_ERROR( format, ... )

#define QC_LOG_VERBOSE( format, ... )
#define QC_LOG_DEBUG( format, ... )
#define QC_LOG_INFO( format, ... )
#define QC_LOG_WARN( format, ... )
#define QC_LOG_ERROR( format, ... )
#endif

/** @brief The message log level */
typedef enum
{
    LOGGER_LEVEL_VERBOSE, /**< The level for the verbose message */
    LOGGER_LEVEL_DEBUG,   /**< The level for the debug message */
    LOGGER_LEVEL_INFO,    /**< The level for the information message */
    LOGGER_LEVEL_WARN,    /**< The level for the warning message */
    LOGGER_LEVEL_ERROR,   /**< The level for the error message */
    LOGGER_LEVEL_MAX
} Logger_Level_e;


/** @brief The handle to the log system  */
typedef void *Logger_Handle_t;

/**
 * @brief A callback to log a message
 * @param[in] hHandle the customer used log system handle
 * @param[in] level the message log level
 * @param[in] pFormat the message format
 * @param[in] args variable arguments
 * @return void
 */
typedef void ( *Logger_Log_t )( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                                va_list args );

/**
 * @brief A callback to create the customer used log system handle
 * @param[in] pName the name of the logger
 * @param[in] level the message log level
 * @param[out] pHandle [out] the pointer to return the created customer used log system handle
 * @return QC_STATUS_OK on success, others on failure
 */
typedef QCStatus_e ( *Logger_Create_t )( const char *pName, Logger_Level_e level,
                                         Logger_Handle_t *pHandle );

/**
 * @brief A callback to destroy the customer used log system handle
 * @param[in] hHandle the customer used log system handle
 * @return void
 */
typedef void ( *Logger_Destroy_t )( Logger_Handle_t hHandle );

/**
 * @brief qcnode::Logger
 *
 * Logger
 */
class Logger
{
public:
    Logger();
    ~Logger();

    /**
     * @brief Initialize the logger
     * @param[in] pName the name of the logger
     * @param[in] level the message log level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief deinitialize the logger
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

    /**
     * @brief Do log a message
     * @param[in] level the message log level
     * @param[in] pFormat the message format
     * @param[in] ... variable arguments
     * @return void
     * Note: if the logger object is not initialized or the initialization is failed, the message
     * will be dropped, and this API will return without any fault or error.
     */
    void Log( Logger_Level_e level, const char *pFormat, ... );

    /**
     * @brief Do log a message
     * @param[in] level the message log level
     * @param[in] pFormat the message format
     * @param[in] args variable arguments
     * @return void
     * Note: if the logger object is not initialized or the initialization is failed, the message
     * will be dropped, and this API will return without any fault or error.
     */
    void Log( Logger_Level_e level, const char *pFormat, va_list args );

    /**
     * @brief Get the acutal logger level of this logger
     * @return The current the logger level
     */
    Logger_Level_e GetLevel() { return m_level; }

    /**
     * @brief Setup the logger backend fuction pointers
     * @param[in] logFnc the function pointer that do log
     * @param[in] createFnc the function pointer that do create the implementation related handle
     * @param[in] destoryFnc the function pointer that do destroy the implementation related handle
     * @return QC_STATUS_OK on success, others on failure
     */
    static QCStatus_e Setup( Logger_Log_t logFnc, Logger_Create_t createFnc,
                             Logger_Destroy_t destoryFnc );
    /**
     * @brief Get a default QC logger
     * @return void
     */
    static Logger &GetDefault();

private:
    /**
     * @brief Decide the logger actual level used
     * @param[in] pName the name of the logger
     * @param[in] level the message log level
     * @return The actual level used
     */
    Logger_Level_e DecideLoggerLevel( std::string name, Logger_Level_e level );

#ifdef LOGGER_UNIT_TEST
public:
#endif
    static void DefaultLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                            va_list args );
    static QCStatus_e DefaultCreate( const char *pName, Logger_Level_e level,
                                     Logger_Handle_t *pHandle );
    static void DefaultDestory( Logger_Handle_t hHandle );

private:
    Logger_Handle_t m_hHandle = nullptr;
    Logger_Level_e m_level = LOGGER_LEVEL_ERROR;

    static Logger_Log_t s_logFnc;
    static Logger_Create_t s_createFnc;
    static Logger_Destroy_t s_destroyFnc;

    static std::mutex s_lock;
    static Logger s_defaultLogger;

};   // class Logger

}   // namespace QC

#endif   // QC_LOGGER_HPP