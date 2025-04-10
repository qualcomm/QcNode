// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#define CAMERA_UNIT_TEST

#include "ridehal/common/Types.hpp"
#include "ridehal/component/Camera.hpp"
#include "ridehal/sample/SampleIF.hpp"
#include "gtest/gtest.h"
#include <stdio.h>
#include <string>
#include <unistd.h>


using namespace ridehal;
using namespace ridehal::common;
using namespace ridehal::component;
using namespace ridehal::sample;

#define RUNTIME_SECOND ( 3 )
#define BUFFFER_COUNT ( 5 )

typedef struct
{
    std::string name;
    SampleConfig_t config;
    Camera_Config_t camConfig;
    std::string type;
} PipelineConfig_t;

static std::vector<PipelineConfig_t> s_pieplineConfigs;


class CameraConfigParser : public SampleIF
{
public:
    CameraConfigParser() { RIDEHAL_LOGGER_INIT( "CAM_CFG_PARSER", LOGGER_LEVEL_INFO ); }

    RideHalError_e ParseConfig( SampleConfig_t &config, Camera_Config_t &camConfig )
    {
        RideHalError_e ret = RIDEHAL_ERROR_NONE;
        camConfig.numStream = Get( config, "number", 1 );
        camConfig.inputId = Get( config, "input_id", -1 );
        if ( -1 == camConfig.inputId )
        {
            RIDEHAL_ERROR( "invalid input id = %d", camConfig.inputId );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        camConfig.srcId = Get( config, "src_id", 0 );
        camConfig.inputMode = Get( config, "input_mode", 0 );

        camConfig.bRequestMode = Get( config, "request_mode", false );
        camConfig.bAllocator = true;
        camConfig.ispUserCase = Get( config, "isp_use_case", 3 );

        camConfig.clientId = Get( config, "client_id", 0u );
        camConfig.bPrimary = Get( config, "is_primary", false );

        for ( uint32_t i = 0; i < camConfig.numStream; i++ )
        {
            std::string suffix = "";
            if ( i > 0 )
            {
                suffix = std::to_string( i );
            }
            camConfig.streamConfig[i].width = Get( config, "width" + suffix, 0 );
            if ( 0 == camConfig.streamConfig[i].width )
            {
                RIDEHAL_ERROR( "invalid width for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            camConfig.streamConfig[i].height = Get( config, "height" + suffix, 0 );
            if ( 0 == camConfig.streamConfig[i].height )
            {
                RIDEHAL_ERROR( "invalid height for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            camConfig.streamConfig[i].format =
                    Get( config, "format" + suffix, RIDEHAL_IMAGE_FORMAT_NV12 );
            if ( RIDEHAL_IMAGE_FORMAT_MAX == camConfig.streamConfig[i].format )
            {
                RIDEHAL_ERROR( "invalid format for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            camConfig.streamConfig[i].bufCnt = Get( config, "pool_size" + suffix, 4 );
            if ( 0 == camConfig.streamConfig[i].bufCnt )
            {
                RIDEHAL_ERROR( "invalid pool_size for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            camConfig.streamConfig[i].submitRequestPattern =
                    Get( config, "submit_request_pattern" + suffix, 0 );
            if ( camConfig.streamConfig[i].submitRequestPattern > 10 )
            {
                RIDEHAL_ERROR( "invalid submit_request_pattern for stream %u", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            camConfig.streamConfig[i].streamId = Get( config, "stream_id" + suffix, i );
        }

        camConfig.camFrameDropPattern = Get( config, "frame_drop_patten", (uint32_t) 0 );
        camConfig.camFrameDropPeriod = Get( config, "frame_drop_period", (uint32_t) 0 );

        camConfig.opMode = Get( config, "op_mode", (uint32_t) QCARCAM_OPMODE_OFFLINE_ISP );

        return ret;
    }

    RideHalError_e Init( std::string name, SampleConfig_t &config )
    {
        return RIDEHAL_ERROR_UNSUPPORTED;
    }
    RideHalError_e Start() { return RIDEHAL_ERROR_UNSUPPORTED; }
    RideHalError_e Stop() { return RIDEHAL_ERROR_UNSUPPORTED; }
    RideHalError_e Deinit() { return RIDEHAL_ERROR_UNSUPPORTED; }
};

static const char *pDumpPath = "/tmp/camera_frame.bin";

static std::FILE *g_Dumpfile = nullptr;

static int DumpFrame( CameraFrame_t *pFrame, const char *path )
{
    int ret = 0;

    if ( nullptr == g_Dumpfile )
    {
        g_Dumpfile = std::fopen( path, "wb+" );
    }

    if ( nullptr != g_Dumpfile )
    {
        int frameSize = pFrame->sharedBuffer.size;
        void *buffer = pFrame->sharedBuffer.data();

        ret = std::fwrite( buffer, frameSize, 1, g_Dumpfile );
    }

    return ret;
}

static uint32_t s_FrameCount = 0;

static void FrameCallBack( CameraFrame_t *pFrame, void *pPrivData )
{
    RideHalError_e ret;

    Camera *pCamera = (Camera *) pPrivData;

#ifdef DUMPFRAME
    uint32_t writeBytes = DumpFrame( pFrame, pDumpPath );
#endif
    if ( RIDEHAL_COMPONENT_STATE_RUNNING == pCamera->GetState() )
    {
        ret = pCamera->ReleaseFrame( pFrame );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    s_FrameCount++;
}

static void FrameCallBack_RequestMode( CameraFrame_t *pFrame, void *pPrivData )
{
    RideHalError_e ret;

    Camera *pCamera = (Camera *) pPrivData;

#ifdef DUMPFRAME
    uint32_t writeBytes = DumpFrame( pFrame, pDumpPath );
#endif

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == pCamera->GetState() )
    {
        ret = pCamera->RequestFrame( pFrame );
        if ( RIDEHAL_ERROR_NONE != ret )
        { /* TODO: research why request mode for multi-stream the submit request may failed once
             after restart */
            printf( "  WARN: submit request failed bufferlistId: %u, bufferIdx: %u ",
                    pFrame->streamId, pFrame->frameIndex );
        }
    }

    s_FrameCount++;
}

static void EventCallBack( const uint32_t eventId, const void *pPayload, void *pPrivData )
{
    RIDEHAL_LOG_ERROR( "Received event: %d, pPrivData:%p\n", eventId, pPrivData );
}

TEST( Camera, Query_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;
    CameraInputs_t camInputs;

    ret = pCamera->GetInputsInfo( &camInputs );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    printf( "Number of camera connected: %d\n", camInputs.numInputs );
    for ( uint32_t i = 0; i < camInputs.numInputs; i++ )
    {
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[i];
        printf( "camera input_id %u, resolution %ux%u\n", camInputs.pCameraInputs[i].inputId,
                pCamInputModes->pModes[0].sources[0].width,
                pCamInputModes->pModes[0].sources[0].height );
    }

    delete pCamera;
}

TEST( Camera, SANITY_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    for ( auto &pipeline : s_pieplineConfigs )
    {
        std::string name = pipeline.name;
        std::string type = pipeline.type;
        Camera_Config_t camConfig = pipeline.camConfig;
        printf( "testing camera %s input_id %u, resolution %ux%u\n", name.c_str(),
                camConfig.inputId, camConfig.streamConfig[0].width,
                camConfig.streamConfig[0].height );

        /* for sanity, go with non request mode and only open 1 stream */
        camConfig.bRequestMode = false;
        camConfig.numStream = 1;
        ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames after restart\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        /* no frame after stop */
        s_FrameCount = 0;
        sleep( 1 );
        ASSERT_EQ( s_FrameCount, 0 );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete pCamera;
}


TEST( Camera, SetBuffer_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    for ( auto &pipeline : s_pieplineConfigs )
    {
        std::string name = pipeline.name;
        std::string type = pipeline.type;
        Camera_Config_t camConfig = pipeline.camConfig;

        printf( "testing camera %s input_id %u, resolution %ux%u\n", name.c_str(),
                camConfig.inputId, camConfig.streamConfig[0].width,
                camConfig.streamConfig[0].height );

        camConfig.bAllocator = false;
        camConfig.numStream = 1;
        camConfig.streamConfig[0].bufCnt = BUFFFER_COUNT;
        RideHal_SharedBuffer_t *pSharedBuffer = new RideHal_SharedBuffer_t[BUFFFER_COUNT];

        for ( int i = 0; i < BUFFFER_COUNT; i++ )
        {
            ret = pSharedBuffer[i].Allocate( camConfig.streamConfig[0].width,
                                             camConfig.streamConfig[0].height,
                                             camConfig.streamConfig[0].format );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT, 0 );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT, 0 );
        ASSERT_EQ( RIDEHAL_ERROR_ALREADY, ret );

        if ( false == camConfig.bRequestMode )
        {
            ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
        else
        {
            ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                             (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        /* no frame after stop */
        s_FrameCount = 0;
        sleep( 1 );
        ASSERT_EQ( s_FrameCount, 0 );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        for ( int i = 0; i < BUFFFER_COUNT; i++ )
        {
            ret = pSharedBuffer[i].Free();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        delete[] pSharedBuffer;
    }

    delete pCamera;
}

TEST( Camera, GetBuffer_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    std::string name = s_pieplineConfigs[0].name;
    std::string type = s_pieplineConfigs[0].type;
    Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;

    printf( "testing camera %s input_id %u, resolution %ux%u\n", name.c_str(), camConfig.inputId,
            camConfig.streamConfig[0].width, camConfig.streamConfig[0].height );

    camConfig.bAllocator = false;
    camConfig.numStream = 1;
    camConfig.streamConfig[0].bufCnt = BUFFFER_COUNT;
    RideHal_SharedBuffer_t *pSharedBuffer = new RideHal_SharedBuffer_t[BUFFFER_COUNT];
    RideHal_SharedBuffer_t *pSharedBufferGet = new RideHal_SharedBuffer_t[BUFFFER_COUNT];

    for ( int i = 0; i < BUFFFER_COUNT; i++ )
    {
        ret = pSharedBuffer[i].Allocate( camConfig.streamConfig[0].width,
                                         camConfig.streamConfig[0].height,
                                         camConfig.streamConfig[0].format );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = pCamera->GetBuffers( pSharedBufferGet, BUFFFER_COUNT, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = pCamera->GetBuffers( pSharedBufferGet, BUFFFER_COUNT, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

    ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT, 100 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_ALREADY, ret );

    ret = pCamera->GetBuffers( pSharedBufferGet, BUFFFER_COUNT, 100 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = pCamera->GetBuffers( pSharedBufferGet, BUFFFER_COUNT * 2, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = pCamera->GetBuffers( nullptr, BUFFFER_COUNT, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

    ret = pCamera->GetBuffers( pSharedBufferGet, BUFFFER_COUNT, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( false == camConfig.bRequestMode )
    {
        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    else
    {
        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    s_FrameCount = 0;
    ret = pCamera->Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = pCamera->GetBuffers( pSharedBufferGet, BUFFFER_COUNT, 0 );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    // sanity test to run few seconds and then stop
    sleep( RUNTIME_SECOND );

    ret = pCamera->Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    printf( "  Get %u frames\n", s_FrameCount );
    ASSERT_GT( s_FrameCount, 0 );

    /* no frame after stop */
    s_FrameCount = 0;
    sleep( 1 );
    ASSERT_EQ( s_FrameCount, 0 );

    ret = pCamera->Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    for ( int i = 0; i < BUFFFER_COUNT; i++ )
    {
        ret = pSharedBuffer[i].Free();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete[] pSharedBuffer;
    delete[] pSharedBufferGet;

    delete pCamera;
}

TEST( Camera, NegtiveUnImportBuffers )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    std::string name = s_pieplineConfigs[0].name;
    std::string type = s_pieplineConfigs[0].type;
    Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;

    printf( "testing camera %s input_id %u, resolution %ux%u\n", name.c_str(), camConfig.inputId,
            camConfig.streamConfig[0].width, camConfig.streamConfig[0].height );

    camConfig.bAllocator = true;
    camConfig.numStream = 1;
    camConfig.streamConfig[0].bufCnt = BUFFFER_COUNT;

    ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    if ( false == camConfig.bRequestMode )
    {
        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }
    else
    {
        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    ret = pCamera->Start();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = pCamera->Stop();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    ret = pCamera->UnImportBuffers();
    ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

    ret = pCamera->Deinit();
    ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

    delete pCamera;
}


TEST( Camera, PauseResume_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    for ( auto &pipeline : s_pieplineConfigs )
    {
        for ( int i = 0; i < 2; i++ )
        {
            std::string name = pipeline.name;
            std::string type = pipeline.type;
            Camera_Config_t camConfig = pipeline.camConfig;
            /* TODO: see issue for multiple stream when do pause and resume */
            camConfig.numStream = 1;
            if ( 0 == i )
            {
                camConfig.bRequestMode = true;
            }
            else
            {
                camConfig.bRequestMode = false;
            }

            printf( "testing camera %s input_id %u, resolution %ux%u\n", name.c_str(),
                    camConfig.inputId, camConfig.streamConfig[0].width,
                    camConfig.streamConfig[0].height );

            ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

            if ( false == camConfig.bRequestMode )
            {
                ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
                ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
            }
            else
            {
                ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                                 (void *) pCamera );
                ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
            }

            s_FrameCount = 0;
            ret = pCamera->Start();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

            // sanity test to run few seconds and then stop
            sleep( RUNTIME_SECOND );

            ret = pCamera->Pause();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

            printf( "  Get %u frames\n", s_FrameCount );
            ASSERT_GT( s_FrameCount, 0 );

            /* no frame after pause */
            s_FrameCount = 0;
            sleep( 1 );
            ASSERT_EQ( s_FrameCount, 0 );

            s_FrameCount = 0;
            ret = pCamera->Resume();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

            sleep( RUNTIME_SECOND );

            ret = pCamera->Pause();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

            printf( "  Get %u frames after resume\n", s_FrameCount );
            ASSERT_GT( s_FrameCount, 0 );

            sleep( 1 );

            ret = pCamera->Resume();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

            sleep( 1 );

            ret = pCamera->Stop();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

            ret = pCamera->Deinit();
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
    }

    delete pCamera;
}


TEST( Camera, RequestMode_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    for ( auto &pipeline : s_pieplineConfigs )
    {
        std::string name = pipeline.name;
        std::string type = pipeline.type;
        Camera_Config_t camConfig = pipeline.camConfig;

        printf( "testing camera %s input_id %u, resolution %ux%u\n", name.c_str(),
                camConfig.inputId, camConfig.streamConfig[0].width,
                camConfig.streamConfig[0].height );

        camConfig.bRequestMode = true;

        ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        /* no frame after stop */
        s_FrameCount = 0;
        sleep( 1 );
        ASSERT_EQ( s_FrameCount, 0 );

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete pCamera;
}

TEST( Camera, Coverage_QcarCam )
{
    RideHalError_e ret;
    char componentName[20] = "Camera";

    /* Negative - config is nullptr */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Init( componentName, nullptr, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
        delete pCamera;
    }


    /* Negative - camConfig.bufCnt is 0 */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        QCarCamInputModes_t *pCamInputModes = &camInputs.pCamInputModes[0];

        char componentName[20] = "Camera";
        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.streamConfig[0].bufCnt = 0;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - start in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - stop in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - deinit in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - pause in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Pause();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - resume in invalid state */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( &camInputs );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Resume();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );
        delete pCamera;
    }

    /* Negative - GetInputsInfo with null input */
    {
        Camera *pCamera = new Camera;
        CameraInputs_t camInputs;

        ret = pCamera->GetInputsInfo( nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        delete pCamera;
    }

    /* Negative - WRONG FORMAT */
    for ( auto &pipeline : s_pieplineConfigs )
    {
        Camera *pCamera = new Camera;
        std::string name = pipeline.name;
        std::string type = pipeline.type;
        Camera_Config_t camConfig = pipeline.camConfig;
        printf( "testing camera %s with invalid format \n", name.c_str() );
        if ( ( camConfig.streamConfig[0].format >= RIDEHAL_IMAGE_FORMAT_UYVY ) &&
             ( camConfig.streamConfig[0].format <= RIDEHAL_IMAGE_FORMAT_NV12_UBWC ) )
        {
            camConfig.streamConfig[0].format = RIDEHAL_IMAGE_FORMAT_RGB888;
        }
        else
        {
            camConfig.streamConfig[0].format = RIDEHAL_IMAGE_FORMAT_UYVY;
        }
        ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        if ( false == camConfig.bRequestMode )
        {
            ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
        else
        {
            ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                             (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Positive - All valid camera format */
    for ( int i = 0; i < (int) RIDEHAL_IMAGE_FORMAT_MAX; i++ )
    {
        Camera *pCamera = new Camera;
        std::string name = s_pieplineConfigs[0].name;
        std::string type = s_pieplineConfigs[0].type;
        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;

        camConfig.streamConfig[0].format = (RideHal_ImageFormat_e) i;

        ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - SetBuffer with nullptr */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bAllocator = false;
        RideHal_SharedBuffer_t *pSharedBuffer = nullptr;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT, 0 );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        delete pCamera;
    }

    /* Negative - SetBuffer in invalid state */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        RideHal_SharedBuffer_t *pSharedBuffer = new RideHal_SharedBuffer_t[BUFFFER_COUNT];
        ret = pCamera->SetBuffers( pSharedBuffer, BUFFFER_COUNT, 0 );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        delete[] pSharedBuffer;
        delete pCamera;
    }

    /* Negative - RequestFrame with nullptr input */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bRequestMode = true;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RequestFrame( nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - RequestFrame in invalid state */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bAllocator = true;
        camConfig.bRequestMode = true;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        CameraFrame_t Frame;
        ret = pCamera->RequestFrame( &Frame );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - RequestFrame in non request mode */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bRequestMode = false;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        CameraFrame_t Frame;
        ret = pCamera->RequestFrame( &Frame );
        ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - unsupport color format */
    {
        Camera *pCamera = new Camera;
        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.streamConfig[0].format = RIDEHAL_IMAGE_FORMAT_MAX;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - Init twice */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - ReleaseFrame with nullptr input */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bRequestMode = false;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->ReleaseFrame( nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - ReleaseFrame in invalid state */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bRequestMode = false;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        CameraFrame_t Frame;
        ret = pCamera->ReleaseFrame( &Frame );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        Frame.streamId = 100;
        Frame.frameIndex = 30;
        ret = pCamera->ReleaseFrame( &Frame );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* Negative - ReleaseFrame in request mode */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bRequestMode = true;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        CameraFrame_t Frame;
        ret = pCamera->ReleaseFrame( &Frame );
        ASSERT_EQ( RIDEHAL_ERROR_OUT_OF_BOUND, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }

    /* 2 camera instances */
    {
        Camera *pCamera = new Camera;
        Camera *pCamera1 = new Camera;
        delete pCamera;
        delete pCamera1;
    }

    /* Negative - invalid number of streams */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.numStream = 0;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        camConfig.numStream = 128;
        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        delete pCamera;
    }

    /* Negative - multi-client, invalid client id */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.numStream = 1;
        camConfig.bPrimary = true;
        camConfig.clientId = 0;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        delete pCamera;
    }

    /* Negative - multi-stream, not valid reference stream */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.numStream = 2;
        camConfig.bRequestMode = true;
        camConfig.streamConfig[0].submitRequestPattern = 2;
        camConfig.streamConfig[1] = camConfig.streamConfig[0];

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        delete pCamera;
    }

    /* multi-client: invalid client id */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.numStream = 1;
        camConfig.bPrimary = true;
        camConfig.clientId = 1234;
        camConfig.bRequestMode = true;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        delete pCamera;
    }

    /* invalid isp use case */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.ispUserCase = 10079;
        camConfig.bRequestMode = false;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_FAIL, ret );

        (void) pCamera->Deinit();

        delete pCamera;
    }

    // Negative, invalid input id
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.inputId = 100;
        camConfig.bAllocator = false;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        delete pCamera;
    }

    // Negative, subscriber but without primary started
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.clientId = 3;
        camConfig.bPrimary = false;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_NE( RIDEHAL_ERROR_NONE, ret );

        delete pCamera;
    }

    /* Negative - invalid callback or not regiter and start */
    {
        RideHalError_e ret;
        Camera *pCamera = new Camera;

        Camera_Config_t camConfig = s_pieplineConfigs[0].camConfig;
        camConfig.bRequestMode = true;

        ret = pCamera->Init( componentName, &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = pCamera->RegisterCallback( nullptr, EventCallBack, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, nullptr, (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );
        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack, nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                         (void *) pCamera );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_STATE, ret );

        ret = pCamera->RequestFrame( nullptr );
        ASSERT_EQ( RIDEHAL_ERROR_BAD_ARGUMENTS, ret );

        (void) pCamera->Stop();
        (void) pCamera->Deinit();
        delete pCamera;
    }
}

TEST( Camera, MultiStreamSameId_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    for ( auto &pipeline : s_pieplineConfigs )
    {
        std::string name = pipeline.name;
        std::string type = pipeline.type;
        Camera_Config_t camConfig = pipeline.camConfig;
        if ( 1 == camConfig.numStream )
        {
            continue;
        }

        camConfig.bRequestMode = true;

        printf( "testing camera %s input_id %u, resolution %ux%u\n", name.c_str(),
                camConfig.inputId, camConfig.streamConfig[0].width,
                camConfig.streamConfig[0].height );

        ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        if ( false == camConfig.bRequestMode )
        {
            ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
        else
        {
            ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                             (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        /* no frame after stop */
        s_FrameCount = 0;
        sleep( 1 );
        ASSERT_EQ( s_FrameCount, 0 );

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete pCamera;
}

TEST( Camera, DropPattern_QcarCam )
{
    RideHalError_e ret;
    Camera *pCamera = new Camera;

    for ( auto &pipeline : s_pieplineConfigs )
    {
        std::string name = pipeline.name;
        std::string type = pipeline.type;
        Camera_Config_t camConfig = pipeline.camConfig;
        if ( camConfig.camFrameDropPattern == 0 )
        {
            continue;
        }

        ret = pCamera->Init( name.c_str(), &camConfig, LOGGER_LEVEL_VERBOSE );
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        if ( false == camConfig.bRequestMode )
        {
            ret = pCamera->RegisterCallback( FrameCallBack, EventCallBack, (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }
        else
        {
            ret = pCamera->RegisterCallback( FrameCallBack_RequestMode, EventCallBack,
                                             (void *) pCamera );
            ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
        }

        s_FrameCount = 0;
        ret = pCamera->Start();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        // sanity test to run few seconds and then stop
        sleep( RUNTIME_SECOND );

        ret = pCamera->Stop();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );

        printf( "  Get %u frames after resume\n", s_FrameCount );
        ASSERT_GT( s_FrameCount, 0 );

        ret = pCamera->Deinit();
        ASSERT_EQ( RIDEHAL_ERROR_NONE, ret );
    }

    delete pCamera;
}

#ifndef GTEST_RIDEHAL
int main( int argc, char **argv )
{
    std::string key;
    int opt;
    while ( ( opt = getopt( argc, argv, "n:t:k:v:" ) ) != -1 )
    {
        switch ( opt )
        {
            case 'n':
            {
                PipelineConfig_t config;
                config.name = optarg;
                s_pieplineConfigs.push_back( config );
                break;
            }
            case 't':
            {
                PipelineConfig_t &config = s_pieplineConfigs.back();
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
                PipelineConfig_t &config = s_pieplineConfigs.back();
                config.config[key] = optarg;
                break;
            }
            default:
                break;
        }
    }
    CameraConfigParser parser;
    for ( auto &pieplie : s_pieplineConfigs )
    {
        RideHalError_e ret = parser.ParseConfig( pieplie.config, pieplie.camConfig );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            printf( "Invalid config for camera %s\n", pieplie.name.c_str() );
            return -1;
        }
    }

    if ( 0 == s_pieplineConfigs.size() )
    {
        printf( "At least need 1 camera config through option -n/-t/-k/-v\n" );
        return -1;
    }

    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
