// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "gtest/gtest.h"
#include <stdio.h>
#include <stdlib.h>

#define LOGGER_UNIT_TEST
#include "ridehal/common/Logger.hpp"

using namespace ridehal::common;

typedef struct
{
    std::string name;
} Logger_HandleContextUser_t;

static char s_LoggerMsg[1024] = { 0 };

#define MSG_PREFIX( name, b )                                                                      \
    std::string( #name ) + " " + std::string( __FILE__ ) + ":" + std::to_string( __LINE__ - b ) +  \
            " "

static void UserLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                     va_list args )
{
    int len = 0;
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    len = snprintf( s_LoggerMsg, sizeof( s_LoggerMsg ), "%s ", pContext->name.c_str() );
    (void) vsnprintf( &s_LoggerMsg[len], sizeof( s_LoggerMsg ) - len, pFormat, args );
}

static RideHalError_e UserLoggerHandleCreate( const char *pName, Logger_Level_e level,
                                              Logger_Handle_t *pHandle )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( nullptr == pName ) || ( nullptr == pHandle ) )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        Logger_HandleContextUser_t *pContext = new Logger_HandleContextUser_t;
        if ( nullptr != pContext )
        {
            pContext->name = pName;
            (void) level;
            *pHandle = (Logger_Handle_t) pContext;
        }
        else
        {
            ret = RIDEHAL_ERROR_NOMEM;
        }
    }

    return ret;
}

static void UserLoggerHandleDestroy( Logger_Handle_t hHandle )
{
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    if ( nullptr != pContext )
    {
        delete pContext;
    }
}


static void TestDefaultLogger()
{
    std::string rst;
    /* default logger level is ERROR */
    RIDEHAL_LOG_WARN( "A warn messgae is ignored", (uint32_t) 1234, 1.23431 );
    ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
    s_LoggerMsg[0] = '\0';

    RIDEHAL_LOG_ERROR( "A Fatal error: %.2f", 1.2345 );
    rst = MSG_PREFIX( RIHDEHAL, 1 ) + std::string( "ERROR: A Fatal error: 1.23" );
    ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
    s_LoggerMsg[0] = '\0';
}

class LoggerUser
{
public:
    LoggerUser() {}
    ~LoggerUser() {}

    RideHalError_e Init( const char *pName, Logger_Level_e level )
    {
        return RIDEHAL_LOGGER_INIT( pName, level );
    }

    RideHalError_e Deinit() { return RIDEHAL_LOGGER_DEINIT(); }

    void TestLoggerVerbose()
    {
        std::string rst;
        RIDEHAL_VERBOSE( "a=%d", 1234 );
        rst = MSG_PREFIX( TEST, 1 ) + std::string( "VERBOSE: a=1234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_VERBOSE( "str=%s a=%d", "hello world", 1234 );
        rst = MSG_PREFIX( TEST, 1 ) + std::string( "VERBOSE: str=hello world a=1234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_INFO( "a=%u c=%.3f", (uint32_t) 1234, 1.23431 );
        rst = MSG_PREFIX( TEST, 1 ) + std::string( "INFO: a=1234 c=1.234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_ERROR( "A Fatal error: 0x%x", 0xdeadbeef );
        rst = MSG_PREFIX( TEST, 1 ) + std::string( "ERROR: A Fatal error: 0xdeadbeef" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';
    }

    void TestLoggerInfo()
    {
        std::string rst;
        RIDEHAL_DEBUG( "a debug message is ignored" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_INFO( "a=%u c=%.3f", (uint32_t) 1234, 1.23431 );
        rst = MSG_PREFIX( TEST, 1 ) + std::string( "INFO: a=1234 c=1.234" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        RIDEHAL_ERROR( "A Fatal error: 0x%x", 0xdeadbeef );
        rst = MSG_PREFIX( TEST, 1 ) + std::string( "ERROR: A Fatal error: 0xdeadbeef" );
        ASSERT_EQ( rst, std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';
    }

private:
    RIDEHAL_DECLARE_LOGGER();
};

TEST( Logger, SANITY_Logger )
{
    RideHalError_e ret;
    LoggerUser loggerUser;

    ret = Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    /* only alow to setup once, the second setup will fail */
    ret = Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );
    ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

    unsetenv( "RIDEHAL_LOG_LEVEL" );
    unsetenv( "TEST_RIDEHAL_LOG_LEVEL" );

    ret = loggerUser.Init( "TEST", LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    loggerUser.TestLoggerVerbose();
    ret = loggerUser.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = loggerUser.Init( "TEST", LOGGER_LEVEL_INFO );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    loggerUser.TestLoggerInfo();
    ret = loggerUser.Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    TestDefaultLogger();
}

static void TestLoggerLog2( Logger &logger, Logger_Level_e level, const char *pFormat, ... )
{
    va_list args;

    va_start( args, pFormat );
    logger.Log( level, pFormat, args );
    va_end( args );
}

TEST( Logger, L2_Logger )
{

    (void) Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );

    unsetenv( "RIDEHAL_LOG_LEVEL" );
    unsetenv( "TEST_L2_RIDEHAL_LOG_LEVEL" );

    {
        RideHalError_e ret;
        Logger logger;

        logger.Log( LOGGER_LEVEL_ERROR, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        TestLoggerLog2( logger, LOGGER_LEVEL_ERROR, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Init( nullptr, LOGGER_LEVEL_INFO );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_MAX );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = logger.Init( "TEST_L2", (Logger_Level_e) -7 );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_INFO );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        TestLoggerLog2( logger, LOGGER_LEVEL_VERBOSE, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        TestLoggerLog2( logger, LOGGER_LEVEL_ERROR, "Hello %d", 1234 );
        ASSERT_EQ( std::string( "TEST_L2 Hello 1234" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_INFO );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
    }

    {
        RideHalError_e ret;
        Logger logger;
        setenv( "TEST_L2_RIDEHAL_LOG_LEVEL", "VERBOSE", 1 );
        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_INFO );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        logger.Log( LOGGER_LEVEL_VERBOSE, "Hello" );
        ASSERT_EQ( std::string( "TEST_L2 Hello" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    {
        RideHalError_e ret;
        Logger logger;
        setenv( "TEST_L2_RIDEHAL_LOG_LEVEL", "DEBUG", 1 );
        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_INFO );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        logger.Log( LOGGER_LEVEL_VERBOSE, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        logger.Log( LOGGER_LEVEL_DEBUG, "Hello" );
        ASSERT_EQ( std::string( "TEST_L2 Hello" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    {
        RideHalError_e ret;
        Logger logger;
        setenv( "TEST_L2_RIDEHAL_LOG_LEVEL", "INFO", 1 );
        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_DEBUG );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        logger.Log( LOGGER_LEVEL_DEBUG, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        logger.Log( LOGGER_LEVEL_INFO, "Hello" );
        ASSERT_EQ( std::string( "TEST_L2 Hello" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    {
        RideHalError_e ret;
        Logger logger;
        setenv( "TEST_L2_RIDEHAL_LOG_LEVEL", "WARN", 1 );
        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_ERROR );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        logger.Log( LOGGER_LEVEL_INFO, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        logger.Log( LOGGER_LEVEL_WARN, "Hello" );
        ASSERT_EQ( std::string( "TEST_L2 Hello" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    {
        RideHalError_e ret;
        Logger logger;
        setenv( "TEST_L2_RIDEHAL_LOG_LEVEL", "ERROR", 1 );
        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        logger.Log( LOGGER_LEVEL_WARN, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        logger.Log( LOGGER_LEVEL_ERROR, "Hello" );
        ASSERT_EQ( std::string( "TEST_L2 Hello" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    {
        RideHalError_e ret;
        Logger logger;
        setenv( "TEST_L2_RIDEHAL_LOG_LEVEL", "invalid_level", 1 );
        ret = logger.Init( "TEST_L2", LOGGER_LEVEL_INFO );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        logger.Log( LOGGER_LEVEL_DEBUG, "Hello" );
        ASSERT_EQ( std::string( "" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        logger.Log( LOGGER_LEVEL_INFO, "Hello" );
        ASSERT_EQ( std::string( "TEST_L2 Hello" ), std::string( s_LoggerMsg ) );
        s_LoggerMsg[0] = '\0';

        ret = logger.Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    unsetenv( "TEST_L2_RIDEHAL_LOG_LEVEL" );
}


static void TestLoggerDefaultL2( Logger_Handle_t handle, Logger_Level_e level, const char *pFormat,
                                 ... )
{
    va_list args;

    va_start( args, pFormat );
    Logger::DefaultLog( handle, level, pFormat, args );
    va_end( args );
}

TEST( Logger, L2_LoggerDefault )
{
    RideHalError_e ret;
    {
        Logger_Handle_t handle;
        ret = Logger::DefaultCreate( nullptr, LOGGER_LEVEL_INFO, &handle );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = Logger::DefaultCreate( "DEFAULT", LOGGER_LEVEL_INFO, nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = Logger::DefaultCreate( "DEFAULT", LOGGER_LEVEL_INFO, &handle );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        TestLoggerDefaultL2( handle, LOGGER_LEVEL_INFO, "Hello World" );
#if defined( __QNXNTO__ )
        /* test large message */
        char msg[1024] = { 0 };
        memset( msg, 'A', sizeof( msg ) - 1 );
        TestLoggerDefaultL2( handle, LOGGER_LEVEL_INFO, msg );
#endif
    }
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
