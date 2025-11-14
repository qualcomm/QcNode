// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QnnInterfaceMock.hpp"

typedef Qnn_ErrorHandle_t ( *QnnInterfaceGetProvidersFn_t )( const QnnInterface_t ***providerList,
                                                             uint32_t *numProviders );

static void *s_hDll = nullptr;
static QnnInterfaceGetProvidersFn_t s_qnnInterfaceGetProvidersFn = nullptr;

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

        s_qnnInterfaceGetProvidersFn =
                (QnnInterfaceGetProvidersFn_t) dlsym( s_hDll, "QnnInterface_getProviders" );
        const char *error = dlerror();
        if ( error != nullptr )
        {
            printf( "Failed to load symbol QnnInterface_getProviders: %s\n", error );
            s_qnnInterfaceGetProvidersFn = nullptr;
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

static QnnInterface_t s_realInterface;

static QnnInterface_t s_mockInterface;

static QnnHtpDevice_Infrastructure_t s_realHtpInfras;
static QnnHtpDevice_Infrastructure_t s_mockHtpInfras;

static const QnnInterface_t *s_qnnInterfaceHolder[32];

static QnnDevice_PlatformInfo_t s_qnnDevicePlatformInfo;

Qnn_ErrorHandle_t QnnLog_CreateFn( QnnLog_Callback_t callback, QnnLog_Level_t maxLogLevel,
                                   Qnn_LogHandle_t *logger )
{
    printf( "call real QNN logCreate\n" );
    Qnn_ErrorHandle_t ret =
            s_realInterface.QNN_INTERFACE_VER_NAME.logCreate( callback, maxLogLevel, logger );
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
            s_realInterface.QNN_INTERFACE_VER_NAME.backendCreate( logger, config, backend );
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
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.backendGetApiVersion( pVersion );
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
            s_realInterface.QNN_INTERFACE_VER_NAME.deviceGetPlatformInfo( logger, platformInfo );
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
            s_realInterface.QNN_INTERFACE_VER_NAME.deviceCreate( logger, config, device );
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

Qnn_ErrorHandle_t QnnBackend_RegisterOpPackageFn( Qnn_BackendHandle_t backend,
                                                  const char *packagePath,
                                                  const char *interfaceProvider,
                                                  const char *target )
{
    printf( "call real QNN backendRegisterOpPackage\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.backendRegisterOpPackage(
            backend, packagePath, interfaceProvider, target );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_BACKEND_REGISTER_OP_PACKAGE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_BACKEND_REGISTER_OP_PACKAGE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_BACKEND_REGISTER_OP_PACKAGE]
                               .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_BACKEND_REGISTER_OP_PACKAGE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnContext_CreateFn( Qnn_BackendHandle_t backend, Qnn_DeviceHandle_t device,
                                       const QnnContext_Config_t **config,
                                       Qnn_ContextHandle_t *context )
{
    printf( "call real QNN contextCreate\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.contextCreate( backend, device,
                                                                                  config, context );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_CONTEXT_CREATE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_CONTEXT_CREATE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_CONTEXT_CREATE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_CONTEXT_CREATE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnGraph_FinalizeFn( Qnn_GraphHandle_t graphHandle,
                                       Qnn_ProfileHandle_t profileHandle,
                                       Qnn_SignalHandle_t signalHandle )
{
    printf( "call real QNN graphFinalize\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.graphFinalize(
            graphHandle, profileHandle, signalHandle );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_GRAPH_FINALIZE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_GRAPH_FINALIZE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_GRAPH_FINALIZE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_GRAPH_FINALIZE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnGraph_RetrieveFn( Qnn_ContextHandle_t contextHandle, const char *graphName,
                                       Qnn_GraphHandle_t *graphHandle )
{
    printf( "call real QNN graphRetrieve\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.graphRetrieve(
            contextHandle, graphName, graphHandle );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_GRAPH_RETRIEVE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_GRAPH_RETRIEVE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_GRAPH_RETRIEVE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_GRAPH_RETRIEVE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnHtpPerfInfrastructure_CreatePowerConfigIdFn( uint32_t deviceId,
                                                                  uint32_t coreId,
                                                                  uint32_t *powerConfigId )
{
    printf( "call real QNN createPowerConfigId\n" );
    Qnn_ErrorHandle_t ret =
            s_realHtpInfras.perfInfra.createPowerConfigId( deviceId, coreId, powerConfigId );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_INFRAS_CREAT_POWER_CONFIG_ID].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_INFRAS_CREAT_POWER_CONFIG_ID].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_INFRAS_CREAT_POWER_CONFIG_ID]
                               .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_INFRAS_CREAT_POWER_CONFIG_ID].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t
QnnHtpPerfInfrastructure_SetPowerConfigFn( uint32_t powerConfigId,
                                           const QnnHtpPerfInfrastructure_PowerConfig_t **config )
{
    printf( "call real QNN setPowerConfig\n" );
    Qnn_ErrorHandle_t ret = s_realHtpInfras.perfInfra.setPowerConfig( powerConfigId, config );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_INFRAS_SET_POWER_CONFIG_ID].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_INFRAS_SET_POWER_CONFIG_ID].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_INFRAS_SET_POWER_CONFIG_ID]
                               .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_INFRAS_SET_POWER_CONFIG_ID].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnHtpPerfInfrastructure_DestroyPowerConfigIdFn( uint32_t powerConfigId )
{
    printf( "call real QNN destroyPowerConfigId\n" );
    Qnn_ErrorHandle_t ret = s_realHtpInfras.perfInfra.destroyPowerConfigId( powerConfigId );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_INFRAS_DESTROY_POWER_CONFIG_ID].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_INFRAS_DESTROY_POWER_CONFIG_ID].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *)
                               s_MockParams[MOCK_API_QNN_INFRAS_DESTROY_POWER_CONFIG_ID]
                                       .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_INFRAS_DESTROY_POWER_CONFIG_ID].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnDevice_GetInfrastructureFn( QnnDevice_Infrastructure_t *deviceInfra )
{
    printf( "call real QNN deviceGetInfrastructure\n" );
    Qnn_ErrorHandle_t ret =
            s_realInterface.QNN_INTERFACE_VER_NAME.deviceGetInfrastructure( deviceInfra );

    if ( QNN_SUCCESS == ret )
    {
        s_realHtpInfras = *static_cast<QnnHtpDevice_Infrastructure_t *>( *deviceInfra );
        s_mockHtpInfras = s_realHtpInfras;
        s_mockHtpInfras.perfInfra.createPowerConfigId =
                QnnHtpPerfInfrastructure_CreatePowerConfigIdFn;
        s_mockHtpInfras.perfInfra.setPowerConfig = QnnHtpPerfInfrastructure_SetPowerConfigFn;
        s_mockHtpInfras.perfInfra.destroyPowerConfigId =
                QnnHtpPerfInfrastructure_DestroyPowerConfigIdFn;
        *deviceInfra = static_cast<const QnnDevice_Infrastructure_t>( &s_mockHtpInfras );
    }
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_DEVICE_GET_INFRAS].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_DEVICE_GET_INFRAS].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_DEVICE_GET_INFRAS].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_DEVICE_GET_INFRAS].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnMem_RegisterFn( Qnn_ContextHandle_t context,
                                     const Qnn_MemDescriptor_t *memDescriptors,
                                     uint32_t numDescriptors, Qnn_MemHandle_t *memHandles )
{
    printf( "call real QNN memRegister\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.memRegister(
            context, memDescriptors, numDescriptors, memHandles );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_MEMORY_REGISTER].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_MEMORY_REGISTER].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_MEMORY_REGISTER].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_MEMORY_REGISTER].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnProfile_CreateFn( Qnn_BackendHandle_t backend, QnnProfile_Level_t level,
                                       Qnn_ProfileHandle_t *profile )
{
    Qnn_ErrorHandle_t ret = QNN_MIN_ERROR_COMMON;
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_PROFILE_CREATE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_PROFILE_CREATE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_PROFILE_CREATE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_PROFILE_CREATE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    else
    {
        printf( "call real QNN profileCreate\n" );
        ret = s_realInterface.QNN_INTERFACE_VER_NAME.profileCreate( backend, level, profile );
    }
    return ret;
}


Qnn_ErrorHandle_t QnnProfile_FreeFn( Qnn_ProfileHandle_t profile )
{
    Qnn_ErrorHandle_t ret = QNN_MIN_ERROR_COMMON;
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_PROFILE_FREE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_PROFILE_FREE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_PROFILE_FREE].param;

                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_PROFILE_FREE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    else
    {
        printf( "call real QNN profileFree\n" );
        ret = s_realInterface.QNN_INTERFACE_VER_NAME.profileFree( profile );
    }
    return ret;
}


Qnn_ErrorHandle_t QnnProfile_GetEventsFn( Qnn_ProfileHandle_t profile,
                                          const QnnProfile_EventId_t **profileEventIds,
                                          uint32_t *numEvents )
{
    printf( "call real QNN profileGetEvents\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.profileGetEvents(
            profile, profileEventIds, numEvents );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENTS].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENTS].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENTS].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENTS].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnProfile_GetEventDataFn( QnnProfile_EventId_t eventId,
                                             QnnProfile_EventData_t *eventData )
{
    printf( "call real QNN profileGetEventData\n" );
    Qnn_ErrorHandle_t ret =
            s_realInterface.QNN_INTERFACE_VER_NAME.profileGetEventData( eventId, eventData );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENT_DATA].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENT_DATA].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENT_DATA]
                               .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_PROFILE_GET_EVENT_DATA].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnMem_DeRegisterFn( const Qnn_MemHandle_t *memHandles, uint32_t numHandles )
{
    printf( "call real QNN memDeRegister\n" );
    Qnn_ErrorHandle_t ret =
            s_realInterface.QNN_INTERFACE_VER_NAME.memDeRegister( memHandles, numHandles );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_MEMORY_DEREGISTER].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_MEMORY_DEREGISTER].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_MEMORY_DEREGISTER].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_MEMORY_DEREGISTER].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnContext_FreeFn( Qnn_ContextHandle_t context, Qnn_ProfileHandle_t profile )
{
    printf( "call real QNN contextFree\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.contextFree( context, profile );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_CONTEXT_FREE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_CONTEXT_FREE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_CONTEXT_FREE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_CONTEXT_FREE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}


Qnn_ErrorHandle_t QnnDevice_FreeFn( Qnn_DeviceHandle_t device )
{
    printf( "call real QNN deviceFree\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.deviceFree( device );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_DEVICE_FREE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_DEVICE_FREE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_DEVICE_FREE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_DEVICE_FREE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnDevice_FreePlatformInfoFn( Qnn_LogHandle_t logger,
                                                const QnnDevice_PlatformInfo_t *platformInfo )
{
    printf( "call real QNN deviceFreePlatformInfo\n" );
    Qnn_ErrorHandle_t ret =
            s_realInterface.QNN_INTERFACE_VER_NAME.deviceFreePlatformInfo( logger, platformInfo );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_DEVICE_FREE_PLATFORM_INFO].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_DEVICE_FREE_PLATFORM_INFO].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_DEVICE_FREE_PLATFORM_INFO]
                               .param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_DEVICE_FREE_PLATFORM_INFO].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnBackend_FreeFn( Qnn_BackendHandle_t backend )
{
    printf( "call real QNN backendFree\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.backendFree( backend );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_BACKEND_FREE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_BACKEND_FREE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_BACKEND_FREE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_BACKEND_FREE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnLog_FreeFn( Qnn_LogHandle_t logger )
{
    printf( "call real QNN logFree\n" );
    Qnn_ErrorHandle_t ret = s_realInterface.QNN_INTERFACE_VER_NAME.logFree( logger );
    if ( MOCK_CONTROL_API_NONE != s_MockParams[MOCK_API_QNN_LOG_FREE].action )
    {
        switch ( s_MockParams[MOCK_API_QNN_LOG_FREE].action )
        {
            case MOCK_CONTROL_API_RETURN:
                ret = *(Qnn_ErrorHandle_t *) s_MockParams[MOCK_API_QNN_LOG_FREE].param;
                break;
            default:
                break;
        }
        s_MockParams[MOCK_API_QNN_LOG_FREE].action =
                MOCK_CONTROL_API_NONE; /* consume it and invalide the control */
    }
    return ret;
}

Qnn_ErrorHandle_t QnnInterface_getProviders( const QnnInterface_t ***providerList,
                                             uint32_t *numProviders )
{
    Qnn_ErrorHandle_t ret;
    (void) s_qnnclientLoader;

    if ( nullptr != s_qnnInterfaceGetProvidersFn )
    {
        printf( "call real QNN qnnInterfaceGetProvidersFn\n" );
        ret = s_qnnInterfaceGetProvidersFn( providerList, numProviders );
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
            s_realInterface = *( ( *providerList )[0] );
            s_mockInterface = s_realInterface;
            s_mockInterface.QNN_INTERFACE_VER_NAME.logCreate = QnnLog_CreateFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.backendCreate = QnnBackend_CreateFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.backendGetApiVersion =
                    QnnBackend_GetApiVersionFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.deviceGetPlatformInfo =
                    QnnDevice_GetPlatformInfoFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.deviceCreate = QnnDevice_CreateFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.backendRegisterOpPackage =
                    QnnBackend_RegisterOpPackageFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.contextCreate = QnnContext_CreateFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.graphFinalize = QnnGraph_FinalizeFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.graphRetrieve = QnnGraph_RetrieveFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.deviceGetInfrastructure =
                    (QnnDevice_GetInfrastructureFn_t) QnnDevice_GetInfrastructureFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.memRegister = QnnMem_RegisterFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.profileCreate = QnnProfile_CreateFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.profileFree = QnnProfile_FreeFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.profileGetEvents = QnnProfile_GetEventsFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.profileGetEventData = QnnProfile_GetEventDataFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.memDeRegister = QnnMem_DeRegisterFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.contextFree = QnnContext_FreeFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.deviceFree = QnnDevice_FreeFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.deviceFreePlatformInfo =
                    QnnDevice_FreePlatformInfoFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.backendFree = QnnBackend_FreeFn;
            s_mockInterface.QNN_INTERFACE_VER_NAME.logFree = QnnLog_FreeFn;
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
            case MOCK_CONTROL_API_OUT_PARAM0_SET_CONTEXT_CREATE_FROM_BINARY_INFO_NULL:
                if ( QNN_SUCCESS == ret )
                {
                    s_mockInterface.QNN_INTERFACE_VER_NAME.contextCreateFromBinary = nullptr;
                }
                break;
            case MOCK_CONTROL_API_OUT_PARAM0_SET_GRAPH_RETRIEVE_NULL:
                if ( QNN_SUCCESS == ret )
                {
                    s_mockInterface.QNN_INTERFACE_VER_NAME.graphRetrieve = nullptr;
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