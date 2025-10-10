// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef QC_CL2DFLEX_CLH
#define QC_CL2DFLEX_CLH

#define KernelCode( ... ) #__VA_ARGS__

inline const char *Kernels()
{
    static const char *kernels =
#include "kernel/CL2DConstant.cl.h"
#include "kernel/CL2DPipelineConvert.cl.h"
#include "kernel/CL2DPipelineConvertUBWC.cl.h"
#include "kernel/CL2DPipelineLetterbox.cl.h"
#include "kernel/CL2DPipelineLetterboxMultiple.cl.h"
#include "kernel/CL2DPipelineRemap.cl.h"
#include "kernel/CL2DPipelineResize.cl.h"
#include "kernel/CL2DPipelineResizeMultiple.cl.h"
            ;
    return kernels;
}

static const char *s_pSourceCL2DFlex = Kernels();

#endif   // QC_CL2DFLEX_CLH
