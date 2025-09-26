// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_CL2D_PIPELINE_CONVERTUBWC_HPP
#define QC_CL2D_PIPELINE_CONVERTUBWC_HPP

#include "pipeline/CL2DPipelineBase.hpp"

namespace QC
{
namespace Node
{

class CL2DPipelineConvertUBWC : public CL2DPipelineBase
{
public:
    CL2DPipelineConvertUBWC();

    ~CL2DPipelineConvertUBWC();

    QCStatus_e Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                     OpenclSrv *pOpenclSrvObj,
                     std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );

    QCStatus_e Deinit();

    QCStatus_e Execute( ImageDescriptor_t &input, ImageDescriptor_t &output );

private:
    QCStatus_e ConvertUBWCFromNV12UBWCToNV12( ImageDescriptor_t &input, ImageDescriptor_t &output );

};   // class PipelineConvertUBWC

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_CONVERTUBWC_HPP
