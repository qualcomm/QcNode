// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleRemap.hpp"


namespace ridehal
{
namespace sample
{

static uint32_t s_rideHalFormatToBytesPerPixel[RIDEHAL_IMAGE_FORMAT_MAX] = {
        3, /* RIDEHAL_IMAGE_FORMAT_RGB888 */
        3, /* RIDEHAL_IMAGE_FORMAT_BGR888 */
        2, /* RIDEHAL_IMAGE_FORMAT_UYVY */
        1, /* RIDEHAL_IMAGE_FORMAT_NV12 */
        2, /* RIDEHAL_IMAGE_FORMAT_P010 */
        1  /* RIDEHAL_IMAGE_FORMAT_NV12_UBWC */
};

SampleRemap::SampleRemap() {}
SampleRemap::~SampleRemap() {}

RideHalError_e SampleRemap::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    float quantScale = Get( config, "quant_scale", 0.0186584480106831f );
    int32_t quantOffset = Get( config, "quant_offset", 114 );

    m_config.processor = Get( config, "processor", RIDEHAL_PROCESSOR_HTP0 );
    if ( RIDEHAL_PROCESSOR_MAX == m_config.processor )
    {
        RIDEHAL_ERROR( "invalid processor %s\n", Get( config, "processor", "" ).c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputWidth = Get( config, "output_width", 1152 );
    if ( 0 == m_config.outputWidth )
    {
        RIDEHAL_ERROR( "invalid output_width\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.outputHeight = Get( config, "output_height", 800 );
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

    m_config.bEnableUndistortion = Get( config, "map_table", false );
    m_config.bEnableNormalize = Get( config, "normalize", true );
    if ( true == m_config.bEnableNormalize )
    {
        m_config.normlzR.sub = Get( config, "Rsub", 123.675f );
        m_config.normlzR.mul = Get( config, "Rmul", 0.0171f );
        m_config.normlzR.add = Get( config, "Radd", 0.0f );
        m_config.normlzG.sub = Get( config, "Gsub", 116.28f );
        m_config.normlzG.mul = Get( config, "Gmul", 0.0175f );
        m_config.normlzG.add = Get( config, "Gadd", 0.0f );
        m_config.normlzB.sub = Get( config, "Bsub", 103.53f );
        m_config.normlzB.mul = Get( config, "Bmul", 0.0174f );
        m_config.normlzB.add = Get( config, "Badd", 0.0f );
        m_config.normlzR.add = m_config.normlzR.add / quantScale + quantOffset;
        m_config.normlzR.mul = m_config.normlzR.mul / quantScale;
        m_config.normlzG.add = m_config.normlzG.add / quantScale + quantOffset;
        m_config.normlzG.mul = m_config.normlzG.mul / quantScale;
        m_config.normlzB.add = m_config.normlzB.add / quantScale + quantOffset;
        m_config.normlzB.mul = m_config.normlzB.mul / quantScale;
    }

    for ( uint32_t i = 0; i < m_config.numOfInputs; i++ )
    {
        m_config.inputConfigs[i].inputWidth =
                Get( config, "input_width" + std::to_string( i ), 1920 );
        if ( 0 == m_config.inputConfigs[i].inputWidth )
        {
            RIDEHAL_ERROR( "invalid input_width%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputHeight =
                Get( config, "input_height" + std::to_string( i ), 1024 );
        if ( 0 == m_config.inputConfigs[i].inputHeight )
        {
            RIDEHAL_ERROR( "invalid input_height%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].inputFormat =
                Get( config, "input_format" + std::to_string( i ), RIDEHAL_IMAGE_FORMAT_UYVY );
        if ( RIDEHAL_IMAGE_FORMAT_MAX == m_config.inputConfigs[i].inputFormat )
        {
            RIDEHAL_ERROR( "invalid input_format%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].mapWidth =
                Get( config, "map_width" + std::to_string( i ), m_config.outputWidth );
        if ( 0 == m_config.inputConfigs[i].mapWidth )
        {
            RIDEHAL_ERROR( "invalid map_width%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].mapHeight =
                Get( config, "map_height" + std::to_string( i ), m_config.outputHeight );
        if ( 0 == m_config.inputConfigs[i].mapHeight )
        {
            RIDEHAL_ERROR( "invalid map_height%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.x = Get( config, "roi_x" + std::to_string( i ), 0 );
        if ( m_config.inputConfigs[i].ROI.x >= m_config.inputConfigs[i].mapWidth )
        {
            RIDEHAL_ERROR( "invalid roi_x%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.y = Get( config, "roi_y" + std::to_string( i ), 0 );
        if ( m_config.inputConfigs[i].ROI.y >= m_config.inputConfigs[i].mapHeight )
        {
            RIDEHAL_ERROR( "invalid roi_y%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.width =
                Get( config, "roi_width" + std::to_string( i ), m_config.outputWidth );
        if ( 0 == m_config.inputConfigs[i].ROI.width )
        {
            RIDEHAL_ERROR( "invalid roi_width%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        m_config.inputConfigs[i].ROI.height =
                Get( config, "roi_height" + std::to_string( i ), m_config.outputHeight );
        if ( 0 == m_config.inputConfigs[i].ROI.height )
        {
            RIDEHAL_ERROR( "invalid roi_height%u\n", i );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        if ( true == m_config.bEnableUndistortion )
        {
            bool bAllocateOK = true;
            RideHal_TensorProps_t mapXProp;
            mapXProp = {
                    RIDEHAL_TENSOR_TYPE_FLOAT_32,
                    { m_config.inputConfigs[i].mapWidth, m_config.inputConfigs[i].mapHeight, 0 },
                    2,
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
                    { m_config.inputConfigs[i].mapWidth, m_config.inputConfigs[i].mapHeight, 0 },
                    2,
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
                                            "./data/test/remap/mapX.raw" );
                std::string mapYPath = Get( config, "mapY_path" + std::to_string( i ),
                                            "./data/test/remap/mapY.raw" );
                ret = LoadFile( m_mapXBuffer[i], mapXPath );
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    m_config.inputConfigs[i].remapTable.pMapX = &m_mapXBuffer[i];
                }
                else
                {
                    RIDEHAL_ERROR( "failed to read mapX table for input %u!\n", i );
                    bReadOK = false;
                }
                ret = LoadFile( m_mapYBuffer[i], mapYPath );
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    m_config.inputConfigs[i].remapTable.pMapY = &m_mapYBuffer[i];
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

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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

RideHalError_e SampleRemap::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_ImageProps_t imgProp;
        imgProp.format = m_config.outputFormat;
        imgProp.batchSize = m_config.numOfInputs;
        imgProp.width = m_config.outputWidth;
        imgProp.height = m_config.outputHeight;
        imgProp.stride[0] =
                m_config.outputWidth * s_rideHalFormatToBytesPerPixel[m_config.outputFormat];
        imgProp.actualHeight[0] = m_config.outputHeight;
        imgProp.numPlanes = 1;
        imgProp.planeBufSize[0] = 0;

        ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                RIDEHAL_BUFFER_USAGE_HTP );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = SampleIF::Init( m_config.processor );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_remap.Init( name.c_str(), &m_config );
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

RideHalError_e SampleRemap::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_remap.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleRemap::ThreadMain, this );
    }

    return ret;
}

void SampleRemap::ThreadMain()
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

                ret = SampleIF::Lock();
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    PROFILER_BEGIN();
                    TRACE_BEGIN( frames.FrameId( 0 ) );
                    ret = m_remap.Execute( inputs.data(), inputs.size(), &buffer->sharedBuffer );
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
                        RIDEHAL_ERROR( "remap failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                                       ret );
                    }
                    (void) SampleIF::Unlock();
                }
            }
        }
    }
}

RideHalError_e SampleRemap::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_remap.Stop();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        RIDEHAL_ERROR( "failed to stop remap!\n" );
    }
    TRACE_END( SYSTRACE_TASK_STOP );

    return ret;
}

RideHalError_e SampleRemap::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_remap.Deinit();
    if ( true == m_config.bEnableUndistortion )
    {
        for ( uint32_t i = 0; i < m_config.numOfInputs; i++ )
        {
            ret = m_mapXBuffer[i].Free();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "failed to free mapX buffer!\n" );
            }
            ret = m_mapYBuffer[i].Free();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "failed to free mapY buffer!\n" );
            }
        }
    }
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( Remap, SampleRemap );

}   // namespace sample
}   // namespace ridehal
