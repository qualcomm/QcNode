// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/SampleTinyViz.hpp"

namespace QC
{
namespace sample
{

SampleTinyViz::SampleTinyViz() {}
SampleTinyViz::~SampleTinyViz() {}

QCStatus_e SampleTinyViz::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_winW = Get( config, "winW", 1920 );
    if ( 0 == m_winW )
    {
        QC_ERROR( "invalid winW = %d\n", m_winW );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_winH = Get( config, "winH", 1080 );
    if ( 0 == m_winH )
    {
        QC_ERROR( "invalid winH = %d\n", m_winH );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_camNames = Get( config, "cameras", m_camNames );
    if ( 0 == m_camNames.size() )
    {
        QC_ERROR( "invalid cameras\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t idx = 0;
    for ( auto camName : m_camNames )
    {
        std::string camTopic = Get( config, "cam_topic" + std::to_string( idx ),
                                    "/sensor/camera/" + camName + "/raw" );
        if ( "" == camTopic )
        {
            QC_ERROR( "no cam_topic for camera %s\n", camName.c_str() );
            ret = QC_STATUS_BAD_ARGUMENTS;
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

QCStatus_e SampleTinyViz::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        bool bOK = m_tinyViz.init( m_winW, m_winH );
        if ( false == bOK )
        {
            QC_ERROR( "init tinyviz failed\n" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            for ( auto camName : m_camNames )
            {
                m_tinyViz.addCamera( camName );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_camSubs.resize( m_camNames.size() );
        for ( uint32_t idx = 0; ( idx < m_camNames.size() ) && ( QC_STATUS_OK == ret ); idx++ )
        {
            ret = m_camSubs[idx].Init( name + "_" + m_camNames[idx], m_camTopicNames[idx], 1 );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_objSubs.resize( m_camNames.size() );
        for ( uint32_t idx = 0; ( idx < m_camNames.size() ) && ( QC_STATUS_OK == ret ); idx++ )
        {
            if ( "" != m_objTopicNames[idx] )
            {
                ret = m_objSubs[idx].Init( name + "_" + m_camNames[idx], m_objTopicNames[idx] );
            }
        }
    }

    return ret;
}

QCStatus_e SampleTinyViz::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( !m_tinyViz.start() )
    {
        QC_ERROR( "start tinyviz failed\n" );
        ret = QC_STATUS_FAIL;
    }

    m_stop = false;
    for ( uint32_t idx = 0; ( idx < m_camNames.size() ) && ( QC_STATUS_OK == ret ); idx++ )
    {
        std::thread *thread = new std::thread( &SampleTinyViz::CamThreadMain, this, idx );
        if ( nullptr != thread )
        {
            m_threads.push_back( thread );
        }
        else
        {
            ret = QC_STATUS_NOMEM;
        }
        if ( ( QC_STATUS_OK == ret ) && ( "" != m_objTopicNames[idx] ) )
        {
            std::thread *thread = new std::thread( &SampleTinyViz::ObjThreadMain, this, idx );
            if ( nullptr != thread )
            {
                m_threads.push_back( thread );
            }
            else
            {
                ret = QC_STATUS_NOMEM;
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
            QC_DEBUG( "%s receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n ", camName.c_str(),
                      frame.frameId, frame.timestamp );
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
            QC_DEBUG( "%s receive objects for frameId %" PRIu64 ", timestamp %" PRIu64 "\n ",
                      camName.c_str(), objs.frameId, objs.timestamp );
            m_tinyViz.addData( camName, objs );
        }
    }
}


QCStatus_e SampleTinyViz::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

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

QCStatus_e SampleTinyViz::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    return ret;
}

REGISTER_SAMPLE( TinyViz, SampleTinyViz );

}   // namespace sample
}   // namespace QC
