// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_CL2D_PIPELINE_RESIZEMULTIPLE_HPP
#define QC_CL2D_PIPELINE_RESIZEMULTIPLE_HPP

#include "pipeline/CL2DPipelineBase.hpp"

namespace QC
{
namespace Node
{

class CL2DPipelineResizeMultiple : public CL2DPipelineBase
{
public:
    CL2DPipelineResizeMultiple();

    ~CL2DPipelineResizeMultiple();

    QCStatus_e Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                     OpenclSrv *pOpenclSrvObj,
                     std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );

    QCStatus_e Deinit();

    QCStatus_e Execute( ImageDescriptor_t &input, ImageDescriptor_t &output );

private:
    QCStatus_e ResizeFromNV12ToRGBMultiple( cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
                                            uint32_t dstOffset, ImageDescriptor_t &input,
                                            ImageDescriptor_t &output );

private:
    cl_mem m_bufferROIs;

};   // class PipelineResizeMultiple

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_RESIZEMULTIPLE_HPP
