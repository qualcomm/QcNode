// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef RIDEHAL_FADAS_REMAP_HPP
#define RIDEHAL_FADAS_REMAP_HPP

#include "FadasSrv.hpp"

using namespace ridehal::common;

namespace ridehal
{
namespace libs
{
namespace FadasIface
{

class FadasRemap : public FadasSrv
{
public:
    FadasRemap();
    ~FadasRemap();
    RideHalError_e SetRemapParams( uint32_t numOfInputs, uint32_t outputWidth,
                                   uint32_t outputHeight, RideHal_ImageFormat_e outputFormat,
                                   FadasNormlzParams_t normlzR, FadasNormlzParams_t normlzG,
                                   FadasNormlzParams_t normlzB, bool bEnableUndistortion,
                                   bool bEnableNormalize );
    RideHalError_e CreatRemapTable( uint32_t inputId, uint32_t mapWidth, uint32_t mapHeight,
                                    const RideHal_SharedBuffer_t *pMapX,
                                    const RideHal_SharedBuffer_t *pMapY );
    RideHalError_e CreateRemapWorker( uint32_t inputId, RideHal_ImageFormat_e inputFormat,
                                      uint32_t inputWidth, uint32_t inputHeight, FadasROI_t ROI );
    RideHalError_e RemapRun( const RideHal_SharedBuffer_t *inputs,
                             const RideHal_SharedBuffer_t *output );
    RideHalError_e DestroyWorkers();
    RideHalError_e DestroyMap();

private:
    RideHalError_e RemapRunCPU( const RideHal_SharedBuffer_t *inputs,
                                const RideHal_SharedBuffer_t *output );
    RideHalError_e RemapRunDSP( const RideHal_SharedBuffer_t *inputs,
                                const RideHal_SharedBuffer_t *output );
    FadasRemapPipeline_e RemapGetPipelineCPU( RideHal_ImageFormat_e inputFormat,
                                              RideHal_ImageFormat_e outputFormat,
                                              bool bEnableNormalize );
    FadasIface_FadasRemapPipeline_e RemapGetPipelineDSP( RideHal_ImageFormat_e inputFormat,
                                                         RideHal_ImageFormat_e outputFormat,
                                                         bool bEnableNormalize );

private:
    remote_handle64 m_handle64;
    uint32_t m_numOfInputs;
    RideHal_ImageFormat_e m_inputFormats[RIDEHAL_MAX_INPUTS];
    RideHal_ImageFormat_e m_outputFormat;
    uint32_t m_inputWidths[RIDEHAL_MAX_INPUTS];
    uint32_t m_inputHeights[RIDEHAL_MAX_INPUTS];
    uint32_t m_mapWidths[RIDEHAL_MAX_INPUTS];
    uint32_t m_mapHeights[RIDEHAL_MAX_INPUTS];
    FadasROI_t m_ROIs[RIDEHAL_MAX_INPUTS];
    uint64 m_workerPtrsDSP[RIDEHAL_MAX_INPUTS];
    uint64 m_remapPtrsDSP[RIDEHAL_MAX_INPUTS];
    void *m_workerPtrsCPU[RIDEHAL_MAX_INPUTS];
    FadasRemapMap_t *m_remapPtrsCPU[RIDEHAL_MAX_INPUTS];
    FadasNormlzParams_t m_normlz[3];
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    bool m_bEnableUndistortion;
    bool m_bEnableNormalize;
};

}   // namespace FadasIface
}   // namespace libs
}   // namespace ridehal

#endif   // RIDEHAL_FADAS_REMAP_HPP
