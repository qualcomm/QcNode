// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleDataReader.hpp"
#include <chrono>
#include <time.h>

namespace ridehal
{
namespace sample
{

static std::string s_rideHalFormatToStr[RIDEHAL_IMAGE_FORMAT_MAX] = {
        ".rgb",       /* RIDEHAL_IMAGE_FORMAT_RGB888 */
        ".bgr",       /* RIDEHAL_IMAGE_FORMAT_BGR888 */
        ".uyvy",      /* RIDEHAL_IMAGE_FORMAT_UYVY */
        ".nv12",      /* RIDEHAL_IMAGE_FORMAT_NV12 */
        ".p010",      /* RIDEHAL_IMAGE_FORMAT_P010 */
        ".nv12_ubwc", /* RIDEHAL_IMAGE_FORMAT_NV12_UBWC */
};

#define SIZE_OF_FLOAT16 2
static uint32_t s_rideHalTensorTypeToDataSize[RIDEHAL_TENSOR_TYPE_MAX] = {
        sizeof( int8_t ),  /* RIDEHAL_TENSOR_TYPE_INT_8 */
        sizeof( int16_t ), /* RIDEHAL_TENSOR_TYPE_INT_16 */
        sizeof( int32_t ), /* RIDEHAL_TENSOR_TYPE_INT_32 */
        sizeof( int64_t ), /* RIDEHAL_TENSOR_TYPE_INT_64 */

        sizeof( uint8_t ),  /* RIDEHAL_TENSOR_TYPE_UINT_8 */
        sizeof( uint16_t ), /* RIDEHAL_TENSOR_TYPE_UINT_16 */
        sizeof( uint32_t ), /* RIDEHAL_TENSOR_TYPE_UINT_32 */
        sizeof( uint64_t ), /* RIDEHAL_TENSOR_TYPE_UINT_64 */

        SIZE_OF_FLOAT16,  /* RIDEHAL_TENSOR_TYPE_FLOAT_16 */
        sizeof( float ),  /* RIDEHAL_TENSOR_TYPE_FLOAT_32 */
        sizeof( double ), /* RIDEHAL_TENSOR_TYPE_FLOAT_64 */

        sizeof( int8_t ),  /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8 */
        sizeof( int16_t ), /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16 */
        sizeof( int32_t ), /* RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32 */

        sizeof( uint8_t ),  /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8 */
        sizeof( uint16_t ), /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16 */
        sizeof( uint32_t )  /* RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32 */
};

SampleDataReader::SampleDataReader() {}
SampleDataReader::~SampleDataReader() {}

RideHalError_e SampleDataReader::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_numOfDataReaders = Get( config, "number", 1 );
    if ( 0 == m_numOfDataReaders )
    {
        RIDEHAL_ERROR( "invalid number\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        m_configs.reserve( m_numOfDataReaders );
    }

    for ( uint32_t i = 0; ( i < m_numOfDataReaders ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
    {
        DataReaderConfig_t cfg;

        auto typeStr = Get( config, "type" + std::to_string( i ), "image" );

        if ( "image" == typeStr )
        {
            cfg.type = DATA_READER_TYPE_IMAGE;
            cfg.format = Get( config, "format" + std::to_string( i ), RIDEHAL_IMAGE_FORMAT_NV12 );
            if ( RIDEHAL_IMAGE_FORMAT_MAX == cfg.format )
            {
                RIDEHAL_ERROR( "invalid format%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            cfg.width = Get( config, "width" + std::to_string( i ), 1920 );
            if ( 0 == cfg.width )
            {
                RIDEHAL_ERROR( "invalid width%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }

            cfg.height = Get( config, "height" + std::to_string( i ), 1024 );
            if ( 0 == cfg.height )
            {
                RIDEHAL_ERROR( "invalid height%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
        }
        else if ( "tensor" == typeStr )
        {
            cfg.type = DATA_READER_TYPE_TENSOR;

            cfg.tensorProps.type = Get( config, "tensor_type" + std::to_string( i ),
                                        RIDEHAL_TENSOR_TYPE_FLOAT_32 );

            if ( RIDEHAL_TENSOR_TYPE_MAX == cfg.tensorProps.type )
            {
                RIDEHAL_ERROR( "invalid tensor_type%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            std::vector<uint32_t> dims;
            dims = Get( config, "dims" + std::to_string( i ), dims );
            if ( 0 == dims.size() )
            {
                RIDEHAL_ERROR( "invalid dims%u\n", i );
                ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
            }
            cfg.tensorProps.numDims = dims.size();
            for ( size_t i = 0; i < dims.size(); i++ )
            {
                cfg.tensorProps.dims[i] = dims[i];
            }
        }
        else
        {
            RIDEHAL_ERROR( "invalid data reader type %s\n", typeStr.c_str() );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }

        cfg.dataPath = Get( config, "data_path" + std::to_string( i ), "" );
        if ( "" == cfg.dataPath )
        {
            RIDEHAL_INFO( "using dummy camera frame for %u\n", i );
        }

        m_configs.push_back( cfg );
    }

    m_fps = Get( config, "fps", 30 );
    if ( 0 == m_fps )
    {
        RIDEHAL_ERROR( "invalid fps = %d\n", m_fps );
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

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        RIDEHAL_ERROR( "no topic\n" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleDataReader::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_ON( DATA_READER );
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_bufferPools.resize( m_numOfDataReaders );
        for ( uint32_t i = 0; ( i < m_numOfDataReaders ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
        {
            if ( DATA_READER_TYPE_IMAGE == m_configs[i].type )
            {
                ret = m_bufferPools[i].Init( name + std::to_string( i ), LOGGER_LEVEL_INFO,
                                             m_poolSize, m_configs[i].width, m_configs[i].height,
                                             m_configs[i].format, RIDEHAL_BUFFER_USAGE_CAMERA,
                                             m_bufferFlags );
            }
            else
            {
                ret = m_bufferPools[i].Init( name + std::to_string( i ), LOGGER_LEVEL_INFO,
                                             m_poolSize, m_configs[i].tensorProps,
                                             RIDEHAL_BUFFER_USAGE_DEFAULT, m_bufferFlags );
            }
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_topicName );
    }

    return ret;
}

RideHalError_e SampleDataReader::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    m_stop = false;
    m_thread = std::thread( &SampleDataReader::ThreadMain, this );
    return ret;
}

RideHalError_e SampleDataReader::LoadImage( std::shared_ptr<SharedBuffer_t> image,
                                            std::string path )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    FILE *file = nullptr;
    size_t length = 0;

    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        RIDEHAL_DEBUG( "Failed to open file %s", path.c_str() );
        ret = RIDEHAL_ERROR_ALREADY;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        if ( image->sharedBuffer.size < length )
        {
            RIDEHAL_ERROR( "Invalid image file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_SET );
        auto r = fread( image->sharedBuffer.data(), 1, length, file );
        if ( length != r )
        {
            RIDEHAL_ERROR( "failed to read image file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( nullptr != file )
    {
        fclose( file );
    }

    RIDEHAL_DEBUG( "Loading Image %s %s", path.c_str(),
                   ( RIDEHAL_ERROR_NONE == ret ) ? "OK" : "FAIL" );

    return ret;
}

RideHalError_e SampleDataReader::LoadTensor( std::shared_ptr<SharedBuffer_t> tensor,
                                             std::string path )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    FILE *file = nullptr;
    size_t length = 0;
    uint32_t batchSize = 0;
    uint32_t oneSize = s_rideHalTensorTypeToDataSize[tensor->sharedBuffer.tensorProps.type];

    for ( uint32_t i = 1; i < tensor->sharedBuffer.tensorProps.numDims; i++ )
    {
        oneSize *= tensor->sharedBuffer.tensorProps.dims[i];
    }

    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        RIDEHAL_DEBUG( "Failed to open file %s", path.c_str() );
        ret = RIDEHAL_ERROR_ALREADY;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        batchSize = length / oneSize;
        if ( tensor->sharedBuffer.size < length )
        {
            RIDEHAL_ERROR( "Invalid Tensor file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_SET );
        auto r = fread( tensor->sharedBuffer.data(), 1, length, file );
        if ( length != r )
        {
            RIDEHAL_ERROR( "failed to read PointCloud file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            tensor->sharedBuffer.tensorProps.dims[0] = batchSize;
        }
    }

    if ( nullptr != file )
    {
        fclose( file );
    }

    RIDEHAL_DEBUG( "Loading %u batch tensor %s %s", batchSize, path.c_str(),
                   ( RIDEHAL_ERROR_NONE == ret ) ? "OK" : "FAIL" );

    return ret;
}

void SampleDataReader::ThreadMain()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    uint32_t index = 0;
    uint64_t frameId = 0;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = RIDEHAL_ERROR_NONE;
        auto start = std::chrono::high_resolution_clock::now();
        PROFILER_BEGIN();
        for ( uint32_t i = 0; ( i < m_numOfDataReaders ) && ( RIDEHAL_ERROR_NONE == ret ); i++ )
        {
            std::shared_ptr<SharedBuffer_t> buffer = m_bufferPools[i].Get();
            if ( nullptr != buffer )
            {
                if ( m_configs[i].dataPath != "" )
                {
                    std::string path;
                    if ( DATA_READER_TYPE_IMAGE == m_configs[i].type )
                    {
                        path = m_configs[i].dataPath + "/" + std::to_string( index ) +
                               s_rideHalFormatToStr[m_configs[i].format];
                        ret = LoadImage( buffer, path );
                    }
                    else
                    {
                        path = m_configs[i].dataPath + "/" + std::to_string( index ) + ".raw";
                        ret = LoadTensor( buffer, path );
                    }

                    if ( ( RIDEHAL_ERROR_NONE != ret ) && ( 0 == index ) )
                    {
                        RIDEHAL_ERROR( "Invalid data path %u %s: no file %s", i,
                                       m_configs[i].dataPath.c_str(), path.c_str() );
                        m_stop = true;
                    }
                }
                else
                {
                    // skip loading, using dummy data
                }
                if ( RIDEHAL_ERROR_NONE == ret )
                {
                    DataFrame_t frame;
                    struct timespec ts;
                    clock_gettime( CLOCK_MONOTONIC, &ts );
                    frame.buffer = buffer;
                    frame.frameId = frameId;
                    frame.timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
                    frames.Add( frame );
                }
                else
                {
                    ret = RIDEHAL_ERROR_ALREADY;
                }
            }
            else
            {
                ret = RIDEHAL_ERROR_NOMEM;
            }
        }

        if ( RIDEHAL_ERROR_NONE == ret )
        {
            PROFILER_END();
            TRACE_EVENT( frameId );
            m_pub.Publish( frames );
            index++;
            frameId++;
        }
        else if ( RIDEHAL_ERROR_ALREADY == ret )
        {
            index = 0;
            continue; /* retry load from beginning */
        }
        else if ( RIDEHAL_ERROR_NOMEM == ret )
        { /* sleep to wait buffer resource ready */
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
        else
        {
            /* OK */
        }
        auto end = std::chrono::high_resolution_clock::now();
        uint64_t elapsedMs =
                std::chrono::duration_cast<std::chrono::milliseconds>( end - start ).count();
        RIDEHAL_DEBUG( "Loading frame %" PRIu64 " cost %" PRIu64 "ms", frameId, elapsedMs );
        if ( ( 1000 / m_fps ) > elapsedMs )
        {
            std::this_thread::sleep_for(
                    std::chrono::milliseconds( ( 1000 / m_fps ) - elapsedMs ) );
        }
    }
}

RideHalError_e SampleDataReader::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleDataReader::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    return ret;
}

REGISTER_SAMPLE( DataReader, SampleDataReader );

}   // namespace sample
}   // namespace ridehal
