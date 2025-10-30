// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QNN_INTERFACE_MOCK_HPP
#define QNN_INTERFACE_MOCK_HPP

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
    MOCK_CONTROL_API_NONE,
    MOCK_CONTROL_API_RETURN,
    MOCK_CONTROL_API_OUT_PARAM0,
    MOCK_CONTROL_API_OUT_PARAM1,
    MOCK_CONTROL_API_OUT_PARAM2,
    MOCK_CONTROL_API_OUT_PARAM3,
    MOCK_CONTROL_API_OUT_PARAM5,
    MOCK_CONTROL_API_OUT_PARAM6,
    MOCK_CONTROL_API_OUT_PARAM7,
    MOCK_CONTROL_API_OUT_PARAM8,
    MOCK_CONTROL_API_OUT_PARAM0_DECREASE_MAJOR_VERSION,
    MOCK_CONTROL_API_OUT_PARAM0_DECREASE_MINOR_VERSION,
    MOCK_CONTROL_API_OUT_PARAM0_SET_PLATFORM_INFO_VERSION_INVALID,
    MOCK_CONTROL_API_OUT_PARAM0_SET_PLATFORM_INFO_NUM_DEVICE_0,
} MockAPI_Action_e;

typedef enum
{
    MOCK_API_QNN_GET_INTERFACE_PROVIDER,
    MOCK_API_QNN_LOG_CREATE,
    MOCK_API_QNN_BACKEND_CREATE,
    MOCK_API_QNN_BACKEND_GET_API_VERSION,
    MOCK_API_QNN_DEVICE_GET_PLATFORM_INFO,
    MOCK_API_QNN_DEVICE_CREATE,
    MOCK_API_MAX
} MockAPI_ID_e;

typedef struct
{
    MockAPI_Action_e action;
    void *param; /* a pointer to a control parameter, how to use it
                  * depends on the code coverage test */
} MockControlParam_t;

typedef void ( *MockApi_ControlFnc_t )( MockAPI_ID_e apiId, MockAPI_Action_e action, void *param );

static inline MockApi_ControlFnc_t MockQnn_GetControlFnc( std::string librayPath )
{
    MockApi_ControlFnc_t fnc = nullptr;
    void *hDll = dlopen( librayPath.c_str(), RTLD_NOW | RTLD_GLOBAL );
    if ( nullptr == hDll )
    {
        printf( "Failed to load %s: %s\n", librayPath.c_str(), dlerror() );
    }
    else
    {
        printf( "Successfully loaded %s\n", librayPath.c_str() );
    }

    fnc = (MockApi_ControlFnc_t) dlsym( hDll, "MockApi_Control" );
    const char *error = dlerror();
    if ( error != nullptr )
    {
        printf( "Failed to load symbol MockApi_Control: %s\n", error );
        fnc = nullptr;
    }
    else
    {
        printf( "Successfully loaded symbol MockApi_Control\n" );
    }

    return fnc;
}

#endif /* QNN_INTERFACE_MOCK_HPP */
