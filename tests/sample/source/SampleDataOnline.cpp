// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/sample/SampleDataOnline.hpp"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace ridehal
{
namespace sample
{

SampleDataOnline::SampleDataOnline() {}
SampleDataOnline::~SampleDataOnline() {}

RideHalError_e SampleDataOnline::ParseConfig( SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferFlags = 0;
    }
    else
    {
        m_bufferFlags = RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA;
    }

    m_port = Get( config, "port", 6666 );

    m_timeout = Get( config, "timeout", 2 );

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

    m_modelInOutInfoTopicName =
            Get( config, "model_io_info_topic", "/data/online/" + m_name + "/model/info" );

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        RIDEHAL_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }

    return ret;
}

RideHalError_e SampleDataOnline::Init( std::string name, SampleConfig_t &config )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    assert( sizeof( RideHal_BufferType_e ) == 4 );
    assert( sizeof( Meta ) == 128 );
    assert( sizeof( MetaModelInfo ) == 128 );
    assert( sizeof( DataMeta ) == 128 );
    assert( sizeof( DataImageMeta ) == 128 );
    assert( sizeof( DataTensorMeta ) == 128 );

    ret = SampleIF::Init( name );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        TRACE_ON( DATA_ONLINE );
        ret = ParseConfig( config );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_modelInOutInfoSub.Init( name, m_modelInOutInfoTopicName );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

RideHalError_e SampleDataOnline::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    m_stop = false;
    m_bOnline = false;
    m_bHasInfInfo = false;

    m_server = socket( AF_INET, SOCK_STREAM, 0 );
    if ( m_server < 0 )
    {
        perror( "socket:" );
        RIDEHAL_ERROR( "Failed to create server socket" );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        const int enable = 1;
        int rv = setsockopt( m_server, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof( int ) );
        if ( 0 != rv )
        {
            RIDEHAL_ERROR( "Failed to enable socket to reuse address: %d", rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        struct sockaddr_in serverAddr;
        memset( &serverAddr, 0, sizeof( struct sockaddr_in ) );
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
        serverAddr.sin_port = htons( m_port );
        int rv = bind( m_server, (struct sockaddr *) ( &serverAddr ), sizeof( struct sockaddr ) );
        if ( 0 != rv )
        {
            close( m_server );
            RIDEHAL_ERROR( "Failed to bind server port = %d, errno = %d", m_port, rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        int rv = listen( m_server, 1 ); /* only allow 1 client connection */
        if ( 0 != rv )
        {
            close( m_server );
            RIDEHAL_ERROR( "Failed to listen to port = %d, errno = %d", m_port, rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_subThread = std::thread( &SampleDataOnline::ThreadSubMain, this );
        m_pubThread = std::thread( &SampleDataOnline::ThreadPubMain, this );
    }

    return ret;
}

RideHalError_e SampleDataOnline::ReceiveData( Meta &meta )
{

    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    std::vector<DataMeta> dataMetas;
    DataFrames_t outputs;
    uint8_t *pData;
    uint64_t offset;
    uint64_t size = 0;
    int rv;

    dataMetas.resize( meta.numOfDatas );
    pData = (uint8_t *) dataMetas.data();
    offset = 0;
    size = sizeof( DataMeta ) * meta.numOfDatas;
    do
    {
        rv = recv( m_client, &pData[offset], size - offset, 0 );
        offset += rv;
    } while ( ( rv > 0 ) && ( offset < size ) );


    if ( rv > 0 )
    {
        if ( 0 == m_bufferPools.size() )
        { /* the first datas, create frame pool for it */
            m_bufferPools.resize( meta.numOfDatas );
            for ( uint32_t i = 0; i < meta.numOfDatas; i++ )
            {
                auto &dataMeta = dataMetas[i];
                if ( RIDEHAL_BUFFER_TYPE_IMAGE == dataMeta.dataType )
                {
                    DataImageMeta *pImageMeta = (DataImageMeta *) &dataMeta;
                    ret = m_bufferPools[i].Init( m_name + std::to_string( i ), LOGGER_LEVEL_INFO,
                                                 m_poolSize, pImageMeta->imageProps,
                                                 RIDEHAL_BUFFER_USAGE_DEFAULT, m_bufferFlags );
                }
                else if ( RIDEHAL_BUFFER_TYPE_TENSOR == dataMeta.dataType )
                {
                    DataTensorMeta *pTensorMeta = (DataTensorMeta *) &dataMeta;
                    ret = m_bufferPools[i].Init(
                            m_name + "_" + pTensorMeta->name, LOGGER_LEVEL_INFO, m_poolSize,
                            pTensorMeta->tensorProps, RIDEHAL_BUFFER_USAGE_DEFAULT, m_bufferFlags );
                }
                else
                {
                    RIDEHAL_ERROR( "Invalid buffer type %d", dataMetas[i].dataType );
                    ret = RIDEHAL_ERROR_FAIL;
                }

                if ( RIDEHAL_ERROR_NONE != ret )
                {
                    RIDEHAL_ERROR( "Failed to create data buffer pool %d", i );
                    rv = (int) -ret;
                    break;
                }
            }
        }
    }

    if ( rv > 0 )
    {
        for ( uint32_t i = 0; i < meta.numOfDatas; i++ )
        {
            auto &dataMeta = dataMetas[i];
            std::shared_ptr<SharedBuffer_t> buffer = m_bufferPools[i].Get();
            if ( nullptr == buffer )
            {
                RIDEHAL_ERROR( "Failed to get data buffer for input %u", i );
                rv = -ENOENT;
            }
            else if ( buffer->sharedBuffer.size != dataMeta.size )
            {
                RIDEHAL_ERROR( "size mismatch for input %u, expect %u, given %u", i, dataMeta.size,
                               buffer->sharedBuffer.size );
                rv = -EINVAL;
            }
            else
            {
                if ( RIDEHAL_BUFFER_TYPE_IMAGE == dataMeta.dataType )
                {
                    DataFrame_t frame;
                    frame.buffer = buffer;
                    frame.frameId = meta.Id;
                    frame.timestamp = meta.timestamp;
                    outputs.Add( frame );
                }
                else
                {
                    DataFrame_t frame;
                    DataTensorMeta *pTensorMeta = (DataTensorMeta *) &dataMeta;
                    frame.buffer = buffer;
                    frame.frameId = meta.Id;
                    frame.timestamp = meta.timestamp;
                    frame.name = pTensorMeta->name;
                    frame.quantScale = pTensorMeta->quantScale;
                    frame.quantOffset = pTensorMeta->quantOffset;
                    outputs.Add( frame );
                }
                size += dataMeta.size;
            }

            if ( rv < 0 )
            {
                break;
            }
        }

        if ( size != meta.payloadSize )
        {
            RIDEHAL_ERROR( "invalid payload size, expect %" PRIu64 ", given %" PRIu64, size,
                           meta.payloadSize );
            rv = -EINVAL;
        }
        else
        {
            RIDEHAL_DEBUG( "publish frame %" PRIu64 " timestamp %" PRIu64, meta.Id,
                           meta.timestamp );
        }
    }

    for ( uint32_t i = 0; ( rv > 0 ) && ( i < meta.numOfDatas ); i++ )
    {
        auto &dataMeta = dataMetas[i];
        offset = 0;
        size = dataMeta.size;
        pData = (uint8_t *) outputs.data( i );
        do
        {
            rv = recv( m_client, &pData[offset], size - offset, 0 );
            offset += rv;
        } while ( ( rv > 0 ) && ( offset < size ) );
    }

    if ( rv <= 0 )
    {
        RIDEHAL_ERROR( "recv socket failed, errno = %d", rv );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        TRACE_EVENT( meta.Id );
        m_pub.Publish( outputs );
    }

    return ret;
}

RideHalError_e SampleDataOnline::ReplyModelInfo()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rv;
    std::vector<uint8_t> payload;

    if ( false == m_bHasInfInfo )
    {
        ret = m_modelInOutInfoSub.Receive( m_modelInOutInfo );
        if ( RIDEHAL_ERROR_NONE == ret )
        {
            m_bHasInfInfo = true;
        }
    }

    if ( false == m_bHasInfInfo )
    {
        RIDEHAL_ERROR( "no inference info" );
        Meta meta;
        memset( &meta, 0, sizeof( meta ) );
        meta.command = MetaCommand::META_COMMAND_MODEL_INFO;
        rv = send( m_client, &meta, sizeof( meta ), 0 );
        if ( rv != sizeof( meta ) )
        {
            RIDEHAL_ERROR( "send failed: %d", rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        size_t payloadSize =
                sizeof( Meta ) + sizeof( DataTensorMeta ) * ( m_modelInOutInfo.inputs.size() +
                                                              m_modelInOutInfo.outputs.size() );
        payload.resize( payloadSize );
        memset( payload.data(), 0, payloadSize );
        MetaModelInfo *pMeta = (MetaModelInfo *) payload.data();
        pMeta->command = MetaCommand::META_COMMAND_MODEL_INFO;
        pMeta->numOfDatas = m_modelInOutInfo.inputs.size() + m_modelInOutInfo.outputs.size();
        pMeta->numInputs = m_modelInOutInfo.inputs.size();
        pMeta->numOuputs = m_modelInOutInfo.outputs.size();
        DataTensorMeta *pTensorMeta = (DataTensorMeta *) &pMeta[1];
        for ( size_t i = 0; i < m_modelInOutInfo.inputs.size(); i++ )
        {
            auto &input = m_modelInOutInfo.inputs[i];
            pTensorMeta->dataType = RIDEHAL_BUFFER_TYPE_TENSOR;
            pTensorMeta->tensorProps = input.properties;
            pTensorMeta->quantScale = input.quantScale;
            pTensorMeta->quantOffset = input.quantOffset;
            snprintf( pTensorMeta->name, sizeof( pTensorMeta->name ), "%s", input.name.c_str() );
            pTensorMeta++;
        }
        for ( size_t i = 0; i < m_modelInOutInfo.outputs.size(); i++ )
        {
            auto &output = m_modelInOutInfo.outputs[i];
            pTensorMeta->dataType = RIDEHAL_BUFFER_TYPE_TENSOR;
            pTensorMeta->tensorProps = output.properties;
            pTensorMeta->quantScale = output.quantScale;
            pTensorMeta->quantOffset = output.quantOffset;
            snprintf( pTensorMeta->name, sizeof( pTensorMeta->name ), "%s", output.name.c_str() );
            pTensorMeta++;
        }
        rv = send( m_client, payload.data(), payloadSize, 0 );
        if ( rv != payloadSize )
        {
            RIDEHAL_ERROR( "send failed: %d", rv );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e SampleDataOnline::ReceiveMain()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    Meta meta;
    std::vector<DataMeta> dataMetas;
    DataFrames_t outputs;
    uint8_t *pData;
    uint64_t offset;
    uint64_t size = 0;
    int r;

    pData = (uint8_t *) &meta;
    offset = 0;
    do
    {
        r = recv( m_client, &pData[offset], sizeof( Meta ) - offset, 0 );
        offset += r;
    } while ( ( r > 0 ) && ( offset < sizeof( Meta ) ) );

    if ( r > 0 )
    {
        if ( MetaCommand::META_COMMAND_MODEL_INFO == meta.command )
        {
            ret = ReplyModelInfo();
        }
        else
        {
            ret = ReceiveData( meta );
        }
    }
    else
    {
        RIDEHAL_ERROR( "recv socket failed, errno = %d", r );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

void SampleDataOnline::ThreadPubMain()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    while ( false == m_stop )
    {
        struct sockaddr_in clientAddr;
        socklen_t socklen = sizeof( clientAddr );
        m_client = accept( m_server, (struct sockaddr *) &clientAddr, &socklen );
        if ( m_client < 0 )
        {
            RIDEHAL_ERROR( "accept failed, try again" );
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = m_timeout;
        timeout.tv_usec = 0;
        int rv = setsockopt( m_client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof( timeout ) );
        if ( 0 != rv )
        {
            RIDEHAL_WARN( "Failed to set socket %u s timeout, errno = %d", m_timeout, rv );
        }

        RIDEHAL_INFO( "remote client online from port %d", m_port );
        m_bOnline = true;

        while ( false == m_stop )
        {
            ret = ReceiveMain();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
        close( m_client );
        m_client = -1;
        RIDEHAL_INFO( "remote client offline" );
        m_bOnline = false;
    }
}


void SampleDataOnline::ThreadSubMain()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    int rv;
    size_t payloadSize;
    uint8_t *pData;

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( ( RIDEHAL_ERROR_NONE == ret ) && ( m_bOnline ) )
        {
            RIDEHAL_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                           frames.FrameId( 0 ), frames.Timestamp( 0 ) );
            payloadSize = sizeof( DataMeta ) * frames.frames.size();
            for ( auto &frame : frames.frames )
            {
                payloadSize += frame.size();
            }
            m_payload.resize( sizeof( Meta ) + payloadSize );
            Meta *pMeta = (Meta *) m_payload.data();
            pMeta->payloadSize = payloadSize;
            pMeta->Id = frames.FrameId( 0 );
            pMeta->timestamp = frames.Timestamp( 0 );
            pMeta->numOfDatas = frames.frames.size();
            pMeta->command = MetaCommand::META_COMMAND_DATA;
            DataMeta *pDataMeta = (DataMeta *) &pMeta[1];
            for ( auto &frame : frames.frames )
            {
                if ( RIDEHAL_BUFFER_TYPE_IMAGE == frame.BufferType() )
                {
                    DataImageMeta *pImageMeta = (DataImageMeta *) pDataMeta;
                    pImageMeta->dataType = RIDEHAL_BUFFER_TYPE_IMAGE;
                    pImageMeta->size = frame.size();
                    pImageMeta->imageProps = frame.buffer->sharedBuffer.imgProps;
                }
                else
                {
                    DataTensorMeta *pTensorMeta = (DataTensorMeta *) pDataMeta;
                    pTensorMeta->dataType = RIDEHAL_BUFFER_TYPE_TENSOR;
                    pTensorMeta->size = frame.size();
                    pTensorMeta->tensorProps = frame.buffer->sharedBuffer.tensorProps;
                    pTensorMeta->quantScale = frame.quantScale;
                    pTensorMeta->quantOffset = frame.quantOffset;
                    snprintf( pTensorMeta->name, sizeof( pTensorMeta->name ), "%s",
                              frame.name.c_str() );
                }
                pDataMeta++;
            }
            pData = (uint8_t *) pDataMeta;
            for ( auto &frame : frames.frames )
            {
                memcpy( pData, frame.data(), frame.size() );
                pData += frame.size();
            }

            size_t offset = 0;
            size_t totalSize = m_payload.size();
            pData = m_payload.data();
            do
            {
                rv = send( m_client, &pData[offset], totalSize - offset, 0 );
                if ( rv >= 0 )
                {
                    offset += rv;
                }
                else
                {
                    RIDEHAL_ERROR( "send failed: %d", rv );
                    break;
                }
            } while ( offset < totalSize );
        }
    }
}

RideHalError_e SampleDataOnline::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( m_server >= 0 )
    {
        close( m_server );
        m_server = -1;
    }

    m_stop = true;
    if ( m_subThread.joinable() )
    {
        m_subThread.join();
    }

    if ( m_pubThread.joinable() )
    {
        m_pubThread.join();
    }

    PROFILER_SHOW();

    return ret;
}

RideHalError_e SampleDataOnline::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    m_bufferPools.clear();

    return ret;
}

REGISTER_SAMPLE( DataOnline, SampleDataOnline );

}   // namespace sample
}   // namespace ridehal
