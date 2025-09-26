// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



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
                     OpenclSrv *pOpenclSrvObj,
                     std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );

    QCStatus_e Deinit();

    QCStatus_e Execute( ImageDescriptor_t &input, ImageDescriptor_t &output );

private:
    QCStatus_e LetterboxFromNV12ToRGBMultiple( cl_mem bufferSrc, uint32_t srcOffset,
                                               cl_mem bufferDst, uint32_t dstOffset,
                                               ImageDescriptor_t &input,
                                               ImageDescriptor_t &output );

private:
    cl_mem m_bufferROIs;

};   // class PipelineLetterboxMultiple

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_LETTERBOXMULTIPLE_HPP
