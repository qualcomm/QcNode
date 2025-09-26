// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_FADAS_PLR_POST_PROC_HPP
#define QC_FADAS_PLR_POST_PROC_HPP

#include "FadasSrv.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include <vector>

namespace QC
{
namespace libs
{
namespace FadasIface
{
using namespace QC::Memory;

#ifdef __QNXNTO__
typedef Fadas3DBBoxMetadata_t FadasPlr3DBBoxMetadata_t;
#else
/* The Ubuntu or Linux FadasVM mainline code dosn't support meanIntensity, with this workaround and
 * with engineer version library to support the meanIntensity. */
typedef struct
{
    FadasPt_3Df32_t meanPt;
    uint32_t numPts;
    float32_t meanIntensity;
} FadasPlr3DBBoxMetadata_t;
#endif

class FadasPlrPostProc : public FadasSrv
{
public:
    FadasPlrPostProc();
    ~FadasPlrPostProc();

    QCStatus_e SetParams( float pillarXSize, float pillarYSize, float minXRange, float minYRange,
                          float maxXRange, float maxYRange, uint32_t numClass, uint32_t maxNumInPts,
                          uint32_t numInFeatureDim, uint32_t maxNumDetOut, float32_t threshScore,
                          float32_t threshIOU, bool bMapPtsToBBox );

    QCStatus_e SetFilterParams( float minCentreX, float minCentreY, float minCentreZ,
                                float maxCentreX, float maxCentreY, float maxCentreZ,
                                bool *labelSelect, uint32_t maxNumFilter );

    QCStatus_e CreatePostProc();
    QCStatus_e ExtractBBoxRun( const TensorDescriptor_t *pHeatmap, const TensorDescriptor_t *pXY,
                               const TensorDescriptor_t *pZ, const TensorDescriptor_t *pSize,
                               const TensorDescriptor_t *pTheta, const TensorDescriptor_t *pInPts,
                               const TensorDescriptor_t *pBBoxList,
                               const TensorDescriptor_t *pLabels, const TensorDescriptor_t *pScores,
                               const TensorDescriptor_t *pMetadata, uint32_t *pNumDetOut );
    QCStatus_e DestroyPostProc();

private:
    QCStatus_e CreatePostProcCPU();
    QCStatus_e CreatePostProcDSP();

    QCStatus_e ExtractBBoxRunCPU( const TensorDescriptor_t *pHeatmap, const TensorDescriptor_t *pXY,
                                  const TensorDescriptor_t *pZ, const TensorDescriptor_t *pSize,
                                  const TensorDescriptor_t *pTheta,
                                  const TensorDescriptor_t *pInPts,
                                  const TensorDescriptor_t *pBBoxList,
                                  const TensorDescriptor_t *pLabels,
                                  const TensorDescriptor_t *pScores,
                                  const TensorDescriptor_t *pMetadata, uint32_t *pNumDetOut );
    QCStatus_e ExtractBBoxRunDSP( const TensorDescriptor_t *pHeatmap, const TensorDescriptor_t *pXY,
                                  const TensorDescriptor_t *pZ, const TensorDescriptor_t *pSize,
                                  const TensorDescriptor_t *pTheta,
                                  const TensorDescriptor_t *pInPts,
                                  const TensorDescriptor_t *pBBoxList,
                                  const TensorDescriptor_t *pLabels,
                                  const TensorDescriptor_t *pScores,
                                  const TensorDescriptor_t *pMetadata, uint32_t *pNumDetOut );

    QCStatus_e DestroyPostProcCPU();
    QCStatus_e DestroyPostProcDSP();


private:
    typedef union
    {
        void *hHandle;
        uint64_t handle64;
    } FadasPlrPostProc_PillarHandler_t;


private:
    remote_handle64 m_handle64;
    FadasPlrPostProc_PillarHandler_t m_plrHandler;

    float m_pillarXSize;
    float m_pillarYSize;
    float m_minXRange;
    float m_minYRange;
    float m_maxXRange;
    float m_maxYRange;
    uint32_t m_numClass;
    uint32_t m_maxNumInPts;
    uint32_t m_numInFeatureDim;
    uint32_t m_maxNumDetOut;
    float32_t m_threshScore;
    float32_t m_threshIOU;
    bool m_bMapPtsToBBox;

    /* filter related parameters */
    float m_minCentreX;
    float m_minCentreY;
    float m_minCentreZ;
    float m_maxCentreX;
    float m_maxCentreY;
    float m_maxCentreZ;
    uint32_t m_maxNumFilter;
    std::vector<uint8_t> m_labelSelect;

    bool m_bBBoxFilter = false;
    bool m_bParamSet = false;
};

}   // namespace FadasIface
}   // namespace libs
}   // namespace QC

#endif   // QC_FADAS_PLR_POST_PROC_HPP
