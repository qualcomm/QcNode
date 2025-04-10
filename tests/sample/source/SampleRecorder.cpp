// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleRecorder.hpp"


namespace ridehal
{
namespace sample
{

SampleRecorder::SampleRecorder() {}
SampleRecorder::~SampleRecorder() {}

RideHalError_e SampleRecorder::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_maxImages = Get( config, "max", 1000 );
    if ( 0 == m_maxImages )
    {
        RIDEHAL_ERROR( "invalid max = %d\n", m_maxImages );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleRecorder::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_topicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::string path = "/tmp/" + name + ".raw";
        m_file = fopen( path.c_str(), "wb" );
        if ( nullptr == m_file )
        {
            RIDEHAL_ERROR( "can't create file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        std::string path = "/tmp/" + name + ".meta";
        m_meta = fopen( path.c_str(), "wb" );
        if ( nullptr == m_meta )
        {
            RIDEHAL_ERROR( "can't create meta file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e SampleRecorder::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = false;
    m_thread = std::thread( &SampleRecorder::ThreadMain, this );

    return ret;
}

void SampleRecorder::ThreadMain()
{
    RideHalError_e ret;
    uint32_t num = 0;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        DataFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            frame = frames.frames[0];
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frame.frameId,
                           frame.timestamp );
            if ( num < m_maxImages )
            {
                PROFILER_BEGIN();
                if ( RIDEHAL_BUFFER_TYPE_IMAGE == frame.BufferType() )
                { /* for Image, dump the first one only */
                    auto &buffer = frame.buffer->sharedBuffer;
                    if ( buffer.imgProps.format < RIDEHAL_IMAGE_FORMAT_MAX )
                    {
                        uint32_t sizeOne = buffer.size / buffer.imgProps.batchSize;
                        for ( uint32_t i = 0; i < buffer.imgProps.batchSize; i++ )
                        {
                            std::string path = "/tmp/" + m_name + "_" + std::to_string( num ) +
                                               "_" + std::to_string( i ) + ".raw";
                            uint8_t *ptr = (uint8_t *) buffer.data() + sizeOne * i;
                            FILE *fp = fopen( path.c_str(), "wb" );
                            if ( nullptr != fp )
                            {
                                fwrite( ptr, sizeOne, 1, fp );
                                fclose( fp );
                            }
                            else
                            {
                                RIDEHAL_ERROR( "failed to create file: %s", path.c_str() );
                            }
                        }
                        fprintf( m_meta,
                                 "%u: frameId %" PRIu64 " timestamp %" PRIu64
                                 ": batch=%u resolution=%ux%u stride=%u actual_height=%u "
                                 "format=%d\n",
                                 num, frame.frameId, frame.timestamp, buffer.imgProps.batchSize,
                                 buffer.imgProps.width, buffer.imgProps.height,
                                 buffer.imgProps.stride[0], buffer.imgProps.actualHeight[0],
                                 buffer.imgProps.format );
                    }
                    else
                    { /* compressed image */
                        fwrite( buffer.data(), buffer.size, 1, m_file );
                        fprintf( m_meta,
                                 "%u: frameId %" PRIu64 " timestamp %" PRIu64
                                 ": resolution=%ux%u size=%" PRIu64 " format=%d\n",
                                 num, frame.frameId, frame.timestamp, buffer.imgProps.width,
                                 buffer.imgProps.height, buffer.size, buffer.imgProps.format );
                    }
                }
                else
                { /* for Tensor, dump all */
                    for ( size_t i = 0; i < frames.frames.size(); i++ )
                    {
                        frame = frames.frames[i];
                        auto &buffer = frame.buffer->sharedBuffer;
                        std::string path = "/tmp/" + m_name + "_" + std::to_string( num ) + "_" +
                                           std::to_string( i ) + ".raw";
                        FILE *fp = fopen( path.c_str(), "wb" );
                        if ( nullptr != fp )
                        {
                            fwrite( buffer.data(), buffer.size, 1, fp );
                            fclose( fp );
                        }
                        else
                        {
                            RIDEHAL_ERROR( "failed to create file: %s", path.c_str() );
                        }
                    }
                }
                num++;
                PROFILER_END();
            }
            else if ( nullptr != m_meta )
            {
                fclose( m_meta );
                fclose( m_file );
                m_meta = nullptr;
                m_file = nullptr;
                RIDEHAL_INFO( "recording done!" );
                printf( "recording done!\n" );
            }
            else
            {
            }
        }
    }
}

RideHalError_e SampleRecorder::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    if ( nullptr != m_meta )
    {
        fclose( m_meta );
        fclose( m_file );
        m_meta = nullptr;
        m_file = nullptr;
        RIDEHAL_INFO( "recording done!" );
        printf( "recording done!\n" );
    }

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleRecorder::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    return ret;
}

REGISTER_SAMPLE( Recorder, SampleRecorder );

}   // namespace sample
}   // namespace ridehal
