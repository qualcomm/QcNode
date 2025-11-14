// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QNN_ADD_MODEL_MOCK_HPP
#define QNN_ADD_MODEL_MOCK_HPP

#include <dlfcn.h>
#include <libgen.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum
{
    MOCK_ADD_MODEL_CONTROL_API_NONE,
    MOCK_ADD_MODEL_CONTROL_API_RETURN,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM0,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM1,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM2,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM3,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM5,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM6,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM7,
    MOCK_ADD_MODEL_CONTROL_API_OUT_PARAM8,
} MockAddModelAPI_Action_e;

typedef enum
{
    MOCK_ADD_MODEL_API_COMPOSE_GRAPH,
    MOCK_ADD_MODEL_API_MAX
} MockAddModelAPI_ID_e;

typedef struct
{
    MockAddModelAPI_Action_e action;
    void *param; /* a pointer to a control parameter, how to use it
                  * depends on the code coverage test */
} MockAddModelControlParam_t;

typedef void ( *MockAddModelApi_ControlFnc_t )( MockAddModelAPI_ID_e apiId,
                                                MockAddModelAPI_Action_e action, void *param );

static inline MockAddModelApi_ControlFnc_t MockAddModel_GetControlFnc( std::string librayPath )
{
    MockAddModelApi_ControlFnc_t fnc = nullptr;
    void *hDll = dlopen( librayPath.c_str(), RTLD_NOW | RTLD_GLOBAL );
    if ( nullptr == hDll )
    {
        printf( "Failed to load %s: %s\n", librayPath.c_str(), dlerror() );
    }
    else
    {
        printf( "Successfully loaded %s\n", librayPath.c_str() );
    }

    fnc = (MockAddModelApi_ControlFnc_t) dlsym( hDll, "MockAddModelApi_ControlFnc" );
    const char *error = dlerror();
    if ( error != nullptr )
    {
        printf( "Failed to load symbol MockAddModelApi_ControlFnc: %s\n", error );
        fnc = nullptr;
    }
    else
    {
        printf( "Successfully loaded symbol MockApi_Control\n" );
    }

    return fnc;
}

#endif /* QNN_ADD_MODEL_MOCK_HPP */
