// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2D_PIPELINE_LETTERBOX_HPP
#define QC_CL2D_PIPELINE_LETTERBOX_HPP

#include "include/CL2DPipelineBase.hpp"

namespace QC
{
namespace component
{

class CL2DPipelineLetterbox : public CL2DPipelineBase
{
public:
    CL2DPipelineLetterbox();

    ~CL2DPipelineLetterbox();

    QCStatus_e Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                     OpenclSrv *pOpenclSrvObj );

    QCStatus_e Deinit();

    QCStatus_e Execute( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput );

    QCStatus_e ExecuteWithROI( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput,
                               const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );

private:
    QCStatus_e LetterboxFromNV12ToRGB( cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
                                       uint32_t dstOffset, const QCSharedBuffer_t *pInput,
                                       const QCSharedBuffer_t *pOutput );

};   // class PipelineLetterbox

}   // namespace component
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_LETTERBOX_HPP
