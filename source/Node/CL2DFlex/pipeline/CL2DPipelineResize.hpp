// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_CL2D_PIPELINE_RESIZE_HPP
#define QC_CL2D_PIPELINE_RESIZE_HPP

#include "pipeline/CL2DPipelineBase.hpp"

namespace QC
{
namespace Node
{

class CL2DPipelineResize : public CL2DPipelineBase
{
public:
    CL2DPipelineResize();

    ~CL2DPipelineResize();

    QCStatus_e Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                     OpenclSrv *pOpenclSrvObj,
                     std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );

    QCStatus_e Deinit();

    QCStatus_e Execute( ImageDescriptor_t &input, ImageDescriptor_t &output );

private:
    QCStatus_e ResizeFromNV12ToRGB( cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
                                    uint32_t dstOffset, ImageDescriptor_t &input,
                                    ImageDescriptor_t &output );
    QCStatus_e ResizeFromUYVYToRGB( cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
                                    uint32_t dstOffset, ImageDescriptor_t &input,
                                    ImageDescriptor_t &output );
    QCStatus_e ResizeFromUYVYToNV12( cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
                                     uint32_t dstOffset, ImageDescriptor_t &input,
                                     ImageDescriptor_t &output );
    QCStatus_e ResizeFromRGBToRGB( cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
                                   uint32_t dstOffset, ImageDescriptor_t &input,
                                   ImageDescriptor_t &output );
    QCStatus_e ResizeFromNV12ToNV12( cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
                                     uint32_t dstOffset, ImageDescriptor_t &input,
                                     ImageDescriptor_t &output );

};   // class PipelineResize

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_RESIZE_HPP
