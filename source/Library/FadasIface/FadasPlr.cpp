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

QCStatus_e FadasPlrPreProc::PointPillarRunCPU( const QCBufferDescriptorBase_t &inputPts,
                                               const QCBufferDescriptorBase_t &outputPlrs,
                                               const QCBufferDescriptorBase_t &outputFeature )

{
    QCStatus_e ret = QC_STATUS_OK;
    FadasError_e error;
    int fdPts = -1;
    int fdOutPlrs = -1;
    int fdOutFeature = -1;
    uint32_t numInputPts = 0;
    uint32_t numOutputPlrs = 0;
    void *pInputData = nullptr;
    void *pOutputPlrData = nullptr;
    void *pOutputFeatData = nullptr;

    const TensorDescriptor_t *pInputTensor = dynamic_cast<const TensorDescriptor_t *>( &inputPts );
    const TensorDescriptor_t *pOutputPlrTensor =
            dynamic_cast<const TensorDescriptor_t *>( &outputPlrs );
    const TensorDescriptor_t *pOutputFeatTensor =
            dynamic_cast<const TensorDescriptor_t *>( &outputFeature );

    fdPts = RegBuf( inputPts, FADAS_BUF_TYPE_IN );
    if ( fdPts < 0 )
    {
        ret = QC_STATUS_INVALID_BUF;
        QC_ERROR( "register inputPts buffer fail!" );
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutPlrs = RegBuf( outputPlrs, FADAS_BUF_TYPE_OUT );
        if ( fdOutPlrs < 0 )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "register outputPlrs buffer fail!" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutFeature = RegBuf( outputFeature, FADAS_BUF_TYPE_OUT );
        if ( fdOutFeature < 0 )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "register outputFeature buffer fail!" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pInputTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "pointer cast for pInputTensor is invalid" );
        }
        else
        {
            pInputData = pInputTensor->GetDataPtr();
            numInputPts = pInputTensor->dims[0];
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pOutputPlrTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "pointer cast for pOutputPlrTensor is invalid" );
        }
        else
        {
            pOutputPlrData = pOutputPlrTensor->GetDataPtr();
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pOutputFeatTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "pointer cast for pOutputFeatTensor is invalid" );
        }
        else
        {
            pOutputFeatData = pOutputFeatTensor->GetDataPtr();
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        const float32_t *pInputDataFloat = (const float32_t *) pInputData;
        FadasVM_PointPillar_t *pOutputPlrDataFvm = (FadasVM_PointPillar_t *) pOutputPlrData;
        float32_t *pOutputFeatDataFloat = (float32_t *) pOutputFeatData;
        error = FadasVM_PointPillar_Run( m_plrHandler.hHandle, numInputPts, pInputDataFloat,
                                         pOutputPlrDataFvm, pOutputFeatDataFloat, &numOutputPlrs );
        if ( FADAS_ERROR_NONE != error )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "CPU PointPillar Run fail: %d!", error );
        }
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::PointPillarRunDSP( const QCBufferDescriptorBase_t &inputPts,
                                               const QCBufferDescriptorBase_t &outputPlrs,
                                               const QCBufferDescriptorBase_t &outputFeature )
{
    QCStatus_e ret = QC_STATUS_OK;
    FadasError_e error;
    int fdPts = -1;
    int fdOutPlrs = -1;
    int fdOutFeature = -1;
    uint32_t numInputPts = 0;
    uint32_t numOutputPlrs = 0;
    size_t inputOffset = 0;
    size_t outputPlrOffset = 0;
    size_t outputFeatOffset = 0;
    size_t outputPlrSize = 0;
    size_t outputFeatSize = 0;

    const TensorDescriptor_t *pInputTensor = dynamic_cast<const TensorDescriptor_t *>( &inputPts );
    const TensorDescriptor_t *pOutputPlrTensor =
            dynamic_cast<const TensorDescriptor_t *>( &outputPlrs );
    const TensorDescriptor_t *pOutputFeatTensor =
            dynamic_cast<const TensorDescriptor_t *>( &outputFeature );

    fdPts = RegBuf( inputPts, FADAS_BUF_TYPE_IN );
    if ( fdPts < 0 )
    {
        ret = QC_STATUS_INVALID_BUF;
        QC_ERROR( "register inputPts buffer fail!" );
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutPlrs = RegBuf( outputPlrs, FADAS_BUF_TYPE_OUT );
        if ( fdOutPlrs < 0 )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "register outputPlrs buffer fail!" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        fdOutFeature = RegBuf( outputFeature, FADAS_BUF_TYPE_OUT );
        if ( fdOutFeature < 0 )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "register outputFeature buffer fail!" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pInputTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "pointer cast for pInputTensor is invalid" );
        }
        else
        {
            inputOffset = pInputTensor->offset;
            numInputPts = pInputTensor->dims[0];
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pOutputPlrTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "pointer cast for pOutputPlrTensor is invalid" );
        }
        else
        {
            outputPlrOffset = pOutputPlrTensor->offset;
            outputPlrSize = pOutputPlrTensor->GetDataSize();
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        const TensorDescriptor_t *pOutputFeatTensor =
                dynamic_cast<const TensorDescriptor_t *>( &outputFeature );
        if ( nullptr == pOutputFeatTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "pointer cast for pOutputFeatTensor is invalid" );
        }
        else
        {
            outputFeatOffset = pOutputFeatTensor->offset;
            outputFeatSize = pOutputFeatTensor->GetDataSize();
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        AEEResult result = FadasIface_PointPillarRun(
                m_handle64, m_plrHandler.handle64, numInputPts, fdPts, inputOffset,
                (uint32_t) ( numInputPts * m_numInFeatureDim * (uint32_t) sizeof( float ) ),
                fdOutPlrs, (uint32_t) outputPlrOffset, outputPlrSize, fdOutFeature,
                outputFeatOffset, outputFeatSize, &numOutputPlrs );
        if ( AEE_SUCCESS != result )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "DSP PointPillar Run fail: 0x%x!", result );
        }
    }

    return ret;
}

QCStatus_e FadasPlrPreProc::PointPillarRun( const QCBufferDescriptorBase_t &inputPts,
                                            const QCBufferDescriptorBase_t &outputPlrs,
                                            const QCBufferDescriptorBase_t &outputFeature )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_PROCESSOR_HTP0 == m_processor ) || ( QC_PROCESSOR_HTP1 == m_processor ) )
    {
        ret = PointPillarRunDSP( inputPts, outputPlrs, outputFeature );
    }
    else
    {
        ret = PointPillarRunCPU( inputPts, outputPlrs, outputFeature );
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

