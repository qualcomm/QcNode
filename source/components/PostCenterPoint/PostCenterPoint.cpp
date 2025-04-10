// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/component/PostCenterPoint.hpp"
#include <cmath>

namespace ridehal
{
namespace component
{

PostCenterPoint::PostCenterPoint() {}

PostCenterPoint::~PostCenterPoint() {}

RideHalError_e PostCenterPoint::Init( const char *pName, const PostCenterPoint_Config_t *pConfig,
                                      Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    bool bIFInitOK = false;
    bool bFadasInitOK = false;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        bIFInitOK = true;
        if ( nullptr == pConfig )
        {
            RIDEHAL_ERROR( "pConfig is nullptr!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            m_config = *pConfig;
            if ( ( 0 == m_config.stride ) || ( 0 != ( m_config.stride % 2 ) ) )
            {
                RIDEHAL_ERROR( "stride is invalid!" );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                if ( m_config.stride > 2 )
                { /* Now the FadasVM Extract BBox only support stride = 2, so for the other case,
                     further divide the pillar size to support large stride */
                    m_config.pillarXSize /= ( m_config.stride / 2 );
                    m_config.pillarYSize /= ( m_config.stride / 2 );
                    m_config.stride = 2;
                }
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_plrPost.Init( m_config.processor, pName, level );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to init FadasPlrPost!" );
        }
        else
        {
            bFadasInitOK = TRUE;
            ret = m_plrPost.SetParams(
                    m_config.pillarXSize, m_config.pillarYSize, m_config.minXRange,
                    m_config.minYRange, m_config.maxXRange, m_config.maxYRange, m_config.numClass,
                    m_config.maxNumInPts, m_config.numInFeatureDim, m_config.maxNumDetOut,
                    m_config.threshScore, m_config.threshIOU, m_config.bMapPtsToBBox );
            if ( ( RIDEHAL_ERROR_NONE == ret ) && ( true == m_config.bBBoxFilter ) )
            {
                ret = m_plrPost.SetFilterParams(
                        m_config.filterParams.minCentreX, m_config.filterParams.minCentreY,
                        m_config.filterParams.minCentreZ, m_config.filterParams.maxCentreX,
                        m_config.filterParams.maxCentreY, m_config.filterParams.maxCentreZ,
                        m_config.filterParams.labelSelect, m_config.filterParams.maxNumFilter );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t tensorProps;
        tensorProps.type = RIDEHAL_TENSOR_TYPE_FLOAT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.dims[1] = sizeof( FadasCuboidf32_t ) / sizeof( float );
        tensorProps.numDims = 2;
        ret = m_BBoxList.Allocate( &tensorProps );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t tensorProps;
        tensorProps.type = RIDEHAL_TENSOR_TYPE_UINT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.numDims = 1;
        ret = m_labels.Allocate( &tensorProps );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t tensorProps;
        tensorProps.type = RIDEHAL_TENSOR_TYPE_FLOAT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.numDims = 1;
        ret = m_scores.Allocate( &tensorProps );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t tensorProps;
        tensorProps.type = RIDEHAL_TENSOR_TYPE_UINT_8;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.dims[1] = sizeof( FadasPlr3DBBoxMetadata_t );
        tensorProps.numDims = 2;
        ret = m_metadata.Allocate( &tensorProps );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_SharedBuffer_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
        ret = RegisterBuffersToFadas( buffers, 4, FADAS_BUF_TYPE_OUT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_plrPost.CreatePostProc();
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            /* do deregister */
            RideHal_SharedBuffer_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
            (void) DeRegisterBuffersToFadas( buffers, 4 );
        }
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        if ( bIFInitOK )
        { /* do error clean up */
            RIDEHAL_ERROR( "PlrPost Init failed: %d!", ret );
            /* do clean up */
            if ( nullptr != m_BBoxList.buffer.pData )
            {
                (void) m_BBoxList.Free();
            }
            if ( nullptr != m_labels.buffer.pData )
            {
                (void) m_labels.Free();
            }
            if ( nullptr != m_scores.buffer.pData )
            {
                (void) m_scores.Free();
            }
            if ( nullptr != m_metadata.buffer.pData )
            {
                (void) m_metadata.Free();
            }

            if ( bFadasInitOK )
            {
                (void) m_plrPost.Deinit();
            }

            (void) ComponentIF::Deinit();
        }
    }
    else
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
        m_height = (uint32_t) std::round( ( m_config.maxYRange - m_config.minYRange ) /
                                          m_config.pillarYSize / m_config.stride );
        m_width = (uint32_t) std::round( ( m_config.maxXRange - m_config.minXRange ) /
                                         m_config.pillarXSize / m_config.stride );
        RIDEHAL_INFO( "PlrPost with feature dim [%u, %u]", m_height, m_width );
    }


    return ret;
}

RideHalError_e PostCenterPoint::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        RIDEHAL_ERROR( "Start is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e PostCenterPoint::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else
    {
        RIDEHAL_ERROR( "Stop is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e PostCenterPoint::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "Deinit is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        RideHalError_e ret2;
        /* do deregister */
        RideHal_SharedBuffer_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
        ret2 = DeRegisterBuffers( buffers, 4 );
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPost deregister buffers failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_BBoxList.Free();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPost Free m_BBoxList failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_labels.Free();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPost Free m_labels failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_scores.Free();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPost Free m_scores failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_metadata.Free();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPost Free m_metadata failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_plrPost.DestroyPostProc();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPost DestroyPostProc failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_plrPost.Deinit();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "PlrPost Deinit failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            ret = ret2;
        }
    }

    return ret;
}

RideHalError_e PostCenterPoint::RegisterBuffersToFadas( const RideHal_SharedBuffer_t *pBuffers,
                                                        uint32_t numBuffers,
                                                        FadasBufType_e bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    for ( uint32_t i = 0; i < numBuffers; i++ )
    {
        const RideHal_SharedBuffer_t *pBuf = &pBuffers[i];
        if ( RIDEHAL_BUFFER_TYPE_TENSOR == pBuf->type )
        {
            int32_t fd = m_plrPost.RegBuf( pBuf, bufferType );
            if ( 0 > fd )
            {
                RIDEHAL_ERROR( "Failed to register buffer[%d]!", i );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_ERROR( "buffer[%d] is not tensor!", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            break;
        }
    }

    return ret;
}

RideHalError_e PostCenterPoint::RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                                 uint32_t numBuffers, FadasBufType_e bufferType )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "PlrPost component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        ret = RegisterBuffersToFadas( pBuffers, numBuffers, bufferType );
    }

    return ret;
}

RideHalError_e PostCenterPoint::DeRegisterBuffersToFadas( const RideHal_SharedBuffer_t *pBuffers,
                                                          uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    for ( uint32_t i = 0; i < numBuffers; i++ )
    {
        const RideHal_SharedBuffer_t *pBuf = &pBuffers[i];
        if ( RIDEHAL_BUFFER_TYPE_TENSOR == pBuf->type )
        {
            m_plrPost.DeregBuf( pBuf->data() );
        }
        else
        {
            RIDEHAL_ERROR( "buffer[%d] is not tensor!", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            break;
        }
    }

    return ret;
}

RideHalError_e PostCenterPoint::DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                                   uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "PlrPost component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        ret = DeRegisterBuffersToFadas( pBuffers, numBuffers );
    }

    return ret;
}

RideHalError_e PostCenterPoint::Execute( const RideHal_SharedBuffer_t *pHeatmap,
                                         const RideHal_SharedBuffer_t *pXY,
                                         const RideHal_SharedBuffer_t *pZ,
                                         const RideHal_SharedBuffer_t *pSize,
                                         const RideHal_SharedBuffer_t *pTheta,
                                         const RideHal_SharedBuffer_t *pInPts,
                                         RideHal_SharedBuffer_t *pDetections )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t numDetOut = 0;
    PostCenterPoint_Object3D_t *pObj;
    FadasCuboidf32_t *pBBoxList;
    uint32_t *pLabels;
    float32_t *pScores;
    FadasPlr3DBBoxMetadata_t *pMetadata;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "Execute is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pHeatmap )
    {
        RIDEHAL_ERROR( "pHeatmap is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pHeatmap->type ) ||
              ( nullptr == pHeatmap->buffer.pData ) || ( 4 != pHeatmap->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pHeatmap->tensorProps.type ) ||
              ( 1 != pHeatmap->tensorProps.dims[0] ) ||
              ( m_height != pHeatmap->tensorProps.dims[1] ) ||
              ( m_width != pHeatmap->tensorProps.dims[2] ) ||
              ( m_config.numClass != pHeatmap->tensorProps.dims[3] ) )
    {
        RIDEHAL_ERROR( "pHeatmap is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pXY )
    {
        RIDEHAL_ERROR( "pXY is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pXY->type ) || ( nullptr == pXY->buffer.pData ) ||
              ( 4 != pXY->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pXY->tensorProps.type ) ||
              ( 1 != pXY->tensorProps.dims[0] ) || ( m_height != pXY->tensorProps.dims[1] ) ||
              ( m_width != pXY->tensorProps.dims[2] ) || ( 2 != pXY->tensorProps.dims[3] ) )
    {
        RIDEHAL_ERROR( "pXY is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pZ )
    {
        RIDEHAL_ERROR( "pZ is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pZ->type ) || ( nullptr == pZ->buffer.pData ) ||
              ( 4 != pZ->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pZ->tensorProps.type ) ||
              ( 1 != pZ->tensorProps.dims[0] ) || ( m_height != pZ->tensorProps.dims[1] ) ||
              ( m_width != pZ->tensorProps.dims[2] ) || ( 1 != pZ->tensorProps.dims[3] ) )
    {
        RIDEHAL_ERROR( "pZ is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pSize )
    {
        RIDEHAL_ERROR( "pSize is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pSize->type ) || ( nullptr == pSize->buffer.pData ) ||
              ( 4 != pSize->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pSize->tensorProps.type ) ||
              ( 1 != pSize->tensorProps.dims[0] ) || ( m_height != pSize->tensorProps.dims[1] ) ||
              ( m_width != pSize->tensorProps.dims[2] ) || ( 3 != pSize->tensorProps.dims[3] ) )
    {
        RIDEHAL_ERROR( "pSize is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pTheta )
    {
        RIDEHAL_ERROR( "pTheta is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pTheta->type ) ||
              ( nullptr == pTheta->buffer.pData ) || ( 4 != pTheta->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pTheta->tensorProps.type ) ||
              ( 1 != pTheta->tensorProps.dims[0] ) || ( m_height != pTheta->tensorProps.dims[1] ) ||
              ( m_width != pTheta->tensorProps.dims[2] ) || ( 2 != pTheta->tensorProps.dims[3] ) )
    {
        RIDEHAL_ERROR( "pTheta is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pInPts )
    {
        RIDEHAL_ERROR( "pInPts is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pInPts->type ) ||
              ( nullptr == pInPts->buffer.pData ) || ( 2 != pInPts->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pInPts->tensorProps.type ) ||
              ( m_config.numInFeatureDim != pInPts->tensorProps.dims[1] ) )
    {
        RIDEHAL_ERROR( "pInPts is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pDetections )
    {
        RIDEHAL_ERROR( "pDetections is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pDetections->type ) ||
              ( nullptr == pDetections->buffer.pData ) ||
              ( 2 != pDetections->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_FLOAT_32 != pDetections->tensorProps.type ) ||
              ( m_config.maxNumDetOut != pDetections->tensorProps.dims[0] ) ||
              ( POSTCENTERPOINT_OBJECT_3D_DIM != pDetections->tensorProps.dims[1] ) )
    {
        RIDEHAL_ERROR( "pDetections is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else
    {
        ret = m_plrPost.ExtractBBoxRun( pHeatmap, pXY, pZ, pSize, pTheta, pInPts, &m_BBoxList,
                                        &m_labels, &m_scores, &m_metadata, &numDetOut );

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            pObj = (PostCenterPoint_Object3D_t *) pDetections->data();
            pBBoxList = (FadasCuboidf32_t *) m_BBoxList.data();
            pLabels = (uint32_t *) m_labels.data();
            pScores = (float32_t *) m_scores.data();
            pMetadata = (FadasPlr3DBBoxMetadata_t *) m_metadata.data();
            pDetections->tensorProps.dims[0] = numDetOut;
            RIDEHAL_DEBUG( "number of detections %" PRIu32, numDetOut );
            for ( uint32_t i = 0; i < numDetOut; i++ )
            {
                pObj->label = pLabels[i];
                pObj->score = pScores[i];
                pObj->x = pBBoxList[i].x;
                pObj->y = pBBoxList[i].y;
                pObj->z = pBBoxList[i].z;
                pObj->length = pBBoxList[i].length;
                pObj->width = pBBoxList[i].width;
                pObj->height = pBBoxList[i].height;
                pObj->theta = pBBoxList[i].theta;
                if ( true == m_config.bMapPtsToBBox )
                {
                    pObj->meanPtX = pMetadata[i].meanPt.x;
                    pObj->meanPtY = pMetadata[i].meanPt.y;
                    pObj->meanPtZ = pMetadata[i].meanPt.z;
                    pObj->numPts = pMetadata[i].numPts;
                    pObj->meanIntensity = pMetadata[i].meanIntensity;
                }
                else
                {
                    pObj->meanPtX = 0;
                    pObj->meanPtY = 0;
                    pObj->meanPtZ = 0;
                    pObj->numPts = 0;
                    pObj->meanIntensity = 0;
                }
                RIDEHAL_DEBUG( "class=%d score=%.3f bbox=[%.3f %.3f %.3f %.3f %.3f %.3f] "
                               "theta=%.3f, mean=[%.3f %.3f %.3f %.3f]x%u",
                               pObj->label, pObj->score, pObj->x, pObj->y, pObj->z, pObj->length,
                               pObj->width, pObj->height, pObj->theta, pObj->meanPtX, pObj->meanPtY,
                               pObj->meanPtZ, pObj->meanIntensity, pObj->numPts );
                pObj++;
            }
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
