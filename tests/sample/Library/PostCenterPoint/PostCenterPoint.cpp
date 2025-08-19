// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "PostCenterPoint.hpp"
#include <cmath>

namespace QC
{
namespace sample
{

PostCenterPoint::PostCenterPoint( Logger &logger ) : m_logger( logger ) {}

PostCenterPoint::~PostCenterPoint() {}

QCStatus_e PostCenterPoint::Init( const char *pName, const PostCenterPoint_Config_t *pConfig )
{
    QCStatus_e ret = QC_STATUS_OK;
    bool bInitStart = false;
    bool bFadasInitOK = false;

    if ( QC_OBJECT_STATE_INITIAL == m_state )
    {
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
    else
    {
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        bInitStart = true;
        ret = m_plrPost.Init( m_config.processor, pName, m_logger.GetLevel() );
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
        m_pBufMgr = BufferManager::Get( { pName, QC_NODE_TYPE_CUSTOM_0, m_config.nodeId },
                                        m_logger.GetLevel() );
        if ( nullptr == m_pBufMgr )
        {
            QC_ERROR( "Failed to create buffer manager!" );
            ret = QC_STATUS_NOMEM;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        TensorProps_t tensorProps;
        tensorProps.tensorType = QC_TENSOR_TYPE_FLOAT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.dims[1] = sizeof( FadasCuboidf32_t ) / sizeof( float );
        tensorProps.numDims = 2;
        ret = m_pBufMgr->Allocate( tensorProps, m_BBoxList );
    }

    if ( QC_STATUS_OK == ret )
    {
        TensorProps_t tensorProps;
        tensorProps.tensorType = QC_TENSOR_TYPE_UINT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.numDims = 1;
        ret = m_pBufMgr->Allocate( tensorProps, m_labels );
    }

    if ( QC_STATUS_OK == ret )
    {
        TensorProps_t tensorProps;
        tensorProps.tensorType = QC_TENSOR_TYPE_FLOAT_32;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.numDims = 1;
        ret = m_pBufMgr->Allocate( tensorProps, m_scores );
    }

    if ( QC_STATUS_OK == ret )
    {
        TensorProps_t tensorProps;
        tensorProps.tensorType = QC_TENSOR_TYPE_UINT_8;
        tensorProps.dims[0] = m_config.maxNumDetOut;
        tensorProps.dims[1] = sizeof( FadasPlr3DBBoxMetadata_t );
        tensorProps.numDims = 2;
        ret = m_pBufMgr->Allocate( tensorProps, m_metadata );
    }

    if ( QC_STATUS_OK == ret )
    {
        TensorDescriptor_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
        ret = RegisterBuffersToFadas( buffers, 4, FADAS_BUF_TYPE_OUT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_plrPost.CreatePostProc();
        if ( QC_STATUS_OK != ret )
        {
            /* do deregister */
            TensorDescriptor_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
            (void) DeRegisterBuffersToFadas( buffers, 4 );
        }
    }

    if ( QC_STATUS_OK != ret )
    {
        /* do error clean up */
        QC_ERROR( "PlrPost Init failed: %d!", ret );
        if ( true == bInitStart )
        { /* do clean up */
            if ( nullptr != m_pBufMgr )
            {
                if ( nullptr != m_BBoxList.pBuf )
                {
                    (void) m_pBufMgr->Free( m_BBoxList );
                    m_BBoxList.pBuf = nullptr;
                }
                if ( nullptr != m_labels.pBuf )
                {
                    (void) m_pBufMgr->Free( m_labels );
                    m_labels.pBuf = nullptr;
                }
                if ( nullptr != m_scores.pBuf )
                {
                    (void) m_pBufMgr->Free( m_scores );
                    m_scores.pBuf = nullptr;
                }
                if ( nullptr != m_metadata.pBuf )
                {
                    (void) m_pBufMgr->Free( m_metadata );
                    m_metadata.pBuf = nullptr;
                }
                BufferManager::Put( m_pBufMgr );
                m_pBufMgr = nullptr;
            }

            if ( true == bFadasInitOK )
            {
                (void) m_plrPost.Deinit();
            }
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
        TensorDescriptor_t buffers[4] = { m_BBoxList, m_labels, m_scores, m_metadata };
        ret2 = DeRegisterBuffers( buffers, 4 );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost deregister buffers failed: %d!", ret2 );
            ret = ret2;
        }

        ret2 = m_pBufMgr->Free( m_BBoxList );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_BBoxList failed: %d!", ret2 );
            ret = ret2;
        }
        m_BBoxList.pBuf = nullptr;

        ret2 = m_pBufMgr->Free( m_labels );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_labels failed: %d!", ret2 );
            ret = ret2;
        }
        m_labels.pBuf = nullptr;

        ret2 = m_pBufMgr->Free( m_scores );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_scores failed: %d!", ret2 );
            ret = ret2;
        }
        m_scores.pBuf = nullptr;

        ret2 = m_pBufMgr->Free( m_metadata );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Free m_metadata failed: %d!", ret2 );
            ret = ret2;
        }
        m_metadata.pBuf = nullptr;

        ret2 = m_plrPost.DestroyPostProc();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost DestroyPostProc failed: %d!", ret2 );
            ret = ret2;
        }

        BufferManager::Put( m_pBufMgr );
        m_pBufMgr = nullptr;

        ret2 = m_plrPost.Deinit();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "PlrPost Deinit failed: %d!", ret2 );
            ret = ret2;
        }
        m_state = QC_OBJECT_STATE_INITIAL;
    }

    return ret;
}

QCStatus_e PostCenterPoint::RegisterBuffersToFadas( const TensorDescriptor_t *pBuffers,
                                                    uint32_t numBuffers, FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( uint32_t i = 0; i < numBuffers; i++ )
    {
        const TensorDescriptor_t *pBuf = &pBuffers[i];
        if ( QC_BUFFER_TYPE_TENSOR == pBuf->type )
        {
            int32_t fd = m_plrPost.RegBuf( *pBuf, bufferType );
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

QCStatus_e PostCenterPoint::RegisterBuffers( const TensorDescriptor_t *pBuffers,
                                             uint32_t numBuffers, FadasBufType_e bufferType )
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

QCStatus_e PostCenterPoint::DeRegisterBuffersToFadas( const TensorDescriptor_t *pBuffers,
                                                      uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    for ( uint32_t i = 0; i < numBuffers; i++ )
    {
        const TensorDescriptor_t *pBuf = &pBuffers[i];
        if ( QC_BUFFER_TYPE_TENSOR == pBuf->type )
        {
            m_plrPost.DeregBuf( pBuf->GetDataPtr() );
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

QCStatus_e PostCenterPoint::DeRegisterBuffers( const TensorDescriptor_t *pBuffers,
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

QCStatus_e PostCenterPoint::Execute( const TensorDescriptor_t *pHeatmap,
                                     const TensorDescriptor_t *pXY, const TensorDescriptor_t *pZ,
                                     const TensorDescriptor_t *pSize,
                                     const TensorDescriptor_t *pTheta,
                                     const TensorDescriptor_t *pInPts,
                                     TensorDescriptor_t *pDetections )
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
    else if ( ( QC_BUFFER_TYPE_TENSOR != pHeatmap->type ) || ( nullptr == pHeatmap->pBuf ) ||
              ( 4 != pHeatmap->numDims ) || ( QC_TENSOR_TYPE_FLOAT_32 != pHeatmap->tensorType ) ||
              ( 1 != pHeatmap->dims[0] ) || ( m_height != pHeatmap->dims[1] ) ||
              ( m_width != pHeatmap->dims[2] ) || ( m_config.numClass != pHeatmap->dims[3] ) )
    {
        QC_ERROR( "pHeatmap is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pXY )
    {
        QC_ERROR( "pXY is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pXY->type ) || ( nullptr == pXY->pBuf ) ||
              ( 4 != pXY->numDims ) || ( QC_TENSOR_TYPE_FLOAT_32 != pXY->tensorType ) ||
              ( 1 != pXY->dims[0] ) || ( m_height != pXY->dims[1] ) ||
              ( m_width != pXY->dims[2] ) || ( 2 != pXY->dims[3] ) )
    {
        QC_ERROR( "pXY is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pZ )
    {
        QC_ERROR( "pZ is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pZ->type ) || ( nullptr == pZ->pBuf ) ||
              ( 4 != pZ->numDims ) || ( QC_TENSOR_TYPE_FLOAT_32 != pZ->tensorType ) ||
              ( 1 != pZ->dims[0] ) || ( m_height != pZ->dims[1] ) || ( m_width != pZ->dims[2] ) ||
              ( 1 != pZ->dims[3] ) )
    {
        QC_ERROR( "pZ is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pSize )
    {
        QC_ERROR( "pSize is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pSize->type ) || ( nullptr == pSize->pBuf ) ||
              ( 4 != pSize->numDims ) || ( QC_TENSOR_TYPE_FLOAT_32 != pSize->tensorType ) ||
              ( 1 != pSize->dims[0] ) || ( m_height != pSize->dims[1] ) ||
              ( m_width != pSize->dims[2] ) || ( 3 != pSize->dims[3] ) )
    {
        QC_ERROR( "pSize is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pTheta )
    {
        QC_ERROR( "pTheta is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pTheta->type ) || ( nullptr == pTheta->pBuf ) ||
              ( 4 != pTheta->numDims ) || ( QC_TENSOR_TYPE_FLOAT_32 != pTheta->tensorType ) ||
              ( 1 != pTheta->dims[0] ) || ( m_height != pTheta->dims[1] ) ||
              ( m_width != pTheta->dims[2] ) || ( 2 != pTheta->dims[3] ) )
    {
        QC_ERROR( "pTheta is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pInPts )
    {
        QC_ERROR( "pInPts is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pInPts->type ) || ( nullptr == pInPts->pBuf ) ||
              ( 2 != pInPts->numDims ) || ( QC_TENSOR_TYPE_FLOAT_32 != pInPts->tensorType ) ||
              ( m_config.numInFeatureDim != pInPts->dims[1] ) )
    {
        QC_ERROR( "pInPts is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pDetections )
    {
        QC_ERROR( "pDetections is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pDetections->type ) || ( nullptr == pDetections->pBuf ) ||
              ( 2 != pDetections->numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pDetections->tensorType ) ||
              ( m_config.maxNumDetOut != pDetections->dims[0] ) ||
              ( POSTCENTERPOINT_OBJECT_3D_DIM != pDetections->dims[1] ) )
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
            pObj = (PostCenterPoint_Object3D_t *) pDetections->GetDataPtr();
            pBBoxList = (FadasCuboidf32_t *) m_BBoxList.GetDataPtr();
            pLabels = (uint32_t *) m_labels.GetDataPtr();
            pScores = (float32_t *) m_scores.GetDataPtr();
            pMetadata = (FadasPlr3DBBoxMetadata_t *) m_metadata.GetDataPtr();
            pDetections->dims[0] = numDetOut;
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

}   // namespace sample
}   // namespace QC
