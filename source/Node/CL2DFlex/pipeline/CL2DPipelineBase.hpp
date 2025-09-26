// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_CL2D_PIPELINE_BASE_HPP
#define QC_CL2D_PIPELINE_BASE_HPP

#include "CL2DFlexImpl.hpp"

using namespace QC;
using namespace QC::libs::OpenclIface;

namespace QC
{
namespace Node
{

/** @brief CL2DFlex valid pipelines */
typedef enum
{
    CL2DFLEX_PIPELINE_CONVERT_NV12_TO_RGB,  /**<color convert only from nv12 to rgb, roi.width
                                               should be equal to output width and roi.height should
                                               be equal to output height*/
    CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_RGB,  /**<color convert only from uyvy to rgb, roi.width
                                                should be equal to output width and roi.height should
                                                be equal to output height*/
    CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_NV12, /**<color convert only from uyvy to nv12, roi.width
                                               should be equal to output width and roi.height should
                                               be equal to output height*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB,  /**<color convert and resize use nearest point
                                                      from nv12 to rgb*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_RGB,  /**<color convert and resize use nearest point
                                                      from uyvy to rgb*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_UYVY_TO_NV12, /**<color convert and resize use nearest point
                                                      from uyvy to nv12*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_RGB_TO_RGB,   /**<resize use nearest point from rgb to rgb*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_NV12, /**<resize use nearest point from nv12 to nv12*/
    CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB, /**<color convert and letterbox with fixed
                                                        height/width ratio use nearest point from
                                                        nv12 to rgb, padding 0 to the redundant
                                                        bottom or right edge*/
    CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB_MULTIPLE, /**<color convert and letterbox with
                                                                 fixed height/width ratio use
                                                                 nearest point from nv12 to rgb,
                                                                 padding 0 to the redundant bottom
                                                                 or right edge, execute on multiple
                                                                 batches with different ROI
                                                                 paramters*/
    CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB_MULTIPLE, /**<color convert and resize use nearest
                                                              point from nv12 to rgb, execute on
                                                              multiple batches with different ROI
                                                              paramters*/
    CL2DFLEX_PIPELINE_CONVERT_NV12UBWC_TO_NV12,  /**<decompress from nv12 ubwc to nv12, input width
                                                    should be equal to output width, input height
                                                    should be equal to output height, and ROI
                                                    parameters should not be used, only valid on
                                                    qnx currently*/
    CL2DFLEX_PIPELINE_REMAP_NEAREST_NV12_TO_RGB, /**<color convert and remap use nearest point in
                                                    the map table from nv12 to rgb*/
    CL2DFLEX_PIPELINE_REMAP_NEAREST_NV12_TO_BGR, /**<color convert and remap use nearest point in
                                                    the map table from nv12 to bgr*/
    CL2DFLEX_PIPELINE_MAX
} CL2DFlex_Pipeline_e;

class CL2DPipelineBase
{
public:
    CL2DPipelineBase();

    ~CL2DPipelineBase();

    void InitLogger( const char *pName, Logger_Level_e level );

    void DeinitLogger();

    virtual QCStatus_e
    Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
          OpenclSrv *pOpenclSrvObj,
          std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers ) = 0;

    virtual QCStatus_e Deinit() = 0;

    virtual QCStatus_e Execute( ImageDescriptor_t &input, ImageDescriptor_t &output ) = 0;

protected:
    std::string m_name;
    uint32_t m_inputId;
    cl_kernel *m_pKernel;
    CL2DFlex_Config_t m_config;
    OpenclSrv *m_pOpenclSrvObj;
    CL2DFlex_Pipeline_e m_pipeline;
    QC_DECLARE_LOGGER();

};   // class CL2DPipelineBase

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2D_PIPELINE_BASE_HPP
