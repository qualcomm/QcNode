// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "include/CL2DPipelineRemap.hpp"

namespace ridehal
{
namespace component
{

CL2DPipelineRemap::CL2DPipelineRemap() {}

CL2DPipelineRemap::~CL2DPipelineRemap() {}

RideHalError_e CL2DPipelineRemap::Init( uint32_t inputId, cl_kernel *pKernel,
                                        CL2DFlex_Config_t *pConfig, OpenclSrv *pOpenclSrvObj )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
         ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_REMAP_NEAREST_NV12_TO_RGB;
        ret = m_pOpenclSrvObj->CreateKernel( pKernel, "RemapNV12ToRGB" );
    }
    else if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
              ( RIDEHAL_IMAGE_FORMAT_BGR888 == m_config.outputFormat ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_REMAP_NEAREST_NV12_TO_BGR;
        ret = m_pOpenclSrvObj->CreateKernel( pKernel, "RemapNV12ToBGR" );
    }
    else
    {
        RIDEHAL_ERROR( "Invalid CL2DFlex Remap pipeline for inputId=%d!", m_inputId );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pOpenclSrvObj->RegBuf( &( m_config.remapTable[inputId].pMapX->buffer ),
                                       &m_bufferMapX );
    }
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register mapX buffer!" );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pOpenclSrvObj->RegBuf( &( m_config.remapTable[inputId].pMapY->buffer ),
                                       &m_bufferMapY );
    }
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to register mapY buffer!" );
    }

    return ret;
}

RideHalError_e CL2DPipelineRemap::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    // empty function

    return ret;
}

RideHalError_e CL2DPipelineRemap::Execute( const RideHal_SharedBuffer_t *pInput,
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

            if ( CL2DFLEX_PIPELINE_REMAP_NEAREST_NV12_TO_RGB == m_pipeline )
            {
                ret = RemapFromNV12ToRGB( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                          pOutput );
            }
            else if ( CL2DFLEX_PIPELINE_REMAP_NEAREST_NV12_TO_BGR == m_pipeline )
            {
                ret = RemapFromNV12ToBGR( bufferSrc, srcOffset, bufferDst, dstOffset, pInput,
                                          pOutput );
            }
            else
            {
                RIDEHAL_ERROR( "Invalid CL2DFlex Remap pipeline for m_inputId=%d!", m_inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DPipelineRemap::ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                                  const RideHal_SharedBuffer_t *pOutput,
                                                  const CL2DFlex_ROIConfig_t *pROIs,
                                                  const uint32_t numROIs )
{
    RideHalError_e ret = RIDEHAL_ERROR_BAD_ARGUMENTS;

    // empty function

    return ret;
}

RideHalError_e CL2DPipelineRemap::RemapFromNV12ToRGB( cl_mem bufferSrc, uint32_t srcOffset,
                                                      cl_mem bufferDst, uint32_t dstOffset,
                                                      const RideHal_SharedBuffer_t *pInput,
                                                      const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 16;
    OpenclIfcae_Arg_t OpenclArgs[16];
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
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( pInput->imgProps.stride[1] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    OpenclArgs[12].pArg = (void *) &( m_config.ROIs[m_inputId].x );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( m_config.ROIs[m_inputId].y );
    OpenclArgs[13].argSize = sizeof( cl_int );
    OpenclArgs[14].pArg = (void *) &m_bufferMapX;
    OpenclArgs[14].argSize = sizeof( cl_mem );
    OpenclArgs[15].pArg = (void *) &m_bufferMapY;
    OpenclArgs[15].argSize = sizeof( cl_mem );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute remap NV12 to RGB OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e CL2DPipelineRemap::RemapFromNV12ToBGR( cl_mem bufferSrc, uint32_t srcOffset,
                                                      cl_mem bufferDst, uint32_t dstOffset,
                                                      const RideHal_SharedBuffer_t *pInput,
                                                      const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    size_t numOfArgs = 16;
    OpenclIfcae_Arg_t OpenclArgs[16];
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
    OpenclArgs[8].pArg = (void *) &( pInput->imgProps.stride[0] );
    OpenclArgs[8].argSize = sizeof( cl_int );
    OpenclArgs[9].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
    OpenclArgs[9].argSize = sizeof( cl_int );
    OpenclArgs[10].pArg = (void *) &( pInput->imgProps.stride[1] );
    OpenclArgs[10].argSize = sizeof( cl_int );
    OpenclArgs[11].pArg = (void *) &( pOutput->imgProps.stride[0] );
    OpenclArgs[11].argSize = sizeof( cl_int );
    OpenclArgs[12].pArg = (void *) &( m_config.ROIs[m_inputId].x );
    OpenclArgs[12].argSize = sizeof( cl_int );
    OpenclArgs[13].pArg = (void *) &( m_config.ROIs[m_inputId].y );
    OpenclArgs[13].argSize = sizeof( cl_int );
    OpenclArgs[14].pArg = (void *) &m_bufferMapX;
    OpenclArgs[14].argSize = sizeof( cl_mem );
    OpenclArgs[15].pArg = (void *) &m_bufferMapY;
    OpenclArgs[15].argSize = sizeof( cl_mem );

    OpenclIface_WorkParams_t OpenclWorkParams;
    OpenclWorkParams.workDim = 2;
    size_t globalWorkSize[2] = { pOutput->imgProps.width, pOutput->imgProps.height };
    OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
    size_t globalWorkOffset[2] = { 0, 0 };
    OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
    /*set local work size to NULL, device would choose optimal size automatically*/
    OpenclWorkParams.pLocalWorkSize = NULL;

    ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to execute remap NV12 to BGR OpenCL kernel!" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
