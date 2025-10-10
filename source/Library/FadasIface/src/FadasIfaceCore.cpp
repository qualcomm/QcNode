// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "AEEStdErr.h"
#include "FadasIface.h"
#include "HAP_farf.h"
#include "HAP_mem.h"
#include "HAP_perf.h"
#include "HAP_power.h"
#include "qurt.h"
#include "remote.h"
#include <fadas.h>
#include <stdio.h>
#include <string.h>

#define PLRPOST_NUM_INPUTS 10

#define PLRPOST_IN_PTS 0
#define PLRPOST_IN_HEATMAP 1
#define PLRPOST_IN_XY 2
#define PLRPOST_IN_Z 3
#define PLRPOST_IN_SIZE 4
#define PLRPOST_IN_THETA 5
#define PLRPOST_OUT_BBOX 6
#define PLRPOST_OUT_LABELS 7
#define PLRPOST_OUT_SCORES 8
#define PLRPOST_OUT_METADATA 9

typedef struct
{
    qurt_mutex_t mutex;
} dspContext_t;

// FIXME: Maybe need better way to map those enums
const FadasRemapPipeline_e g_MapImageConversion[FADAS_REMAP_PIPELINE_MAX_NSP] = {
        FADAS_REMAP_PIPELINE_1C8,
        FADAS_REMAP_PIPELINE_1C8_ROISCALE,
        FADAS_REMAP_PIPELINE_3C888,
        FADAS_REMAP_PIPELINE_3C888_ROISCALE,
        FADAS_REMAP_PIPELINE_YUV888_TO_RGB888,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888,
        FADAS_REMAP_PIPELINE_VYUY_TO_RGB888,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_ROISCALE,
        FADAS_REMAP_PIPELINE_VYUY_TO_RGB888_ROISCALE,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMI8,
        FADAS_REMAP_PIPELINE_UYVY_TO_RGB888_NORMU8,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_BGR888,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMI8,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_NORMU8,
        FADAS_REMAP_PIPELINE_Y8UV8_TO_RGB888_ROISCALE,
        FADAS_REMAP_PIPELINE_UYVY_TO_BGR888,
};

const FadasImageFormat_e g_MapImageFormat[FADAS_IMAGE_FORMAT_COUNT_NSP] = {
        FADAS_IMAGE_FORMAT_UNKNOWN, FADAS_IMAGE_FORMAT_Y,      FADAS_IMAGE_FORMAT_Y12,
        FADAS_IMAGE_FORMAT_UYVY,    FADAS_IMAGE_FORMAT_UYVY10, FADAS_IMAGE_FORMAT_VYUY,
        FADAS_IMAGE_FORMAT_YUV888,  FADAS_IMAGE_FORMAT_RGB888, FADAS_IMAGE_FORMAT_Y10UV10,
        FADAS_IMAGE_FORMAT_Y8UV8,
};

const FadasBufType_e g_MapBufType[FADAS_BUF_TYPE_MAX_NSP] = {
        FADAS_BUF_TYPE_IN,
        FADAS_BUF_TYPE_OUT,
        FADAS_BUF_TYPE_INOUT,
};

AEEResult SetClocks( remote_handle64 handle )
{
    AEEResult ret = AEE_SUCCESS;
    HAP_power_request_t request;
    memset( &request, 0, sizeof( HAP_power_request_t ) );
    request.type = HAP_power_set_apptype;
    request.apptype = HAP_POWER_COMPUTE_CLIENT_CLASS;
    void *ctx = (void *) ( handle );
    int retVal = HAP_power_set( ctx, &request );

    if ( 0 != retVal )
    {
        ret = AEE_EFAILED;
    }
    else
    {
        memset( &request, 0, sizeof( HAP_power_request_t ) );
        request.type = HAP_power_set_DCVS_v2;

        request.dcvs_v2.dcvs_enable = TRUE;
        request.dcvs_v2.dcvs_params.target_corner = (HAP_dcvs_voltage_corner_t) 7;

        request.dcvs_v2.dcvs_params.min_corner = request.dcvs_v2.dcvs_params.target_corner;
        request.dcvs_v2.dcvs_params.max_corner = request.dcvs_v2.dcvs_params.target_corner;

        request.dcvs_v2.dcvs_option = HAP_DCVS_V2_PERFORMANCE_MODE;
        request.dcvs_v2.set_dcvs_params = TRUE;
        request.dcvs_v2.set_latency = TRUE;
        request.dcvs_v2.latency = 100;
        retVal = HAP_power_set( ctx, &request );
    }

    if ( 0 != retVal )
    {
        ret = AEE_EFAILED;
    }
    else
    {
        memset( &request, 0, sizeof( HAP_power_request_t ) );
        request.type = HAP_power_set_HVX;
        request.hvx.power_up = TRUE;
        retVal = HAP_power_set( ctx, &request );
    }

    if ( 0 != retVal )
    {
        FARF( ERROR, "Failed to set clocks!" );
        ret = AEE_EFAILED;
    }

    return ret;
}

void *FadasIface_GetBufPtr( int32_t bufFd )
{
    void *bufPtr = nullptr;
    if ( 0 < bufFd )
    {
        AEEResult retVal = HAP_mmap_get( bufFd, (void **) &bufPtr, NULL );
        if ( AEE_SUCCESS != retVal )
        {
            FARF( ERROR, "Failed to get mmap!" );
        }
        else
        {
            HAP_mmap_put( bufFd );
        }
    }
    else
    {
        FARF( ERROR, "bufFd %d!", bufFd );
    }

    return bufPtr;
}

AEEResult FadasIface_open( const char *uri, remote_handle64 *handle )
{
    AEEResult ret = AEE_SUCCESS;
    dspContext_t *dspContext = (dspContext_t *) malloc( sizeof( dspContext_t ) );
    *handle = (remote_handle64) dspContext;
    if ( 0 == *handle )
    {
        FARF( ERROR, "Null handle pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        qurt_mutex_init( &dspContext->mutex );
        ret = SetClocks( *handle );
    }

    if ( AEE_SUCCESS != ret )
    {
        FARF( ERROR, "Failed to do FadasIface_open!" );
    }

    return ret;
}

AEEResult FadasIface_close( remote_handle64 handle )
{
    dspContext_t *dspContext = (dspContext_t *) handle;
    if ( NULL == dspContext )
    {
        FARF( ERROR, "Null handle pointer!" );
    }
    else
    {
        free( dspContext );
    }
    HAP_power_destroy( NULL );

    return AEE_SUCCESS;
}


AEEResult FadasIface_FadasInit( remote_handle64 handle, int32_t *status )
{
    *status = static_cast<int32_t>( FadasInit( nullptr ) );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasVersion( remote_handle64 handle, uint8_t *ver_int, int ver_intLen )
{

    strlcpy( reinterpret_cast<char *>( ver_int ), FadasVersion(), ver_intLen );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasDeInit( remote_handle64 handle )
{
    FadasDeInit();

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateMapFromMap(
        remote_handle64 handle, uint64 *mapPtr, uint32_t camWidth, uint32_t camHeight,
        uint32_t mapWidth, uint32_t mapHeight, int32_t mapXFd, int32_t mapYFd, uint32_t mapStride,
        FadasIface_FadasRemapPipeline_e imgFormat, uint8_t borderConst )
{
    AEEResult ret = AEE_SUCCESS;

    float *mapX = (float *) FadasIface_GetBufPtr( mapXFd );
    float *mapY = (float *) FadasIface_GetBufPtr( mapYFd );
    FadasRemapMap *map = FadasRemap_CreateMapFromMap(
            camWidth, camHeight, mapWidth, mapHeight, mapStride, mapX, mapY,
            g_MapImageConversion[static_cast<int>( imgFormat )], borderConst );

    if ( nullptr == map )
    {
        FARF( ERROR, "Null map pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        *mapPtr = reinterpret_cast<uint64>( map );
    }

    return ret;
}

AEEResult FadasIface_FadasRemap_CreateMapNoUndistortion( remote_handle64 handle, uint64 *mapPtr,
                                                         uint32_t camWidth, uint32_t camHeight,
                                                         uint32_t mapWidth, uint32_t mapHeight,
                                                         FadasIface_FadasRemapPipeline_e imgFormat,
                                                         uint8_t borderConst )
{
    AEEResult ret = AEE_SUCCESS;
    FadasRemapMap *map = FadasRemap_CreateMapNoUndistortion(
            camWidth, camHeight, mapWidth, mapHeight,
            g_MapImageConversion[static_cast<int>( imgFormat )], borderConst );
    if ( nullptr == map )
    {
        FARF( ERROR, "Null map pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        *mapPtr = reinterpret_cast<uint64>( map );
    }

    return ret;
}

AEEResult FadasIface_FadasRemap_DestroyMap( remote_handle64 handle, uint64 mapPtr )
{
    FadasRemapMap *map = reinterpret_cast<FadasRemapMap *>( mapPtr );

    FadasRemap_DestroyMap( map );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_CreateWorkers( remote_handle64 handle, uint64 *worker_ptr,
                                               uint32_t nThreads,
                                               FadasIface_FadasRemapPipeline_e imgFormat )
{
    AEEResult ret = AEE_SUCCESS;
    int32_t pThreadsAffinity[] = { 0, 1, 2, 3 };
    void *worker = FadasRemap_CreateWorkers( nThreads, pThreadsAffinity,
                                             g_MapImageConversion[static_cast<int>( imgFormat )] );
    if ( nullptr == worker )
    {
        FARF( ERROR, "Null worker pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        *worker_ptr = reinterpret_cast<uint64>( worker );
    }

    return ret;
}

AEEResult FadasIface_FadasRemap_DestroyWorkers( remote_handle64 handle, uint64 worker_ptr )
{
    void *worker = reinterpret_cast<void *>( worker_ptr );
    FadasRemap_DestroyWorkers( worker );

    return AEE_SUCCESS;
}

AEEResult FadasIface_FadasRemap_RunMT( remote_handle64 handle, const uint64 *workerPtrs,
                                       int workerPtrsLen, const uint64 *mapPtrs, int mapPtrsLen,
                                       const int32_t *srcFds, int srcFdsLen,
                                       const uint32_t *offsets, int offsetsLen,
                                       const FadasIface_FadasImgProps_t *srcProps, int srcPropsLen,
                                       int32_t dstFd, uint32_t dstLen,
                                       const FadasIface_FadasImgProps_t *dstProps,
                                       const FadasIface_FadasROI_t *dstROIs, int dstROIsLen,
                                       const FadasIface_FadasNormlzParams_t *normlz, int normlzLen )
{
    AEEResult ret = AEE_SUCCESS;
    dspContext_t *dspContext = (dspContext_t *) handle;
    const uint8_t *src[srcFdsLen];
    uint8_t *dst = (uint8_t *) FadasIface_GetBufPtr( dstFd );
    if ( nullptr == dst )
    {
        FARF( ERROR, "Null dst pointer!" );
        ret = AEE_EFAILED;
    }
    else if ( ( srcFdsLen != offsetsLen ) || ( srcFdsLen != srcPropsLen ) )
    {
        FARF( ERROR, "Fd length not equal to props length" );
        ret = AEE_EFAILED;
    }
    else
    {
        for ( int i = 0; i < srcFdsLen; i++ )
        {
            src[i] = (uint8_t *) FadasIface_GetBufPtr( srcFds[i] );
            if ( nullptr == src[i] )
            {
                FARF( ERROR, "Null src pointer!" );
                ret = AEE_EFAILED;
                break;
            }
            else
            {
                src[i] += offsets[i];
            }
        }
    }

    if ( AEE_SUCCESS == ret )
    {
        FadasError_e retVal;
        FadasImage_t srcImg = {};
        FadasImage_t dstImg = {};
        dstImg.bAllocated = false;
        dstImg.props.width = static_cast<uint32_t>( dstProps->width );
        dstImg.props.height = static_cast<uint32_t>( dstProps->height );
        dstImg.props.format = g_MapImageFormat[dstProps->format];
        memcpy( dstImg.props.stride, dstProps->stride, dstProps->numPlanes * sizeof( uint32_t ) );
        dstImg.props.numPlanes = dstProps->numPlanes;

        for ( int i = 0; i < srcFdsLen; i++ )
        {
            srcImg.bAllocated = false;
            srcImg.props.width = static_cast<uint32_t>( srcProps[i].width );
            srcImg.props.height = static_cast<uint32_t>( srcProps[i].height );
            srcImg.props.format = g_MapImageFormat[srcProps[i].format];
            memcpy( srcImg.props.stride, srcProps[i].stride,
                    srcProps[i].numPlanes * sizeof( uint32_t ) );
            srcImg.props.numPlanes = srcProps[i].numPlanes;
            void *worker = reinterpret_cast<void *>( workerPtrs[0] );
            if ( i < workerPtrsLen )
            {
                worker = reinterpret_cast<void *>( workerPtrs[i] );
            }
            FadasRemapMap *map = reinterpret_cast<FadasRemapMap *>( mapPtrs[0] );
            if ( i < mapPtrsLen )
            {
                map = reinterpret_cast<FadasRemapMap *>( mapPtrs[i] );
            }
            FadasROI_t roiStruct = {};
            roiStruct.x = dstROIs[i].x;
            roiStruct.y = dstROIs[i].y;
            roiStruct.width = dstROIs[i].width;
            roiStruct.height = dstROIs[i].height;
            srcImg.plane[0] = const_cast<uint8_t *>( src[i] );
            if ( FADAS_IMAGE_FORMAT_Y8UV8 == srcImg.props.format )
            {
                srcImg.plane[1] = const_cast<uint8_t *>(
                        src[i] + srcImg.props.stride[0] * srcProps[i].actualHeight[0] );
            }
            dstImg.plane[0] = dst + i * dstLen;
            uint32_t srcLen = srcImg.props.height * srcImg.props.stride[0];
            qurt_mem_cache_clean( (qurt_addr_t) src[i], srcLen, QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL,
                                  QURT_MEM_DCACHE );
            qurt_mutex_lock( &dspContext->mutex );
            if ( 3 == normlzLen )
            {
                FadasNormlzParams_t normlzParams[3];
                normlzParams[0].sub = normlz[0].sub;
                normlzParams[0].mul = normlz[0].mul;
                normlzParams[0].add = normlz[0].add;
                normlzParams[1].sub = normlz[1].sub;
                normlzParams[1].mul = normlz[1].mul;
                normlzParams[1].add = normlz[1].add;
                normlzParams[2].sub = normlz[2].sub;
                normlzParams[2].mul = normlz[2].mul;
                normlzParams[2].add = normlz[2].add;
                retVal = FadasRemap_RunMT( worker, map, &srcImg, &dstImg, &roiStruct, 1.0,
                                           normlzParams );
            }
            else
            {
                retVal = FadasRemap_RunMT( worker, map, &srcImg, &dstImg, &roiStruct );
            }
            qurt_mutex_unlock( &dspContext->mutex );

            if ( FADAS_ERROR_NONE != retVal )
            {
                FARF( ERROR, "Failed to do FadasRemap_RunMT" );
                ret = AEE_EOFFSET + retVal;
                break;
            }
        }
    }
    qurt_mem_cache_clean( (qurt_addr_t) dst, dstLen * srcFdsLen, QURT_MEM_CACHE_FLUSH_ALL,
                          QURT_MEM_DCACHE );

    return ret;
}

AEEResult FadasIface_mmap( remote_handle64 handle, int32_t bufFd, uint32_t bufSize )
{
    AEEResult ret = AEE_SUCCESS;
    void *buf = FadasIface_GetBufPtr( bufFd );
    if ( nullptr != buf )
    {
        FARF( ERROR, "Already used buf pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        int32_t prot = HAP_PROT_READ | HAP_PROT_WRITE;
        int32_t flags = 0;
        buf = HAP_mmap( NULL, bufSize, prot, flags, bufFd, 0 );
        if ( ( ( (void *) 0xFFFFFFFF ) == buf ) || ( nullptr == buf ) )
        {
            FARF( ERROR, "Null buf pointer!" );
            ret = AEE_EFAILED;
        }
    }

    return ret;
}

AEEResult FadasIface_munmap( remote_handle64 handle, int32_t bufFd, uint32_t bufSize )
{
    AEEResult ret = AEE_SUCCESS;
    void *buf = nullptr;
    ret = HAP_mmap_get( bufFd, (void **) &buf, NULL );
    if ( AEE_SUCCESS == ret )
    {
        int32_t err = -1;
        do
        {
            // decrement user count to 0
            err = HAP_mmap_put( bufFd );
        } while ( 0 == err );
        ret = HAP_munmap( buf, bufSize );
    }

    if ( AEE_SUCCESS != ret )
    {
        FARF( ERROR, "Failed to do FadasIface_munmap!" );
    }

    return ret;
}

AEEResult FadasIface_FadasRegBuf( remote_handle64 handle, FadasIface_FadasBufType_e bufType,
                                  int32_t bufFd, uint32_t bufSize, uint32_t bufOffset,
                                  uint32_t batch )
{
    AEEResult ret = AEE_SUCCESS;
    uint8_t *ptr = (uint8_t *) FadasIface_GetBufPtr( bufFd );
    uint32_t i;

    if ( nullptr == ptr )
    {
        FARF( ERROR, "Null bufFd pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        FadasError_e retVal;
        ptr += bufOffset;
        for ( i = 0; i < batch; i++ )
        {
            retVal = FadasRegBuf( g_MapBufType[bufType], ptr, bufSize );
            ptr += bufSize;
            if ( FADAS_ERROR_NONE != retVal )
            {
                FARF( ERROR, "Failed to do FadasRegBuf!" );
                ret = AEE_EFAILED;
                break;
            }
        }
    }

    return ret;
}

AEEResult FadasIface_FadasDeregBuf( remote_handle64 handle, int32_t bufFd, uint32_t bufSize,
                                    uint32_t bufOffset, uint32_t batch )
{
    AEEResult ret = AEE_SUCCESS;
    uint8_t *ptr = (uint8_t *) FadasIface_GetBufPtr( bufFd );
    uint32_t i;

    if ( nullptr == ptr )
    {
        FARF( ERROR, "Null bufFd pointer!" );
        ret = AEE_EFAILED;
    }
    else
    {
        FadasError_e retVal;
        ptr += bufOffset;
        for ( i = 0; i < batch; i++ )
        {
            retVal = FadasDeregBuf( ptr );
            ptr += bufSize;
            if ( FADAS_ERROR_NONE != retVal )
            {
                FARF( ERROR, "Failed to do FadasDeregBuf!" );
                ret = AEE_EFAILED;
                break;
            }
        }
    }

    return ret;
}


AEEResult FadasIface_PointPillarCreate( remote_handle64 handle, const FadasIface_Pt3D_t *pPlrSize,
                                        const FadasIface_Pt3D_t *pMinRange,
                                        const FadasIface_Pt3D_t *pMaxRange, uint32_t maxNumInPts,
                                        uint32_t numInFeatureDim, uint32_t maxNumPlrs,
                                        uint32_t maxNumPtsPerPlr, uint32_t numOutFeatureDim,
                                        uint64_t *phPreProc )
{
    AEEResult ret = AEE_SUCCESS;
    if ( ( nullptr == pPlrSize ) || ( nullptr == pMinRange ) || ( nullptr == pMaxRange ) ||
         ( nullptr == phPreProc ) )
    {
        FARF( ERROR, "PointPillarCreate with nullptr" );
        ret = AEE_EFAILED;
    }
    else
    {
        FadasPt_3Df32_t plrSize = { pPlrSize->x, pPlrSize->y, pPlrSize->z };
        FadasPt_3Df32_t minRange = { pMinRange->x, pMinRange->y, pMinRange->z };
        FadasPt_3Df32_t maxRange = { pMaxRange->x, pMaxRange->y, pMaxRange->z };

        *phPreProc = (uint64_t) FadasVM_PointPillar_Create(
                plrSize, minRange, maxRange, maxNumInPts, numInFeatureDim, maxNumPlrs,
                maxNumPtsPerPlr, numOutFeatureDim );
        if ( 0 == ( *phPreProc ) )
        {
            FARF( ERROR, "PointPillarCreate failed!" );
            ret = AEE_EFAILED;
        }
    }

    return ret;
}

AEEResult FadasIface_PointPillarRun( remote_handle64 handle, uint64_t hPreProc, uint32_t numPts,
                                     int32_t fdInPts, uint32_t inPtsOffset, uint32_t inPtsSize,
                                     int32_t fdOutPlrs, uint32_t outPlrsOffset,
                                     uint32_t outPlrsSize, int32_t fdOutFeature,
                                     uint32_t outFeatureOffset, uint32_t outFeatureSize,
                                     uint32_t *pNumOutPlrs )
{
    dspContext_t *dspContext = (dspContext_t *) handle;
    AEEResult ret = AEE_SUCCESS;
    const float32_t *pInPtsData = (const float32_t *) FadasIface_GetBufPtr( fdInPts );
    FadasVM_PointPillar_t *pOutPlrsData =
            (FadasVM_PointPillar_t *) FadasIface_GetBufPtr( fdOutPlrs );
    float32_t *pOutFeatureData = (float32_t *) FadasIface_GetBufPtr( fdOutFeature );

    if ( ( nullptr == dspContext ) || ( nullptr == pInPtsData ) || ( nullptr == pOutPlrsData ) ||
         ( nullptr == pOutFeatureData ) || ( 0 == hPreProc ) )
    {
        FARF( ERROR, "FadasIface_PointPillarRun with nullptr" );
        ret = AEE_EFAILED;
    }
    else
    {
        pInPtsData = (const float32_t *) ( ( (uint8_t *) pInPtsData ) + inPtsOffset );
        pOutPlrsData = (FadasVM_PointPillar_t *) ( ( (uint8_t *) pOutPlrsData ) + outPlrsOffset );
        pOutFeatureData = (float32_t *) ( ( (uint8_t *) pOutFeatureData ) + outFeatureOffset );
        qurt_mem_cache_clean( (qurt_addr_t) pInPtsData, inPtsSize,
                              QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, QURT_MEM_DCACHE );
        qurt_mutex_lock( &dspContext->mutex );
        FadasError_e error = FadasVM_PointPillar_Run( (void *) hPreProc, numPts, pInPtsData,
                                                      pOutPlrsData, pOutFeatureData, pNumOutPlrs );
        qurt_mutex_unlock( &dspContext->mutex );
        if ( FADAS_ERROR_NONE != error )
        {
            FARF( ERROR, "Failed to do FadasVM_PointPillar_Run: ret=%d", error );
            ret = AEE_EOFFSET + error;
        }
        else
        {
            qurt_mem_cache_clean( (qurt_addr_t) pOutPlrsData, outPlrsSize, QURT_MEM_CACHE_FLUSH_ALL,
                                  QURT_MEM_DCACHE );
            qurt_mem_cache_clean( (qurt_addr_t) pOutFeatureData, outFeatureSize,
                                  QURT_MEM_CACHE_FLUSH_ALL, QURT_MEM_DCACHE );
        }
    }

    return ret;
}

AEEResult FadasIface_PointPillarDestroy( remote_handle64 handle, uint64_t hPreProc )
{
    AEEResult ret = AEE_SUCCESS;

    FadasError_e error = FadasVM_PointPillar_Destroy( (void *) hPreProc );
    if ( FADAS_ERROR_NONE != error )
    {
        FARF( ERROR, "Failed to do FadasVM_PointPillar_Destroy: ret=%d", error );
        ret = AEE_EOFFSET + error;
    }

    return ret;
}


AEEResult FadasIface_ExtractBBoxCreate( remote_handle64 handle, uint32_t maxNumInPts,
                                        uint32_t numInFeatureDim, uint32_t maxNumDetOut,
                                        uint32_t numClass, const FadasIface_Grid2D_t *pGrid,
                                        float threshScore, float threshIOU, float minCentreX,
                                        float minCentreY, float minCentreZ, float maxCentreX,
                                        float maxCentreY, float maxCentreZ,
                                        const uint8_t *labelSelect, int labelSelectLen,
                                        uint32_t maxNumFilter, uint64_t *phPostProc )
{
    AEEResult ret = AEE_SUCCESS;
    if ( ( nullptr == pGrid ) || ( nullptr == phPostProc ) )
    {
        FARF( ERROR, "ExtractBBoxCreate with nullptr" );
        ret = AEE_EFAILED;
    }
    else
    {
        Fadas2DGrid_t grid = { { pGrid->tlX, pGrid->tlY },
                               { pGrid->brX, pGrid->brY },
                               pGrid->cellSizeX,
                               pGrid->cellSizeY };

        Fadas3DBBoxInitParams_t bboxInitParams = { 0 };

        bboxInitParams.strideInPts = numInFeatureDim * sizeof( float32_t );
        bboxInitParams.numClass = numClass;
        bboxInitParams.grid = grid;
        bboxInitParams.maxNumInPts = maxNumInPts;
        bboxInitParams.maxNumDetOut = maxNumDetOut;
        bboxInitParams.threshScore = threshScore;
        bboxInitParams.threshIOU = threshIOU;
        if ( nullptr != labelSelect )
        {
            bboxInitParams.filterParams.minCentre.x = minCentreX;
            bboxInitParams.filterParams.minCentre.y = minCentreY;
            bboxInitParams.filterParams.minCentre.z = minCentreZ;
            bboxInitParams.filterParams.maxCentre.x = maxCentreX;
            bboxInitParams.filterParams.maxCentre.y = maxCentreY;
            bboxInitParams.filterParams.maxCentre.z = maxCentreZ;
            bboxInitParams.filterParams.maxNumFilter = maxNumFilter;
            bboxInitParams.filterParams.labelSelect = (bool *) labelSelect;
        }

        *phPostProc = (uint64_t) FadasVM_ExtractBBox_Create( &bboxInitParams );
        if ( 0 == ( *phPostProc ) )
        {
            FARF( ERROR, "ExtractBBoxCreate failed!" );
            FARF( ERROR,
                  "plrSize=[%.2f %.2f], minRange=[%.2f %.2f], maxRange=[%.2f %.2f], "
                  "pcd=%ux%u, %u class, thresh=[%.2f %.2f], max=%u, numFilter=%d",
                  pGrid->cellSizeX, pGrid->cellSizeY, pGrid->tlX, pGrid->tlY, pGrid->brX,
                  pGrid->brY, maxNumInPts, numInFeatureDim, numClass, threshScore, threshIOU,
                  maxNumDetOut, labelSelectLen );
            ret = AEE_EFAILED;
        }
    }

    return ret;
}

AEEResult FadasIface_ExtractBBoxRun( remote_handle64 handle, uint64_t hPostProc, uint32_t numPts,
                                     const int32_t *fds, int fdsLen, const uint32_t *offsets,
                                     int offsetsLen, const uint32_t *sizes, int sizesLen,
                                     uint8_t bMapPtsToBBox, uint8_t bBBoxFilter,
                                     uint32_t *pNumDetOut )
{
    dspContext_t *dspContext = (dspContext_t *) handle;
    AEEResult ret = AEE_SUCCESS;
    FadasError_e error = FADAS_ERROR_UNKNOWN;
    if ( ( nullptr == dspContext ) || ( nullptr == fds ) || ( nullptr == offsets ) ||
         ( nullptr == sizes ) || ( PLRPOST_NUM_INPUTS != fdsLen ) ||
         ( PLRPOST_NUM_INPUTS != offsetsLen ) || ( PLRPOST_NUM_INPUTS != sizesLen ) ||
         ( nullptr == pNumDetOut ) )
    {
        FARF( ERROR, "FadasIface_ExtractBBoxRun with bad arguments" );
        ret = AEE_EFAILED;
    }
    else
    {
        float32_t *pInPts = (float32_t *) FadasIface_GetBufPtr( fds[PLRPOST_IN_PTS] );
        float32_t *pHeatmap = (float32_t *) FadasIface_GetBufPtr( fds[PLRPOST_IN_HEATMAP] );
        float32_t *pXY = (float32_t *) FadasIface_GetBufPtr( fds[PLRPOST_IN_XY] );
        float32_t *pZ = (float32_t *) FadasIface_GetBufPtr( fds[PLRPOST_IN_Z] );
        float32_t *pSize = (float32_t *) FadasIface_GetBufPtr( fds[PLRPOST_IN_SIZE] );
        float32_t *pTheta = (float32_t *) FadasIface_GetBufPtr( fds[PLRPOST_IN_THETA] );
        FadasCuboidf32_t *pBBoxList =
                (FadasCuboidf32_t *) FadasIface_GetBufPtr( fds[PLRPOST_OUT_BBOX] );
        uint32_t *pLabelsOut = (uint32_t *) FadasIface_GetBufPtr( fds[PLRPOST_OUT_LABELS] );
        float32_t *pScoresOut = (float32_t *) FadasIface_GetBufPtr( fds[PLRPOST_OUT_SCORES] );
        Fadas3DBBoxMetadata_t *pMetadataOut =
                (Fadas3DBBoxMetadata_t *) FadasIface_GetBufPtr( fds[PLRPOST_OUT_METADATA] );
        if ( ( nullptr == pInPts ) || ( nullptr == pHeatmap ) || ( nullptr == pXY ) ||
             ( nullptr == pZ ) || ( nullptr == pSize ) || ( nullptr == pTheta ) ||
             ( nullptr == pBBoxList ) || ( nullptr == pLabelsOut ) || ( nullptr == pScoresOut ) ||
             ( nullptr == pMetadataOut ) )
        {
            FARF( ERROR, "FadasIface_ExtractBBoxRun with nullptr" );
            ret = AEE_EFAILED;
        }
        else
        {
            pInPts = (float32_t *) ( ( (uint8_t *) pInPts ) + offsets[PLRPOST_IN_PTS] );
            pHeatmap = (float32_t *) ( ( (uint8_t *) pHeatmap ) + offsets[PLRPOST_IN_HEATMAP] );
            pXY = (float32_t *) ( ( (uint8_t *) pXY ) + offsets[PLRPOST_IN_XY] );
            pZ = (float32_t *) ( ( (uint8_t *) pZ ) + offsets[PLRPOST_IN_Z] );
            pSize = (float32_t *) ( ( (uint8_t *) pSize ) + offsets[PLRPOST_IN_SIZE] );
            pTheta = (float32_t *) ( ( (uint8_t *) pTheta ) + offsets[PLRPOST_IN_THETA] );
            pBBoxList =
                    (FadasCuboidf32_t *) ( ( (uint8_t *) pBBoxList ) + offsets[PLRPOST_OUT_BBOX] );
            pLabelsOut = (uint32_t *) ( ( (uint8_t *) pLabelsOut ) + offsets[PLRPOST_OUT_LABELS] );
            pScoresOut = (float32_t *) ( ( (uint8_t *) pScoresOut ) + offsets[PLRPOST_OUT_SCORES] );
            pMetadataOut = (Fadas3DBBoxMetadata_t *) ( ( (uint8_t *) pMetadataOut ) +
                                                       offsets[PLRPOST_OUT_METADATA] );

            Fadas3DRPNBufs_t rpnBuf;
            Fadas3DBBoxBufs_t outBuf;

            rpnBuf.pHeatmap = pHeatmap;
            rpnBuf.pXY = pXY;
            rpnBuf.pZ = pZ;
            rpnBuf.pSize = pSize;
            rpnBuf.pTheta = pTheta;

            outBuf.pBBoxList = pBBoxList;
            outBuf.pLabels = pLabelsOut;
            outBuf.pScores = pScoresOut;
            outBuf.pMetadata = pMetadataOut;

            qurt_mem_cache_clean( (qurt_addr_t) pInPts, sizes[PLRPOST_IN_PTS],
                                  QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, QURT_MEM_DCACHE );
            qurt_mem_cache_clean( (qurt_addr_t) pHeatmap, sizes[PLRPOST_IN_HEATMAP],
                                  QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, QURT_MEM_DCACHE );
            qurt_mem_cache_clean( (qurt_addr_t) pXY, sizes[PLRPOST_IN_XY],
                                  QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, QURT_MEM_DCACHE );
            qurt_mem_cache_clean( (qurt_addr_t) pZ, sizes[PLRPOST_IN_Z],
                                  QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, QURT_MEM_DCACHE );
            qurt_mem_cache_clean( (qurt_addr_t) pSize, sizes[PLRPOST_IN_SIZE],
                                  QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, QURT_MEM_DCACHE );
            qurt_mem_cache_clean( (qurt_addr_t) pTheta, sizes[PLRPOST_IN_THETA],
                                  QURT_MEM_CACHE_FLUSH_INVALIDATE_ALL, QURT_MEM_DCACHE );
            *pNumDetOut = 0;
            qurt_mutex_lock( &dspContext->mutex );
            error = FadasVM_ExtractBBox_Run( (void *) hPostProc, numPts, pInPts, rpnBuf, outBuf,
                                             pNumDetOut, (bool) bMapPtsToBBox, (bool) bBBoxFilter );
            qurt_mutex_unlock( &dspContext->mutex );
            if ( FADAS_ERROR_NONE != error )
            {

                FARF( ERROR, "Failed to do FadasVM_PointPillar_Run: ret=%d", error );
                for ( int i = 0; i < PLRPOST_NUM_INPUTS; i++ )
                {
                    FARF( ERROR, "[%d]: fd=%d size=%u offset=%u", i, fds[i], sizes[i], offsets[i] );
                }
                ret = AEE_EOFFSET + error;
            }
            else
            {
                qurt_mem_cache_clean( (qurt_addr_t) pBBoxList, sizes[PLRPOST_OUT_BBOX],
                                      QURT_MEM_CACHE_FLUSH_ALL, QURT_MEM_DCACHE );
                qurt_mem_cache_clean( (qurt_addr_t) pLabelsOut, sizes[PLRPOST_OUT_LABELS],
                                      QURT_MEM_CACHE_FLUSH_ALL, QURT_MEM_DCACHE );
                qurt_mem_cache_clean( (qurt_addr_t) pScoresOut, sizes[PLRPOST_OUT_SCORES],
                                      QURT_MEM_CACHE_FLUSH_ALL, QURT_MEM_DCACHE );
                qurt_mem_cache_clean( (qurt_addr_t) pMetadataOut, sizes[PLRPOST_OUT_METADATA],
                                      QURT_MEM_CACHE_FLUSH_ALL, QURT_MEM_DCACHE );
            }
        }
    }

    return ret;
}

AEEResult FadasIface_ExtractBBoxDestroy( remote_handle64 handle, uint64_t hPostProc )
{
    AEEResult ret = AEE_SUCCESS;

    FadasError_e error = FadasVM_ExtractBBox_Destroy( (void *) hPostProc );
    if ( FADAS_ERROR_NONE != error )
    {
        FARF( ERROR, "Failed to do FadasVM_ExtractBBox_Destroy: ret=%d", error );
        ret = AEE_EOFFSET + error;
    }

    return ret;
}
