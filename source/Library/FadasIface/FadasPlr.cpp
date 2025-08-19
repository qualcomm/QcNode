// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "FadasPlr.hpp"
#include <string.h>

namespace QC
{
namespace libs
{
namespace FadasIface
{

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
                (uint32_t) ( numPts * m_numInFeatureDim * (uint32_t) sizeof( float ) ), fdOutPlrs,
                (uint32_t) ( pOutPlrs->offset ), pOutPlrs->size, fdOutFeature, pOutFeature->offset,
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

}   // namespace FadasIface
}   // namespace libs
}   // namespace QC