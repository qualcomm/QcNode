// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleCL2DFlex.hpp"


namespace ridehal
{
namespace sample
{

CL2DFlex_Work_Mode_e SampleCL2DFlex::GetMode( SampleConfig_t &config, std::string key,
                                              CL2DFlex_Work_Mode_e defaultV )
{
    CL2DFlex_Work_Mode_e ret = defaultV;

    auto it = config.find( key );
    if ( it != config.end() )
    {
        std::string mode = it->second;
        if ( "convert" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_CONVERT;
        }
        else if ( "resize_nearest" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
        }
        else if ( "resize_bilinear" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_RESIZE_BILINEAR;
        }
        else if ( "letterbox_nearest" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST;
        }
        else if ( "letterbox_nearest_multiple" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE;
        }
        else if ( "resize_nearest_multiple" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE;
        }
        else if ( "convert_ubwc" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_CONVERT_UBWC;
        }
        else if ( "remap_nearest" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_REMAP_NEAREST;
        }
        else
        {
            ret = CL2DFLEX_WORK_MODE_MAX;
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );

    return ret;
}

SampleCL2DFlex::SampleCL2DFlex() {}
SampleCL2DFlex::~SampleCL2DFlex() {}

RideHalError_e SampleCL2DFlex::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_bNoPadding = Get( config, "no_padding", false );

    m_config.outputWidth = Get( config, "output_width", 1920 );
    if ( 0 == m_config.outputWidth )
    {
        RIDEHAL_ERROR( "invalid output_width\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputHeight = Get( config, "output_height", 1024 );
    if ( 0 == m_config.outputHeight )
    {
        RIDEHAL_ERROR( "invalid output_height\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputFormat = Get( config, "output_format", RIDEHAL_IMAGE_FORMAT_RGB888 );
    if ( RIDEHAL_IMAGE_FORMAT_MAX == m_config.outputFormat )
    {
        RIDEHAL_ERROR( "invalid output_format\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.numOfInputs = Get( config, "batch_size", 1 );
    if ( 0 == m_config.numOfInputs )
    {
        RIDEHAL_ERROR( "invalid batch_size\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    bool bEnableUndistortion = Get( config, "map_table", false );

    for ( uint32_t i = 0; i < m_config.numOfInputs; i++ )
    {
        m_config.inputWidths[i] = Get( config, "input_width" + std::to_string( i ), 1920 );
        if ( 0 == m_config.inputWidths[i] )
        {
            RIDEHAL_ERROR( "invalid input_width%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputHeights[i] = Get( config, "input_height" + std::to_string( i ), 1024 );
        if ( 0 == m_config.inputHeights[i] )
        {
            RIDEHAL_ERROR( "invalid input_height%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputFormats[i] =
                Get( config, "input_format" + std::to_string( i ), RIDEHAL_IMAGE_FORMAT_NV12 );
        if ( RIDEHAL_IMAGE_FORMAT_MAX == m_config.inputFormats[i] )
        {
            RIDEHAL_ERROR( "invalid input_format%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.workModes[i] = GetMode( config, "work_mode" + std::to_string( i ),
                                         CL2DFLEX_WORK_MODE_RESIZE_NEAREST );
        if ( CL2DFLEX_WORK_MODE_MAX == m_config.workModes[i] )
        {
            RIDEHAL_ERROR( "invalid work_mode%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.ROIs[i].x = Get( config, "roi_x" + std::to_string( i ), 0 );
        if ( m_config.ROIs[i].x >= m_config.inputWidths[i] )
        {
            RIDEHAL_ERROR( "invalid roi_x%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.ROIs[i].y = Get( config, "roi_y" + std::to_string( i ), 0 );
        if ( m_config.ROIs[i].y >= m_config.inputHeights[i] )
        {
            RIDEHAL_ERROR( "invalid roi_y%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.ROIs[i].width =
                Get( config, "roi_width" + std::to_string( i ), m_config.inputWidths[i] );
        if ( 0 == m_config.ROIs[i].width )
        {
            RIDEHAL_ERROR( "invalid roi_width%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.ROIs[i].height =
                Get( config, "roi_height" + std::to_string( i ), m_config.inputHeights[i] );
        if ( 0 == m_config.ROIs[i].height )
        {
            RIDEHAL_ERROR( "invalid roi_height%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        if ( true == bEnableUndistortion )
        {
            bool bAllocateOK = true;
            RideHal_TensorProps_t mapXProp;
            mapXProp = {
                    RIDEHAL_TENSOR_TYPE_FLOAT_32,
                    { m_config.outputHeight * m_config.outputWidth, 0 },
                    1,
            };
            ret = m_mapXBuffer[i].Allocate( &mapXProp );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                bAllocateOK = false;
                RIDEHAL_ERROR( "failed to allocate mapX%u!\n", i );
            }
            RideHal_TensorProps_t mapYProp;
            mapYProp = {
                    RIDEHAL_TENSOR_TYPE_FLOAT_32,
                    { m_config.outputHeight * m_config.outputWidth, 0 },
                    1,
            };
            ret = m_mapYBuffer[i].Allocate( &mapYProp );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                bAllocateOK = false;
                RIDEHAL_ERROR( "failed to allocate mapY%u!\n", i );
            }
            if ( true == bAllocateOK )
            {
                bool bReadOK = true;
                std::string mapXPath = Get( config, "mapX_path" + std::to_string( i ),
                                            "./data/test/CL2DFlex/mapX.raw" );
                std::string mapYPath = Get( config, "mapY_path" + std::to_string( i ),
                                            "./data/test/CL2DFlex/mapY.raw" );
                ret = LoadFile( m_mapXBuffer[i], mapXPath );
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    m_config.remapTable[i].pMapX = &m_mapXBuffer[i];
                }
                else
                {
                    RIDEHAL_ERROR( "failed to read mapX table for input %u!\n", i );
                    bReadOK = false;
                }
                ret = LoadFile( m_mapYBuffer[i], mapYPath );
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    m_config.remapTable[i].pMapY = &m_mapYBuffer[i];
                }
                else
                {
                    RIDEHAL_ERROR( "failed to read mapY table for input %u!\n", i );
                    bReadOK = false;
                }
                if ( false == bReadOK )
                {
                    ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
                }
            }
        }
    }

    if ( ( 1 == m_config.numOfInputs ) &&
         ( ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE == m_config.workModes[0] ) ||
           ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE == m_config.workModes[0] ) ) )
    {
        m_executeWithROIs = true;

        m_roiNumber = Get( config, "roi_number", 1 );
        if ( 0 == m_roiNumber )
        {
            RIDEHAL_ERROR( "invalid roi_number\n" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        for ( uint32_t i = 0; i < m_roiNumber; i++ )
        {
            m_ROIs[i].x = Get( config, "roi_x" + std::to_string( i ), 0 );
            if ( m_ROIs[i].x >= m_config.inputWidths[0] )
            {
                RIDEHAL_ERROR( "invalid roi_x%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_ROIs[i].y = Get( config, "roi_y" + std::to_string( i ), 0 );
            if ( m_ROIs[i].y >= m_config.inputHeights[0] )
            {
                RIDEHAL_ERROR( "invalid roi_y%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_ROIs[i].width =
                    Get( config, "roi_width" + std::to_string( i ), m_config.inputWidths[0] );
            if ( 0 == m_ROIs[i].width )
            {
                RIDEHAL_ERROR( "invalid roi_width%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            m_ROIs[i].height =
                    Get( config, "roi_height" + std::to_string( i ), m_config.inputHeights[0] );
            if ( 0 == m_ROIs[i].height )
            {
                RIDEHAL_ERROR( "invalid roi_height%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferFlags = 0;
    }
    else
    {
        m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;
    }

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        RIDEHAL_ERROR( "no input topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        RIDEHAL_ERROR( "no output topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleCL2DFlex::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_ON( GPU );
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_ImageProps_t imgProp;
        if ( m_executeWithROIs == true )
        {
            imgProp.batchSize = m_roiNumber;
        }
        else
        {
            imgProp.batchSize = m_config.numOfInputs;
        }
        imgProp.width = m_config.outputWidth;
        imgProp.height = m_config.outputHeight;
        if ( ( RIDEHAL_IMAGE_FORMAT_RGB888 == m_config.outputFormat ) ||
             ( RIDEHAL_IMAGE_FORMAT_BGR888 == m_config.outputFormat ) )
        {
            imgProp.numPlanes = 1;
            imgProp.format = m_config.outputFormat;
            imgProp.stride[0] = m_config.outputWidth * 3;
            imgProp.actualHeight[0] = m_config.outputHeight;
            imgProp.planeBufSize[0] = 0;
            ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                    RIDEHAL_BUFFER_USAGE_GPU, m_bufferFlags );
        }
        else
        {
            if ( true == m_bNoPadding )
            {
                if ( ( RIDEHAL_IMAGE_FORMAT_NV12 == m_config.outputFormat ) ||
                     ( RIDEHAL_IMAGE_FORMAT_P010 == m_config.outputFormat ) )
                {
                    uint32_t bpp = 1;
                    if ( RIDEHAL_IMAGE_FORMAT_P010 == m_config.outputFormat )
                    {
                        bpp = 2;
                    }
                    imgProp.numPlanes = 2;
                    imgProp.format = m_config.outputFormat;
                    imgProp.stride[0] = m_config.outputWidth * bpp;
                    imgProp.actualHeight[0] = m_config.outputHeight;
                    imgProp.planeBufSize[0] = 0;
                    imgProp.stride[1] = m_config.outputWidth * bpp;
                    imgProp.actualHeight[1] = m_config.outputHeight / 2;
                    imgProp.planeBufSize[1] = 0;
                    ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                            RIDEHAL_BUFFER_USAGE_GPU, m_bufferFlags );
                }
                else if ( RIDEHAL_IMAGE_FORMAT_UYVY == m_config.outputFormat )
                {
                    imgProp.numPlanes = 1;
                    imgProp.format = m_config.outputFormat;
                    imgProp.stride[0] = m_config.outputWidth * 2;
                    imgProp.actualHeight[0] = m_config.outputHeight;
                    imgProp.planeBufSize[0] = 0;
                    ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                            RIDEHAL_BUFFER_USAGE_GPU, m_bufferFlags );
                }
                else
                {
                    RIDEHAL_ERROR( "no padding for format %d is not supported",
                                   m_config.outputFormat );
                    ret = RIDEHAL_ERROR_FAIL;
                }
            }
            else
            {
                ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp.batchSize,
                                        m_config.outputWidth, m_config.outputHeight,
                                        m_config.outputFormat, RIDEHAL_BUFFER_USAGE_GPU,
                                        m_bufferFlags );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_CL2DFlex.Init( name.c_str(), &m_config );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleCL2DFlex::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_CL2DFlex.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleCL2DFlex::ThreadMain, this );
    }

    return ret;
}

void SampleCL2DFlex::ThreadMain()
{
    RideHalError_e ret;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            std::shared_ptr<SharedBuffer_t> buffer = m_imagePool.Get();
            if ( nullptr != buffer )
            {
                std::vector<RideHal_SharedBuffer_t> inputs;
                for ( auto &frame : frames.frames )
                {
                    inputs.push_back( frame.buffer->sharedBuffer );
                }

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                if ( m_executeWithROIs == true )
                {
                    ret = m_CL2DFlex.ExecuteWithROI( inputs.data(), &buffer->sharedBuffer, m_ROIs,
                                                     m_roiNumber );
                }
                else
                {
                    ret = m_CL2DFlex.Execute( inputs.data(), inputs.size(), &buffer->sharedBuffer );
                }
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    PROFILER_END();
                    TRACE_END( frames.FrameId( 0 ) );
                    DataFrames_t outFrames;
                    DataFrame_t frame;
                    frame.buffer = buffer;
                    frame.frameId = frames.FrameId( 0 );
                    frame.timestamp = frames.Timestamp( 0 );
                    outFrames.Add( frame );
                    m_pub.Publish( outFrames );
                }
                else
                {
                    RIDEHAL_ERROR( "CL2D execute failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                                   ret );
                }
            }
        }
    }
}

RideHalError_e SampleCL2DFlex::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_CL2DFlex.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleCL2DFlex::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_CL2DFlex.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( CL2DFlex, SampleCL2DFlex );

}   // namespace sample
}   // namespace ridehal
