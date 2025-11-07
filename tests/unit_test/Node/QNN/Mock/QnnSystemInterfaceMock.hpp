// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QNN_SYSTEM_INTERFACE_MOCK_HPP
#define QNN_SYSTEM_INTERFACE_MOCK_HPP

#include "HTP/QnnHtpCommon.h"
#include "HTP/QnnHtpDevice.h"
#include "HTP/QnnHtpMem.h"
#include "HTP/QnnHtpProfile.h"
#include "QnnInterface.h"
#include "QnnProfile.h"
#include "QnnSdkBuildId.h"
#include "QnnTypeMacros.hpp"
#include "QnnWrapperUtils.hpp"
#include "System/QnnSystemInterface.h"

#include <dlfcn.h>
#include <libgen.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum
{
    MOCK_QNN_SYSTEM_CONTROL_API_NONE,
    MOCK_QNN_SYSTEM_CONTROL_API_RETURN,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM1,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM2,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM3,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM5,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM6,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM7,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM8,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_DECREASE_MAJOR_VERSION,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_DECREASE_MINOR_VERSION,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_SET_SYSTEM_CONTEXT_CREATE_NULLPTR,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_SET_SYSTEM_CONTEXT_GET_BINARY_INFO_NULLPTR,
    MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_SET_SYSTEM_CONTEXT_FREE_NULLPTR,
} MockQnnSystemAPI_Action_e;

typedef enum
{
    MOCK_QNN_SYSTEM_API_QNN_GET_INTERFACE_PROVIDER,
    MOCK_QNN_SYSTEM_API_QNN_SYSTEM_CONTEXT_CREATE,
    MOCK_QNN_SYSTEM_API_MAX
} MockQnnSystemAPI_ID_e;

typedef struct
{
    MockQnnSystemAPI_Action_e action;
    void *param; /* a pointer to a control parameter, how to use it
                  * depends on the code coverage test */
} MockQnnSystemControlParam_t;

typedef void ( *MockQnnSystemApi_ControlFnc_t )( MockQnnSystemAPI_ID_e apiId,
                                                 MockQnnSystemAPI_Action_e action, void *param );

static inline MockQnnSystemApi_ControlFnc_t MockQnnSystem_GetControlFnc( std::string librayPath )
{
    MockQnnSystemApi_ControlFnc_t fnc = nullptr;
    void *hDll = dlopen( librayPath.c_str(), RTLD_NOW | RTLD_GLOBAL );
    if ( nullptr == hDll )
    {
        printf( "Failed to load %s: %s\n", librayPath.c_str(), dlerror() );
    }
    else
    {
        printf( "Successfully loaded %s\n", librayPath.c_str() );
    }

    fnc = (MockQnnSystemApi_ControlFnc_t) dlsym( hDll, "MockQnnSystemApi_ControlFnc" );
    const char *error = dlerror();
    if ( error != nullptr )
    {
        printf( "Failed to load symbol MockQnnSystemApi_ControlFnc: %s\n", error );
        fnc = nullptr;
    }
    else
    {
        printf( "Successfully loaded symbol MockApi_Control\n" );
    }

    return fnc;
}

#endif /* QNN_SYSTEM_INTERFACE_MOCK_HPP */
