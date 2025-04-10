// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/component/CL2DFlex.hpp"
#include "include/CL2DPipelineBase.hpp"
#include "include/CL2DPipelineConvert.hpp"
#include "include/CL2DPipelineConvertUBWC.hpp"
#include "include/CL2DPipelineLetterbox.hpp"
#include "include/CL2DPipelineLetterboxMultiple.hpp"
#include "include/CL2DPipelineRemap.hpp"
#include "include/CL2DPipelineResize.hpp"
#include "include/CL2DPipelineResizeMultiple.hpp"
#include "kernel/CL2DFlex.cl.h"

namespace ridehal
{
namespace component
{

CL2DFlex::CL2DFlex() {}

CL2DFlex::~CL2DFlex() {}

RideHalError_e CL2DFlex::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
    }
    else
    {
        RIDEHAL_ERROR( "CL2DFlex component start failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e CL2DFlex::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING == m_state )
    {
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }
    else
    {
        RIDEHAL_ERROR( "CL2DFlex component stop failed due to wrong state!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e CL2DFlex::Init( const char *pName, const CL2DFlex_Config_t *pConfig,
                               Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "Failed to init component!" );
    }
    else
    {
        m_state = RIDEHAL_COMPONENT_STATE_INITIALIZING;

        if ( nullptr == pConfig )
        {
            RIDEHAL_ERROR( "Empty config pointer!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else if ( RIDEHAL_MAX_INPUTS < pConfig->numOfInputs )
        {
            RIDEHAL_ERROR( "Invalid inputs number!" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        else
        {
            for ( uint32_t inputId = 0; inputId < pConfig->numOfInputs; inputId++ )
            {
                if ( ( 0 != ( pConfig->inputWidths[inputId] % 2 ) ) ||
                     ( 0 == pConfig->inputWidths[inputId] ) )
                {
                    RIDEHAL_ERROR( "Invalid input width!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( ( 0 != ( pConfig->inputHeights[inputId] % 2 ) ) ||
                          ( 0 == pConfig->inputHeights[inputId] ) )
                {
                    RIDEHAL_ERROR( "Invalid input height!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( pConfig->inputWidths[inputId] <
                          ( pConfig->ROIs[inputId].x + pConfig->ROIs[inputId].width ) )
                {
                    RIDEHAL_ERROR( "Invalid ROI values, (ROI.x + ROI.width) > inputWidth!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( pConfig->inputHeights[inputId] <
                          ( pConfig->ROIs[inputId].y + pConfig->ROIs[inputId].height ) )
                {
                    RIDEHAL_ERROR( "Invalid ROI values, (ROI.y + ROI.height) > inputHeight!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == pConfig->workModes[inputId] ) &&
                          ( nullptr == pConfig->remapTable[inputId].pMapX ) )
                {
                    RIDEHAL_ERROR( "Invalid map table X!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else if ( ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == pConfig->workModes[inputId] ) &&
                          ( nullptr == pConfig->remapTable[inputId].pMapY ) )
                {
                    RIDEHAL_ERROR( "Invalid map table Y!" );
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
                else
                {
                    // empty else block
                }
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_config = *pConfig;
            ret = m_OpenclSrvObj.Init( pName, level, m_config.priority );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Init OpenCL failed!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                ret = m_OpenclSrvObj.LoadFromSource( s_pSourceCL2DFlex );
            }
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Load program from source file s_pSourceCL2DFlex failed!" );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
                {
                    m_pCL2DPipeline[inputId] = nullptr;

                    if ( CL2DFLEX_WORK_MODE_CONVERT == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineConvert();
                    }
                    else if ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineResize();
                    }
                    else if ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineLetterbox();
                    }
                    else if ( CL2DFLEX_WORK_MODE_CONVERT_UBWC == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineConvertUBWC();
                    }
                    else if ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE ==
                              m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineLetterboxMultiple();
                    }
                    else if ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE ==
                              m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineResizeMultiple();
                    }
                    else if ( CL2DFLEX_WORK_MODE_REMAP_NEAREST == m_config.workModes[inputId] )
                    {
                        m_pCL2DPipeline[inputId] = new CL2DPipelineRemap();
                    }
                    else
                    {
                        RIDEHAL_ERROR( "Invalid CL2DFlex work mode for inputId=%d!", inputId );
                        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    }

                    if ( nullptr != m_pCL2DPipeline[inputId] )
                    {
                        m_pCL2DPipeline[inputId]->InitLogger( pName, level );
                        ret = m_pCL2DPipeline[inputId]->Init( inputId, &m_kernel[inputId],
                                                              &m_config, &m_OpenclSrvObj );
                    }
                    else
                    {
                        RIDEHAL_ERROR( "CL2D pipeline create failed for inputId=%d!", inputId );
                        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    }

                    if ( RIDEHAL_ERROR_NONE != ret )
                    {
                        RIDEHAL_ERROR( "Setup pipeline failed for inputId=%d!", inputId );
                        break;
                    }
                }
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_state = RIDEHAL_COMPONENT_STATE_READY;
        }
        else
        {
            m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
            RideHalError_e retVal;
            retVal = ComponentIF::Deinit();
            if ( RIDEHAL_ERROR_NONE != retVal )
            {
                RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "CL2DFlex component not in ready status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        RideHalError_e retVal;

        retVal = m_OpenclSrvObj.Deinit();
        if ( RIDEHAL_ERROR_NONE != retVal )
        {
            RIDEHAL_ERROR( "Release CL resources failed!" );
            ret = RIDEHAL_ERROR_FAIL;
        }

        for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
        {
            if ( nullptr != m_pCL2DPipeline[inputId] )
            {
                m_pCL2DPipeline[inputId]->DeinitLogger();
                retVal = m_pCL2DPipeline[inputId]->Deinit();
                if ( RIDEHAL_ERROR_NONE != retVal )
                {
                    RIDEHAL_ERROR( "Deinit pipeline failed for inputId=%d!", inputId );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                delete m_pCL2DPipeline[inputId];
            }
        }

        retVal = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE != retVal )
        {
            RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}


RideHalError_e CL2DFlex::RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                          uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            if ( RIDEHAL_IMAGE_FORMAT_NV12_UBWC == pBuffers[i].imgProps.format )
            {
                RideHalError_e retVal = RIDEHAL_ERROR_NONE;
                cl_mem bufferSrc;
                cl_mem bufferSrcY;
                cl_mem bufferSrcUV;
                cl_image_format inputImageFormat = { 0 };
                inputImageFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12;
                inputImageFormat.image_channel_data_type = CL_UNORM_INT8;
                cl_image_desc inputImageDesc = { 0 };
                inputImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
                inputImageDesc.image_width = (size_t) pBuffers[i].imgProps.width;
                inputImageDesc.image_height = (size_t) pBuffers[i].imgProps.height;
                retVal = m_OpenclSrvObj.RegImage( pBuffers[i].data(), pBuffers[i].buffer.dmaHandle,
                                                  &bufferSrc, &inputImageFormat, &inputImageDesc );
                if ( RIDEHAL_ERROR_NONE != retVal )
                {
                    ret = RIDEHAL_ERROR_FAIL;
                    RIDEHAL_ERROR( "Failed to register input NV12 UBWC image!" );
                }
                else
                {
                    cl_image_format inputYFormat = { 0 };
                    inputYFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_Y;
                    inputYFormat.image_channel_data_type = CL_UNORM_INT8;
                    cl_image_desc inputYDesc = { 0 };
                    inputYDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
                    inputYDesc.image_width = (size_t) m_config.outputWidth;
                    inputYDesc.image_height = (size_t) m_config.outputHeight;
                    inputYDesc.mem_object = bufferSrc;
                    retVal = m_OpenclSrvObj.RegPlane( pBuffers[i].data(), &bufferSrcY,
                                                      &inputYFormat, &inputYDesc );
                }
                if ( RIDEHAL_ERROR_NONE != retVal )
                {
                    ret = RIDEHAL_ERROR_FAIL;
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
                    retVal = m_OpenclSrvObj.RegPlane( pBuffers[i].data(), &bufferSrcUV,
                                                      &inputUVFormat, &inputUVDesc );
                }
                if ( RIDEHAL_ERROR_NONE != retVal )
                {
                    ret = RIDEHAL_ERROR_FAIL;
                    RIDEHAL_ERROR( "Failed to register input UV plane!" );
                }
            }
            else
            {
                cl_mem bufferCL;
                ret = m_OpenclSrvObj.RegBuf( &( pBuffers[i].buffer ), &bufferCL );
            }

            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to register buffer for number %d!", i );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                            uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not in ready or running status!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            if ( RIDEHAL_IMAGE_FORMAT_NV12_UBWC == pBuffers[i].imgProps.format )
            {
                RideHalError_e retVal = RIDEHAL_ERROR_NONE;
                retVal = m_OpenclSrvObj.DeregImage( pBuffers[i].data() );
                if ( RIDEHAL_ERROR_NONE != retVal )
                {
                    ret = RIDEHAL_ERROR_FAIL;
                    RIDEHAL_ERROR( "Failed to deregister input NV12 UBWC image!" );
                }
                else
                {
                    cl_image_format inputYFormat = { 0 };
                    inputYFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_Y;
                    retVal = m_OpenclSrvObj.DeregPlane( pBuffers[i].data(), &inputYFormat );
                }
                if ( RIDEHAL_ERROR_NONE != retVal )
                {
                    ret = RIDEHAL_ERROR_FAIL;
                    RIDEHAL_ERROR( "Failed to deregister input Y plane!" );
                }
                else
                {
                    cl_image_format inputUVFormat = { 0 };
                    inputUVFormat.image_channel_order = CL_QCOM_COMPRESSED_NV12_UV;
                    retVal = m_OpenclSrvObj.DeregPlane( pBuffers[i].data(), &inputUVFormat );
                }
                if ( RIDEHAL_ERROR_NONE != retVal )
                {
                    ret = RIDEHAL_ERROR_FAIL;
                    RIDEHAL_ERROR( "Failed to deregister input UV plane!" );
                }
            }
            else
            {
                ret = m_OpenclSrvObj.DeregBuf( &( pBuffers[i].buffer ) );
            }
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to deregister buffer for number %d!", i );
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::Execute( const RideHal_SharedBuffer_t *pInputs, const uint32_t numInputs,
                                  const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not initialized!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pOutput )
    {
        RIDEHAL_ERROR( "Output buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pInputs )
    {
        RIDEHAL_ERROR( "Input buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( numInputs != m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "number of inputs not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pOutput->type )
    {
        RIDEHAL_ERROR( "Output buffer is not image type!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputFormat != pOutput->imgProps.format )
    {
        RIDEHAL_ERROR( "Output image format not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputWidth != pOutput->imgProps.width )
    {
        RIDEHAL_ERROR( "Output image width not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputHeight != pOutput->imgProps.height )
    {
        RIDEHAL_ERROR( "Output image height not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( numInputs != pOutput->imgProps.batchSize )
    {
        RIDEHAL_ERROR( "Output image batch not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < numInputs; inputId++ )
        {
            if ( nullptr == pInputs[inputId].data() )
            {
                RIDEHAL_ERROR( "Input buffer data is null for inputId=%d!", inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pInputs[inputId].type )
            {
                RIDEHAL_ERROR( "Input buffer is not image type for inputId=%d!", inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( m_config.inputFormats[inputId] != pInputs[inputId].imgProps.format )
            {
                RIDEHAL_ERROR( "Input image format not match for inputId=%d!", inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( m_config.inputWidths[inputId] != pInputs[inputId].imgProps.width )
            {
                RIDEHAL_ERROR( "Input image width not match for inputId=%d!", inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else if ( m_config.inputHeights[inputId] != pInputs[inputId].imgProps.height )
            {
                RIDEHAL_ERROR( "Input image height not match for inputId=%d!", inputId );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            else
            {
                if ( nullptr == m_pCL2DPipeline[inputId] )
                {
                    RIDEHAL_ERROR( "Pipeline not setup for inputId=%d!", inputId );
                    ret = RIDEHAL_ERROR_FAIL;
                }
                else
                {
                    ret = m_pCL2DPipeline[inputId]->Execute( &pInputs[inputId], pOutput );
                }
            }
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to execute pipeline for inputId=%d!", inputId );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e CL2DFlex::ExecuteWithROI( const RideHal_SharedBuffer_t *pInput,
                                         const RideHal_SharedBuffer_t *pOutput,
                                         const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "CL2DFlex component not initialized!" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pInput )
    {
        RIDEHAL_ERROR( "Input buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pOutput )
    {
        RIDEHAL_ERROR( "Output buffer is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pROIs )
    {
        RIDEHAL_ERROR( "ROI configurations is null!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 1 != m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "number of inputs not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pOutput->data() )
    {
        RIDEHAL_ERROR( "Output buffer data is null" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pOutput->type )
    {
        RIDEHAL_ERROR( "Output buffer is not image type!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputFormat != pOutput->imgProps.format )
    {
        RIDEHAL_ERROR( "Output image format not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputWidth != pOutput->imgProps.width )
    {
        RIDEHAL_ERROR( "Output image width not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.outputHeight != pOutput->imgProps.height )
    {
        RIDEHAL_ERROR( "Output image height not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( numROIs != pOutput->imgProps.batchSize )
    {
        RIDEHAL_ERROR( "Output image batch not match!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( nullptr == pInput->data() )
    {
        RIDEHAL_ERROR( "Input buffer data is null" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( RIDEHAL_BUFFER_TYPE_IMAGE != pInput->type )
    {
        RIDEHAL_ERROR( "Input buffer is not image type" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputFormats[0] != pInput->imgProps.format )
    {
        RIDEHAL_ERROR( "Input image format not match" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputWidths[0] != pInput->imgProps.width )
    {
        RIDEHAL_ERROR( "Input image width not match" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( m_config.inputHeights[0] != pInput->imgProps.height )
    {
        RIDEHAL_ERROR( "Input image height not match" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        if ( nullptr == m_pCL2DPipeline[0] )
        {
            RIDEHAL_ERROR( "Pipeline not setup for inputId=0!" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            ret = m_pCL2DPipeline[0]->ExecuteWithROI( pInput, pOutput, pROIs, numROIs );
        }

        if ( RIDEHAL_ERROR_NONE != ret )
        {
            RIDEHAL_ERROR( "Failed to execute pipeline with ROI!" );
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
