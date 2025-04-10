// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "ridehal/sample/SampleIF.hpp"
#include <assert.h>

namespace ridehal
{
namespace sample
{

std::map<std::string, Sample_CreateFunction_t> SampleIF::s_SampleMap;

std::mutex SampleIF::s_locks[RIDEHAL_PROCESSOR_MAX];

std::mutex SampleIF::s_bufMapLock;
std::map<std::string, std::vector<RideHal_SharedBuffer_t>> SampleIF::s_bufferMaps;

SampleIF *SampleIF::Create( std::string name )
{
    SampleIF *sample = nullptr;

    auto it = s_SampleMap.find( name );
    if ( it != s_SampleMap.end() )
    {
        Sample_CreateFunction_t createFnc = it->second;
        sample = createFnc();
    }

    return sample;
}

void SampleIF::RegisterSample( std::string name, Sample_CreateFunction_t createFnc )
{
    auto it = s_SampleMap.find( name );
    if ( it == s_SampleMap.end() )
    {
        s_SampleMap[name] = createFnc;
        printf( "register sample type %s\n", name.c_str() );
    }
    else
    {
        printf( "sample %s is already registered\n", name.c_str() );
        assert( 0 );
    }
}

RideHalError_e SampleIF::Init( std::string name )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_name = name;
    ret = RIDEHAL_LOGGER_INIT( name.c_str(), LOGGER_LEVEL_INFO );

    m_profiler.Init( name );
    m_systrace.Init( name );

    return ret;
}

RideHalError_e SampleIF::Init( RideHal_ProcessorType_e processor )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

#if defined( WITH_RSM_V2 )
    const char *envValue = getenv( "RIDEHAL_DISABLE_RSM" );
    if ( nullptr != envValue )
    {
        std::string strEnv = envValue;
        if ( "YES" == strEnv )
        {
            m_bRsmDisabled = true;
        }
    }
    if ( ( processor <= RIDEHAL_PROCESSOR_HTP1 ) && ( false == m_bRsmDisabled ) )
    {
        memset( &m_acquireCmdV2, 0, sizeof( m_acquireCmdV2 ) );
        m_acquireCmdV2.resource = (rsm_resource_group) processor;
        m_acquireCmdV2.priority = QUEUE_PRIORITY_DEFAULT;
        m_acquireCmdV2.configure.priority = REQUEST_PRIORITY_DEFAULT;
        m_acquireCmdV2.duration_us = 0;
        m_acquireCmdV2.timeout_us = 1000000;
        int rc = rsm_register_v2( &m_handle );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "rsm init failed: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_INFO( "rsm init for processor %d", processor );
            m_processor = processor;
        }
    }
    else
#endif
            if ( processor < RIDEHAL_PROCESSOR_MAX )
    {
        RIDEHAL_INFO( "global mutex for processor %d", processor );
        m_processor = processor;
    }
    else
    {
        RIDEHAL_ERROR( "invalid processor %d", processor );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_systrace.Init( (SysTrace_ProcessorType_e) processor );
    }

    return ret;
}

RideHalError_e SampleIF::Lock()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

#if defined( WITH_RSM_V2 )
    if ( ( m_processor <= RIDEHAL_PROCESSOR_HTP1 ) && ( false == m_bRsmDisabled ) )
    {
        int rc = rsm_acquire_v2( m_handle, &m_acquireCmdV2, &m_acquireRspV2 );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "rsm acquire failed: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
#endif
            if ( m_processor < RIDEHAL_PROCESSOR_MAX )
    {
        s_locks[m_processor].lock();
    }
    else
    {
        RIDEHAL_ERROR( "the processor lock not ready" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

RideHalError_e SampleIF::Unlock()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

#if defined( WITH_RSM_V2 )
    if ( ( m_processor <= RIDEHAL_PROCESSOR_HTP1 ) && ( false == m_bRsmDisabled ) )
    {
        int rc = rsm_release_v2( m_handle, m_acquireRspV2.token );
        if ( 0 != rc )
        {
            RIDEHAL_ERROR( "rsm release failed: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
#endif
            if ( m_processor < RIDEHAL_PROCESSOR_MAX )
    {
        s_locks[m_processor].unlock();
    }
    else
    {
        RIDEHAL_ERROR( "the processor lock not ready" );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }

    return ret;
}

const char *SampleIF::GetName()
{
    return m_name.c_str();
}

uint32_t SampleIF::StringToU32( std::string strV )
{
    uint32_t retV = 0;

    if ( 0 == strncmp( strV.c_str(), "0x", 2 ) )
    {
        retV = strtoul( strV.c_str(), nullptr, 16 );
    }
    else if ( 0 == strncmp( strV.c_str(), "0b", 2 ) )
    {
        retV = strtoul( &strV.c_str()[2], nullptr, 2 );
    }
    else if ( 0 == strncmp( strV.c_str(), "0", 1 ) )
    {
        retV = strtoul( strV.c_str(), nullptr, 8 );
    }
    else
    {
        retV = strtoul( strV.c_str(), nullptr, 10 );
    }
    return retV;
}

std::string SampleIF::Get( SampleConfig_t &config, std::string key, const char *defaultV )
{
    std::string ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = it->second;
    }

    RIDEHAL_DEBUG( "Get config %s = %s\n", key.c_str(), ret.c_str() );

    return ret;
}

std::string SampleIF::Get( SampleConfig_t &config, std::string key, std::string defaultV )
{
    std::string ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = it->second;
    }

    RIDEHAL_DEBUG( "Get config %s = %s\n", key.c_str(), ret.c_str() );

    return ret;
}

std::vector<std::string> SampleIF::Get( SampleConfig_t &config, std::string key,
                                        std::vector<std::string> defaultV )
{
    std::vector<std::string> ret = defaultV;
    std::string strV = "";
    std::string::size_type prev_pos = 0, pos = 0;

    auto it = config.find( key );
    if ( it != config.end() )
    {
        strV = it->second;

        while ( ( pos = strV.find( ',', pos ) ) != std::string::npos )
        {
            std::string substring( strV.substr( prev_pos, pos - prev_pos ) );

            if ( "" != substring )
            {
                ret.push_back( substring );
            }

            prev_pos = ++pos;
        }

        std::string substring( strV.substr( prev_pos, pos - prev_pos ) );
        if ( "" != substring )
        {
            ret.push_back( substring );
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %s\n", key.c_str(), strV.c_str() );

    return ret;
}

std::vector<uint32_t> SampleIF::Get( SampleConfig_t &config, std::string key,
                                     std::vector<uint32_t> defaultV )
{
    std::vector<uint32_t> ret = defaultV;
    std::string strV = "";
    std::string::size_type prev_pos = 0, pos = 0;

    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret.resize( 0 );
        strV = it->second;

        while ( ( pos = strV.find( ',', pos ) ) != std::string::npos )
        {
            std::string substring( strV.substr( prev_pos, pos - prev_pos ) );

            if ( "" != substring )
            {
                uint32_t value = StringToU32( substring );
                ret.push_back( value );
            }

            prev_pos = ++pos;
        }

        std::string substring( strV.substr( prev_pos, pos - prev_pos ) );
        if ( "" != substring )
        {
            uint32_t value = StringToU32( substring );
            ret.push_back( value );
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %s\n", key.c_str(), strV.c_str() );

    return ret;
}

std::vector<float> SampleIF::Get( SampleConfig_t &config, std::string key,
                                  std::vector<float> defaultV )
{
    std::vector<float> ret = defaultV;
    std::string strV = "";
    std::string::size_type prev_pos = 0, pos = 0;

    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret.resize( 0 );
        strV = it->second;

        while ( ( pos = strV.find( ',', pos ) ) != std::string::npos )
        {
            std::string substring( strV.substr( prev_pos, pos - prev_pos ) );

            if ( "" != substring )
            {
                float value = std::stof( substring );
                ret.push_back( value );
            }

            prev_pos = ++pos;
        }

        std::string substring( strV.substr( prev_pos, pos - prev_pos ) );
        if ( "" != substring )
        {
            float value = std::stof( substring );
            ret.push_back( value );
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %s\n", key.c_str(), strV.c_str() );

    return ret;
}

int32_t SampleIF::Get( SampleConfig_t &config, std::string key, int32_t defaultV )
{
    int32_t ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = std::stoi( it->second );
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );

    return ret;
}

uint32_t SampleIF::Get( SampleConfig_t &config, std::string key, uint32_t defaultV )
{
    uint32_t ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = StringToU32( it->second );
    }

    RIDEHAL_DEBUG( "Get config %s = %u\n", key.c_str(), ret );

    return ret;
}

float SampleIF::Get( SampleConfig_t &config, std::string key, float defaultV )
{
    float ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        ret = std::stof( it->second );
    }

    RIDEHAL_DEBUG( "Get config %s = %f\n", key.c_str(), ret );

    return ret;
}

RideHal_ImageFormat_e SampleIF::Get( SampleConfig_t &config, std::string key,
                                     RideHal_ImageFormat_e defaultV )
{
    RideHal_ImageFormat_e ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        std::string format = it->second;
        if ( "rgb" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_RGB888;
        }
        else if ( "bgr" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_BGR888;
        }
        else if ( "uyvy" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_UYVY;
        }
        else if ( "nv12" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_NV12;
        }
        else if ( "nv12_ubwc" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_NV12_UBWC;
        }
        else if ( "p010" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_P010;
        }
        else if ( "tp10_ubwc" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_TP10_UBWC;
        }
        else if ( "h264" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_COMPRESSED_H264;
        }
        else if ( "h265" == format )
        {
            ret = RIDEHAL_IMAGE_FORMAT_COMPRESSED_H265;
        }
        else
        {
            ret = RIDEHAL_IMAGE_FORMAT_MAX;
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );
    return ret;
}

RideHal_TensorType_e SampleIF::Get( SampleConfig_t &config, std::string key,
                                    RideHal_TensorType_e defaultV )
{
    RideHal_TensorType_e ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        std::string format = it->second;
        if ( "int8" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_INT_8;
        }
        else if ( "int16" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_INT_16;
        }
        else if ( "int32" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_INT_32;
        }
        else if ( "int64" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_INT_64;
        }
        else if ( "uint8" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_UINT_8;
        }
        else if ( "uint16" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_UINT_16;
        }
        else if ( "uint32" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_UINT_32;
        }
        else if ( "uint64" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_UINT_64;
        }
        else if ( "float16" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_FLOAT_16;
        }
        else if ( "float32" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_FLOAT_32;
        }
        else if ( "float64" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_FLOAT_64;
        }
        else if ( "sfixed_point8" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_8;
        }
        else if ( "sfixed_point16" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_16;
        }
        else if ( "sfixed_point32" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_SFIXED_POINT_32;
        }
        else if ( "ufixed_point8" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_8;
        }
        else if ( "ufixed_point16" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_16;
        }
        else if ( "ufixed_point32" == format )
        {
            ret = RIDEHAL_TENSOR_TYPE_UFIXED_POINT_32;
        }
        else
        {
            ret = RIDEHAL_TENSOR_TYPE_MAX;
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );
    return ret;
}

RideHal_ProcessorType_e SampleIF::Get( SampleConfig_t &config, std::string key,
                                       RideHal_ProcessorType_e defaultV )
{
    RideHal_ProcessorType_e ret = defaultV;

    auto it = config.find( key );
    if ( it != config.end() )
    {
        std::string processor = it->second;
        if ( "htp0" == processor )
        {
            ret = RIDEHAL_PROCESSOR_HTP0;
        }
        else if ( "htp1" == processor )
        {
            ret = RIDEHAL_PROCESSOR_HTP1;
        }
        else if ( "cpu" == processor )
        {
            ret = RIDEHAL_PROCESSOR_CPU;
        }
        else if ( "gpu" == processor )
        {
            ret = RIDEHAL_PROCESSOR_GPU;
        }
        else
        {
            ret = RIDEHAL_PROCESSOR_MAX;
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );

    return ret;
}

bool SampleIF::Get( SampleConfig_t &config, std::string key, bool defaultV )
{
    bool ret = defaultV;
    auto it = config.find( key );
    if ( it != config.end() )
    {
        if ( "true" == it->second )
        {
            ret = true;
        }
        else
        {
            ret = false;
        }
    }

    RIDEHAL_DEBUG( "Get config %s = %d\n", key.c_str(), ret );

    return ret;
}

RideHalError_e SampleIF::RegisterBuffers( std::string name, const RideHal_SharedBuffer_t *pBuffers,
                                          uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( nullptr == pBuffers ) || ( 0 >= numBuffers ) )
    {
        RIDEHAL_LOG_ERROR( "invalid parameter pBuffers: %p, numBuffers: %u", pBuffers, numBuffers );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        std::lock_guard<std::mutex> guard( s_bufMapLock );
        auto it = s_bufferMaps.find( name );
        if ( it == s_bufferMaps.end() )
        {
            std::vector<RideHal_SharedBuffer_t> buffers;
            buffers.resize( numBuffers );
            memcpy( buffers.data(), pBuffers, numBuffers * sizeof( RideHal_SharedBuffer_t ) );
            s_bufferMaps[name] = buffers;
            RIDEHAL_LOG_INFO( "regsiter buffers %s", name.c_str() );
        }
        else
        {
            RIDEHAL_LOG_ERROR( "buffers %s already in map", name.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e SampleIF::GetBuffers( std::string name, RideHal_SharedBuffer_t *pBuffers,
                                     uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( ( nullptr == pBuffers ) || ( 0 >= numBuffers ) )
    {
        RIDEHAL_LOG_ERROR( "invalid parameter pBuffers: %p, numBuffers: %u", pBuffers, numBuffers );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        std::lock_guard<std::mutex> guard( s_bufMapLock );
        auto it = s_bufferMaps.find( name );
        if ( it != s_bufferMaps.end() )
        {
            std::vector<RideHal_SharedBuffer_t> &buffers = it->second;
            if ( buffers.size() == (size_t) numBuffers )
            {
                memcpy( pBuffers, buffers.data(), numBuffers * sizeof( RideHal_SharedBuffer_t ) );
            }
            else
            {
                RIDEHAL_LOG_ERROR( "buffers %s number is %" PRIu64, name.c_str(), buffers.size() );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        else
        {
            RIDEHAL_LOG_ERROR( "buffers %s not found in map", name.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e SampleIF::DeRegisterBuffers( std::string name )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    std::lock_guard<std::mutex> guard( s_bufMapLock );
    auto it = s_bufferMaps.find( name );
    if ( it != s_bufferMaps.end() )
    {
        (void) s_bufferMaps.erase( it );
        RIDEHAL_LOG_INFO( "deregsiter buffers %s", name.c_str() );
    }
    else
    {
        RIDEHAL_LOG_ERROR( "buffers %s not found in map", name.c_str() );
        ret = RIDEHAL_ERROR_FAIL;
    }


    return ret;
}

RideHalError_e SampleIF::LoadFile( RideHal_SharedBuffer_t buffer, std::string path )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    FILE *file = nullptr;
    size_t length = 0;
    size_t size = buffer.size;

    file = fopen( path.c_str(), "rb" );
    if ( nullptr == file )
    {
        RIDEHAL_ERROR( "Failed to open file %s", path.c_str() );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_END );
        length = (size_t) ftell( file );
        if ( size != length )
        {
            RIDEHAL_ERROR( "Invalid file size for %s, need %d but got %d", path.c_str(), size,
                           length );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        fseek( file, 0, SEEK_SET );
        auto r = fread( buffer.data(), 1, length, file );
        if ( length != r )
        {
            RIDEHAL_ERROR( "failed to read map table file %s", path.c_str() );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( nullptr != file )
    {
        fclose( file );
    }

    return ret;
}

}   // namespace sample
}   // namespace ridehal
