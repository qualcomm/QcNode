// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/component/PostCenterPoint.hpp"
#include <cmath>

namespace QC
{
namespace component
{

PostCenterPoint::PostCenterPoint() {}

PostCenterPoint::~PostCenterPoint() {}

QCStatus_e PostCenterPoint::Init( const char *pName, const PostCenterPoint_Config_t *pConfig,
                                  Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;
    bool bIFInitOK = false;
    bool bFadasInitOK = false;

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK == ret )
    {
        bIFInitOK = true;
        if ( nullptr == pConfig )
        {
            QC_ERROR( "pConfig is nullptr!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            m_config = *pConfig;
            if ( ( 0 == m_config.stride ) || ( 0 != ( m_config.stride % 2 ) ) )
            {
                QC_ERROR( "stride is invalid!" );
                ret = QC_STATUS_BAD_ARGUMENTS;
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

    if ( QC_STATUS_OK == ret )
    {
        ret = m_plrPost.Init( m_config.processor, pName, level );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to init FadasPlrPost!" );
        }
        else
        {
            bFadasInitOK = TRUE;
            ret = m_plrPost.SetParams(
                    m_config.pillarXSize, m_config.pillarYSize, m_config.minXRange,
                    m_config.minYRange, m_config.maxXRange, m_config.maxYRange, m_config.numClass,
                    m_config.maxNumInPts, m_config.numInFeatureDim, m_config.maxNumDetOut,
                    m_config.threshScore, m_config.threshIOU, m_config.bMapPtsToBBox );
            if ( ( QC_STATUS_OK == ret ) && ( true == m_config.bBBoxFilter ) )
            {
                ret = m_plrPost.SetFilterParams(
                        m_config.filterParams.minCentreX, m_config.filterParams.minCentreY,
                        m_config.filterParams.minCentreZ, m_config.filterParams.maxCentreX,
                        m_config.filterParams.maxCentreY, m_config.filterParams.maxCentreZ,
                        m_config.filterParams.labelSelect, m_config.filterParams.maxNumFilter );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t tensorProps;
        tensorProps.type = QC_TENSOR_TYPE_FLOAT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.dims[1] = sizeof( FadasCuboidf32_t ) / sizeof( float );
        tensorProps.numDims = 2;
        ret = m_BBoxList.Allocate( &tensorProps );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t tensorProps;
        tensorProps.type = QC_TENSOR_TYPE_UINT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.numDims = 1;
        ret = m_labels.Allocate( &tensorProps );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t tensorProps;
        tensorProps.type = QC_TENSOR_TYPE_FLOAT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.numDims = 1;
        ret = m_scores.Allocate( &tensorProps );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t tensorProps;
        tensorProps.type = QC_TENSOR_TYPE_UINT_8;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.dims[1] = sizeof( FadasPlr3DBBoxMetadata_t );
        tensorProps.numDims = 2;
        ret = m_metadata.Allocate( &tensorProps );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCSharedBuffer_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
        ret = RegisterBuffersToFadas( buffers, 4, FADAS_BUF_TYPE_OUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_plrPost.CreatePostProc();
        if ( QC_STATUS_OK != ret )
        {
            /* do deregister */
            QCSharedBuffer_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
            (void) DeRegisterBuffersToFadas( buffers, 4 );
        }
    }

    if ( QC_STATUS_OK != ret )
    {
        if ( bIFInitOK )
        { /* do error clean up */
            QC_ERROR( "PlrPost Init failed: %d!", ret );
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
        m_state = QC_OBJECT_STATE_READY;
        m_height = (uint32_t) std::round( ( m_config.maxYRange - m_config.minYRange ) /
                                          m_config.pillarYSize / m_config.stride );
        m_width = (uint32_t) std::round( ( m_config.maxXRange - m_config.minXRange ) /
                                         m_config.pillarXSize / m_config.stride );
        QC_INFO( "PlrPost with feature dim [%u, %u]", m_height, m_width );
    }


    return ret;
}

QCStatus_e PostCenterPoint::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "Start is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e PostCenterPoint::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "Stop is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e PostCenterPoint::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Deinit is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        QCStatus_e ret2;
        /* do deregister */
        QCSharedBuffer_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
        ret2 = DeRegisterBuffers( buffers, 4 );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost deregister buffers failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_BBoxList.Free();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_BBoxList failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_labels.Free();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_labels failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_scores.Free();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_scores failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_metadata.Free();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_metadata failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_plrPost.DestroyPostProc();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost DestroyPostProc failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_plrPost.Deinit();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Deinit failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = ComponentIF::Deinit();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "Deinit ComponentIF failed!" );
            ret = ret2;
        }
    }

    return ret;
}

QCStatus_e PostCenterPoint::RegisterBuffersToFadas( const QCSharedBuffer_t *pBuffers,
                                                    uint32_t numBuffers, FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( uint32_t i = 0; i < numBuffers; i++ )
    {
        const QCSharedBuffer_t *pBuf = &pBuffers[i];
        if ( QC_BUFFER_TYPE_TENSOR == pBuf->type )
        {
            int32_t fd = m_plrPost.RegBuf( pBuf, bufferType );
            if ( 0 > fd )
            {
                QC_ERROR( "Failed to register buffer[%d]!", i );
                ret = QC_STATUS_FAIL;
            }
        }
        else
        {
            QC_ERROR( "buffer[%d] is not tensor!", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        if ( QC_STATUS_OK != ret )
        {
            break;
        }
    }

    return ret;
}

QCStatus_e PostCenterPoint::RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers,
                                             FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "PlrPost component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        ret = RegisterBuffersToFadas( pBuffers, numBuffers, bufferType );
    }

    return ret;
}

QCStatus_e PostCenterPoint::DeRegisterBuffersToFadas( const QCSharedBuffer_t *pBuffers,
                                                      uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( uint32_t i = 0; i < numBuffers; i++ )
    {
        const QCSharedBuffer_t *pBuf = &pBuffers[i];
        if ( QC_BUFFER_TYPE_TENSOR == pBuf->type )
        {
            m_plrPost.DeregBuf( pBuf->data() );
        }
        else
        {
            QC_ERROR( "buffer[%d] is not tensor!", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        if ( QC_STATUS_OK != ret )
        {
            break;
        }
    }

    return ret;
}

QCStatus_e PostCenterPoint::DeRegisterBuffers( const QCSharedBuffer_t *pBuffers,
                                               uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "PlrPost component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        ret = DeRegisterBuffersToFadas( pBuffers, numBuffers );
    }

    return ret;
}

QCStatus_e PostCenterPoint::Execute( const QCSharedBuffer_t *pHeatmap, const QCSharedBuffer_t *pXY,
                                     const QCSharedBuffer_t *pZ, const QCSharedBuffer_t *pSize,
                                     const QCSharedBuffer_t *pTheta, const QCSharedBuffer_t *pInPts,
                                     QCSharedBuffer_t *pDetections )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint32_t numDetOut = 0;
    PostCenterPoint_Object3D_t *pObj;
    FadasCuboidf32_t *pBBoxList;
    uint32_t *pLabels;
    float32_t *pScores;
    FadasPlr3DBBoxMetadata_t *pMetadata;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Execute is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pHeatmap )
    {
        QC_ERROR( "pHeatmap is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pHeatmap->type ) ||
              ( nullptr == pHeatmap->buffer.pData ) || ( 4 != pHeatmap->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pHeatmap->tensorProps.type ) ||
              ( 1 != pHeatmap->tensorProps.dims[0] ) ||
              ( m_height != pHeatmap->tensorProps.dims[1] ) ||
              ( m_width != pHeatmap->tensorProps.dims[2] ) ||
              ( m_config.numClass != pHeatmap->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pHeatmap is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pXY )
    {
        QC_ERROR( "pXY is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pXY->type ) || ( nullptr == pXY->buffer.pData ) ||
              ( 4 != pXY->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pXY->tensorProps.type ) ||
              ( 1 != pXY->tensorProps.dims[0] ) || ( m_height != pXY->tensorProps.dims[1] ) ||
              ( m_width != pXY->tensorProps.dims[2] ) || ( 2 != pXY->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pXY is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pZ )
    {
        QC_ERROR( "pZ is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pZ->type ) || ( nullptr == pZ->buffer.pData ) ||
              ( 4 != pZ->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pZ->tensorProps.type ) ||
              ( 1 != pZ->tensorProps.dims[0] ) || ( m_height != pZ->tensorProps.dims[1] ) ||
              ( m_width != pZ->tensorProps.dims[2] ) || ( 1 != pZ->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pZ is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pSize )
    {
        QC_ERROR( "pSize is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pSize->type ) || ( nullptr == pSize->buffer.pData ) ||
              ( 4 != pSize->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pSize->tensorProps.type ) ||
              ( 1 != pSize->tensorProps.dims[0] ) || ( m_height != pSize->tensorProps.dims[1] ) ||
              ( m_width != pSize->tensorProps.dims[2] ) || ( 3 != pSize->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pSize is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pTheta )
    {
        QC_ERROR( "pTheta is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pTheta->type ) || ( nullptr == pTheta->buffer.pData ) ||
              ( 4 != pTheta->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pTheta->tensorProps.type ) ||
              ( 1 != pTheta->tensorProps.dims[0] ) || ( m_height != pTheta->tensorProps.dims[1] ) ||
              ( m_width != pTheta->tensorProps.dims[2] ) || ( 2 != pTheta->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pTheta is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pInPts )
    {
        QC_ERROR( "pInPts is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pInPts->type ) || ( nullptr == pInPts->buffer.pData ) ||
              ( 2 != pInPts->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pInPts->tensorProps.type ) ||
              ( m_config.numInFeatureDim != pInPts->tensorProps.dims[1] ) )
    {
        QC_ERROR( "pInPts is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pDetections )
    {
        QC_ERROR( "pDetections is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pDetections->type ) ||
              ( nullptr == pDetections->buffer.pData ) ||
              ( 2 != pDetections->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pDetections->tensorProps.type ) ||
              ( m_config.maxNumDetOut != pDetections->tensorProps.dims[0] ) ||
              ( POSTCENTERPOINT_OBJECT_3D_DIM != pDetections->tensorProps.dims[1] ) )
    {
        QC_ERROR( "pDetections is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        ret = m_plrPost.ExtractBBoxRun( pHeatmap, pXY, pZ, pSize, pTheta, pInPts, &m_BBoxList,
                                        &m_labels, &m_scores, &m_metadata, &numDetOut );

        if ( QC_STATUS_OK == ret )
        {
            pObj = (PostCenterPoint_Object3D_t *) pDetections->data();
            pBBoxList = (FadasCuboidf32_t *) m_BBoxList.data();
            pLabels = (uint32_t *) m_labels.data();
            pScores = (float32_t *) m_scores.data();
            pMetadata = (FadasPlr3DBBoxMetadata_t *) m_metadata.data();
            pDetections->tensorProps.dims[0] = numDetOut;
            QC_DEBUG( "number of detections %" PRIu32, numDetOut );
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
                QC_DEBUG( "class=%d score=%.3f bbox=[%.3f %.3f %.3f %.3f %.3f %.3f] "
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
}   // namespace QC
