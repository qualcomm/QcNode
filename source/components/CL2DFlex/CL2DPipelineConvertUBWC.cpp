// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "include/CL2DPipelineConvertUBWC.hpp"

namespace ridehal
{
namespace component
{

CL2DPipelineConvertUBWC::CL2DPipelineConvertUBWC() {}

CL2DPipelineConvertUBWC::~CL2DPipelineConvertUBWC() {}

RideHalError_e CL2DPipelineConvertUBWC::Init( uint32_t inputId, cl_kernel *pKernel,
                                              CL2DFlex_Config_t *pConfig, OpenclSrv *pOpenclSrvObj )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( m_config.outputWidth == m_config.ROIs[m_inputId].width ) &&
         ( m_config.outputHeight == m_config.ROIs[m_inputId].height ) &&
         ( RIDEHAL_IMAGE_FORMAT_NV12_UBWC == m_config.inputFormats[m_inputId] ) &&
         ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_CONVERT_NV12UBWC_TO_NV12;
        ret = m_pOpenclSrvObj->CreateKernel( pKernel, "ConvertUBWC" );
    }
    else
    {
        RIDEHAL_ERROR( "Invalid CL2DFlex ConvertUBWC pipeline for inputId=%d!", m_inputId );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    return ret;
}

RideHalError_e CL2DPipelineConvertUBWC::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    // empty function

    return ret;
}

RideHalError_e CL2DPipelineConvertUBWC::Execute( const RideHal_SharedBuffer_t *pInput,
                                                 const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( CL2DFLEX_PIPELINE_CONVERT_NV12UBWC_TO_NV12 == m_pipeline )
    {
        ret = ConvertUBWCFromNV12UBWCToNV12( pInput, pOutput );
    }
    else
    {
        RIDEHAL_ERROR( "Invalid CL2DFlex ConvertUBWC pipeline for m_inputId=%d!", m_inputId );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e CL2DPipelineConvertUBWC::ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                                        const RideHal_SharedBuffer_t *pOutput,
                                                        const CL2DFlex_ROIConfig_t *pROIs,
                                                        const uint32_t numROIs )
{
    RideHalError_e ret = RIDEHAL_ERROR_BAD_ARGUMENTS;

    // empty function

    return ret;
}

RideHalError_e
CL2DPipelineConvertUBWC::ConvertUBWCFromNV12UBWCToNV12( const RideHal_SharedBuffer_t *pInput,
                                                        const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    uint32_t sizeOne = ( uint32_t )( pOutput->size ) / ( pOutput->imgProps.batchSize );
    uint32_t dstOffset = ( uint32_t )( pOutput->offset ) + m_inputId * sizeOne;
    cl_mem bufferSrc;
    cl_mem bufferDst;
    cl_mem bufferSrcY;
    cl_mem bufferSrcUV;

    cl_image_format inputImageFormat = { 0 };
    inputImageFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12;
    inputImageFormat.image_channel_data_type = CL_UNORM_INT8;
    cl_image_desc inputImageDesc = { 0 };
    inputImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
    inputImageDesc.image_width = (size_t) pInput->imgProps.width;
    inputImageDesc.image_height = (size_t) pInput->imgProps.height;
    ret = m_pOpenclSrvObj->RegImage( pInput->data(), pInput->buffer.dmaHandle, &bufferSrc,
                                     &inputImageFormat, &inputImageDesc );

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input image!" );
    }
    else
    {
        ret = m_pOpenclSrvObj->RegBuf( &( pOutput->buffer ), &bufferDst );
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register output image!" );
    }
    else
    {
        cl_image_format inputYFormat = { 0 };
        inputYFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_Y;
        inputYFormat.image_channel_data_type = CL_UNORM_INT8;
        cl_image_desc inputYDesc = { 0 };
        inputYDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
        inputYDesc.image_width = (size_t) pOutput->imgProps.width;
        inputYDesc.image_height = (size_t) pOutput->imgProps.height;
        inputYDesc.mem_object = bufferSrc;
        ret = m_pOpenclSrvObj->RegPlane( pInput->data(), &bufferSrcY, &inputYFormat, &inputYDesc );
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input Y plane!" );
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
        ret = m_pOpenclSrvObj->RegPlane( pInput->data(), &bufferSrcUV, &inputUVFormat,
                                         &inputUVDesc );
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register input UV plane!" );
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
        OpenclArgs[5].pArg = (void *) &( pOutput->imgProps.stride[0] );
        OpenclArgs[5].argSize = sizeof( cl_int );
        OpenclArgs[6].pArg = (void *) &( pOutput->imgProps.planeBufSize[0] );
        OpenclArgs[6].argSize = sizeof( cl_int );
        OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[1] );
        OpenclArgs[7].argSize = sizeof( cl_int );
        uint32_t kernelROIX = m_config.ROIs[m_inputId].x;
        uint32_t kernelROIY = m_config.ROIs[m_inputId].y;
        OpenclArgs[8].pArg = (void *) &( kernelROIX );
        OpenclArgs[8].argSize = sizeof( cl_int );
        OpenclArgs[9].pArg = (void *) &( kernelROIY );
        OpenclArgs[9].argSize = sizeof( cl_int );

        OpenclIface_WorkParams_t OpenclWorkParams;
        OpenclWorkParams.workDim = 2;
        size_t globalWorkSize[2] = { (size_t) pOutput->imgProps.width,
                                     (size_t) pOutput->imgProps.height };
        OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
        size_t globalWorkOffset[2] = { 0, 0 };
        OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
        /*set local work size to NULL, device would choose optimal size automatically*/
        OpenclWorkParams.pLocalWorkSize = NULL;

        ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    }

    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute ConvertUBWC UBWC to NV12 OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
