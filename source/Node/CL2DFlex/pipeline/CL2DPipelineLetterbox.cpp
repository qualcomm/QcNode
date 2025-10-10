// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "pipeline/CL2DPipelineLetterbox.hpp"

namespace QC
{
namespace Node
{

CL2DPipelineLetterbox::CL2DPipelineLetterbox() {}

CL2DPipelineLetterbox::~CL2DPipelineLetterbox() {}

QCStatus_e
CL2DPipelineLetterbox::Init( uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig,
                             OpenclSrv *pOpenclSrvObj,
                             std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( QC_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
         ( QC_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB;
        ret = m_pOpenclSrvObj->CreateKernel( pKernel, "LetterboxNV12ToRGB" );
    }
    else
    {
        QC_ERROR( "Invalid CL2DFlex letterbox pipeline for inputId=%d!", m_inputId );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    return ret;
}

QCStatus_e CL2DPipelineLetterbox::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    // empty function

    return ret;
}

QCStatus_e CL2DPipelineLetterbox::Execute( ImageDescriptor_t &input, ImageDescriptor_t &output )
{
    QCStatus_e ret = QC_STATUS_OK;

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
            uint32_t sizeOne = (uint32_t) ( output.size ) / ( output.batchSize );
            uint32_t dstOffset = (uint32_t) ( output.offset ) + m_inputId * sizeOne;

            if ( CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB == m_pipeline )
            {
                ret = LetterboxFromNV12ToRGB( bufferSrc, srcOffset, bufferDst, dstOffset, input,
                                              output );
            }
            else
            {
                QC_ERROR( "Invalid CL2DFlex letterbox pipeline for m_inputId=%d!", m_inputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }

    return ret;
}

QCStatus_e CL2DPipelineLetterbox::LetterboxFromNV12ToRGB( cl_mem bufferSrc, uint32_t srcOffset,
                                                          cl_mem bufferDst, uint32_t dstOffset,
                                                          ImageDescriptor_t &input,
                                                          ImageDescriptor_t &output )
{
    QCStatus_e ret = QC_STATUS_OK;

    size_t numOfArgs = 17;
    OpenclIfcae_Arg_t OpenclArgs[17];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( m_config.ROIs[m_inputId].height );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( m_config.ROIs[m_inputId].width );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( m_config.outputHeight );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( m_config.outputWidth );
    OpenclArgs[7].argSize = sizeof( cl_int );
    OpenclArgs[8].pArg = (void *) &( input.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( input.planeBufSize[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( input.stride[1] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( output.stride[0] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    uint32_t kernelROIX = m_config.ROIs[m_inputId].x;
    uint32_t kernelROIY = m_config.ROIs[m_inputId].y;
    OpenclArgs[12].pArg = (void *) &( kernelROIX );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( kernelROIY );
    OpenclArgs[13].argSize = sizeof( cl_int );
    float inputRatio =
            (float) m_config.ROIs[m_inputId].height / (float) m_config.ROIs[m_inputId].width;
    float outputRatio = (float) ( output.height ) / (float) ( output.width );
    OpenclArgs[14].pArg = (void *) &( inputRatio );
    OpenclArgs[14].argSize = sizeof( cl_float );
    OpenclArgs[15].pArg = (void *) &( outputRatio );
    OpenclArgs[15].argSize = sizeof( cl_float );
    OpenclArgs[16].pArg = (void *) &( m_config.letterboxPaddingValue );
    OpenclArgs[16].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { output.width, output.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute letterbox NV12 to RGB OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

}   // namespace Node
}   // namespace QC
