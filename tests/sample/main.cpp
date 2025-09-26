// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "QC/sample/SampleIF.hpp"

#include <chrono>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <time.h>
#include <unistd.h>

using namespace QC::sample;

typedef struct
{
    std::string name;
} Logger_HandleContextUser_t;

std::condition_variable CV;

static void UserLog( Logger_Handle_t hHandle, Logger_Level_e level, const char *pFormat,
                     va_list args )
{
    std::string strFmt;
    Logger_HandleContextUser_t *pContext = (Logger_HandleContextUser_t *) hHandle;
    time_t rawtime;
    struct tm *info;
    char timeStr[80];
    time( &rawtime );
    info = localtime( &rawtime );
    strftime( timeStr, 80, "%Y-%m-%d %H:%M:%S ", info );
    strFmt = std::string( timeStr ) + pContext->name + " : " + std::string( pFormat ) + "\n";
    vprintf( strFmt.c_str(), args );
}

static QCStatus_e UserLoggerHandleCreate( const char *pName, Logger_Level_e level,
                                          Logger_Handle_t *pHandle )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( nullptr == pName ) || ( nullptr == pHandle ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
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
            ret = QC_STATUS_NOMEM;
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

void SignalHandler( int signal )
{
    std::cout << "Caught signal " << signal << std::endl;
    CV.notify_one();
}

typedef struct
{
    std::string name;
    SampleConfig_t config;
    std::string type;
} PipelineConfig_t;

int Usage( const char *program, int error )
{
    printf( "Usage: %s -n name -t type -k key -v value [-d] [-T run_time_seconds] [-h]\n"
            "examples:\n"
            "%s -n CAM0 -t camera -k input_id -v 0 -k width -v 1920 -k height -v 1024 \\\n"
            "    -k topic -v /sensor/camera/CAM0/raw \\\n"
            "  -n CAM0_REMAP -t remap -k width -v 1152 -k height -v 768 \\\n"
            "    -k input_topic -v /sensor/camera/CAM0/raw \\\n"
            "    -k output_topic -v /sensor/camera/CAM0/rgb\n",
            program, program );
    return error;
}

int main( int argc, char *argv[] )
{
    QCStatus_e ret;
    int timeS = 0;
    std::vector<SampleIF *> samples;

    signal( SIGINT, SignalHandler );
    signal( SIGTERM, SignalHandler );

    std::vector<PipelineConfig_t> pipelineConfigs;
    std::string key;
    int opt;
    while ( ( opt = getopt( argc, argv, "dn:t:k:v:hT:" ) ) != -1 )
    {
        switch ( opt )
        {
            case 'n':
            {
                PipelineConfig_t config;
                config.name = optarg;
                pipelineConfigs.push_back( config );
                break;
            }
            case 't':
            {
                PipelineConfig_t &config = pipelineConfigs.back();
                config.type = optarg;
                break;
            }
            case 'k':
            {
                key = optarg;
                break;
            }
            case 'v':
            {
                PipelineConfig_t &config = pipelineConfigs.back();
                config.config[key] = optarg;
                break;
            }
            case 'd':
                (void) Logger::Setup( UserLog, UserLoggerHandleCreate, UserLoggerHandleDestroy );
                break;
            case 'h':
                return Usage( argv[0], 0 );
                break;
            case 'T':
                timeS = atoi( optarg );
                break;
            default:
                return Usage( argv[0], -1 );
                break;
        }
    }

    if ( 0 == pipelineConfigs.size() )
    {
        return Usage( argv[0], -1 );
    }

    for ( auto &config : pipelineConfigs )
    {
        SampleIF *pSample = SampleIF::Create( config.type );
        if ( nullptr != pSample )
        {
            ret = pSample->Init( config.name, config.config );
            if ( ret != QC_STATUS_OK )
            {
                printf( "Init %s failed: ret = %d\n", config.name.c_str(), ret );
                return -1;
            }
            else
            {
                printf( "Init %s OK\n", config.name.c_str() );
                samples.push_back( pSample );
            }
        }
        else
        {
            printf( "Create %s with type %s failed\n", config.name.c_str(), config.type.c_str() );
            return -1;
        }
    }

    for ( auto sample : samples )
    {
        ret = sample->Start();
        if ( ret != QC_STATUS_OK )
        {
            printf( "Start %s failed: ret = %d\n", sample->GetName(), ret );
            return -1;
        }
        else
        {
            printf( "Start %s OK\n", sample->GetName() );
        }
    }

    {
        // wait for signal
        std::mutex m;
        std::unique_lock<std::mutex> lock( m );
        if ( 0 == timeS )
        { /* run forever until stop signal */
            CV.wait( lock );
        }
        else
        {
            CV.wait_for( lock, std::chrono::seconds( timeS ) );
        }
    }

    for ( int i = (int) samples.size() - 1; i >= 0; i-- )
    {
        auto sample = samples[i];
        ret = sample->Stop();
        if ( ret != QC_STATUS_OK )
        {
            printf( "Stop %s failed: ret = %d\n", sample->GetName(), ret );
        }
        else
        {
            printf( "Stop %s OK\n", sample->GetName() );
        }
    }

    for ( int i = (int) samples.size() - 1; i >= 0; i-- )
    {
        auto sample = samples[i];
        ret = sample->Deinit();
        if ( ret != QC_STATUS_OK )
        {
            printf( "Deinit %s failed: ret = %d\n", sample->GetName(), ret );
        }
        else
        {
            printf( "Deinit %s OK\n", sample->GetName() );
        }
    }

    for ( int i = (int) samples.size() - 1; i >= 0; i-- )
    {
        auto sample = samples[i];
        delete sample;
    }

    return 0;
}