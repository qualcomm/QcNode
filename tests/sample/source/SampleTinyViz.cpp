// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleTinyViz.hpp"

namespace ridehal
{
namespace sample
{

SampleTinyViz::SampleTinyViz() {}
SampleTinyViz::~SampleTinyViz() {}

RideHalError_e SampleTinyViz::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_winW = Get( config, "winW", 1920 );
    if ( 0 == m_winW )
    {
        RIDEHAL_ERROR( "invalid winW = %d\n", m_winW );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_winH = Get( config, "winH", 1080 );
    if ( 0 == m_winH )
    {
        RIDEHAL_ERROR( "invalid winH = %d\n", m_winH );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_camNames = Get( config, "cameras", m_camNames );
    if ( 0 == m_camNames.size() )
    {
        RIDEHAL_ERROR( "invalid cameras\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    uint32_t idx = 0;
    for ( auto camName : m_camNames )
    {
        std::string camTopic = Get( config, "cam_topic" + std::to_string( idx ),
                                    "/sensor/camera/" + camName + "/raw" );
        if ( "" == camTopic )
        {
            RIDEHAL_ERROR( "no cam_topic for camera %s\n", camName.c_str() );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        m_camTopicNames.push_back( camTopic );

        std::string objTopic = Get( config, "obj_topic" + std::to_string( idx ),
                                    "/sensor/camera/" + camName + "/objs" );
        m_objTopicNames.push_back( objTopic );

        uint32_t batchIndex = Get( config, "batch_index" + std::to_string( idx ), (uint32_t) 0 );
        m_batchIndexs.push_back( batchIndex );

        idx++;
    }

    return ret;
}

RideHalError_e SampleTinyViz::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        bool bOK = m_tinyViz.init( m_winW, m_winH );
        if ( false == bOK )
        {
            RIDEHAL_ERROR( "init tinyviz failed\n" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            for ( auto camName : m_camNames )
            {
                m_tinyViz.addCamera( camName );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_camSubs.resize( m_camNames.size() );
        for ( uint32_t idx = 0; ( idx < m_camNames.size() ) && ( RIDEHAL_ERROR_NONE == ret );
              idx++ )
        {
            ret = m_camSubs[idx].Init( name + "_" + m_camNames[idx], m_camTopicNames[idx], 1 );
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_objSubs.resize( m_camNames.size() );
        for ( uint32_t idx = 0; ( idx < m_camNames.size() ) && ( RIDEHAL_ERROR_NONE == ret );
              idx++ )
        {
            if ( "" != m_objTopicNames[idx] )
            {
                ret = m_objSubs[idx].Init( name + "_" + m_camNames[idx], m_objTopicNames[idx] );
            }
        }
    }

    return ret;
}

RideHalError_e SampleTinyViz::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( !m_tinyViz.start() )
    {
        RIDEHAL_ERROR( "start tinyviz failed\n" );
        ret = RIDEHAL_ERROR_FAIL;
    }

    m_stop = false;
    for ( uint32_t idx = 0; ( idx < m_camNames.size() ) && ( RIDEHAL_ERROR_NONE == ret ); idx++ )
    {
        std::thread *thread = new std::thread( &SampleTinyViz::CamThreadMain, this, idx );
        if ( nullptr != thread )
        {
            m_threads.push_back( thread );
        }
        else
        {
            ret = RIDEHAL_ERROR_NOMEM;
        }
        if ( ( RIDEHAL_ERROR_NONE == ret ) && ( "" != m_objTopicNames[idx] ) )
        {
            std::thread *thread = new std::thread( &SampleTinyViz::ObjThreadMain, this, idx );
            if ( nullptr != thread )
            {
                m_threads.push_back( thread );
            }
            else
            {
                ret = RIDEHAL_ERROR_NOMEM;
            }
        }
    }

    return ret;
}

void SampleTinyViz::CamThreadMain( uint32_t idx )
{
    int ret;
    std::string camName = m_camNames[idx];
    auto &camSub = m_camSubs[idx];
    uint32_t batchIndex = m_batchIndexs[idx];

    while ( false == m_stop )
    {
        DataFrames_t frames;
        DataFrame_t frame;
        ret = camSub.Receive( frames );
        if ( 0 == ret )
        {
            if ( batchIndex >= frames.frames.size() )
            {
                batchIndex = 0;
            }
            frame = frames.frames[batchIndex];
            RIDEHAL_DEBUG( "%s receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n ",
                           camName.c_str(), frame.frameId, frame.timestamp );
            m_tinyViz.addData( camName, frame );
        }
    }
}

void SampleTinyViz::ObjThreadMain( uint32_t idx )
{
    int ret;
    std::string camName = m_camNames[idx];
    auto &objSub = m_objSubs[idx];

    while ( false == m_stop )
    {
        Road2DObjects_t objs;
        ret = objSub.Receive( objs );
        if ( 0 == ret )
        {
            RIDEHAL_DEBUG( "%s receive objects for frameId %" PRIu64 ", timestamp %" PRIu64 "\n ",
                           camName.c_str(), objs.frameId, objs.timestamp );
            m_tinyViz.addData( camName, objs );
        }
    }
}


RideHalError_e SampleTinyViz::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    for ( auto &th : m_threads )
    {
        if ( th->joinable() )
        {
            th->join();
        }
        delete th;
    }
    m_threads.clear();

    m_tinyViz.stop();

    return ret;
}

RideHalError_e SampleTinyViz::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    return ret;
}

REGISTER_SAMPLE( TinyViz, SampleTinyViz );

}   // namespace sample
}   // namespace ridehal
