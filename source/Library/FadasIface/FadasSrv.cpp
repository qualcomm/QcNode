// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "FadasSrv.hpp"

#include <rpcmem.h>

namespace QC
{
namespace libs
{
namespace FadasIface
{

std::mutex FadasSrv::s_coreLock[QC_PROCESSOR_MAX];
std::mutex FadasSrv::s_FadasLock;
remote_handle64 FadasSrv::s_handle64[QC_PROCESSOR_MAX] = { 0, 0, 0, 0 };
bool FadasSrv::s_initialized[QC_PROCESSOR_MAX] = { false, false, false, false };
uint64_t FadasSrv::s_useRef[QC_PROCESSOR_MAX] = { 0, 0, 0, 0 };
std::map<void *, FadasSrv::MemInfo> FadasSrv::s_memMaps[QC_PROCESSOR_MAX];
long int FadasSrv::s_client = 1;
void *FadasSrv::s_libGPUHandle = nullptr;
FuncFadasInitGPU_t FadasSrv::s_FadasInitGPU = nullptr;
FuncFadasDeInitGPU_t FadasSrv::s_FadasDeInitGPU = nullptr;
FuncFadasRegBufGPU_t FadasSrv::s_FadasRegBufGPU = nullptr;
FuncFadasDeregBufGPU_t FadasSrv::s_FadasDeregBufGPU = nullptr;
FuncFadasRemap_CreateMapFromMapGPU_t FadasSrv::s_FadasRemap_CreateMapFromMapGPU = nullptr;
FuncFadasRemap_CreateMapNoUndistortionGPU_t FadasSrv::s_FadasRemap_CreateMapNoUndistortionGPU =
        nullptr;
FuncFadasRemap_RunGPU_t FadasSrv::s_FadasRemap_RunGPU = nullptr;
FuncFadasRemap_DestroyMapGPU_t FadasSrv::s_FadasRemap_DestroyMapGPU = nullptr;

extern "C"
{
    int get_extended_domains_id( int domain, int session );
    void remote_register_buf_v2( int ext_domain_id, void *buf, int size, int fd );
    void remote_register_buf_attr_v2( int ext_domain_id, void *buf, int size, int fd, int attr );
}

QCStatus_e FadasSrv::InitCPU()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( FADAS_ERROR_NONE != FadasInit( nullptr ) )
    {
        QC_ERROR( "FadasInit failed!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e FadasSrv::InitGPU()
{
    QCStatus_e ret = QC_STATUS_OK;

    s_libGPUHandle = dlopen( "libfadasGpu.so", RTLD_LAZY );

    if ( nullptr == s_libGPUHandle )
    {
        QC_ERROR( "Failed to load libfadasGpu.so library!" );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        s_FadasInitGPU = (FuncFadasInitGPU_t) dlsym( s_libGPUHandle, "FadasInit" );
        if ( nullptr == s_FadasInitGPU )
        {
            QC_ERROR( "Failed to load FadasInit functions!" );
            ret = QC_STATUS_FAIL;
        }
        s_FadasDeInitGPU = (FuncFadasDeInitGPU_t) dlsym( s_libGPUHandle, "FadasDeInit" );
        if ( nullptr == s_FadasDeInitGPU )
        {
            QC_ERROR( "Failed to load FadasDeInit functions!" );
            ret = QC_STATUS_FAIL;
        }
        s_FadasRegBufGPU = (FuncFadasRegBufGPU_t) dlsym( s_libGPUHandle, "FadasRegBuf" );
        if ( nullptr == s_FadasRegBufGPU )
        {
            QC_ERROR( "Failed to load FadasRegBuf functions!" );
            ret = QC_STATUS_FAIL;
        }
        s_FadasDeregBufGPU = (FuncFadasDeregBufGPU_t) dlsym( s_libGPUHandle, "FadasDeregBuf" );
        if ( nullptr == s_FadasDeregBufGPU )
        {
            QC_ERROR( "Failed to load FadasDeregBuf functions!" );
            ret = QC_STATUS_FAIL;
        }
        s_FadasRemap_CreateMapFromMapGPU = (FuncFadasRemap_CreateMapFromMapGPU_t) dlsym(
                s_libGPUHandle, "FadasRemap_CreateMapFromMap" );
        if ( nullptr == s_FadasRemap_CreateMapFromMapGPU )
        {
            QC_ERROR( "Failed to load FadasRemap_CreateMapFromMap functions!" );
            ret = QC_STATUS_FAIL;
        }
        s_FadasRemap_CreateMapNoUndistortionGPU =
                (FuncFadasRemap_CreateMapNoUndistortionGPU_t) dlsym(
                        s_libGPUHandle, "FadasRemap_CreateMapNoUndistortion" );
        if ( nullptr == s_FadasRemap_CreateMapNoUndistortionGPU )
        {
            QC_ERROR( "Failed to load FadasRemap_CreateMapNoUndistortion functions!" );
            ret = QC_STATUS_FAIL;
        }
        s_FadasRemap_RunGPU = (FuncFadasRemap_RunGPU_t) dlsym( s_libGPUHandle, "FadasRemap_Run" );
        if ( nullptr == s_FadasRemap_RunGPU )
        {
            QC_ERROR( "Failed to load FadasRemap_Run functions!" );
            ret = QC_STATUS_FAIL;
        }
        s_FadasRemap_DestroyMapGPU =
                (FuncFadasRemap_DestroyMapGPU_t) dlsym( s_libGPUHandle, "FadasRemap_DestroyMap" );
        if ( nullptr == s_FadasRemap_DestroyMapGPU )
        {
            QC_ERROR( "Failed to load FadasRemap_DestroyMap functions!" );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( FADAS_ERROR_NONE != s_FadasInitGPU( nullptr ) )
        {
            QC_ERROR( "FadasInitGPU failed!" );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( ( QC_STATUS_OK != ret ) && ( nullptr != s_libGPUHandle ) )
    {
        (void) dlclose( s_libGPUHandle );
    }

    return ret;
}

QCStatus_e FadasSrv::InitDSP( QCProcessorType_e coreId )
{
    QCStatus_e ret = QC_STATUS_OK;

    std::string envName = "QC_FADAS_CLIENT_ID";
    const char *envValue = getenv( envName.c_str() );
    if ( nullptr != envValue )
    {
        char *endptr;
        errno = 0;
        s_client = strtol( envValue, &endptr, 10 );
        if ( 0 != errno )
        {
            QC_INFO( "Invalid client reset to 1!" );
            s_client = 1;
        }
    }
    if ( ( 0 > s_client ) || ( 12 < s_client ) )
    {
        QC_INFO( "Invalid client id = %d, reset to 1!", s_client );
        s_client = 1;
    }

    std::string uriFadas = FadasIface_URI;
    std::string uriDomain = CDSP_DOMAIN;
    if ( QC_PROCESSOR_HTP1 == coreId )
    {
        uriDomain = CDSP1_DOMAIN;
    }
    std::string uriClient = FADAS_CLIENT_URI + std::to_string( s_client );
    std::string uriFull = uriFadas + uriDomain + uriClient;
    const char *uri = const_cast<char *>( uriFull.c_str() );
    QC_INFO( "uri = %s", uri );

    remote_handle64 handle64 = 0;
    int domain = CDSP_DOMAIN_ID;
    if ( QC_PROCESSOR_HTP1 == coreId )
    {
        domain = CDSP1_DOMAIN_ID;
    }

    if ( 0 == s_handle64[coreId] )
    {
        // Use unsigned PD for DSP
        struct remote_rpc_control_unsigned_module data;
        data.enable = 1;
        data.domain = domain;
        (void) remote_session_control( DSPRPC_CONTROL_UNSIGNED_MODULE,
                                       reinterpret_cast<void *>( &data ), sizeof( data ) );
        auto retVal = FadasIface_open( uri, &handle64 );
        if ( AEE_SUCCESS != retVal )
        {
            QC_ERROR( "Failed to open fadas: %d", retVal );
            ret = QC_STATUS_FAIL;
        }

        s_handle64[coreId] = handle64;
    }
    else
    {
        handle64 = s_handle64[coreId];
    }

    if ( QC_STATUS_OK == ret )
    {
        int32_t ans = FADAS_ERROR_MAX;
        (void) FadasIface_FadasInit( handle64, &ans );
        if ( FADAS_ERROR_NONE != ans )
        {
            QC_ERROR( "FAILED:  FadasIface_FadasInit - 0x%x", ans );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            constexpr size_t FADAS_VERSION_LEN = 64;
            char version[FADAS_VERSION_LEN] = { 0 };
            auto retVal = FadasIface_FadasVersion( handle64, reinterpret_cast<uint8_t *>( version ),
                                                   FADAS_VERSION_LEN );
            if ( retVal != AEE_SUCCESS )
            {
                QC_ERROR( "FAILED: FadasIface_FadasVersion - 0x%x", retVal );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                QC_INFO( "Initialized FastADAS(DSP, %s) successfully", version );
            }
        }
    }

    return ret;
}

QCStatus_e FadasSrv::Init( QCProcessorType_e coreId, const char *pName, Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = QC_LOGGER_INIT( pName, level );
    if ( QC_STATUS_OK != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to create logger for FadasSrv %s: ret = %d\n",
                        pName, ret );
        ret = QC_STATUS_OK; /* ignore logger init error */
    }

    std::lock_guard<std::mutex> l( s_FadasLock );
    if ( ( QC_PROCESSOR_HTP0 == coreId ) || ( QC_PROCESSOR_HTP1 == coreId ) ||
         ( QC_PROCESSOR_CPU == coreId ) || ( QC_PROCESSOR_GPU == coreId ) )
    {
        m_processor = coreId;
    }
    else
    {
        QC_ERROR( "Invalid processor type!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( ( false == s_initialized[coreId] ) && ( QC_STATUS_OK == ret ) )
    {
        if ( ( QC_PROCESSOR_HTP0 == coreId ) || ( QC_PROCESSOR_HTP1 == coreId ) )
        {
            ret = InitDSP( coreId );
        }
        else if ( QC_PROCESSOR_GPU == coreId )
        {
            ret = InitGPU();
        }
        else
        {
            ret = InitCPU();
        }

        if ( QC_STATUS_OK == ret )
        {
            s_initialized[coreId] = true;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        s_useRef[coreId]++;
    }
    else
    {
        (void) QC_LOGGER_DEINIT();
    }

    return ret;
}

QCStatus_e FadasSrv::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    std::lock_guard<std::mutex> l( s_FadasLock );
    if ( ( 0 != s_initialized[m_processor] ) && ( 0 < s_useRef[m_processor] ) )
    {
        s_useRef[m_processor]--;
        if ( 0 == s_useRef[m_processor] )
        {
            auto &memMap = s_memMaps[m_processor];
            std::vector<void *> ptrs;
            for ( auto &it : memMap )
            {
                ptrs.push_back( it.first );
            }
            for ( auto ptr : ptrs )
            {
                DeregBuf( ptr );
            }
            if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
            {
                if ( AEE_SUCCESS != FadasIface_FadasDeInit( s_handle64[m_processor] ) )
                {
                    QC_ERROR( "FadasIface_FadasDeInit failed!" );
                    ret = QC_STATUS_FAIL;
                }
            }
            else if ( QC_PROCESSOR_GPU == m_processor )
            {
                if ( FADAS_ERROR_NONE != s_FadasDeInitGPU() )
                {
                    QC_ERROR( "FadasDeInitGPU failed!" );
                    ret = QC_STATUS_FAIL;
                }
                (void) dlclose( s_libGPUHandle );
            }
            else
            {
                if ( FADAS_ERROR_NONE != FadasDeInit() )
                {
                    QC_ERROR( "FadasDeInit failed!" );
                    ret = QC_STATUS_FAIL;
                }
            }
            memMap.clear();
            s_initialized[m_processor] = false;
        }
    }

    ret = QC_LOGGER_DEINIT();
    if ( QC_STATUS_OK != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to deinit logger for FadasSrv: ret = %d\n", ret );
        ret = QC_STATUS_OK; /* ignore logger deinit error */
    }

    return ret;
}

remote_handle64 FadasSrv::GetRemoteHandle64()
{
    return s_handle64[m_processor];
}

int32_t FadasSrv::FadasMemMapDSP( const QCSharedBuffer_t *pBuffer )
{
    int ret = AEE_SUCCESS;
    int32_t fd = -1;
    int extDomainId = 0;
    int domain = CDSP_DOMAIN_ID;
    if ( QC_PROCESSOR_HTP1 == m_processor )
    {
        domain = CDSP1_DOMAIN_ID;
    }
    int client = s_client;
    auto handle64 = s_handle64[m_processor];

    extDomainId = get_extended_domains_id( domain, client );

    void *ptr = pBuffer->buffer.pData;
    size_t size = pBuffer->buffer.size;

    fd = rpcmem_to_fd( ptr );
    if ( fd < 0 )
    {
#if defined( __QNXNTO__ )
        remote_register_buf_v2( extDomainId, ptr, size, 0 );
#else
        remote_register_buf_v2( extDomainId, ptr, size, (int) pBuffer->buffer.dmaHandle );
#endif
        fd = rpcmem_to_fd( (void *) ptr );
        if ( fd < 0 )
        {
            QC_ERROR( "rpcmem_to_fd failed, fd = %d", fd );
            ret = AEE_EFAILED;
        }
    }

    if ( AEE_SUCCESS == ret )
    {
        ret = fastrpc_mmap( extDomainId, fd, ptr, 0, size, FASTRPC_MAP_FD_DELAYED );
        if ( ( AEE_EALREADY != ret ) && ( AEE_SUCCESS != ret ) )
        {
            QC_ERROR( "Failed to fastrpc_mmap ptr %p(%d, %llu): ret = %x\n", ptr, fd, size, ret );
            fd = -1;
        }
        else
        {
            ret = AEE_SUCCESS;
        }
    }

    if ( AEE_SUCCESS == ret )
    {
        ret = FadasIface_mmap( handle64, fd, (uint32_t) size );
        if ( AEE_SUCCESS != ret )
        {
            QC_ERROR( "Failed to map ptr %p(%d, %llu): ret = %x\n", ptr, fd, size, ret );
            fd = -1;
        }
        else
        {
            QC_INFO( "map ptr %p(%d, %llu) OK\n", ptr, fd, size );
        }
    }

    return fd;
}

int32_t FadasSrv::FadasMemMapCPU( const QCSharedBuffer_t *pBuffer )
{
    (void) pBuffer;
    return 1; /* virtual fd for CPU&GPU pipeline, indicates that the register is
               * successful, would not be really used. */
}

int32_t FadasSrv::FadasMemMap( const QCSharedBuffer_t *pBuffer )
{
    int32_t fd = -1;

    if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        fd = FadasMemMapDSP( pBuffer );
    }
    else
    {
        fd = FadasMemMapCPU( pBuffer );
    }

    return fd;
}

QCStatus_e FadasSrv::FadasRegisterBufDSP( FadasBufType_e bufType, const uint8_t *bufPtr,
                                          int32_t bufFd, uint32_t bufSize, uint32_t bufOffset,
                                          uint32_t batch )
{
    QCStatus_e ret = QC_STATUS_OK;
    auto handle64 = s_handle64[m_processor];

    FadasIface_FadasBufType_e bufTypeDsp;
    if ( FADAS_BUF_TYPE_IN == bufType )
    {
        bufTypeDsp = FADAS_BUF_TYPE_IN_NSP;
    }
    else if ( FADAS_BUF_TYPE_OUT == bufType )
    {
        bufTypeDsp = FADAS_BUF_TYPE_OUT_NSP;
    }
    else
    {
        bufTypeDsp = FADAS_BUF_TYPE_INOUT_NSP;
    }

    (void) bufPtr; /* not used by DSP */
    int nErr = FadasIface_FadasRegBuf( handle64, bufTypeDsp, bufFd, bufSize, bufOffset, batch );
    if ( AEE_SUCCESS != nErr )
    {
        QC_ERROR( "FadasIface_FadasRegBuf fail: %d!", nErr );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e FadasSrv::FadasRegisterBufCPU( FadasBufType_e bufType, uint8_t *bufPtr, int32_t bufFd,
                                          uint32_t bufSize, uint32_t bufOffset, uint32_t batch )
{
    QCStatus_e ret = QC_STATUS_OK;
    FadasError_e nErr = FADAS_ERROR_NONE;
    uint8_t *ptr = bufPtr;

    (void) bufFd; /* not used by CPU */
    ptr += bufOffset;
    for ( uint32_t i = 0; i < batch; i++ )
    {
        nErr = FadasRegBuf( bufType, ptr, bufSize );
        if ( FADAS_ERROR_NONE != nErr )
        {
            QC_ERROR( "FadasRegBuf fail: %d!", nErr );
            ret = QC_STATUS_FAIL;
            break;
        }
        ptr += bufSize;
    }

    return ret;
}

QCStatus_e FadasSrv::FadasRegisterBufGPU( FadasBufType_e bufType, uint8_t *bufPtr, int32_t bufFd,
                                          uint32_t bufSize, uint32_t bufOffset, uint32_t batch )
{
    QCStatus_e ret = QC_STATUS_OK;
    FadasError_e nErr = FADAS_ERROR_NONE;
    uint8_t *ptr = bufPtr;

    (void) bufFd; /* not used by GPU */
    ptr += bufOffset;
    for ( uint32_t i = 0; i < batch; i++ )
    {
        nErr = s_FadasRegBufGPU( bufType, ptr, bufSize );
        if ( FADAS_ERROR_NONE != nErr )
        {
            QC_ERROR( "FadasRegBufGPU fail: %d!", nErr );
            ret = QC_STATUS_FAIL;
            break;
        }
        ptr += bufSize;
    }

    return ret;
}

QCStatus_e FadasSrv::FadasRegisterBuf( FadasBufType_e bufType, uint8_t *bufPtr, int32_t bufFd,
                                       uint32_t bufSize, uint32_t bufOffset, uint32_t batch )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = FadasRegisterBufDSP( bufType, bufPtr, bufFd, bufSize, bufOffset, batch );
    }
    else if ( QC_PROCESSOR_GPU == m_processor )
    {
        ret = FadasRegisterBufGPU( bufType, bufPtr, bufFd, bufSize, bufOffset, batch );
    }
    else
    {
        ret = FadasRegisterBufCPU( bufType, bufPtr, bufFd, bufSize, bufOffset, batch );
    }

    return ret;
}

int32_t FadasSrv::RegisterImage( const QCSharedBuffer_t *pBuffer, FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t fd = -1;

    auto &memMap = s_memMaps[m_processor];
    uint8_t *ptr = (uint8_t *) pBuffer->buffer.pData;
    size_t size = pBuffer->buffer.size;
    size_t offset = pBuffer->offset;
    uint32_t batch = pBuffer->imgProps.batchSize;
    size_t sizeOne = (size_t) ( pBuffer->size / batch );
    QCImageFormat_e format = pBuffer->imgProps.format;
    uint32_t sizePlane0 = pBuffer->imgProps.planeBufSize[0];
    uint32_t sizePlane1 = pBuffer->imgProps.planeBufSize[1];

    auto it = memMap.find( pBuffer->data() );
    if ( it == memMap.end() )
    {
        fd = FadasMemMap( pBuffer );
        if ( 0 <= fd )
        {
            if ( FADAS_BUF_TYPE_IN == bufferType )
            {
                /*for input buffer the batch must be 1*/
                if ( QC_IMAGE_FORMAT_NV12_UBWC == pBuffer->imgProps.format )
                /*for UBWC input, register all the 4 planes together once*/
                {
                    uint32_t sizeTotal =
                            pBuffer->imgProps.planeBufSize[0] + pBuffer->imgProps.planeBufSize[1] +
                            pBuffer->imgProps.planeBufSize[2] + pBuffer->imgProps.planeBufSize[3];

                    ret = FadasRegisterBuf( bufferType, ptr, fd, sizeTotal, offset, 1 );
                }
                else
                {
                    ret = FadasRegisterBuf( bufferType, ptr, fd, sizePlane0, offset, 1 );
                    if ( QC_IMAGE_FORMAT_NV12 == format )
                    { /* register both of plane0 and plane1 for NV12 format */
                        ret = FadasRegisterBuf( bufferType, ptr, fd, sizePlane1,
                                                sizePlane0 + offset, 1 );
                    }
                }
            }
            else
            {
                ret = FadasRegisterBuf( bufferType, ptr, fd, sizeOne, offset, batch );
            }

            if ( QC_STATUS_OK == ret )
            {
                memMap[pBuffer->data()] = { fd, size, offset, batch, ptr, sizeOne };
            }
            else
            {
                QC_ERROR( "Register Buffer(%p, %d, %d) failed!", ptr, fd, bufferType );
                fd = -1;
            }
        }
    }
    else
    {
        if ( pBuffer->buffer.pData != it->second.ptr )
        {
            QC_ERROR( "Shared buffer already registered, but stored ptr not match, stored %d "
                      "and given %d",
                      it->second.ptr, pBuffer->buffer.pData );
        }
        else if ( pBuffer->buffer.size != it->second.size )
        {
            QC_ERROR( "Shared buffer already registered, but stored size not match, "
                      "stored %d "
                      "and given %d",
                      it->second.size, pBuffer->buffer.size );
        }
        else if ( pBuffer->offset != it->second.offset )
        {
            QC_ERROR( "Shared buffer already registered, but stored offset not match, "
                      "stored %d "
                      "and given %d",
                      it->second.offset, pBuffer->offset );
        }
        else if ( pBuffer->imgProps.batchSize != it->second.batch )
        {
            QC_ERROR( "Shared buffer already registered, but stored batch not match, "
                      "stored %d "
                      "and given %d",
                      it->second.batch, pBuffer->imgProps.batchSize );
        }
        else
        {
            fd = it->second.fd;
        };
    }

    return fd;
}

int32_t FadasSrv::RegisterTensor( const QCSharedBuffer_t *pBuffer, FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t fd = -1;
    uint8_t *ptr = (uint8_t *) pBuffer->buffer.pData;
    size_t size = pBuffer->buffer.size;
    size_t offset = pBuffer->offset;
    size_t sizeOne = pBuffer->size;
    uint32_t batch = 1;

    auto &memMap = s_memMaps[m_processor];
    auto it = memMap.find( pBuffer->data() );
    if ( it == memMap.end() )
    {
        fd = FadasMemMap( pBuffer );
        if ( 0 <= fd )
        {
            ret = FadasRegisterBuf( bufferType, ptr, fd, sizeOne, offset, batch );
            if ( QC_STATUS_OK == ret )
            {
                memMap[pBuffer->data()] = { fd, size, offset, batch, ptr, sizeOne };
            }
            else
            {
                QC_ERROR( "Register Buffer(%p, %d, %d) failed!", ptr, fd, bufferType );
                fd = -1;
            }
        }
    }
    else
    {
        fd = it->second.fd;
    }

    return fd;
}

int32_t FadasSrv::RegBuf( const QCSharedBuffer_t *pBuffer, FadasBufType_e bufferType )
{
    std::lock_guard<std::mutex> l( s_coreLock[m_processor] );
    int32_t fd = -1;

    if ( nullptr == pBuffer )
    {
        QC_ERROR( "null buffer!" );
    }
    else if ( ( bufferType <= FADAS_BUF_TYPE_NONE ) || ( bufferType >= FADAS_BUF_TYPE_END ) )
    {
        QC_ERROR( "invalid buffer type!" );
    }
    else if ( QC_BUFFER_TYPE_IMAGE == pBuffer->type )
    {
        fd = RegisterImage( pBuffer, bufferType );
    }
    else if ( QC_BUFFER_TYPE_TENSOR == pBuffer->type )
    {
        fd = RegisterTensor( pBuffer, bufferType );
    }
    else
    {
        QC_ERROR( "Shared buffer type is %d!", pBuffer->type );
    }

    return fd;
}

int32_t FadasSrv::RegBuf( const QCBufferDescriptorBase_t &bufDesc, FadasBufType_e bufferType )
{
    /* TODO: FIXME to not use QCSharedBuffer_t */
    QCSharedBuffer_t sbuf = bufDesc;
    return RegBuf( &sbuf, bufferType );
}

void FadasSrv::DeregBuf( void *pBuffer )
{
    if ( nullptr == pBuffer )
    {
        QC_ERROR( "null buffer!" );
    }
    else
    {
        std::lock_guard<std::mutex> l( s_coreLock[m_processor] );
        auto &memMap = s_memMaps[m_processor];
        auto handle64 = s_handle64[m_processor];
        auto it = memMap.find( pBuffer );
        if ( it != memMap.end() )
        {
            int32_t fd = it->second.fd;
            size_t size = it->second.size;
            size_t offset = it->second.offset;
            uint32_t batch = it->second.batch;
            void *ptr = it->second.ptr;
            size_t sizeOne = it->second.sizeOne;
            (void) memMap.erase( it );

            if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
            {
                int extDomainId = 0;
                int domain = CDSP_DOMAIN_ID;
                if ( QC_PROCESSOR_HTP1 == m_processor )
                {
                    domain = CDSP1_DOMAIN_ID;
                }
                int client = s_client;
                extDomainId = get_extended_domains_id( domain, client );
                AEEResult retVal;
                retVal = FadasIface_FadasDeregBuf( handle64, fd, sizeOne, offset, batch );
                if ( AEE_SUCCESS != retVal )
                {
                    QC_ERROR( "Failed to FadasIface_FadasDeregBuf %p(%d, %llu): ret = %x\n", ptr,
                              fd, size, retVal );
                }
                retVal = FadasIface_munmap( handle64, fd, (uint32_t) size );
                if ( AEE_SUCCESS != retVal )
                {
                    QC_ERROR( "Failed to FadasIface_munmap %p(%d, %llu): ret = %x\n", ptr, fd, size,
                              retVal );
                    retVal = fastrpc_munmap( extDomainId, fd, ptr, size );
                    if ( AEE_SUCCESS != retVal )
                    {
                        QC_ERROR( "Failed to fastrpc_munmap %p(%d, %llu): ret = %x\n", ptr, fd,
                                  size, retVal );
                    }
                }
                remote_register_buf_v2( extDomainId, ptr, size, -1 );
            }
            else if ( QC_PROCESSOR_GPU == m_processor )
            {
                for ( int i = 0; i < batch; i++ )
                {
                    FadasError_e retVal = s_FadasDeregBufGPU( (uint8_t *) pBuffer + sizeOne * i );
                    if ( FADAS_ERROR_NONE != retVal )
                    {
                        QC_ERROR( "FadasDeregBufGPU %p(%llu) failed: %d!", ptr, size, retVal );
                    }
                }
            }
            else
            {
                for ( int i = 0; i < batch; i++ )
                {
                    FadasError_e retVal = FadasDeregBuf( (uint8_t *) pBuffer + sizeOne * i );
                    if ( FADAS_ERROR_NONE != retVal )
                    {
                        QC_ERROR( "FadasDeregBuf %p(%llu) failed: %d!", ptr, size, retVal );
                    }
                }
            }
        }
    }

    return;
}

}   // namespace FadasIface
}   // namespace libs
}   // namespace QC