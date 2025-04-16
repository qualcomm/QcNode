// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "FadasPlr.hpp"
#include <string.h>

#ifndef __QNXNTO__
/* The Ubuntu or Linux FadasVM mainline code dosn't support filterParams, with the macro
 * DISABLE_CPU_FILTER defined to disable it for CPU. For DSP, engineer version library can be
 * provided to support the filterParams */
#define DISABLE_CPU_FILTER
#endif

namespace QC
{
namespace libs
{
namespace FadasIface
{

#define PLRPOST_NUM_INPUTS 10

FadasPlrPreProc::FadasPlrPreProc()
{
    m_plrHandler.hHandle = nullptr;
}

FadasPlrPreProc::~FadasPlrPreProc() {}

QCStatus_e FadasPlrPreProc::SetParams( float pillarXSize, float pillarYSize, float pillarZSize,
                                       float minXRange, float minYRange, float minZRange,
                                       float maxXRange, float maxYRange, float maxZRange,
                                       uint32_t maxNumInPts, uint32_t numInFeatureDim,
                                       uint32_t maxNumPlrs, uint32_t maxNumPtsPerPlr,
                                       uint32_t numOutFeatureDim )
{
    m_pillarXSize = pillarXSize;
    m_pillarYSize = pillarYSize;
    m_pillarZSize = pillarZSize;
    m_minXRange = minXRange;
    m_minYRange = minYRange;
    m_minZRange = minZRange;
    m_maxXRange = maxXRange;
    m_maxYRange = maxYRange;
    m_maxZRange = maxZRange;
    m_maxNumInPts = maxNumInPts;
    m_numInFeatureDim = numInFeatureDim;
    m_maxNumPlrs = maxNumPlrs;
    m_maxNumPtsPerPlr = maxNumPtsPerPlr;
    m_numOutFeatureDim = numOutFeatureDim;

    QC_INFO( "plrSize=[%.2f %.2f %.2f], minRange=[%.2f %.2f %.2f], maxRange=[%.2f %.2f %.2f], "
             "pcd=%ux%u, plrs=%ux%ux%u",
             m_pillarXSize, m_pillarYSize, m_pillarZSize, m_minXRange, m_minYRange, m_minZRange,
             m_maxXRange, m_maxYRange, m_maxZRange, m_maxNumInPts, m_numInFeatureDim, m_maxNumPlrs,
             m_maxNumPtsPerPlr, m_numOutFeatureDim );

    m_bParamSet = true;

    return QC_STATUS_OK;
}

QCStatus_e FadasPlrPreProc::CreatePreProcCPU()
{
    QCStatus_e ret = QC_STATUS_OK;

    FadasPt_3Df32_t plrSize = { m_pillarXSize, m_pillarYSize, m_pillarZSize };
    FadasPt_3Df32_t minRange = { m_minXRange, m_minYRange, m_minZRange };
    FadasPt_3Df32_t maxRange = { m_maxXRange, m_maxYRange, m_maxZRange };

    m_plrHandler.hHandle = FadasVM_PointPillar_Create( plrSize, minRange, maxRange, m_maxNumInPts,
                                                       m_numInFeatureDim, m_maxNumPlrs,
                                                       m_maxNumPtsPerPlr, m_numOutFeatureDim );

    if ( nullptr == m_plrHandler.hHandle )
    {
        QC_ERROR( "CPU Create PointPillar Fail!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::DestroyPreProcCPU()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == m_plrHandler.hHandle )
    {
        QC_ERROR( "hHandle is nullptr!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        FadasError_e error = FadasVM_PointPillar_Destroy( m_plrHandler.hHandle );
        if ( FADAS_ERROR_NONE != error )
        {
            QC_ERROR( "CPU destroy pointpiller handle %p fail: %d!", m_plrHandler.hHandle, error );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::CreatePreProcDSP()
{
    QCStatus_e ret = QC_STATUS_OK;
    AEEResult result;

    FadasIface_Pt3D_t plrSize = { m_pillarXSize, m_pillarYSize, m_pillarZSize };
    FadasIface_Pt3D_t minRange = { m_minXRange, m_minYRange, m_minZRange };
    FadasIface_Pt3D_t maxRange = { m_maxXRange, m_maxYRange, m_maxZRange };

    result = FadasIface_PointPillarCreate(
            m_handle64, &plrSize, &minRange, &maxRange, m_maxNumInPts, m_numInFeatureDim,
            m_maxNumPlrs, m_maxNumPtsPerPlr, m_numOutFeatureDim, &m_plrHandler.handle64 );
    if ( AEE_SUCCESS != result )
    {
        QC_ERROR( "DSP create pointpiller fail: 0x%x!", result );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::DestroyPreProcDSP()
{
    QCStatus_e ret = QC_STATUS_OK;

    AEEResult result;

    if ( 0 == m_plrHandler.handle64 )
    {
        QC_ERROR( "handle64 is 0!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        result = FadasIface_PointPillarDestroy( m_handle64, m_plrHandler.handle64 );
        if ( AEE_SUCCESS != result )
        {
            QC_ERROR( "DSP destroy pointpiller fail: 0x%x!", result );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::CreatePreProc()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bParamSet )
    {
        QC_ERROR( "Parameter not set!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        m_handle64 = GetRemoteHandle64();
        ret = CreatePreProcDSP();
    }
    else
    {
        ret = CreatePreProcCPU();
    }


    return ret;
}

QCStatus_e FadasPlrPreProc::PointPillarRunCPU( const QCSharedBuffer_t *pInPts,
                                               const QCSharedBuffer_t *pOutPlrs,
                                               const QCSharedBuffer_t *pOutFeature )
{
    QCStatus_e ret = QC_STATUS_OK;
    FadasError_e error;
    int fdPts = -1;
    int fdOutPlrs = -1;
    int fdOutFeature = -1;

    fdPts = RegBuf( pInPts, FADAS_BUF_TYPE_IN );
    if ( fdPts < 0 )
    {
        QC_ERROR( "register pInPts buffer fail!" );
        ret = QC_STATUS_INVALID_BUF;
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutPlrs = RegBuf( pOutPlrs, FADAS_BUF_TYPE_OUT );
        if ( fdOutPlrs < 0 )
        {
            QC_ERROR( "register pOutPlrs buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutFeature = RegBuf( pOutFeature, FADAS_BUF_TYPE_OUT );
        if ( fdOutFeature < 0 )
        {
            QC_ERROR( "register pOutFeature buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        uint32_t numOutPlrs = 0;
        uint32_t numPts = pInPts->tensorProps.dims[0];
        const float32_t *pInPtsData = (const float32_t *) pInPts->data();
        FadasVM_PointPillar_t *pOutPlrsData = (FadasVM_PointPillar_t *) pOutPlrs->data();
        float32_t *pOutFeatureData = (float32_t *) pOutFeature->data();
        error = FadasVM_PointPillar_Run( m_plrHandler.hHandle, numPts, pInPtsData, pOutPlrsData,
                                         pOutFeatureData, &numOutPlrs );
        if ( FADAS_ERROR_NONE != error )
        {
            QC_ERROR( "CPU PointPillar Run fail: %d!", error );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::PointPillarRunDSP( const QCSharedBuffer_t *pInPts,
                                               const QCSharedBuffer_t *pOutPlrs,
                                               const QCSharedBuffer_t *pOutFeature )
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t fdPts = -1;
    int32_t fdOutPlrs = -1;
    int32_t fdOutFeature = -1;

    fdPts = RegBuf( pInPts, FADAS_BUF_TYPE_IN );
    if ( fdPts < 0 )
    {
        QC_ERROR( "register pInPts buffer fail!" );
        ret = QC_STATUS_INVALID_BUF;
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutPlrs = RegBuf( pOutPlrs, FADAS_BUF_TYPE_OUT );
        if ( fdOutPlrs < 0 )
        {
            QC_ERROR( "register pOutPlrs buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutFeature = RegBuf( pOutFeature, FADAS_BUF_TYPE_OUT );
        if ( fdOutFeature < 0 )
        {
            QC_ERROR( "register pOutFeature buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        uint32_t numOutPlrs = 0;
        uint32_t numPts = pInPts->tensorProps.dims[0];
        AEEResult result = FadasIface_PointPillarRun(
                m_handle64, m_plrHandler.handle64, numPts, fdPts, pInPts->offset,
                ( uint32_t )( numPts * m_numInFeatureDim * (uint32_t) sizeof( float ) ), fdOutPlrs,
                ( uint32_t )( pOutPlrs->offset ), pOutPlrs->size, fdOutFeature, pOutFeature->offset,
                pOutFeature->size, &numOutPlrs );
        if ( AEE_SUCCESS != result )
        {
            QC_ERROR( "DSP PointPillar Run fail: 0x%x!", result );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::PointPillarRun( const QCSharedBuffer_t *pInPts,
                                            const QCSharedBuffer_t *pOutPlrs,
                                            const QCSharedBuffer_t *pOutFeature )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = PointPillarRunDSP( pInPts, pOutPlrs, pOutFeature );
    }
    else
    {
        ret = PointPillarRunCPU( pInPts, pOutPlrs, pOutFeature );
    }


    return ret;
}

QCStatus_e FadasPlrPreProc::DestroyPreProc()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = DestroyPreProcDSP();
    }
    else
    {
        ret = DestroyPreProcCPU();
    }


    return ret;
}

FadasPlrPostProc::FadasPlrPostProc()
{
    m_plrHandler.hHandle = nullptr;
}

FadasPlrPostProc::~FadasPlrPostProc() {}

QCStatus_e FadasPlrPostProc::SetParams( float pillarXSize, float pillarYSize, float minXRange,
                                        float minYRange, float maxXRange, float maxYRange,
                                        uint32_t numClass, uint32_t maxNumInPts,
                                        uint32_t numInFeatureDim, uint32_t maxNumDetOut,
                                        float32_t threshScore, float32_t threshIOU,
                                        bool bMapPtsToBBox )
{
    m_pillarXSize = pillarXSize;
    m_pillarYSize = pillarYSize;
    m_minXRange = minXRange;
    m_minYRange = minYRange;
    m_maxXRange = maxXRange;
    m_maxYRange = maxYRange;

    m_numClass = numClass;
    m_maxNumInPts = maxNumInPts;
    m_numInFeatureDim = numInFeatureDim;
    m_maxNumDetOut = maxNumDetOut;
    m_threshScore = threshScore;
    m_threshIOU = threshIOU;
    m_bMapPtsToBBox = bMapPtsToBBox;

    /* Init filter: default disabled */
    m_bBBoxFilter = false;
    m_labelSelect.clear();

    m_bParamSet = true;

    QC_INFO( "plrSize=[%.2f %.2f], minRange=[%.2f %.2f], maxRange=[%.2f %.2f], "
             "pcd=%ux%u, %u class, thresh=[%.2f %.2f], max=%u, bMapPtsToBBox=%d",
             m_pillarXSize, m_pillarYSize, m_minXRange, m_minYRange, m_maxXRange, m_maxYRange,
             m_maxNumInPts, m_numInFeatureDim, m_numClass, m_threshScore, m_threshIOU,
             m_maxNumDetOut, m_bMapPtsToBBox );
    return QC_STATUS_OK;
}

QCStatus_e FadasPlrPostProc::SetFilterParams( float minCentreX, float minCentreY, float minCentreZ,
                                              float maxCentreX, float maxCentreY, float maxCentreZ,
                                              bool *labelSelect, uint32_t maxNumFilter )
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( false == m_bParamSet )
    {
        QC_ERROR( "Parameter not set!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( false == m_bMapPtsToBBox )
    {
        QC_ERROR( "bMapPtsToBBox is not true while setting filter parameters!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        m_minCentreX = minCentreX;
        m_minCentreY = minCentreY;
        m_minCentreZ = minCentreZ;
        m_maxCentreX = maxCentreX;
        m_maxCentreY = maxCentreY;
        m_maxCentreZ = maxCentreZ;
        m_labelSelect.resize( m_numClass );
        (void) memcpy( m_labelSelect.data(), labelSelect, m_numClass );
        m_bBBoxFilter = true;
        m_maxNumFilter = maxNumFilter;

        QC_INFO( "filter=[%.2f %.2f %.2f %.2f %.2f], numFilter=%u", minCentreX, minCentreY,
                 minCentreZ, maxCentreX, maxCentreY, maxCentreZ, maxNumFilter );
    }

    return ret;
}

QCStatus_e FadasPlrPostProc::CreatePostProcCPU()
{
    QCStatus_e ret = QC_STATUS_OK;

    Fadas2DGrid_t grid = { { m_minXRange, m_minYRange },
                           { m_maxXRange, m_maxYRange },
                           m_pillarXSize,
                           m_pillarYSize };

    Fadas3DBBoxInitParams_t bboxInitParams = { 0 };

    bboxInitParams.strideInPts = m_numInFeatureDim * sizeof( float32_t );
    bboxInitParams.numClass = m_numClass;
    bboxInitParams.grid = grid;
    bboxInitParams.maxNumInPts = m_maxNumInPts;
    bboxInitParams.maxNumDetOut = m_maxNumDetOut;
    bboxInitParams.threshScore = m_threshScore;
    bboxInitParams.threshIOU = m_threshIOU;
    if ( true == m_bBBoxFilter )
    {
#ifndef DISABLE_CPU_FILTER
        bboxInitParams.filterParams.minCentre.x = m_minCentreX;
        bboxInitParams.filterParams.minCentre.y = m_minCentreY;
        bboxInitParams.filterParams.minCentre.z = m_minCentreZ;
        bboxInitParams.filterParams.maxCentre.x = m_maxCentreX;
        bboxInitParams.filterParams.maxCentre.y = m_maxCentreY;
        bboxInitParams.filterParams.maxCentre.z = m_maxCentreZ;
        bboxInitParams.filterParams.maxNumFilter = m_maxNumFilter;
        bboxInitParams.filterParams.labelSelect = (bool *) m_labelSelect.data();
#else
        QC_WARN( "CPU BBoxFilter is not supported, will ignore it!" );
#endif
    }

    m_plrHandler.hHandle = FadasVM_ExtractBBox_Create( &bboxInitParams );

    if ( nullptr == m_plrHandler.hHandle )
    {
        QC_ERROR( "CPU Create ExtractBBox Fail!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

QCStatus_e FadasPlrPostProc::CreatePostProcDSP()
{
    QCStatus_e ret = QC_STATUS_OK;
    AEEResult error;
    FadasIface_Grid2D_t grid = { m_minXRange, m_minYRange,   m_maxXRange,
                                 m_maxYRange, m_pillarXSize, m_pillarYSize };
    error = FadasIface_ExtractBBoxCreate(
            m_handle64, m_maxNumInPts, m_numInFeatureDim, m_maxNumDetOut, m_numClass, &grid,
            m_threshScore, m_threshIOU, m_minCentreX, m_minCentreY, m_minCentreZ, m_maxCentreX,
            m_maxCentreY, m_maxCentreZ, m_labelSelect.data(), (int) m_labelSelect.size(),
            m_maxNumFilter, &m_plrHandler.handle64 );
    if ( AEE_SUCCESS != error )
    {
        QC_ERROR( "DSP Create ExtractBBox Fail: 0x%x!", error );
        ret = QC_STATUS_FAIL;
    }
    return ret;
}

QCStatus_e FadasPlrPostProc::CreatePostProc()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( false == m_bParamSet )
    {
        QC_ERROR( "Parameter not set!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        m_handle64 = GetRemoteHandle64();
        ret = CreatePostProcDSP();
    }
    else
    {
        ret = CreatePostProcCPU();
    }

    return ret;
}

QCStatus_e FadasPlrPostProc::DestroyPostProcCPU()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == m_plrHandler.hHandle )
    {
        QC_ERROR( "hHandle is nullptr!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        FadasError_e error = FadasVM_ExtractBBox_Destroy( m_plrHandler.hHandle );
        if ( FADAS_ERROR_NONE != error )
        {
            QC_ERROR( "CPU destroy ExtractBBox handle %p fail: %d!", m_plrHandler.hHandle, error );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e FadasPlrPostProc::DestroyPostProcDSP()
{
    QCStatus_e ret = QC_STATUS_OK;

    AEEResult result;

    if ( 0 == m_plrHandler.handle64 )
    {
        QC_ERROR( "handle64 is 0!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        result = FadasIface_ExtractBBoxDestroy( m_handle64, m_plrHandler.handle64 );
        if ( AEE_SUCCESS != result )
        {
            QC_ERROR( "DSP destroy pointpiller fail: 0x%x!", result );
            ret = QC_STATUS_FAIL;
        }
    }
    return ret;
}

QCStatus_e FadasPlrPostProc::DestroyPostProc()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = DestroyPostProcDSP();
    }
    else
    {
        ret = DestroyPostProcCPU();
    }


    return ret;
}

QCStatus_e FadasPlrPostProc::ExtractBBoxRunCPU(
        const QCSharedBuffer_t *pHeatmap, const QCSharedBuffer_t *pXY, const QCSharedBuffer_t *pZ,
        const QCSharedBuffer_t *pSize, const QCSharedBuffer_t *pTheta,
        const QCSharedBuffer_t *pInPts, const QCSharedBuffer_t *pBBoxList,
        const QCSharedBuffer_t *pLabels, const QCSharedBuffer_t *pScores,
        const QCSharedBuffer_t *pMetadata, uint32_t *pNumDetOut )
{
    QCStatus_e ret = QC_STATUS_OK;
    FadasError_e error;
    int fdHeatmap = -1;
    int fdXY = -1;
    int fdZ = -1;
    int fdSize = -1;
    int fdTheta = -1;
    int fdInPts = -1;
    int fdBBoxList = -1;
    int fdLabels = -1;
    int fdScores = -1;
    int fdMetadata = -1;

    fdHeatmap = RegBuf( pHeatmap, FADAS_BUF_TYPE_IN );
    if ( fdHeatmap < 0 )
    {
        QC_ERROR( "register pHeatmap buffer fail!" );
        ret = QC_STATUS_INVALID_BUF;
    }

    if ( QC_STATUS_OK == ret )
    {
        fdXY = RegBuf( pXY, FADAS_BUF_TYPE_IN );
        if ( fdXY < 0 )
        {
            QC_ERROR( "register pXY buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdZ = RegBuf( pZ, FADAS_BUF_TYPE_IN );
        if ( fdZ < 0 )
        {
            QC_ERROR( "register pZ buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdSize = RegBuf( pSize, FADAS_BUF_TYPE_IN );
        if ( fdSize < 0 )
        {
            QC_ERROR( "register pSize buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }
    if ( QC_STATUS_OK == ret )
    {
        fdTheta = RegBuf( pTheta, FADAS_BUF_TYPE_IN );
        if ( fdTheta < 0 )
        {
            QC_ERROR( "register pTheta buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdInPts = RegBuf( pInPts, FADAS_BUF_TYPE_IN );
        if ( fdInPts < 0 )
        {
            QC_ERROR( "register pInPts buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdBBoxList = RegBuf( pBBoxList, FADAS_BUF_TYPE_OUT );
        if ( fdBBoxList < 0 )
        {
            QC_ERROR( "register pBBoxList buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdLabels = RegBuf( pLabels, FADAS_BUF_TYPE_OUT );
        if ( fdLabels < 0 )
        {
            QC_ERROR( "register pLabels buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }


    if ( QC_STATUS_OK == ret )
    {
        fdScores = RegBuf( pScores, FADAS_BUF_TYPE_OUT );
        if ( fdScores < 0 )
        {
            QC_ERROR( "register pScores buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdMetadata = RegBuf( pMetadata, FADAS_BUF_TYPE_OUT );
        if ( fdMetadata < 0 )
        {
            QC_ERROR( "register pMetadata buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }


    if ( QC_STATUS_OK == ret )
    {
        uint32_t numPtsIn = pInPts->tensorProps.dims[0];
        const float32_t *pInPtsBuf = (const float32_t *) pInPts->data();
        Fadas3DRPNBufs_t rpnBuf;
        Fadas3DBBoxBufs_t outBuf;
        uint32_t numDetOut = 0;

        rpnBuf.pHeatmap = (float32_t *) pHeatmap->data();
        rpnBuf.pXY = (float32_t *) pXY->data();
        rpnBuf.pZ = (float32_t *) pZ->data();
        rpnBuf.pSize = (float32_t *) pSize->data();
        rpnBuf.pTheta = (float32_t *) pTheta->data();

        outBuf.pBBoxList = (FadasCuboidf32_t *) pBBoxList->data();
        outBuf.pLabels = (uint32_t *) pLabels->data();
        outBuf.pScores = (float32_t *) pScores->data();
        outBuf.pMetadata = (Fadas3DBBoxMetadata_t *) pMetadata->data();
#ifndef DISABLE_CPU_FILTER
        error = FadasVM_ExtractBBox_Run( m_plrHandler.hHandle, numPtsIn, pInPtsBuf, rpnBuf, outBuf,
                                         pNumDetOut, m_bMapPtsToBBox, m_bBBoxFilter );
#else
        error = FadasVM_ExtractBBox_Run( m_plrHandler.hHandle, numPtsIn, pInPtsBuf, rpnBuf, outBuf,
                                         pNumDetOut, m_bMapPtsToBBox );
#endif
        if ( FADAS_ERROR_NONE != error )
        {
            QC_ERROR( "CPU ExtractBBox Run fail: %d!", error );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e FadasPlrPostProc::ExtractBBoxRunDSP(
        const QCSharedBuffer_t *pHeatmap, const QCSharedBuffer_t *pXY, const QCSharedBuffer_t *pZ,
        const QCSharedBuffer_t *pSize, const QCSharedBuffer_t *pTheta,
        const QCSharedBuffer_t *pInPts, const QCSharedBuffer_t *pBBoxList,
        const QCSharedBuffer_t *pLabels, const QCSharedBuffer_t *pScores,
        const QCSharedBuffer_t *pMetadata, uint32_t *pNumDetOut )
{
    QCStatus_e ret = QC_STATUS_OK;
    AEEResult error;
    int fdHeatmap = -1;
    int fdXY = -1;
    int fdZ = -1;
    int fdSize = -1;
    int fdTheta = -1;
    int fdInPts = -1;
    int fdBBoxList = -1;
    int fdLabels = -1;
    int fdScores = -1;
    int fdMetadata = -1;

    fdHeatmap = RegBuf( pHeatmap, FADAS_BUF_TYPE_IN );
    if ( fdHeatmap < 0 )
    {
        QC_ERROR( "register pHeatmap buffer fail!" );
        ret = QC_STATUS_INVALID_BUF;
    }

    if ( QC_STATUS_OK == ret )
    {
        fdXY = RegBuf( pXY, FADAS_BUF_TYPE_IN );
        if ( fdXY < 0 )
        {
            QC_ERROR( "register pXY buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdZ = RegBuf( pZ, FADAS_BUF_TYPE_IN );
        if ( fdZ < 0 )
        {
            QC_ERROR( "register pZ buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdSize = RegBuf( pSize, FADAS_BUF_TYPE_IN );
        if ( fdSize < 0 )
        {
            QC_ERROR( "register pSize buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }
    if ( QC_STATUS_OK == ret )
    {
        fdTheta = RegBuf( pTheta, FADAS_BUF_TYPE_IN );
        if ( fdTheta < 0 )
        {
            QC_ERROR( "register pTheta buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdInPts = RegBuf( pInPts, FADAS_BUF_TYPE_IN );
        if ( fdInPts < 0 )
        {
            QC_ERROR( "register pInPts buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdBBoxList = RegBuf( pBBoxList, FADAS_BUF_TYPE_OUT );
        if ( fdBBoxList < 0 )
        {
            QC_ERROR( "register pBBoxList buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdLabels = RegBuf( pLabels, FADAS_BUF_TYPE_OUT );
        if ( fdLabels < 0 )
        {
            QC_ERROR( "register pLabels buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }


    if ( QC_STATUS_OK == ret )
    {
        fdScores = RegBuf( pScores, FADAS_BUF_TYPE_OUT );
        if ( fdScores < 0 )
        {
            QC_ERROR( "register pScores buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdMetadata = RegBuf( pMetadata, FADAS_BUF_TYPE_OUT );
        if ( fdMetadata < 0 )
        {
            QC_ERROR( "register pMetadata buffer fail!" );
            ret = QC_STATUS_INVALID_BUF;
        }
    }


    if ( QC_STATUS_OK == ret )
    {
        uint32_t numPtsIn = pInPts->tensorProps.dims[0];

        int32_t fds[PLRPOST_NUM_INPUTS] = { fdInPts, fdHeatmap,  fdXY,     fdZ,      fdSize,
                                            fdTheta, fdBBoxList, fdLabels, fdScores, fdMetadata };
        uint32_t offsets[PLRPOST_NUM_INPUTS] = {
                (uint32_t) pInPts->offset,    (uint32_t) pHeatmap->offset,
                (uint32_t) pXY->offset,       (uint32_t) pZ->offset,
                (uint32_t) pSize->offset,     (uint32_t) pTheta->offset,
                (uint32_t) pBBoxList->offset, (uint32_t) pLabels->offset,
                (uint32_t) pScores->offset,   (uint32_t) pMetadata->offset };
        uint32_t sizes[PLRPOST_NUM_INPUTS] = {
                ( uint32_t )( numPtsIn * m_numInFeatureDim * (uint32_t) sizeof( float ) ),
                (uint32_t) pHeatmap->size,
                (uint32_t) pXY->size,
                (uint32_t) pZ->size,
                (uint32_t) pSize->size,
                (uint32_t) pTheta->size,
                (uint32_t) pBBoxList->size,
                (uint32_t) pLabels->size,
                (uint32_t) pScores->size,
                (uint32_t) pMetadata->size };
        error = FadasIface_ExtractBBoxRun( m_handle64, m_plrHandler.handle64, numPtsIn, fds,
                                           PLRPOST_NUM_INPUTS, offsets, PLRPOST_NUM_INPUTS, sizes,
                                           PLRPOST_NUM_INPUTS, (uint8_t) m_bMapPtsToBBox,
                                           (uint8_t) m_bBBoxFilter, pNumDetOut );
        if ( AEE_SUCCESS != error )
        {
            QC_ERROR( "DSP ExtractBBox Run fail: 0x%x!", error );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}


QCStatus_e
FadasPlrPostProc::ExtractBBoxRun( const QCSharedBuffer_t *pHeatmap, const QCSharedBuffer_t *pXY,
                                  const QCSharedBuffer_t *pZ, const QCSharedBuffer_t *pSize,
                                  const QCSharedBuffer_t *pTheta, const QCSharedBuffer_t *pInPts,
                                  const QCSharedBuffer_t *pBBoxList,
                                  const QCSharedBuffer_t *pLabels, const QCSharedBuffer_t *pScores,
                                  const QCSharedBuffer_t *pMetadata, uint32_t *pNumDetOut )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = ExtractBBoxRunDSP( pHeatmap, pXY, pZ, pSize, pTheta, pInPts, pBBoxList, pLabels,
                                 pScores, pMetadata, pNumDetOut );
    }
    else
    {
        ret = ExtractBBoxRunCPU( pHeatmap, pXY, pZ, pSize, pTheta, pInPts, pBBoxList, pLabels,
                                 pScores, pMetadata, pNumDetOut );
    }


    return ret;
}
}   // namespace FadasIface
}   // namespace libs
}   // namespace QC