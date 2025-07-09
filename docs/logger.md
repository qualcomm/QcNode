
*Menu*:
- [1. QC Logger Types](#1-qc-logger-types)
- [2. QC Logger Macros](#2-qc-logger-macros)
- [3. Typical Use Case](#3-typical-use-case)
  - [3.1 The QC default used log system](#31-the-qc-default-used-log-system)
    - [3.1.1 For QNX, the default used log system is slog2.](#311-for-qnx-the-default-used-log-system-is-slog2)
    - [3.1.2 For Linux, the default used log system is syslog.](#312-for-linux-the-default-used-log-system-is-syslog)
  - [3.2 Direct the QC logger message to the standard output(stdout)](#32-direct-the-qc-logger-message-to-the-standard-outputstdout)
    - [3.2.1 Define a customized Logger_HandleContextUser_t for the Logger_Handle_t](#321-define-a-customized-logger_handlecontextuser_t-for-the-logger_handle_t)
    - [3.2.2 Implement a Logger_Create_t API that to create a logger handle](#322-implement-a-logger_create_t-api-that-to-create-a-logger-handle)
    - [3.2.3 Implement a Logger_Log_t API that to do message log](#323-implement-a-logger_log_t-api-that-to-do-message-log)
    - [3.2.4 Implement a Logger_Destroy_t API that to destroy the logger](#324-implement-a-logger_destroy_t-api-that-to-destroy-the-logger)
    - [3.2.5 Call the Logger::Setup to switch to use the stdout](#325-call-the-loggersetup-to-switch-to-use-the-stdout)
- [4. How to dynamic change the logger level through environment variable](#4-how-to-dynamic-change-the-logger-level-through-environment-variable)
  - [4.1 "QC_LOG_LEVEL" to control all the logger level](#41-qcnode_log_level-to-control-all-the-logger-level)
  - [4.2 "${name}_QC_LOG_LEVEL" to control the logger level of the logger with name.](#42-name_qcnode_log_level-to-control-the-logger-level-of-the-logger-with-name)

The QC logger is designed to be used by the QC components or utils only, and with customization, the log messages can be easily directed to the user application used log system.

# 1. QC Logger Types

- [Logger_Level_e](../include/QC/Infras/Log/Logger.hpp#L131)
- [Logger_Handle_t](../include/QC/Infras/Log/Logger.hpp#L142)
- [Logger_Log_t](../include/QC/Infras/Log/Logger.hpp#L152)
- [Logger_Create_t](../include/QC/Infras/Log/Logger.hpp#L162)
- [Logger_Destroy_t](../include/QC/Infras/Log/Logger.hpp#L170)


# 2. QC Logger Macros

Generally, the logger Macros are not suggested to be used by user application as it was design to be used by the QC components or utils only.

- [QC_DECLARE_LOGGER](../include/QC/Infras/Log/Logger.hpp#L34)
- [QC_LOGGER_INIT](../include/QC/Infras/Log/Logger.hpp#L46)
- [QC_LOGGER_DEINIT](../include/QC/Infras/Log/Logger.hpp#L49)
- [QC_LOGGER_LOG](../include/QC/Infras/Log/Logger.hpp#L62)
- [QC_VERBOSE](../include/QC/Infras/Log/Logger.hpp#L69)
- [QC_DEBUG](../include/QC/Infras/Log/Logger.hpp#L73)
- [QC_INFO](../include/QC/Infras/Log/Logger.hpp#L77)
- [QC_WARN](../include/QC/Infras/Log/Logger.hpp#L81)
- [QC_ERROR](../include/QC/Infras/Log/Logger.hpp#L85)
- [QC_LOG_VERBOSE](../include/QC/Infras/Log/Logger.hpp#L90)
- [QC_LOG_DEBUG](../include/QC/Infras/Log/Logger.hpp#L95)
- [QC_LOG_INFO](../include/QC/Infras/Log/Logger.hpp#L99)
- [QC_LOG_WARN](../include/QC/Infras/Log/Logger.hpp#L103)
- [QC_LOG_ERROR](../include/QC/Infras/Log/Logger.hpp#L107)

And please note that if the macro "DISABLE_QC_LOG" was defined, all the QC log will be disabled.

And as the QC logger another main design goal is to be easily integrated with the user used log system, so those macro definitions are allowed to be modified in case the method introduced in section [3.2](#32-direct-the-qc-logger-message-to-the-standard-outputstdout) can't meet the requirement, but this is rare case. But for the purpose to improve performance, this can be a reason but not suggested to do so. For example, for the default log system slog2 used by QNX, the Logger_Log_t API implementation is as below, we can see snprintf was used to firstly format the message to a buffer and then call the slog2f API to do log which is somehow not performance friendly.

```c++
void Logger::DefaultLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                         va_list args )
{
    char msg[QC_LOG_MSG_MAX_LEN];
    slog2_buffer_t hBuffer = (slog2_buffer_t) hHandle;
    int len = 0;
    int rc;

    len = vsnprintf( msg, sizeof( msg ), pFormat, args );
    len += snprintf( &msg[len], sizeof( msg ) - len, "\n" );
    if ( len <= sizeof( msg ) )
    {
        ...
        rc = slog2f( hBuffer, 0, SLOG2_INFO, msg );
        ...
    }
    ...
}
```

So if change the logger member "[m_hHandle](../include/QC/Infras/Log/Logger.hpp#L254)" as public and the macro QC_LOGGER_LOG as below, the performance can be improved (but as log messages are not that frequently, the improvement is not that much).

```c++
#define QC_LOGGER_LOG( logger, level, format, ... )                                           \
    slog2f( (slog2_buffer_t) logger.m_hHandle, 0, SLOG2_INFO, "%s:%d " format, __FILE__, __LINE__, ##__VA_ARGS__ )
```

# 3. Typical Use Case

## 3.1 The QC default used log system

### 3.1.1 For QNX, the default used log system is slog2.

- [Logger::DefaultLog](../source/common/Logger_QNX.cpp#L32)
- [Logger::DefaultCreate](../source/common/Logger_QNX.cpp#L61)
- [Logger::DefaultDestory](../source/common/Logger_QNX.cpp#L107)

Below is a demo command to check the QC logs:

```sh
slog2info | grep qcnode

# below command to clear logs
slog2info -c
```

### 3.1.2 For Linux, the default used log system is syslog.

- [Logger::DefaultLog](../source/common/Logger_Linux.cpp#L25)
- [Logger::DefaultCreate](../source/common/Logger_Linux.cpp#L35)
- [Logger::DefaultDestory](../source/common/Logger_Linux.cpp#L62)

Below is a demo command to check the QC logs:

```sh
journalctl   | grep qcnode

# below command to clear logs
journalctl --flush --rotate --vacuum-size=1
journalctl --flush --rotate --vacuum-time=1s
```

## 3.2 Direct the QC logger message to the standard output(stdout)

The QC Sample App with command option "-d" demonstrates that how the QC logger message can be directed to the standard output(stdout).

And it was the same approach to direct the QC log message to any other user used log system.

### 3.2.1 Define a customized Logger_HandleContextUser_t for the Logger_Handle_t

Refer [Logger_HandleContextUser_t](../tests/sample/main.cpp#L22), as below code shows, it's a customized logger handle data structure that holds the name of the logger.

```c++
typedef struct
{
    std::string name;
} Logger_HandleContextUser_t;
```

### 3.2.2 Implement a Logger_Create_t API that to create a logger handle

Refer [UserLoggerHandleCreate](../tests/sample/main.cpp#L38), as below major code shows, it's that just new a user context handle and saving the logger’s name.

```c++
static QCStatus_e UserLoggerHandleCreate( const char *pName, Logger_Level_e level,
                                              Logger_Handle_t *pHandle )
{
    QCStatus_e ret = QC_STATUS_OK;
    ...
        Logger_HandleContextUser_t *pContext = new Logger_HandleContextUser_t;
        if ( nullptr != pContext )
        {
            pContext->name = pName;
            (void) level;
            *pHandle = (Logger_Handle_t) pContext;
        }
    ...

    return ret;
}
```

### 3.2.3 Implement a Logger_Log_t API that to do message log

Refer [UserLog](../tests/sample/main.cpp#L29), as below code shows, it does forward the message to stdout by using printf.

```c++
static void UserLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                     va_list args )
{
    char msg[256];
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    (void) vsnprintf( msg, sizeof( msg ), pFormat, args );
    printf( "%s: %s\n", pContext->name.c_str(), msg );
}
```

### 3.2.4 Implement a Logger_Destroy_t API that to destroy the logger

Refer [UserLoggerHandleDestroy](../tests/sample/main.cpp#L65), as below code shows, it does the destroy the user handle context.

```c++
static void UserLoggerHandleDestroy( Logger_Handle_t hHandle )
{
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    if ( nullptr != pContext )
    {
        delete pContext;
    }
}
```

### 3.2.5 Call the Logger::Setup to switch to use the stdout

Refer [Logger::Setup](../tests/sample/main.cpp#L141), as below code shows, it does the switch to use the stdout.

```c++
    (void) Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );
```

Please note that the Logger::Setup must be called only once before any logger was created.

# 4. How to dynamic change the logger level through environment variable

The dynamic change of the logger level through the environment variable must be done before starting the application that use the QC logger.

## 4.1 "QC_LOG_LEVEL" to control all the logger level.

```sh
# setting all the loggers' level to VERBOSE
export QC_LOG_LEVEL=VERBOSE

# setting all the loggers' level to DEBUG
export QC_LOG_LEVEL=DEBUG

# setting all the loggers' level to INFO
export QC_LOG_LEVEL=INFO

# setting all the loggers' level to WARN
export QC_LOG_LEVEL=WARN

# setting all the loggers' level to ERROR
export QC_LOG_LEVEL=ERROR
```

## 4.2 "${name}_QC_LOG_LEVEL" to control the logger level of the logger with name.

Generally, it’s recommended that the logger’s name is in upper case.

For example, for a camera component that using the QC logger with name "CAM0", the logger level can be changed through environment variable "CAM0_QC_LOG_LEVEL".

```sh
# setting the component CAM0 logger level to VERBOSE
export CAM0_QC_LOG_LEVEL=VERBOSE

# setting the component CAM0 logger level to DEBUG
export CAM0_QC_LOG_LEVEL=DEBUG

# setting the component CAM0 logger level to INFO
export CAM0_QC_LOG_LEVEL=INFO

# setting the component CAM0 logger level to WARN
export CAM0_QC_LOG_LEVEL=WARN

# setting the component CAM0 logger level to ERROR
export CAM0_QC_LOG_LEVEL=ERROR
``````
