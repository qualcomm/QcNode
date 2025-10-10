// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "pipeline/CL2DPipelineConvertUBWC.hpp"

namespace QC
{
namespace Node
{

CL2DPipelineConvertUBWC::CL2DPipelineConvertUBWC() {}

CL2DPipelineConvertUBWC::~CL2DPipelineConvertUBWC() {}

QCStatus_e CL2DPipelineConvertUBWC::Init(
        uint32_t inputId, cl_kernel *pKernel, CL2DFlex_Config_t *pConfig, OpenclSrv *pOpenclSrvObj,
        std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( m_config.outputWidth == m_config.ROIs[m_inputId].width ) &&
         ( m_config.outputHeight == m_config.ROIs[m_inputId].height ) &&
         ( QC_IMAGE_FORMAT_NV12_UBWC == m_config.inputFormats[m_inputId] ) &&
         ( QC_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_CONVERT_NV12UBWC_TO_NV12;
        ret = m_pOpenclSrvObj->CreateKernel( pKernel, "ConvertUBWC" );
    }
    else
    {
        QC_ERROR( "Invalid CL2DFlex ConvertUBWC pipeline for inputId=%d!", m_inputId );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    return ret;
}

QCStatus_e CL2DPipelineConvertUBWC::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    // empty function

    return ret;
}

QCStatus_e CL2DPipelineConvertUBWC::Execute( ImageDescriptor_t &input, ImageDescriptor_t &output )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( CL2DFLEX_PIPELINE_CONVERT_NV12UBWC_TO_NV12 == m_pipeline )
    {
        ret = ConvertUBWCFromNV12UBWCToNV12( input, output );
    }
    else
    {
        QC_ERROR( "Invalid CL2DFlex ConvertUBWC pipeline for m_inputId=%d!", m_inputId );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e CL2DPipelineConvertUBWC::ConvertUBWCFromNV12UBWCToNV12( ImageDescriptor_t &input,
                                                                   ImageDescriptor_t &output )
{
    QCStatus_e ret = QC_STATUS_OK;

    uint32_t sizeOne = (uint32_t) ( output.size ) / ( output.batchSize );
    uint32_t dstOffset = (uint32_t) ( output.offset ) + m_inputId * sizeOne;
    cl_mem bufferSrc;
    cl_mem bufferDst;
    cl_mem bufferSrcY;
    cl_mem bufferSrcUV;

    cl_image_format inputImageFormat = { 0 };
    inputImageFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12;
    inputImageFormat.image_channel_data_type = CL_UNORM_INT8;
    cl_image_desc inputImageDesc = { 0 };
    inputImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
    inputImageDesc.image_width = (size_t) input.width;
    inputImageDesc.image_height = (size_t) input.height;
    ret = m_pOpenclSrvObj->RegImage( input.pBuf, input.dmaHandle, &bufferSrc, &inputImageFormat,
                                     &inputImageDesc );

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register input image!" );
    }
    else
    {
        ret = m_pOpenclSrvObj->RegBufferDesc( dynamic_cast<QCBufferDescriptorBase_t &>( output ),
                                              bufferDst );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register output image!" );
    }
    else
    {
        cl_image_format inputYFormat = { 0 };
        inputYFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_Y;
        inputYFormat.image_channel_data_type = CL_UNORM_INT8;
        cl_image_desc inputYDesc = { 0 };
        inputYDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
        inputYDesc.image_width = (size_t) output.width;
        inputYDesc.image_height = (size_t) output.height;
        inputYDesc.mem_object = bufferSrc;
        ret = m_pOpenclSrvObj->RegPlane( input.pBuf, &bufferSrcY, &inputYFormat, &inputYDesc );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register input Y plane!" );
    }
    else
    {
        cl_image_format inputUVFormat = { 0 };
        inputUVFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_UV;
        inputUVFormat.image_channel_data_type = CL_UNORM_INT8;
        cl_image_desc inputUVDesc = { 0 };
        inputUVDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
        inputUVDesc.image_width = (size_t) m_config.outputWidth;
        inputUVDesc.image_height = (size_t) m_config.outputHeight;
        inputUVDesc.mem_object = bufferSrc;
        ret = m_pOpenclSrvObj->RegPlane( input.pBuf, &bufferSrcUV, &inputUVFormat, &inputUVDesc );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register input UV plane!" );
    }
    else
    {

        size_t numOfArgs = 10;
        OpenclIfcae_Arg_t OpenclArgs[10];
        OpenclArgs[0].pArg = (void *) &bufferSrcY;
        OpenclArgs[0].argSize = sizeof( bufferSrcY );
        OpenclArgs[1].pArg = (void *) &bufferSrcUV;
        OpenclArgs[1].argSize = sizeof( bufferSrcUV );
        OpenclArgs[2].pArg = (void *) &( m_pOpenclSrvObj->m_sampler );
        OpenclArgs[2].argSize = sizeof( m_pOpenclSrvObj->m_sampler );
        OpenclArgs[3].pArg = (void *) &bufferDst;
        OpenclArgs[3].argSize = sizeof( cl_mem );
        OpenclArgs[4].pArg = (void *) &dstOffset;
        OpenclArgs[4].argSize = sizeof( cl_int );
        OpenclArgs[5].pArg = (void *) &( output.stride[0] );
        OpenclArgs[5].argSize = sizeof( cl_int );
        OpenclArgs[6].pArg = (void *) &( output.planeBufSize[0] );
        OpenclArgs[6].argSize = sizeof( cl_int );
        OpenclArgs[7].pArg = (void *) &( output.stride[1] );
        OpenclArgs[7].argSize = sizeof( cl_int );
        uint32_t kernelROIX = m_config.ROIs[m_inputId].x;
        uint32_t kernelROIY = m_config.ROIs[m_inputId].y;
        OpenclArgs[8].pArg = (void *) &( kernelROIX );
        OpenclArgs[8].argSize = sizeof( cl_int );
        OpenclArgs[9].pArg = (void *) &( kernelROIY );
        OpenclArgs[9].argSize = sizeof( cl_int );

        OpenclIface_WorkParams_t OpenclWorkParams;
        OpenclWorkParams.workDim = 2;
        size_t globalWorkSize[2] = { (size_t) output.width, (size_t) output.height };
        OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
        size_t globalWorkOffset[2] = { 0, 0 };
        OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
        /*set local work size to NULL, device would choose optimal size automatically*/
        OpenclWorkParams.pLocalWorkSize = NULL;

        ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to execute ConvertUBWC UBWC to NV12 OpenCL kernel!" );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

}   // namespace Node
}   // namespace QC
