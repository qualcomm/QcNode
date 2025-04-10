// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include <cinttypes>
#include <cstring>
#include <memory>

#include "ridehal/component/C2D.hpp"

namespace ridehal
{
namespace component
{

C2D::C2D() {}

C2D::~C2D() {}

RideHalError_e C2D::Init( const char *pName, const C2D_Config_t *pConfig, Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "ComponentIF::Init failed" );
    }
    else
    {
        m_numOfInputs = pConfig->numOfInputs;
        if ( m_numOfInputs > RIDEHAL_MAX_INPUTS )
        {
            ret = RIDEHAL_ERROR_OUT_OF_BOUND;
            RIDEHAL_ERROR( "Number of Inputs exceeds maximum limit" );
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            for ( uint32_t i = 0; i < m_numOfInputs; i++ )
            {
                m_inputResolutions[i].width = pConfig->inputConfigs[i].inputResolution.width;
                m_inputResolutions[i].height = pConfig->inputConfigs[i].inputResolution.height;
                m_inputFormats[i] = pConfig->inputConfigs[i].inputFormat;

                if ( pConfig->inputConfigs[i].ROI.topX >= 0 &&
                     pConfig->inputConfigs[i].ROI.topX <= m_inputResolutions[i].width )
                {
                    m_rois[i].topX = pConfig->inputConfigs[i].ROI.topX;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI topX of input %u is out of range", i );
                    break;
                }

                if ( pConfig->inputConfigs[i].ROI.topY >= 0 &&
                     pConfig->inputConfigs[i].ROI.topY <= m_inputResolutions[i].height )
                {
                    m_rois[i].topY = pConfig->inputConfigs[i].ROI.topY;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI topY of input %u is out of range", i );
                    break;
                }

                if ( pConfig->inputConfigs[i].ROI.width >= 0 &&
                     pConfig->inputConfigs[i].ROI.width <=
                             m_inputResolutions[i].width - m_rois[i].topX )
                {
                    m_rois[i].width = pConfig->inputConfigs[i].ROI.width;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI width of input %u is out of range", i );
                    break;
                }

                if ( pConfig->inputConfigs[i].ROI.height >= 0 &&
                     pConfig->inputConfigs[i].ROI.height <=
                             m_inputResolutions[i].height - m_rois[i].topY )
                {
                    m_rois[i].height = pConfig->inputConfigs[i].ROI.height;
                }
                else
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                    RIDEHAL_ERROR( "ROI height of input %u is out of range", i );
                    break;
                }
            }

            /* Complete initialization */
            if ( RIDEHAL_ERROR_NONE == ret )
            {
                m_state = RIDEHAL_COMPONENT_STATE_READY;
                RIDEHAL_INFO( "Component C2D is initialized" );
            }
        }
    }

    return ret;
}

RideHalError_e C2D::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // DO start
        m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
        RIDEHAL_INFO( "Component C2D start to run" );
    }

    return ret;
}

RideHalError_e C2D::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        // DO stop
        m_state = RIDEHAL_COMPONENT_STATE_READY;
        RIDEHAL_INFO( "Component C2D is stopped" );
    }

    return ret;
}

RideHalError_e C2D::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( auto it = m_inputBufferSurfaceMap.begin(); it != m_inputBufferSurfaceMap.end(); it++ )
        {
            auto c2dStatus = c2dDestroySurface( it->second.surface_id );
            if ( C2D_STATUS_OK != c2dStatus )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to destroy surface for input buffer" );
            }
        }
        for ( auto it = m_outputBufferSurfaceMap.begin(); it != m_outputBufferSurfaceMap.end();
              it++ )
        {
            auto c2dStatus = c2dDestroySurface( it->second );
            if ( C2D_STATUS_OK != c2dStatus )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to destroy surface for output buffer" );
            }
        }
        m_inputBufferSurfaceMap.clear();
        m_outputBufferSurfaceMap.clear();

        ret = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            /* Complete deinitialization */
            m_state = RIDEHAL_COMPONENT_STATE_INITIAL;
            RIDEHAL_INFO( "Component C2D is deinitialized" );
        }
        else
        {
            RIDEHAL_ERROR( "ComponentIF::Deinit failed" );
        }
    }

    return ret;
}

RideHalError_e C2D::Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                             const RideHal_SharedBuffer_t *pOutput )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t targetSurfaceId = 0;
    C2D_OBJECT c2dObject;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    if ( numInputs != m_numOfInputs )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of inputs not correct: %u != %u", m_numOfInputs, numInputs );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        if ( nullptr == pInputs )
        {
            ret = RIDEHAL_ERROR_INVALID_BUF;
            RIDEHAL_ERROR( "Input buffer is null" );
        }
        else if ( numInputs != m_numOfInputs )
        {
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Number of inputs not correct: %u != %u", m_numOfInputs, numInputs );
        }
        else if ( nullptr == pOutput )
        {
            ret = RIDEHAL_ERROR_INVALID_BUF;
            RIDEHAL_ERROR( "Output buffer is null" );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < m_numOfInputs; i++ )
        {
            if ( pInputs[i].imgProps.format != m_inputFormats[i] )
            {
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                RIDEHAL_ERROR( "Input %u format is not correct: %d != %d", i,
                               (int) m_inputFormats[i], (int) pOutput->imgProps.format );
            }
            else if ( pInputs[i].imgProps.width != m_inputResolutions[i].width )
            {
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                RIDEHAL_ERROR( "Input %u width is not correct: %u != %u", i,
                               m_inputResolutions[i].width, pInputs[i].imgProps.width );
            }
            else if ( pInputs[i].imgProps.height != m_inputResolutions[i].height )
            {
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                RIDEHAL_ERROR( "Input %u height is not correct: %u != %u", i,
                               m_inputResolutions[i].height, pInputs[i].imgProps.height );
            }
        }
        if ( pOutput->imgProps.batchSize != m_numOfInputs )
        {
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            RIDEHAL_ERROR( "Output batch size is not correct, numOfInputs %u != batchSize %u",
                           m_numOfInputs, pOutput->imgProps.batchSize );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < m_numOfInputs; i++ )
        {

            ret = GetSourceSurface( &pInputs[i], i, c2dObject );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to get source surface for input %u: ", i );
                break;
            }

            ret = GetTargetSurface( pOutput, i, &targetSurfaceId );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to get target surface for output batch %u: ", i );
                break;
            }

            auto c2dStatus =
                    c2dDraw( targetSurfaceId, C2D_TARGET_ROTATE_0, nullptr, 0, 0, &c2dObject, 1 );
            if ( C2D_STATUS_OK != c2dStatus )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to draw blit objects for input %u: ", i );
                break;
            }

            c2dStatus = c2dFinish( targetSurfaceId );
            if ( C2D_STATUS_OK != c2dStatus )
            {
                ret = RIDEHAL_ERROR_FAIL;
                RIDEHAL_ERROR( "Failed to finish target, status = %d", c2dStatus );
                break;
            }
        }
    }

    return ret;
}

RideHalError_e C2D::RegisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                          uint32_t numOfInputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    bool isSource = true;
    uint32_t sourceSurfaceId = 0;
    uint32_t batchIdx = 0;
    C2D_OBJECT c2dObject;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfInputBuffers; i++ )
        {
            bufferAddr = pInputBuffer[i].data();
            if ( m_inputBufferSurfaceMap.find( bufferAddr ) != m_inputBufferSurfaceMap.end() )
            {
                continue;
            }
            else
            {
                ret = CreateSourceSurface( &pInputBuffer[i], i, c2dObject );
                if ( ret != RIDEHAL_ERROR_NONE )
                {
                    RIDEHAL_ERROR( "Failed to register input buffer %u: ", i );
                    break;
                }
            }
        }
    }

    return ret;
}

RideHalError_e C2D::RegisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                           uint32_t numOfOutputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    uint32_t targetSurfaceId = 0;
    bool isSource = false;
    uint32_t outputSize = pOutputBuffer->size / pOutputBuffer->imgProps.batchSize;

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfOutputBuffers; i++ )
        {
            for ( size_t k = 0; k < m_numOfInputs; k++ )
            {
                bufferAddr = (void *) ( (uint8_t *) pOutputBuffer[i].data() + k * outputSize );
                if ( m_outputBufferSurfaceMap.find( bufferAddr ) != m_outputBufferSurfaceMap.end() )
                {
                    continue;
                }
                else
                {
                    ret = CreateTargetSurface( pOutputBuffer, k, &targetSurfaceId );
                    if ( ret != RIDEHAL_ERROR_NONE )
                    {
                        RIDEHAL_ERROR( "Failed to register output buffer %u for batch %u: ", i, k );
                        break;
                    }
                }
            }
        }
    }

    return ret;
}

RideHalError_e C2D::DeregisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffer,
                                            uint32_t numOfInputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    if ( numOfInputBuffers > m_inputBufferSurfaceMap.size() )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of deregister buffers greater than registered buffers " );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfInputBuffers; i++ )
        {
            bufferAddr = pInputBuffer[i].data();

            if ( m_inputBufferSurfaceMap.find( bufferAddr ) != m_inputBufferSurfaceMap.end() )
            {
                auto c2dStatus =
                        c2dDestroySurface( m_inputBufferSurfaceMap[bufferAddr].surface_id );
                if ( C2D_STATUS_OK != c2dStatus )
                {
                    ret = RIDEHAL_ERROR_FAIL;
                    RIDEHAL_ERROR( "Failed to destroy surface for input buffer %u", i );
                }
                (void) m_inputBufferSurfaceMap.erase( bufferAddr );
            }
        }
    }

    return ret;
}

RideHalError_e C2D::DeregisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffer,
                                             uint32_t numOfOutputBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = nullptr;
    uint32_t outputSize = pOutputBuffer->size / pOutputBuffer->imgProps.batchSize;

    if ( numOfOutputBuffers > m_outputBufferSurfaceMap.size() )
    {
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        RIDEHAL_ERROR( "Number of deregister buffers greater than registered buffers " );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        for ( size_t i = 0; i < numOfOutputBuffers; i++ )
        {
            for ( size_t k = 0; k < m_numOfInputs; k++ )
            {
                bufferAddr = (void *) ( (uint8_t *) pOutputBuffer[i].data() + k * outputSize );
                if ( m_outputBufferSurfaceMap.find( bufferAddr ) != m_outputBufferSurfaceMap.end() )
                {
                    auto c2dStatus = c2dDestroySurface( m_outputBufferSurfaceMap[bufferAddr] );
                    if ( C2D_STATUS_OK != c2dStatus )
                    {
                        ret = RIDEHAL_ERROR_FAIL;
                        RIDEHAL_ERROR( "Failed to destroy surface for output buffer %u batch %u", i,
                                       k );
                    }
                    (void) m_outputBufferSurfaceMap.erase( bufferAddr );
                }
            }
        }
    }

    return ret;
}

RideHalError_e C2D::GetSourceSurface( const RideHal_SharedBuffer_t *pSharedBuffer,
                                      uint32_t inputIdx, C2D_OBJECT &c2dObject )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    void *bufferAddr = pSharedBuffer->data();
    uint32_t sourceSurfaceId = 0;

    /* Check if input buffer is registered */
    if ( m_inputBufferSurfaceMap.find( bufferAddr ) == m_inputBufferSurfaceMap.end() )
    {
        ret = CreateSourceSurface( pSharedBuffer, inputIdx, c2dObject );
    }
    else
    {
        c2dObject = m_inputBufferSurfaceMap[bufferAddr];
    }

    return ret;
}

RideHalError_e C2D::GetTargetSurface( const RideHal_SharedBuffer_t *pSharedBuffer,
                                      uint32_t batchIdx, uint32_t *surfaceId )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    uint32_t outputSize = pSharedBuffer->size / m_numOfInputs;
    void *bufferAddr = (void *) ( (uint8_t *) pSharedBuffer->data() + batchIdx * outputSize );

    /* Check if output buffer is registered */
    if ( m_outputBufferSurfaceMap.find( bufferAddr ) == m_outputBufferSurfaceMap.end() )
    {
        ret = CreateTargetSurface( pSharedBuffer, batchIdx, surfaceId );
    }
    else
    {
        *surfaceId = m_outputBufferSurfaceMap[bufferAddr];
    }

    return ret;
}

RideHalError_e C2D::CreateSourceSurface( const RideHal_SharedBuffer_t *pSharedBuffer,
                                         uint32_t inputIdx, C2D_OBJECT &c2dObject )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t width = pSharedBuffer->imgProps.width;
    uint32_t height = pSharedBuffer->imgProps.height;
    uint32_t stride0 = pSharedBuffer->imgProps.stride[0];
    uint32_t stride1 = pSharedBuffer->imgProps.stride[1];
    uint32_t planeBufSize0 = pSharedBuffer->imgProps.planeBufSize[0];
    RideHal_ImageFormat_e format = pSharedBuffer->imgProps.format;

    uint32_t surfaceId = 0;
    bool isSource = true;
    void *bufferAddr = pSharedBuffer->data();

    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_UYVY:
        case RIDEHAL_IMAGE_FORMAT_NV12:
        case RIDEHAL_IMAGE_FORMAT_P010:
            ret = CreateYUVSurface( bufferAddr, &surfaceId, format, width, height, stride0, stride1,
                                    planeBufSize0, isSource );
            break;
        case RIDEHAL_IMAGE_FORMAT_RGB888:
        case RIDEHAL_IMAGE_FORMAT_BGR888:
            ret = CreateRGBSurface( bufferAddr, &surfaceId, format, width, height, stride0,
                                    isSource );
            break;
        default:
            RIDEHAL_ERROR( "Unsupported C2D image format" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            break;
    }

    if ( ret == RIDEHAL_ERROR_NONE )
    {
        c2dObject.surface_id = surfaceId;
        if ( ( m_rois[inputIdx].topX != 0 ) || ( m_rois[inputIdx].topY != 0 ) ||
             ( m_rois[inputIdx].width != 0 ) || ( m_rois[inputIdx].height != 0 ) )
        {
            c2dObject.source_rect.x = m_rois[inputIdx].topX << 16;
            c2dObject.source_rect.y = m_rois[inputIdx].topY << 16;
            c2dObject.source_rect.width = m_rois[inputIdx].width << 16;
            c2dObject.source_rect.height = m_rois[inputIdx].height << 16;
        }
        else
        {
            c2dObject.source_rect.x = 0 << 16;
            c2dObject.source_rect.y = 0 << 16;
            c2dObject.source_rect.width = m_rois[inputIdx].width << 16;
            c2dObject.source_rect.height = m_rois[inputIdx].height << 16;
        }

        c2dObject.config_mask = C2D_SOURCE_RECT_BIT;
        c2dObject.config_mask |= C2D_NO_BILINEAR_BIT;
        c2dObject.config_mask |= C2D_NO_ANTIALIASING_BIT;

        m_inputBufferSurfaceMap[bufferAddr] = c2dObject;
    }
    return ret;
}

RideHalError_e C2D::CreateTargetSurface( const RideHal_SharedBuffer_t *pSharedBuffer,
                                         uint32_t batchIdx, uint32_t *surfaceId )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t width = pSharedBuffer->imgProps.width;
    uint32_t height = pSharedBuffer->imgProps.height;
    uint32_t stride0 = pSharedBuffer->imgProps.stride[0];
    uint32_t stride1 = pSharedBuffer->imgProps.stride[1];
    uint32_t planeBufSize0 = pSharedBuffer->imgProps.planeBufSize[0];
    RideHal_ImageFormat_e format = pSharedBuffer->imgProps.format;

    bool isSource = false;
    uint32_t outputSize = pSharedBuffer->size / m_numOfInputs;
    void *bufferAddr = (void *) ( (uint8_t *) pSharedBuffer->data() + batchIdx * outputSize );

    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_UYVY:
        case RIDEHAL_IMAGE_FORMAT_NV12:
        case RIDEHAL_IMAGE_FORMAT_P010:
            ret = CreateYUVSurface( bufferAddr, surfaceId, format, width, height, stride0, stride1,
                                    planeBufSize0, isSource );
            break;
        case RIDEHAL_IMAGE_FORMAT_RGB888:
        case RIDEHAL_IMAGE_FORMAT_BGR888:
            ret = CreateRGBSurface( bufferAddr, surfaceId, format, width, height, stride0,
                                    isSource );
            break;
        default:
            RIDEHAL_ERROR( "Unsupported C2D image format" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            break;
    }
    if ( ret == RIDEHAL_ERROR_NONE )
    {
        m_outputBufferSurfaceMap[bufferAddr] = *surfaceId;
    }
    return ret;
}

RideHalError_e C2D::CreateYUVSurface( void *bufferAddr, uint32_t *surfaceId,
                                      RideHal_ImageFormat_e format, uint32_t width, uint32_t height,
                                      uint32_t stride0, uint32_t stride1, uint32_t planeBufSize0,
                                      bool isSource )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D_YUV_SURFACE_DEF surfaceDef;
    (void) memset( &surfaceDef, 0, sizeof( C2D_YUV_SURFACE_DEF ) );

    surfaceDef.plane0 = bufferAddr;
    surfaceDef.plane1 = (void *) ( (uint8_t *) surfaceDef.plane0 + planeBufSize0 );
    surfaceDef.format = GetC2DFormatType( format );
    surfaceDef.width = width;
    surfaceDef.height = height;
    surfaceDef.stride0 = stride0;
    surfaceDef.stride1 = stride1;
    surfaceDef.phys0 = surfaceDef.plane0;   // any nonzero value
    surfaceDef.phys1 = surfaceDef.plane1;   // any nonzero value

    auto c2dStatus = c2dCreateSurface(
            surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
            static_cast<C2D_SURFACE_TYPE>( C2D_SURFACE_YUV_HOST | C2D_SURFACE_WITH_PHYS ),
            (void *) &surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDEHAL_ERROR_FAIL;
        RIDEHAL_ERROR( "Failed to create %s YUV surface, format: %d, width: %u, height: %u, "
                       "c2dStatus wrong",
                       isSource ? "source" : "target", (int) format, width, height );
    }

    return ret;
}

RideHalError_e C2D::CreateRGBSurface( void *bufferAddr, uint32_t *surfaceId,
                                      RideHal_ImageFormat_e format, uint32_t width, uint32_t height,
                                      uint32_t stride, bool isSource )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    C2D_RGB_SURFACE_DEF surfaceDef;
    (void) memset( &surfaceDef, 0, sizeof( C2D_RGB_SURFACE_DEF ) );

    surfaceDef.buffer = bufferAddr;
    surfaceDef.format = GetC2DFormatType( format );
    surfaceDef.width = width;
    surfaceDef.height = height;
    surfaceDef.stride = stride;
    surfaceDef.phys = surfaceDef.buffer;   // any nonzero value
    auto c2dStatus = c2dCreateSurface(
            surfaceId, isSource ? C2D_SOURCE : C2D_TARGET,
            static_cast<C2D_SURFACE_TYPE>( C2D_SURFACE_RGB_HOST | C2D_SURFACE_WITH_PHYS ),
            (void *) &surfaceDef );
    if ( C2D_STATUS_OK != c2dStatus )
    {
        ret = RIDEHAL_ERROR_FAIL;
        RIDEHAL_ERROR( "Failed to create %s RGB surface, format: %d, width: %u, height: %u, "
                       "c2dStatus wrong",
                       isSource ? "source" : "target", (int) format, width, height );
    }

    return ret;
}

uint32_t C2D::GetC2DFormatType( RideHal_ImageFormat_e format )
{
    uint32_t c2dFormat = (uint32_t) RIDEHAL_IMAGE_FORMAT_MAX;
    switch ( format )
    {
        case RIDEHAL_IMAGE_FORMAT_UYVY:
            c2dFormat = C2D_COLOR_FORMAT_422_UYVY;
            break;
        case RIDEHAL_IMAGE_FORMAT_NV12:
            c2dFormat = C2D_COLOR_FORMAT_420_NV12;
            break;
        case RIDEHAL_IMAGE_FORMAT_P010:
            c2dFormat = C2D_COLOR_FORMAT_420_P010;
            break;
        case RIDEHAL_IMAGE_FORMAT_RGB888:
            c2dFormat = C2D_RGB_FORMAT( C2D_COLOR_FORMAT_888_RGB | C2D_FORMAT_SWAP_ENDIANNESS );
            break;
        case RIDEHAL_IMAGE_FORMAT_BGR888:
            c2dFormat = C2D_RGB_FORMAT( C2D_COLOR_FORMAT_888_RGB );
            break;
        default:
            RIDEHAL_ERROR( "Unsupported C2D image format" );
            break;
    }

    return c2dFormat;
}

}   // namespace component
}   // namespace ridehal

