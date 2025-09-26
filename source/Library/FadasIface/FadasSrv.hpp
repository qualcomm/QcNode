// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_FADAS_SRV_HPP
#define QC_FADAS_SRV_HPP

#include <fadas.h>
#include <map>
#include <mutex>
#include <vector>
#pragma weak remote_session_control
#include "AEEStdErr.h"
#include <remote.h>
extern "C"
{
#include "FadasIface.h"
#include "fastrpc_api.h"
}
#include <dlfcn.h>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/Ifs/QCBufferDescriptorBase.hpp"
#include "QC/Infras/Memory/SharedBuffer.hpp"
namespace QC
{
namespace libs
{
namespace FadasIface
{

using namespace QC;
using namespace QC::Memory;

#ifndef FADAS_CLIENT_URI
#define FADAS_CLIENT_URI "&_session="
#endif

#ifndef CDSP_DOMAIN
#define CDSP_DOMAIN "&_dom=cdsp0"
#endif

#ifndef CDSP1_DOMAIN
#define CDSP1_DOMAIN "&_dom=cdsp1"
#endif

typedef FadasError_e ( *FuncFadasInitGPU_t )( const char *licenseKey );
typedef FadasError_e ( *FuncFadasDeInitGPU_t )( void );
typedef FadasError_e ( *FuncFadasRegBufGPU_t )( FadasBufType_e bufType, const void *buf,
                                                size_t bufSize );
typedef FadasError_e ( *FuncFadasDeregBufGPU_t )( const void *buf );
typedef FadasRemapMap_t *( *FuncFadasRemap_CreateMapFromMapGPU_t )(
        uint32_t camWidth, uint32_t camHeight, uint32_t mapWidth, uint32_t mapHeight,
        uint32_t mapStride, const float32_t *__restrict mapX, const float32_t *__restrict mapY,
        FadasRemapPipeline_e ePipeline, uint8_t borderConst, uint32_t nThreads );
typedef FadasRemapMap_t *( *FuncFadasRemap_CreateMapNoUndistortionGPU_t )(
        uint32_t srcWidth, uint32_t srcHeight, uint32_t dstWidth, uint32_t dstHeight,
        FadasRemapPipeline_e ePipeline, uint8_t borderConst );
typedef FadasError_e ( *FuncFadasRemap_RunGPU_t )( FadasRemapMap_t *map, FadasImage_t *src,
                                                   FadasImage_t *dst, FadasROI_t *mapROI,
                                                   float32_t roiScale,
                                                   FadasNormlzParams_t *normlz );
typedef FadasError_e ( *FuncFadasRemap_DestroyMapGPU_t )( FadasRemapMap_t *map );

class FadasSrv
{
public:
    QCStatus_e Init( QCProcessorType_e coreId, const char *pName, Logger_Level_e level );
    QCStatus_e Deinit();
    int32_t RegBuf( const QCSharedBuffer_t *pBuffer, FadasBufType_e bufferType );
    int32_t RegBuf( const QCBufferDescriptorBase_t &bufDesc, FadasBufType_e bufferType );
    void DeregBuf( void *pBuffer );
    remote_handle64 GetRemoteHandle64();

protected:
    struct MemInfo
    {
        int32_t fd;
        size_t size;
        size_t offset;
        uint32_t batch;
        void *ptr;
        size_t sizeOne;
    };
    QCProcessorType_e m_processor;

private:
    QCStatus_e InitCPU();
    QCStatus_e InitGPU();
    QCStatus_e InitDSP( QCProcessorType_e coreId );
    int32_t FadasMemMapDSP( const QCSharedBuffer_t *pBuffer );
    int32_t FadasMemMapCPU( const QCSharedBuffer_t *pBuffer );
    int32_t FadasMemMap( const QCSharedBuffer_t *pBuffer );
    QCStatus_e FadasRegisterBufDSP( FadasBufType_e bufType, const uint8_t *bufPtr, int32_t bufFd,
                                    uint32_t bufSize, uint32_t bufOffset, uint32_t batch );
    QCStatus_e FadasRegisterBufCPU( FadasBufType_e bufType, uint8_t *bufPtr, int32_t bufFd,
                                    uint32_t bufSize, uint32_t bufOffset, uint32_t batch );
    QCStatus_e FadasRegisterBufGPU( FadasBufType_e bufType, uint8_t *bufPtr, int32_t bufFd,
                                    uint32_t bufSize, uint32_t bufOffset, uint32_t batch );
    QCStatus_e FadasRegisterBuf( FadasBufType_e bufType, uint8_t *bufPtr, int32_t bufFd,
                                 uint32_t bufSize, uint32_t bufOffset, uint32_t batch );
    int32_t RegisterImage( const QCSharedBuffer_t *pBuffer, FadasBufType_e bufferType );
    int32_t RegisterTensor( const QCSharedBuffer_t *pBuffer, FadasBufType_e bufferType );

private:
    static std::mutex s_coreLock[QC_PROCESSOR_MAX];
    static std::mutex s_FadasLock;
    static remote_handle64 s_handle64[QC_PROCESSOR_MAX];
    static uint64_t s_useRef[QC_PROCESSOR_MAX];
    static bool s_initialized[QC_PROCESSOR_MAX];
    static std::map<void *, MemInfo> s_memMaps[QC_PROCESSOR_MAX];
    static long int s_client;
    static FuncFadasInitGPU_t s_FadasInitGPU;
    static FuncFadasDeInitGPU_t s_FadasDeInitGPU;
    static FuncFadasRegBufGPU_t s_FadasRegBufGPU;
    static FuncFadasDeregBufGPU_t s_FadasDeregBufGPU;

protected:
    static void *s_libGPUHandle;
    static FuncFadasRemap_CreateMapFromMapGPU_t s_FadasRemap_CreateMapFromMapGPU;
    static FuncFadasRemap_CreateMapNoUndistortionGPU_t s_FadasRemap_CreateMapNoUndistortionGPU;
    static FuncFadasRemap_RunGPU_t s_FadasRemap_RunGPU;
    static FuncFadasRemap_DestroyMapGPU_t s_FadasRemap_DestroyMapGPU;


protected:
    QC_DECLARE_LOGGER();
};


}   // namespace FadasIface
}   // namespace libs
}   // namespace QC

#endif   // QC_FADAS_SRV_HPP
