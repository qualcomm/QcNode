// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SamplePlrPre.hpp"


namespace ridehal
{
namespace sample
{

SamplePlrPre::SamplePlrPre() {}
SamplePlrPre::~SamplePlrPre() {}

RideHalError_e SamplePlrPre::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_config.processor = Get( config, "processor", RIDEHAL_PROCESSOR_HTP0 );
    if ( RIDEHAL_PROCESSOR_MAX == m_config.processor )
    {
        RIDEHAL_ERROR( "invalid processor %s\n", Get( config, "processor", "" ).c_str() );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_config.pillarXSize = Get( config, "pillar_size_x", 0.16f );
    m_config.pillarYSize = Get( config, "pillar_size_y", 0.16f );
    m_config.pillarZSize = Get( config, "pillar_size_z", 4.0f );
    m_config.minXRange = Get( config, "min_x", 0.0f );
    m_config.minYRange = Get( config, "min_y", -39.68f );
    m_config.minZRange = Get( config, "min_z", -3.0f );
    m_config.maxXRange = Get( config, "max_x", 69.12f );
    m_config.maxYRange = Get( config, "max_y", 39.68f );
    m_config.maxZRange = Get( config, "max_z", 1.0f );
    m_config.maxNumInPts = Get( config, "max_points", 300000 );
    m_config.numInFeatureDim = Get( config, "in_feature_dim", 4 );
    m_config.maxNumPlrs = Get( config, "max_pillars", 12000 );
    m_config.maxNumPtsPerPlr = Get( config, "max_points_per_pillar", 32 );
    m_config.numOutFeatureDim = Get( config, "out_feature_dim", 10 );
    std::string inputMode = Get( config, "input_mode", "xyzr" );
    if ( "xyzr" == inputMode )
    {
        m_config.inputMode = VOXELIZATION_INPUT_XYZR;
    }
    else if ( "xyzrt" == inputMode )
    {
        m_config.inputMode = VOXELIZATION_INPUT_XYZRT;
    }
    else
    {
        RIDEHAL_ERROR( "invalid input_mode\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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

RideHalError_e SamplePlrPre::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t outPlrsTsProp;
        if ( VOXELIZATION_INPUT_XYZR == m_config.inputMode )
        {
            outPlrsTsProp = {
                    RIDEHAL_TENSOR_TYPE_FLOAT_32,
                    { m_config.maxNumPlrs, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
                    2,
            };
        }
        else if ( VOXELIZATION_INPUT_XYZRT == m_config.inputMode )
        {
            outPlrsTsProp = {
                    RIDEHAL_TENSOR_TYPE_INT_32,
                    { m_config.maxNumPlrs, 2, 0 },
                    2,
            };
        }
        else
        {
            RIDEHAL_ERROR( "invalid input_mode\n" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        ret = m_coordsPool.Init( name + ".coords", LOGGER_LEVEL_INFO, m_poolSize, outPlrsTsProp,
                                 RIDEHAL_BUFFER_USAGE_HTP );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        RideHal_TensorProps_t outFeatureTsProp = {
                RIDEHAL_TENSOR_TYPE_FLOAT_32,
                { m_config.maxNumPlrs, m_config.maxNumPtsPerPlr, m_config.numOutFeatureDim, 0 },
                3,
        };

        ret = m_featuresPool.Init( name + ".features", LOGGER_LEVEL_INFO, m_poolSize,
                                   outFeatureTsProp, RIDEHAL_BUFFER_USAGE_HTP );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = SampleIF::Init( m_config.processor );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_plrPre.Init( name.c_str(), &m_config );
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

RideHalError_e SamplePlrPre::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_plrPre.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SamplePlrPre::ThreadMain, this );
    }

    return ret;
}

void SamplePlrPre::ThreadMain()
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
            std::shared_ptr<SharedBuffer_t> coords = m_coordsPool.Get();
            std::shared_ptr<SharedBuffer_t> features = m_featuresPool.Get();
            if ( ( nullptr != coords ) && ( nullptr != features ) )
            {
                RideHal_SharedBuffer_t &inPts = frames.SharedBuffer( 0 );

                ret = SampleIF::Lock();
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    PROFILER_BEGIN();
                    TRACE_BEGIN( frames.FrameId( 0 ) );
                    ret = m_plrPre.Execute( &inPts, &coords->sharedBuffer,
                                            &features->sharedBuffer );
                    if ( RIDEHAL_ERROR_NONE == ret )
                    {
                        PROFILER_END();
                        TRACE_END( frames.FrameId( 0 ) );
                        DataFrames_t outFrames;
                        DataFrame_t frame;
                        frame.buffer = coords;
                        frame.frameId = frames.FrameId( 0 );
                        frame.timestamp = frames.Timestamp( 0 );
                        outFrames.Add( frame );
                        frame.buffer = features;
                        outFrames.Add( frame );
                        m_pub.Publish( outFrames );
                    }
                    else
                    {
                        RIDEHAL_ERROR( "Pillarize failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                                       ret );
                    }
                    (void) SampleIF::Unlock();
                }
            }
        }
    }
}

RideHalError_e SamplePlrPre::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_plrPre.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );


    return ret;
}

RideHalError_e SamplePlrPre::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_plrPre.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( PlrPre, SamplePlrPre );

}   // namespace sample
}   // namespace ridehal
