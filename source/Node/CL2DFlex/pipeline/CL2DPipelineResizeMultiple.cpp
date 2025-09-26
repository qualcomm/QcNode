// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "pipeline/CL2DPipelineResizeMultiple.hpp"

namespace QC
{
namespace Node
{

CL2DPipelineResizeMultiple::CL2DPipelineResizeMultiple() {}

CL2DPipelineResizeMultiple::~CL2DPipelineResizeMultiple() {}

QCStatus_e CL2DPipelineResizeMultiple::Init(
        uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig, OpenclSrv *pOpenclSrvObj,
        std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( QC_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
         ( QC_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) && ( 1 == m_config.numOfInputs ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB_MULTIPLE;
        ret = m_pOpenclSrvObj->CreateKernel( pKernel, "ResizeNV12ToRGBMultiple" );
    }
    else
    {
        QC_ERROR( "Invalid CL2DFlex resize multiple pipeline for inputId=%d!", m_inputId );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    if ( QC_STATUS_OK == ret )
    {
        uint32_t ROIsBufferId = m_config.ROIsBufferId;
        QCBufferDescriptorBase_t &ROIsBufferDesc = buffers[ROIsBufferId];
        TensorDescriptor_t *pROIsBufferDesc = dynamic_cast<TensorDescriptor_t *>( &ROIsBufferDesc );
        ret = m_pOpenclSrvObj->RegBufferDesc(
                dynamic_cast<QCBufferDescriptorBase_t &>( *pROIsBufferDesc ), m_bufferROIs );
    }
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register ROIs buffer!" );
    }

    return ret;
}

QCStatus_e CL2DPipelineResizeMultiple::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    // empty function

    return ret;
}

QCStatus_e CL2DPipelineResizeMultiple::Execute( ImageDescriptor_t &input,
                                                ImageDescriptor_t &output )
{
    QCStatus_e ret = QC_STATUS_BAD_ARGUMENTS;

    cl_mem bufferDst;
    ret = m_pOpenclSrvObj->RegBufferDesc( dynamic_cast<QCBufferDescriptorBase_t &>( output ),
                                          bufferDst );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register output buffer!" );
    }
    else
    {
        cl_mem bufferSrc;
        ret = m_pOpenclSrvObj->RegBufferDesc( dynamic_cast<QCBufferDescriptorBase_t &>( input ),
                                              bufferSrc );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to register input buffer!" );
        }
        else
        {
            uint32_t srcOffset = input.offset;
            uint32_t dstOffset = output.offset;
            if ( CL2DFLEX_PIPELINE_RESIZE_NEAREST_NV12_TO_RGB_MULTIPLE == m_pipeline )
            {
                ret = ResizeFromNV12ToRGBMultiple( bufferSrc, srcOffset, bufferDst, dstOffset,
                                                   input, output );
            }
            else
            {
                QC_ERROR( "Invalid CL2DFlex resize multiple pipeline for inputId=%d!", m_inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }

    return ret;
}

QCStatus_e CL2DPipelineResizeMultiple::ResizeFromNV12ToRGBMultiple(
        cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst, uint32_t dstOffset,
        ImageDescriptor_t &input, ImageDescriptor_t &output )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numOfArgs = 11;
    OpenclIfcae_Arg_t OpenclArgs[11];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &m_bufferROIs;
    OpenclArgs[4].argSize = sizeof( cl_mem );
    OpenclArgs[5].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( input.stride[0] );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( input.planeBufSize[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( input.stride[1] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( output.stride[0] );
    OpenclArgs[10].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 3;
    size_t globalWorkSize[3] = { m_config.numOfROIs, output.width, output.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[3] = { 0, 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute ResizeMultiple NV12 to RGB OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

}   // namespace Node
}   // namespace QC
