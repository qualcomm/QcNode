// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2D_PIPELINE_LETTERBOXMULTIPLE_HPP
#define QC_CL2D_PIPELINE_LETTERBOXMULTIPLE_HPP

#include "pipeline/CL2DPipelineBase.hpp"

namespace QC
{
namespace Node
{

class CL2DPipelineLetterboxMultiple : public CL2DPipelineBase
{
public:
    CL2DPipelineLetterboxMultiple();

    ~CL2DPipelineLetterboxMultiple();

    QCStatus_e Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                     OpenclSrv *pOpenclSrvObj );

    QCStatus_e Deinit();

    QCStatus_e Execute( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput );

    QCStatus_e ExecuteWithROI( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput,
                               const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );

private:
    QCStatus_e LetterboxFromNV12ToRGBMultiple( uint32_t numROIs, cl_mem bufferSrc,
                                               uint32_t srcOffset, cl_mem bufferDst,
                                               uint32_t dstOffset, const QCSharedBuffer_t *pInput,
                                               const QCSharedBuffer_t *pOutput,
                                               const CL2DFlex_ROIConfig_t *pROIs );

private:
    QCSharedBuffer_t
            m_roiBuffer; /**<internal buffer used to store roi parameters for ExecuteWithROI API*/

};   // class PipelineLetterboxMultiple

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_LETTERBOXMULTIPLE_HPP
