// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_FADAS_REMAP_HPP
#define QC_FADAS_REMAP_HPP

#include "FadasSrv.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Node/Ifs/QCFrameDescriptorNodeIfs.hpp"

namespace QC
{
namespace libs
{
namespace FadasIface
{

class FadasRemap : public FadasSrv
{
public:
    FadasRemap();
    ~FadasRemap();
    QCStatus_e SetRemapParams( uint32_t numOfInputs, uint32_t outputWidth, uint32_t outputHeight,
                               QCImageFormat_e outputFormat, FadasNormlzParams_t normlzR,
                               FadasNormlzParams_t normlzG, FadasNormlzParams_t normlzB,
                               bool bEnableUndistortion, bool bEnableNormalize );
    QCStatus_e CreatRemapTable( uint32_t inputId, uint32_t mapWidth, uint32_t mapHeight,
                                const TensorDescriptor_t &bufDescMapX,
                                const TensorDescriptor_t &bufDescMapY );
    QCStatus_e CreateRemapWorker( uint32_t inputId, QCImageFormat_e inputFormat,
                                  uint32_t inputWidth, uint32_t inputHeight, FadasROI_t ROI );
    QCStatus_e RemapRun( QCFrameDescriptorNodeIfs &frameDesc );
    QCStatus_e DestroyWorkers();
    QCStatus_e DestroyMap();

private:
    QCStatus_e RemapRunCPU( QCFrameDescriptorNodeIfs &frameDesc );
    QCStatus_e RemapRunDSP( QCFrameDescriptorNodeIfs &frameDesc );
    FadasRemapPipeline_e RemapGetPipelineCPU( QCImageFormat_e inputFormat,
                                              QCImageFormat_e outputFormat, bool bEnableNormalize );
    FadasIface_FadasRemapPipeline_e RemapGetPipelineDSP( QCImageFormat_e inputFormat,
                                                         QCImageFormat_e outputFormat,
                                                         bool bEnableNormalize );

private:
    remote_handle64 m_handle64;
    uint32_t m_numOfInputs;
    QCImageFormat_e m_inputFormats[QC_MAX_INPUTS];
    QCImageFormat_e m_outputFormat;
    uint32_t m_inputWidths[QC_MAX_INPUTS];
    uint32_t m_inputHeights[QC_MAX_INPUTS];
    uint32_t m_mapWidths[QC_MAX_INPUTS];
    uint32_t m_mapHeights[QC_MAX_INPUTS];
    FadasROI_t m_ROIs[QC_MAX_INPUTS];
    uint64 m_workerPtrsDSP[QC_MAX_INPUTS];
    uint64 m_remapPtrsDSP[QC_MAX_INPUTS];
    void *m_workerPtrsCPU[QC_MAX_INPUTS];
    FadasRemapMap_t *m_remapPtrsCPU[QC_MAX_INPUTS];
    FadasNormlzParams_t m_normlz[3];
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;
    bool m_bEnableUndistortion;
    bool m_bEnableNormalize;
};

}   // namespace FadasIface
}   // namespace libs
}   // namespace QC

#endif   // QC_FADAS_REMAP_HPP
