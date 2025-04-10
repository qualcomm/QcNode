// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "include/CL2DPipelineConvert.hpp"

namespace ridehal
{
namespace component
{

CL2DPipelineConvert::CL2DPipelineConvert() {}

CL2DPipelineConvert::~CL2DPipelineConvert() {}

RideHalError_e CL2DPipelineConvert::Init( uint32_t inputId, cl_kernel *pKernel,
                                          CL2DFlex_Config_t *pConfig, OpenclSrv *pOpenclSrvObj )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( m_config.outputWidth == m_config.ROIs[m_inputId].width ) &&
         ( m_config.outputHeight == m_config.ROIs[m_inputId].height ) &&
         ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
         ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_CONVERT_NV12_TO_RGB;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ConvertNV12ToRGB" );
    }
    else if ( ( m_config.outputWidth == m_config.ROIs[m_inputId].width ) &&
              ( m_config.outputHeight == m_config.ROIs[m_inputId].height ) &&
              ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[m_inputId] ) &&
              ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_RGB;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ConvertUYVYToRGB" );
    }
    else if ( ( m_config.outputWidth == m_config.ROIs[m_inputId].width ) &&
              ( m_config.outputHeight == m_config.ROIs[m_inputId].height ) &&
              ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.inputFormats[m_inputId] ) &&
              ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_NV12;
        ret = pOpenclSrvObj->CreateKernel( pKernel, "ConvertUYVYToNV12" );
    }
    else
    {
        RIDEHAL_ERROR( "Invalid CL2DFlex convert pipeline for inputId=%d!", m_inputId );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    return ret;
}

RideHalError_e CL2DPipelineConvert::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    // empty function

    return ret;
}

RideHalError_e CL2DPipelineConvert::Execute( const RideHal_SharedBuffer_t *pInput,
                                             const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    cl_mem bufferDst;
    ret = m_pOpenclSrvObj->RegBuf( &( pOutput->buffer ), &bufferDst );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register output buffer!" );
    }
    else
    {
        cl_mem bufferSrc;
        ret = m_pOpenclSrvObj->RegBuf( &( pInput->buffer ), &bufferSrc );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register input buffer!" );
        }
        else
        {
            uint32_t srcOffset = pInput->offset;
            uint32_t sizeOne = ( uint32_t )( pOutput->size ) / ( pOutput->imgProps.batchSize );
            uint32_t dstOffset = ( uint32_t )( pOutput->offset ) + m_inputId * sizeOne;

            if ( CL2DFLEX_PIPELINE_CONVERT_NV12_TO_RGB == m_pipeline )
            {
                ret = ConvertFromNV12ToRGB( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                            pOutput );
            }
            else if ( CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_RGB == m_pipeline )
            {
                ret = ConvertFromUYVYToRGB( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                            pOutput );
            }
            else if ( CL2DFLEX_PIPELINE_CONVERT_UYVY_TO_NV12 == m_pipeline )
            {
                ret = ConvertFromUYVYToNV12( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                             pOutput );
            }
            else
            {
                RIDEHAL_ERROR( "Invalid CL2DFlex convert pipeline for inputId=%d!", m_inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DPipelineConvert::ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                                    const RideHal_SharedBuffer_t *pOutput,
                                                    const CL2DFlex_ROIConfig_t *pROIs,
                                                    const uint32_t numROIs )
{
    RideHalError_e ret = RIDEHAL_ERROR_BAD_ARGUMENTS;

    // empty function

    return ret;
}

RideHalError_e CL2DPipelineConvert::ConvertFromNV12ToRGB( cl_mem bufferSrc, uint32_t srcOffset,
                                                          cl_mem bufferDst, uint32_t dstOffset,
                                                          const RideHal_SharedBuffer_t *pInput,
                                                          const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 10;
    OpenclIfcae_Arg_t OpenclArgs[10];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( pInput->imgProps.stride[1] );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[7].argSize = sizeof( cl_int );
    uint32_t kernelROIX = m_config.ROIs[m_inputId].x / 2;
    uint32_t kernelROIY = m_config.ROIs[m_inputId].y / 2;
    OpenclArgs[8].pArg = (void *) &( kernelROIX );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( kernelROIY );
    OpenclArgs[9].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { ( size_t )( pOutput->imgProps.width ) / 2,
                                 ( size_t )( pOutput->imgProps.height ) / 2 };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert NV12 to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DPipelineConvert::ConvertFromUYVYToRGB( cl_mem bufferSrc, uint32_t srcOffset,
                                                          cl_mem bufferDst, uint32_t dstOffset,
                                                          const RideHal_SharedBuffer_t *pInput,
                                                          const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 8;
    OpenclIfcae_Arg_t OpenclArgs[8];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[5].argSize = sizeof( cl_int );
    uint32_t kernelROIX = m_config.ROIs[m_inputId].x / 2;
    uint32_t kernelROIY = m_config.ROIs[m_inputId].y;
    OpenclArgs[6].pArg = (void *) &( kernelROIX );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( kernelROIY );
    OpenclArgs[7].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { ( size_t )( pOutput->imgProps.width ) / 2,
                                 pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert UYVY to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DPipelineConvert::ConvertFromUYVYToNV12( cl_mem bufferSrc, uint32_t srcOffset,
                                                           cl_mem bufferDst, uint32_t dstOffset,
                                                           const RideHal_SharedBuffer_t *pInput,
                                                           const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 10;
    OpenclIfcae_Arg_t OpenclArgs[10];
    OpenclArgs[0].pArg = (void *) &bufferSrc;
    OpenclArgs[0].argSize = sizeof( cl_mem );
    OpenclArgs[1].pArg = (void *) &srcOffset;
    OpenclArgs[1].argSize = sizeof( cl_int );
    OpenclArgs[2].pArg = (void *) &bufferDst;
    OpenclArgs[2].argSize = sizeof( cl_mem );
    OpenclArgs[3].pArg = (void *) &dstOffset;
    OpenclArgs[3].argSize = sizeof( cl_int );
    OpenclArgs[4].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[4].argSize = sizeof( cl_int );
    OpenclArgs[5].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[5].argSize = sizeof( cl_int );
    OpenclArgs[6].pArg = (void *) &( pOutput->imgProps.planeBufSize[0] );
    OpenclArgs[6].argSize = sizeof( cl_int );
    OpenclArgs[7].pArg = (void *) &( pOutput->imgProps.stride[1] );
    OpenclArgs[7].argSize = sizeof( cl_int );
    uint32_t kernelROIX = m_config.ROIs[m_inputId].x / 2;
    uint32_t kernelROIY = m_config.ROIs[m_inputId].y / 2;
    OpenclArgs[8].pArg = (void *) &( kernelROIX );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( kernelROIY );
    OpenclArgs[9].argSize = sizeof( cl_int );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { ( size_t )( pOutput->imgProps.width ) / 2,
                                 ( size_t )( pOutput->imgProps.height ) / 2 };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute convert UYVY to NV12 OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
