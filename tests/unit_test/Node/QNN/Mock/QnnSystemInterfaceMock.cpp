// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QnnSystemInterfaceMock.hpp"

typedef Qnn_ErrorHandle_t ( *QnnSystemInterfaceGetProvidersFn_t )(
        const QnnSystemInterface_t ***providerList, uint32_t *numProviders );

static void *s_hDll = nullptr;
static QnnSystemInterfaceGetProvidersFn_t s_qnnSystemInterfaceGetProvidersFn = nullptr;

class QNNSystemClientLoader
{

public:
    QNNSystemClientLoader()
    {
        std::string systemLibPath = "lib/libQnnSystem.so";
        const char *envValue = getenv( "QNN_SYSTEM_LIB_PATH" );
        if ( nullptr != envValue )
        {
            systemLibPath = envValue;
        }
        s_hDll = dlopen( systemLibPath.c_str(), RTLD_NOW | RTLD_GLOBAL );
        if ( nullptr == s_hDll )
        {
            printf( "Failed to load %s: %s\n", systemLibPath.c_str(), dlerror() );
        }
        else
        {
            printf( "Successfully loaded %s\n", systemLibPath.c_str() );
        }

        s_qnnSystemInterfaceGetProvidersFn = (QnnSystemInterfaceGetProvidersFn_t) dlsym(
                s_hDll, "QnnSystemInterface_getProviders" );
        const char *error = dlerror();
        if ( error != nullptr )
        {
            printf( "Failed to load symbol QnnSystemInterface_getProviders: %s\n", error );
            s_qnnSystemInterfaceGetProvidersFn = nullptr;
        }
        else
        {
            printf( "Successfully loaded symbol QnnSystemInterface_getProviders\n" );
        }
    }

    ~QNNSystemClientLoader()
    {
        if ( nullptr != s_hDll )
        {
            dlclose( s_hDll );
            printf( "Library unloaded.\n" );
        }
    }
};

static QNNSystemClientLoader s_qnnclientLoader;

static MockQnnSystemControlParam_t s_MockParams[MOCK_QNN_SYSTEM_API_MAX];

extern "C" void MockQnnSystemApi_ControlFnc( MockQnnSystemAPI_ID_e apiId,
                                             MockQnnSystemAPI_Action_e action, void *param )
{
    if ( apiId < MOCK_QNN_SYSTEM_API_MAX )
    {
        s_MockParams[apiId].action = action;
        s_MockParams[apiId].param = param;
    }
}

static QnnSystemInterface_t s_RealInterface;

static QnnSystemInterface_t s_mockInterface;

static const QnnSystemInterface_t *s_qnnInterfaceHolder[32];


Qnn_ErrorHandle_t QnnSystemContext_Create( QnnSystemContext_Handle_t *sysCtxHandle )
{
    printf( "call real QnnSystemContext_Create\n" );
    Qnn_ErrorHandle_t ret =
            s_RealInterface.QNN_SYSTEM_INTERFACE_VER_NAME.systemContextCreate( sysCtxHandle );
    if ( MOCK_QNN_SYSTEM_CONTROL_API_NONE !=
         s_MockParams[MOCK_QNN_SYSTEM_API_QNN_SYSTEM_CONTEXT_CREATE].action )
    {
        switch ( s_MockParams[MOCK_QNN_SYSTEM_API_QNN_SYSTEM_CONTEXT_CREATE].action )
        {
            case MOCK_QNN_SYSTEM_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *)
                               s_MockParams[MOCK_QNN_SYSTEM_API_QNN_SYSTEM_CONTEXT_CREATE]
                                       .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_QNN_SYSTEM_API_QNN_SYSTEM_CONTEXT_CREATE].action =
                MOCK_QNN_SYSTEM_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnSystemInterface_getProviders( const QnnSystemInterface_t ***providerList,
                                                   uint32_t *numProviders )
{
    Qnn_ErrorHandle_t ret;
    (void) s_qnnclientLoader;

    if ( nullptr != s_qnnSystemInterfaceGetProvidersFn )
    {
        printf( "call real QNN qnnSystemInterfaceGetProvidersFn\n" );
        ret = s_qnnSystemInterfaceGetProvidersFn( providerList, numProviders );
        if ( QNN_SUCCESS == ret )
        {
            printf( "numProviders = %u\n", *numProviders );
            for ( size_t pIdx = 0; pIdx < *numProviders; pIdx++ )
            {
                printf( "Found QNN system interface with version %u.%u.%u\n",
                        ( *providerList )[pIdx]->systemApiVersion.major,
                        ( *providerList )[pIdx]->systemApiVersion.minor,
                        ( *providerList )[pIdx]->systemApiVersion.patch );
                s_qnnInterfaceHolder[pIdx] = ( *providerList )[pIdx];
            }
            s_RealInterface = *( ( *providerList )[0] );
            s_mockInterface = s_RealInterface;
            s_mockInterface.QNN_SYSTEM_INTERFACE_VER_NAME.systemContextCreate =
                    QnnSystemContext_Create;
            s_qnnInterfaceHolder[0] = &s_mockInterface;
            *providerList = s_qnnInterfaceHolder;
        }
    }
    else
    {
        printf( "QnnInterface_getProviders function not loaded\n." );
        ret = QNN_MIN_ERROR_COMMON;
    }

    if ( MOCK_QNN_SYSTEM_CONTROL_API_NONE !=
         s_MockParams[MOCK_QNN_SYSTEM_API_QNN_GET_INTERFACE_PROVIDER].action )
    {
        switch ( s_MockParams[MOCK_QNN_SYSTEM_API_QNN_GET_INTERFACE_PROVIDER].action )
        {
            case MOCK_QNN_SYSTEM_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *)
                               s_MockParams[MOCK_QNN_SYSTEM_API_QNN_GET_INTERFACE_PROVIDER]
                                       .param;
                break;
            case MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0:
                *providerList =
                        *(const QnnSystemInterface_t ***)
                                 s_MockParams[MOCK_QNN_SYSTEM_API_QNN_GET_INTERFACE_PROVIDER]
                                         .param;
                break;
            case MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM1:
                *numProviders =
                        *(uint32_t *) s_MockParams[MOCK_QNN_SYSTEM_API_QNN_GET_INTERFACE_PROVIDER]
                                 .param;
                break;
            case MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_DECREASE_MAJOR_VERSION:
                if ( QNN_SUCCESS == ret )
                {
                    if ( s_mockInterface.systemApiVersion.major > 0 )
                    {
                        s_mockInterface.systemApiVersion.major -= 1;
                    }
                }
                break;
            case MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_DECREASE_MINOR_VERSION:
                if ( QNN_SUCCESS == ret )
                {
                    if ( s_mockInterface.systemApiVersion.minor > 0 )
                    {
                        s_mockInterface.systemApiVersion.minor -= 1;
                    }
                }
                break;
            case MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_SET_SYSTEM_CONTEXT_CREATE_NULLPTR:
                if ( QNN_SUCCESS == ret )
                {
                    s_mockInterface.QNN_SYSTEM_INTERFACE_VER_NAME.systemContextCreate = nullptr;
                }
                break;
            case MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_SET_SYSTEM_CONTEXT_GET_BINARY_INFO_NULLPTR:
                if ( QNN_SUCCESS == ret )
                {
                    s_mockInterface.QNN_SYSTEM_INTERFACE_VER_NAME.systemContextGetBinaryInfo =
                            nullptr;
                }
                break;
            case MOCK_QNN_SYSTEM_CONTROL_API_OUT_PARAM0_SET_SYSTEM_CONTEXT_FREE_NULLPTR:
                if ( QNN_SUCCESS == ret )
                {
                    s_mockInterface.QNN_SYSTEM_INTERFACE_VER_NAME.systemContextFree = nullptr;
                }
                break;
            default:
                break;
        }
        s_MockParams[MOCK_QNN_SYSTEM_API_QNN_GET_INTERFACE_PROVIDER].action =
                MOCK_QNN_SYSTEM_CONTROL_API_NONE; /* consume it and invalide the control */
    }

    return ret;
}