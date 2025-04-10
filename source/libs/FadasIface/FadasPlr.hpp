// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef RIDEHAL_FADAS_PLR_HPP
#define RIDEHAL_FADAS_PLR_HPP

#include "FadasSrv.hpp"
#include <vector>

namespace ridehal
{
namespace libs
{
namespace FadasIface
{

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

class FadasPlrPreProc : public FadasSrv
{
public:
    FadasPlrPreProc();
    ~FadasPlrPreProc();

    RideHalError_e SetParams( float pillarXSize, float pillarYSize, float pillarZSize,
                              float minXRange, float minYRange, float minZRange, float maxXRange,
                              float maxYRange, float maxZRange, uint32_t maxNumInPts,
                              uint32_t numInFeatureDim, uint32_t maxNumPlrs,
                              uint32_t maxNumPtsPerPlr, uint32_t numOutFeatureDim );

    RideHalError_e CreatePreProc();
    RideHalError_e PointPillarRun( const RideHal_SharedBuffer_t *pInPts,
                                   const RideHal_SharedBuffer_t *pOutPlrs,
                                   const RideHal_SharedBuffer_t *pOutFeature );
    RideHalError_e DestroyPreProc();

private:
    RideHalError_e CreatePreProcCPU();
    RideHalError_e CreatePreProcDSP();

    RideHalError_e PointPillarRunCPU( const RideHal_SharedBuffer_t *pInPts,
                                      const RideHal_SharedBuffer_t *pOutPlrs,
                                      const RideHal_SharedBuffer_t *pOutFeature );
    RideHalError_e PointPillarRunDSP( const RideHal_SharedBuffer_t *pInPts,
                                      const RideHal_SharedBuffer_t *pOutPlrs,
                                      const RideHal_SharedBuffer_t *pOutFeature );

    RideHalError_e DestroyPreProcCPU();
    RideHalError_e DestroyPreProcDSP();

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

class FadasPlrPostProc : public FadasSrv
{
public:
    FadasPlrPostProc();
    ~FadasPlrPostProc();

    RideHalError_e SetParams( float pillarXSize, float pillarYSize, float minXRange,
                              float minYRange, float maxXRange, float maxYRange, uint32_t numClass,
                              uint32_t maxNumInPts, uint32_t numInFeatureDim, uint32_t maxNumDetOut,
                              float32_t threshScore, float32_t threshIOU, bool bMapPtsToBBox );

    RideHalError_e SetFilterParams( float minCentreX, float minCentreY, float minCentreZ,
                                    float maxCentreX, float maxCentreY, float maxCentreZ,
                                    bool *labelSelect, uint32_t maxNumFilter );

    RideHalError_e CreatePostProc();
    RideHalError_e
    ExtractBBoxRun( const RideHal_SharedBuffer_t *pHeatmap, const RideHal_SharedBuffer_t *pXY,
                    const RideHal_SharedBuffer_t *pZ, const RideHal_SharedBuffer_t *pSize,
                    const RideHal_SharedBuffer_t *pTheta, const RideHal_SharedBuffer_t *pInPts,
                    const RideHal_SharedBuffer_t *pBBoxList, const RideHal_SharedBuffer_t *pLabels,
                    const RideHal_SharedBuffer_t *pScores, const RideHal_SharedBuffer_t *pMetadata,
                    uint32_t *pNumDetOut );
    RideHalError_e DestroyPostProc();

private:
    RideHalError_e CreatePostProcCPU();
    RideHalError_e CreatePostProcDSP();

    RideHalError_e
    ExtractBBoxRunCPU( const RideHal_SharedBuffer_t *pHeatmap, const RideHal_SharedBuffer_t *pXY,
                       const RideHal_SharedBuffer_t *pZ, const RideHal_SharedBuffer_t *pSize,
                       const RideHal_SharedBuffer_t *pTheta, const RideHal_SharedBuffer_t *pInPts,
                       const RideHal_SharedBuffer_t *pBBoxList,
                       const RideHal_SharedBuffer_t *pLabels, const RideHal_SharedBuffer_t *pScores,
                       const RideHal_SharedBuffer_t *pMetadata, uint32_t *pNumDetOut );
    RideHalError_e
    ExtractBBoxRunDSP( const RideHal_SharedBuffer_t *pHeatmap, const RideHal_SharedBuffer_t *pXY,
                       const RideHal_SharedBuffer_t *pZ, const RideHal_SharedBuffer_t *pSize,
                       const RideHal_SharedBuffer_t *pTheta, const RideHal_SharedBuffer_t *pInPts,
                       const RideHal_SharedBuffer_t *pBBoxList,
                       const RideHal_SharedBuffer_t *pLabels, const RideHal_SharedBuffer_t *pScores,
                       const RideHal_SharedBuffer_t *pMetadata, uint32_t *pNumDetOut );

    RideHalError_e DestroyPostProcCPU();
    RideHalError_e DestroyPostProcDSP();


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
}   // namespace ridehal

#endif   // RIDEHAL_FADAS_PLR_HPP
