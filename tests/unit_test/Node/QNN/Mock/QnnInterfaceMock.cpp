// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
#include "QnnInterfaceMock.hpp"

typedef Qnn_ErrorHandle_t ( *QnnInterfaceGetProvidersFn_t )( const QnnInterface_t ***providerList,
                                                             uint32_t *numProviders );

static void *s_hDll = nullptr;
static QnnInterfaceGetProvidersFn_t qnnInterfaceGetProvidersFn = nullptr;

class QNNClientLoader
{

public:
    QNNClientLoader()
    {
        std::string backendLibPath = "lib/libQnnHtp.so";
        const char *envValue = getenv( "QNN_BACKEND_LIB_PATH" );
        if ( nullptr != envValue )
        {
            backendLibPath = envValue;
        }
        s_hDll = dlopen( backendLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL );
        if ( nullptr == s_hDll )
        {
            printf( "Failed to load %s: %s\n", backendLibPath.c_str(), dlerror() );
        }
        else
        {
            printf( "Successfully loaded %s\n", backendLibPath.c_str() );
        }

        qnnInterfaceGetProvidersFn =
                (QnnInterfaceGetProvidersFn_t) dlsym( s_hDll, "QnnInterface_getProviders" );
        const char *error = dlerror();
        if ( error != nullptr )
        {
            printf( "Failed to load symbol QnnInterface_getProviders: %s\n", error );
            qnnInterfaceGetProvidersFn = nullptr;
        }
        else
        {
            printf( "Successfully loaded symbol QnnInterface_getProviders\n" );
        }
    }

    ~QNNClientLoader()
    {
        if ( nullptr != s_hDll )
        {
            dlclose( s_hDll );
            printf( "Library unloaded.\n" );
        }
    }
};

static QNNClientLoader s_qnnclientLoader;


static MockControlParam_t s_MockParams[MOCK_API_MAX];

extern "C" void MockApi_Control( MockAPI_ID_e apiId, MockAPI_Action_e action, void *param )
{
    if ( apiId < MOCK_API_MAX )
    {
        s_MockParams[apiId].action = action;
        s_MockParams[apiId].param = param;
    }
}

static QnnInterface_t s_RealInterface;

static QnnInterface_t s_mockInterface;

static const QnnInterface_t *s_qnnInterfaceHolder[32];

static QnnDevice_PlatformInfo_t s_qnnDevicePlatformInfo;

Qnn_ErrorHandle_t QnnLog_CreateFn( QnnLog_Callback_t callback, QnnLog_Level_t maxLogLevel,
                                   Qnn_LogHandle_t *logger )
{
    printf( "call real QNN logCreate\n" );
    Qnn_ErrorHandle_t ret =
            s_RealInterface.QNN_INTERFACE_VER_NAME.logCreate( callback, maxLogLevel, logger );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_LOG_CREATE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_LOG_CREATE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_LOG_CREATE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_LOG_CREATE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnBackend_CreateFn( Qnn_LogHandle_t logger, const QnnBackend_Config_t **config,
                                       Qnn_BackendHandle_t *backend )
{
    printf( "call real QNN backendCreate\n" );
    Qnn_ErrorHandle_t ret =
            s_RealInterface.QNN_INTERFACE_VER_NAME.backendCreate( logger, config, backend );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_BACKEND_CREATE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_BACKEND_CREATE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_BACKEND_CREATE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_BACKEND_CREATE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnBackend_GetApiVersionFn( Qnn_ApiVersion_t *pVersion )
{
    printf( "call real QNN backendGetApiVersion\n" );
    Qnn_ErrorHandle_t ret = s_RealInterface.QNN_INTERFACE_VER_NAME.backendGetApiVersion( pVersion );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_BACKEND_GET_API_VERSION].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_BACKEND_GET_API_VERSION].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_BACKEND_GET_API_VERSION]
                               .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_BACKEND_GET_API_VERSION].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnDevice_GetPlatformInfoFn( Qnn_LogHandle_t logger,
                                               const QnnDevice_PlatformInfo_t **platformInfo )
{
    printf( "call real QNN deviceGetPlatformInfo\n" );
    Qnn_ErrorHandle_t ret =
            s_RealInterface.QNN_INTERFACE_VER_NAME.deviceGetPlatformInfo( logger, platformInfo );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_DEVICE_GET_PLATFORM_INFO].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_DEVICE_GET_PLATFORM_INFO].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_DEVICE_GET_PLATFORM_INFO]
                               .param;
                break;
            case MOCK_CONTROL_API_OUT_PARAM0_SET_PLATFORM_INFO_VERSION_INVALID:
                if ( QNN_SUCCESS == ret )
                {
                    s_qnnDevicePlatformInfo = **platformInfo;
                    *platformInfo = &s_qnnDevicePlatformInfo;
                    s_qnnDevicePlatformInfo.version = QNN_DEVICE_PLATFORM_INFO_VERSION_UNDEFINED;
                }
                break;
            case MOCK_CONTROL_API_OUT_PARAM0_SET_PLATFORM_INFO_NUM_DEVICE_0:
                if ( QNN_SUCCESS == ret )
                {
                    s_qnnDevicePlatformInfo = **platformInfo;
                    *platformInfo = &s_qnnDevicePlatformInfo;
                    s_qnnDevicePlatformInfo.v1.numHwDevices = 0;
                }
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_DEVICE_GET_PLATFORM_INFO].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnDevice_CreateFn( Qnn_LogHandle_t logger, const QnnDevice_Config_t **config,
                                      Qnn_DeviceHandle_t *device )
{
    printf( "call real QNN deviceCreate\n" );
    Qnn_ErrorHandle_t ret =
            s_RealInterface.QNN_INTERFACE_VER_NAME.deviceCreate( logger, config, device );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_DEVICE_CREATE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_DEVICE_CREATE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_DEVICE_CREATE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_DEVICE_CREATE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnInterface_getProviders( const QnnInterface_t ***providerList,
                                             uint32_t *numProviders )
{
    Qnn_ErrorHandle_t ret;
    (void) s_qnnclientLoader;

    if ( nullptr != qnnInterfaceGetProvidersFn )
    {
        printf( "call real QNN qnnInterfaceGetProvidersFn\n" );
        ret = qnnInterfaceGetProvidersFn( providerList, numProviders );
        if ( QNN_SUCCESS == ret )
        {
            printf( "numProviders = %u\n", *numProviders );
            for ( size_t pIdx = 0; pIdx < *numProviders; pIdx++ )
            {
                printf( "Found QNN interface with version %u.%u.%u\n",
                        ( *providerList )[pIdx]->apiVersion.coreApiVersion.major,
                        ( *providerList )[pIdx]->apiVersion.coreApiVersion.minor,
                        ( *providerList )[pIdx]->apiVersion.coreApiVersion.patch );
                s_qnnInterfaceHolder[pIdx] = ( *providerList )[pIdx];
            }
            s_RealInterface = *( ( *providerList )[0] );
            s_mockInterface = s_RealInterface;
            s_mockInterface.QNN_INTERFACE_VER_NAME.logCreate = QnnLog_CreateFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.backendCreate = QnnBackend_CreateFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.backendGetApiVersion =
                    QnnBackend_GetApiVersionFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.deviceGetPlatformInfo =
                    QnnDevice_GetPlatformInfoFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.deviceCreate = QnnDevice_CreateFn;
            s_qnnInterfaceHolder[0] = &s_mockInterface;
            *providerList = s_qnnInterfaceHolder;
        }
    }
    else
    {
        printf( "QnnInterface_getProviders function not loaded\n." );
        ret = QNN_MIN_ERROR_COMMON;
    }

    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_GET_INTERFACE_PROVIDER].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_GET_INTERFACE_PROVIDER].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_GET_INTERFACE_PROVIDER]
                               .param;
                break;
            case MOCK_CONTROL_API_OUT_PARAM0:
                *providerList = *(const QnnInterface_t ***)
                                         s_MockParams[MOCK_API_QNN_GET_INTERFACE_PROVIDER]
                                                 .param;
                break;
            case MOCK_CONTROL_API_OUT_PARAM1:
                *numProviders =
                        *(uint32_t *) s_MockParams[MOCK_API_QNN_GET_INTERFACE_PROVIDER].param;
                break;
            case MOCK_CONTROL_API_OUT_PARAM0_DECREASE_MAJOR_VERSION:
                if ( QNN_SUCCESS == ret )
                {
                    if ( s_mockInterface.apiVersion.coreApiVersion.major > 0 )
                    {
                        s_mockInterface.apiVersion.coreApiVersion.major -= 1;
                    }
                }
                break;
            case MOCK_CONTROL_API_OUT_PARAM0_DECREASE_MINOR_VERSION:
                if ( QNN_SUCCESS == ret )
                {
                    if ( s_mockInterface.apiVersion.coreApiVersion.minor > 0 )
                    {
                        s_mockInterface.apiVersion.coreApiVersion.minor -= 1;
                    }
                }
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_GET_INTERFACE_PROVIDER].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }

    return ret;
}