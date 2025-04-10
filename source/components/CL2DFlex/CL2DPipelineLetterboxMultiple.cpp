// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "include/CL2DPipelineLetterboxMultiple.hpp"

namespace ridehal
{
namespace component
{

CL2DPipelineLetterboxMultiple::CL2DPipelineLetterboxMultiple() {}

CL2DPipelineLetterboxMultiple::~CL2DPipelineLetterboxMultiple() {}

RideHalError_e CL2DPipelineLetterboxMultiple::Init( uint32_t inputId, cl_kernel *pKernel,
                                                    CL2DFlex_Config_t *pConfig,
                                                    OpenclSrv *pOpenclSrvObj )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_inputId = inputId;
    m_pOpenclSrvObj = pOpenclSrvObj;
    m_config = *pConfig;

    if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.inputFormats[m_inputId] ) &&
         ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) && ( 1 == m_config.numOfInputs ) )
    {
        m_pipeline = CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB_MULTIPLE;
        ret = m_pOpenclSrvObj->CreateKernel( pKernel, "LetterboxNV12ToRGBMultiple" );
    }
    else
    {
        RIDEHAL_ERROR( "Invalid CL2DFlex letterbox multiple pipeline for inputId=%d!", m_inputId );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_pKernel = pKernel;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t roiProp = {
                RIDEHAL_TENSOR_TYPE_INT_32,
                { ( uint32_t )( RIDEHAL_CL2DFLEX_ROI_NUMBER_MAX * 4 ), 0 },
                1,
        };
        ret = m_roiBuffer.Allocate( &roiProp );
    }
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to allocate roi buffer!" );
    }


    return ret;
}

RideHalError_e CL2DPipelineLetterboxMultiple::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = m_roiBuffer.Free();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to deallocate roi buffer!" );
    }

    return ret;
}

RideHalError_e CL2DPipelineLetterboxMultiple::Execute( const RideHal_SharedBuffer_t *pInput,
                                                       const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_BAD_ARGUMENTS;

    // empty function

    return ret;
}

RideHalError_e CL2DPipelineLetterboxMultiple::ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                                              const RideHal_SharedBuffer_t *pOutput,
                                                              const CL2DFlex_ROIConfig_t *pROIs,
                                                              const uint32_t numROIs )
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
            uint32_t dstOffset = pOutput->offset;
            if ( CL2DFLEX_PIPELINE_LETTERBOX_NEAREST_NV12_TO_RGB_MULTIPLE == m_pipeline )
            {
                ret = LetterboxFromNV12ToRGBMultiple( numROIs, bufferSrc, srcOffset, bufferDst,
                                                      dstOffset, pInput, pOutput, pROIs );
            }
            else
            {
                RIDEHAL_ERROR( "Invalid CL2DFlex letterbox multiple pipeline for inputId=%d!",
                               m_inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DPipelineLetterboxMultiple::LetterboxFromNV12ToRGBMultiple(
        uint32_t numROIs, cl_mem bufferSrc, uint32_t srcOffset, cl_mem bufferDst,
        uint32_t dstOffset, const RideHal_SharedBuffer_t *pInput,
        const RideHal_SharedBuffer_t *pOutput, const CL2DFlex_ROIConfig_t *pROIs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    for ( int i = 0; i < numROIs; i++ )
    {
        if ( ( ( pROIs[i].x + pROIs[i].width ) <= pInput->imgProps.width ) &&
             ( ( pROIs[i].y + pROIs[i].height ) <= pInput->imgProps.height ) )
        {
            ( (int *) m_roiBuffer.data() )[i * 4 + 0] = pROIs[i].x;
            ( (int *) m_roiBuffer.data() )[i * 4 + 1] = pROIs[i].y;
            ( (int *) m_roiBuffer.data() )[i * 4 + 2] = pROIs[i].width;
            ( (int *) m_roiBuffer.data() )[i * 4 + 3] = pROIs[i].height;
        }
        else
        {
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Invalid roi parameter for inputId=%d\n!", i );
            break;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        cl_mem roiBufferCL;
        ret = m_pOpenclSrvObj->RegBuf( &( m_roiBuffer.buffer ), &roiBufferCL );
        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to register roi buffer!" );
        }
        else
        {
            size_t numOfArgs = 12;
            OpenclIfcae_Arg_t OpenclArgs[12];
            OpenclArgs[0].pArg = (void *) &bufferSrc;
            OpenclArgs[0].argSize = sizeof( cl_mem );
            OpenclArgs[1].pArg = (void *) &srcOffset;
            OpenclArgs[1].argSize = sizeof( cl_int );
            OpenclArgs[2].pArg = (void *) &bufferDst;
            OpenclArgs[2].argSize = sizeof( cl_mem );
            OpenclArgs[3].pArg = (void *) &dstOffset;
            OpenclArgs[3].argSize = sizeof( cl_int );
            OpenclArgs[4].pArg = (void *) &roiBufferCL;
            OpenclArgs[4].argSize = sizeof( cl_mem );
            OpenclArgs[5].pArg = (void *) &( m_config.outputHeight );
            OpenclArgs[5].argSize = sizeof( cl_int );
            OpenclArgs[6].pArg = (void *) &( m_config.outputWidth );
            OpenclArgs[6].argSize = sizeof( cl_int );
            OpenclArgs[7].pArg = (void *) &( pInput->imgProps.stride[0] );
            OpenclArgs[7].argSize = sizeof( cl_int );
            OpenclArgs[8].pArg = (void *) &( pInput->imgProps.planeBufSize[0] );
            OpenclArgs[8].argSize = sizeof( cl_int );
            OpenclArgs[9].pArg = (void *) &( pInput->imgProps.stride[1] );
            OpenclArgs[9].argSize = sizeof( cl_int );
            OpenclArgs[10].pArg = (void *) &( pOutput->imgProps.stride[0] );
            OpenclArgs[10].argSize = sizeof( cl_int );
            OpenclArgs[11].pArg = (void *) &( m_config.letterboxPaddingValue );
            OpenclArgs[11].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams;
            OpenclWorkParams.workDim = 3;
            size_t globalWorkSize[3] = { numROIs, pOutput->imgProps.width,
                                         pOutput->imgProps.height };
            OpenclWorkParams.pGlobalWorkSize = globalWorkSize;
            size_t globalWorkOffset[3] = { 0, 0, 0 };
            OpenclWorkParams.pGlobalWorkOffset = globalWorkOffset;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams.pLocalWorkSize = NULL;

            ret = m_pOpenclSrvObj->Execute( m_pKernel, OpenclArgs, numOfArgs, &OpenclWorkParams );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute LetterboxMultiple NV12 to RGB OpenCL kernel!" );
                ret = RIDEHAL_ERROR_FAIL;
            }

            ret = m_pOpenclSrvObj->DeregBuf( &( m_roiBuffer.buffer ) );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to deregister roi buffer!" );
            }
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
