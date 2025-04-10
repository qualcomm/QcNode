// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_CL2D_PIPELINE_LETTERBOXMULTIPLE_HPP
#define RIDEHAL_CL2D_PIPELINE_LETTERBOXMULTIPLE_HPP

#include "include/CL2DPipelineBase.hpp"

namespace ridehal
{
namespace component
{

class CL2DPipelineLetterboxMultiple : public CL2DPipelineBase
{
public:
    CL2DPipelineLetterboxMultiple();

    ~CL2DPipelineLetterboxMultiple();

    RideHalError_e Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                         OpenclSrv *pOpenclSrvObj );

    RideHalError_e Deinit();

    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInput,
                            const RideHal_SharedBuffer_t *pOutput );

    RideHalError_e ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                   const RideHal_SharedBuffer_t *pOutput,
                                   const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );

private:
    RideHalError_e LetterboxFromNV12ToRGBMultiple( uint32_t numROIs, cl_mem bufferSrc,
                                                   uint32_t srcOffset, cl_mem bufferDst,
                                                   uint32_t dstOffset,
                                                   const RideHal_SharedBuffer_t *pInput,
                                                   const RideHal_SharedBuffer_t *pOutput,
                                                   const CL2DFlex_ROIConfig_t *pROIs );

private:
    RideHal_SharedBuffer_t
            m_roiBuffer; /**<internal buffer used to store roi parameters for ExecuteWithROI API*/

};   // class PipelineLetterboxMultiple

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_CL2D_PIPELINE_LETTERBOXMULTIPLE_HPP
