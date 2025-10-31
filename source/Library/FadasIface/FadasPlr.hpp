// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_FADAS_PLR_HPP
#define QC_FADAS_PLR_HPP

#include "FadasSrv.hpp"
#include <vector>

namespace QC
{
namespace libs
{
namespace FadasIface
{

class FadasPlrPreProc : public FadasSrv
{
public:
    FadasPlrPreProc();
    ~FadasPlrPreProc();

    QCStatus_e SetParams( float pillarXSize, float pillarYSize, float pillarZSize, float minXRange,
                          float minYRange, float minZRange, float maxXRange, float maxYRange,
                          float maxZRange, uint32_t maxNumInPts, uint32_t numInFeatureDim,
                          uint32_t maxNumPlrs, uint32_t maxNumPtsPerPlr,
                          uint32_t numOutFeatureDim );

    QCStatus_e CreatePreProc();
    QCStatus_e DestroyPreProc();

    QCStatus_e PointPillarRun( const QCBufferDescriptorBase_t &inputPts,
                               const QCBufferDescriptorBase_t &outputPlrs,
                               const QCBufferDescriptorBase_t &outputFeature );

private:
    QCStatus_e CreatePreProcCPU();
    QCStatus_e CreatePreProcDSP();

    QCStatus_e PointPillarRunCPU( const QCBufferDescriptorBase_t &inputPts,
                                  const QCBufferDescriptorBase_t &outputPlrs,
                                  const QCBufferDescriptorBase_t &outputFeature );
    QCStatus_e PointPillarRunDSP( const QCBufferDescriptorBase_t &inputPts,
                                  const QCBufferDescriptorBase_t &outputPlrs,
                                  const QCBufferDescriptorBase_t &outputFeature );

    QCStatus_e DestroyPreProcCPU();
    QCStatus_e DestroyPreProcDSP();

private:
    typedef union
    {
        void *hHandle;
        uint64_t handle64;
    } FadasPlrPreProc_PillarHandler_t;


private:
    remote_handle64 m_handle64;
    FadasPlrPreProc_PillarHandler_t m_plrHandler;

    /*  Parameters */
    float m_pillarXSize;
    float m_pillarYSize;
    float m_pillarZSize;
    float m_minXRange;
    float m_minYRange;
    float m_minZRange;
    float m_maxXRange;
    float m_maxYRange;
    float m_maxZRange;
    uint32_t m_maxNumInPts;
    uint32_t m_numInFeatureDim;
    uint32_t m_maxNumPlrs;
    uint32_t m_maxNumPtsPerPlr;
    uint32_t m_numOutFeatureDim;

    bool m_bParamSet = false;
};

}   // namespace FadasIface
}   // namespace libs
}   // namespace QC

#endif   // QC_FADAS_PLR_HPP

