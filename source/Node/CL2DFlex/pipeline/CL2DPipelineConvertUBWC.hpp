// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


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
                     OpenclSrv *pOpenclSrvObj );

    QCStatus_e Deinit();

    QCStatus_e Execute( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput );

    QCStatus_e ExecuteWithROI( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput,
                               const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );

private:
    QCStatus_e ConvertUBWCFromNV12UBWCToNV12( const QCSharedBuffer_t *pInput,
                                              const QCSharedBuffer_t *pOutput );

};   // class PipelineConvertUBWC

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_CONVERTUBWC_HPP
