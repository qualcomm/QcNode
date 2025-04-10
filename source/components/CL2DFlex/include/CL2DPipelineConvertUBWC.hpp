// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_CL2D_PIPELINE_CONVERTUBWC_HPP
#define RIDEHAL_CL2D_PIPELINE_CONVERTUBWC_HPP

#include "include/CL2DPipelineBase.hpp"

namespace ridehal
{
namespace component
{

class CL2DPipelineConvertUBWC : public CL2DPipelineBase
{
public:
    CL2DPipelineConvertUBWC();

    ~CL2DPipelineConvertUBWC();

    RideHalError_e Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                         OpenclSrv *pOpenclSrvObj );

    RideHalError_e Deinit();

    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInput,
                            const RideHal_SharedBuffer_t *pOutput );

    RideHalError_e ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                   const RideHal_SharedBuffer_t *pOutput,
                                   const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );

private:
    RideHalError_e ConvertUBWCFromNV12UBWCToNV12( const RideHal_SharedBuffer_t *pInput,
                                                  const RideHal_SharedBuffer_t *pOutput );

};   // class PipelineConvertUBWC

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_CL2D_PIPELINE_CONVERTUBWC_HPP
