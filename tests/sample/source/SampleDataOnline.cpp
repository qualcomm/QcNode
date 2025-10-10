// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/sample/SampleDataOnline.hpp"
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace QC
{
namespace sample
{

SampleDataOnline::SampleDataOnline() {}
SampleDataOnline::~SampleDataOnline() {}

QCStatus_e SampleDataOnline::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferCache = QC_CACHEABLE_NON;
    }
    else
    {
        m_bufferCache = QC_CACHEABLE;
    }

    m_port = Get( config, "port", 6666 );

    m_timeout = Get( config, "timeout", 2 );

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_modelInOutInfoTopicName =
            Get( config, "model_io_info_topic", "/data/online/" + m_name + "/model/info" );

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e SampleDataOnline::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    assert( sizeof( QCBufferType_e ) == 4 );
    assert( sizeof( Meta ) == 128 );
    assert( sizeof( MetaModelInfo ) == 128 );
    assert( sizeof( DataMeta ) == 128 );
    assert( sizeof( DataImageMeta ) == 128 );
    assert( sizeof( DataTensorMeta ) == 128 );

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( DATA_ONLINE );
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_modelInOutInfoSub.Init( name, m_modelInOutInfoTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

QCStatus_e SampleDataOnline::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    m_stop = false;
    m_bOnline = false;
    m_bHasInfInfo = false;

    m_server = socket( AF_INET, SOCK_STREAM, 0 );
    if ( m_server < 0 )
    {
        perror( "socket:" );
        QC_ERROR( "Failed to create server socket" );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        const int enable = 1;
        int rv = setsockopt( m_server, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof( int ) );
        if ( 0 != rv )
        {
            QC_ERROR( "Failed to enable socket to reuse address: %d", rv );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
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
            QC_ERROR( "Failed to bind server port = %d, errno = %d", m_port, rv );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        int rv = listen( m_server, 1 ); /* only allow 1 client connection */
        if ( 0 != rv )
        {
            close( m_server );
            QC_ERROR( "Failed to listen to port = %d, errno = %d", m_port, rv );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_subThread = std::thread( &SampleDataOnline::ThreadSubMain, this );
        m_pubThread = std::thread( &SampleDataOnline::ThreadPubMain, this );
    }

    return ret;
}

QCStatus_e SampleDataOnline::ReceiveData( Meta &meta )
{

    QCStatus_e ret = QC_STATUS_OK;
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
                if ( QC_BUFFER_TYPE_IMAGE == dataMeta.dataType )
                {
                    DataImageMeta *pImageMeta = (DataImageMeta *) &dataMeta;
                    ret = m_bufferPools[i].Init(
                            m_name + std::to_string( i ), m_nodeId, LOGGER_LEVEL_INFO, m_poolSize,
                            pImageMeta->imageProps, QC_MEMORY_ALLOCATOR_DMA, m_bufferCache );
                }
                else if ( QC_BUFFER_TYPE_TENSOR == dataMeta.dataType )
                {
                    DataTensorMeta *pTensorMeta = (DataTensorMeta *) &dataMeta;
                    ret = m_bufferPools[i].Init( m_name + "_" + pTensorMeta->name, m_nodeId,
                                                 LOGGER_LEVEL_INFO, m_poolSize,
                                                 pTensorMeta->tensorProps, QC_MEMORY_ALLOCATOR_DMA,
                                                 m_bufferCache );
                }
                else
                {
                    QC_ERROR( "Invalid buffer type %d", dataMetas[i].dataType );
                    ret = QC_STATUS_FAIL;
                }

                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to create data buffer pool %d", i );
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
                QC_ERROR( "Failed to get data buffer for input %u", i );
                rv = -ENOENT;
            }
            else if ( buffer->GetDataSize() != static_cast<size_t>( dataMeta.size ) )
            {
                QC_ERROR( "size mismatch for input %u, expect %u, given %u", i, dataMeta.size,
                          buffer->GetDataSize() );
                rv = -EINVAL;
            }
            else
            {
                if ( QC_BUFFER_TYPE_IMAGE == dataMeta.dataType )
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
            QC_ERROR( "invalid payload size, expect %" PRIu64 ", given %" PRIu64, size,
                      meta.payloadSize );
            rv = -EINVAL;
        }
        else
        {
            QC_DEBUG( "publish frame %" PRIu64 " timestamp %" PRIu64, meta.Id, meta.timestamp );
        }
    }

    for ( uint32_t i = 0; ( rv > 0 ) && ( i < meta.numOfDatas ); i++ )
    {
        auto &dataMeta = dataMetas[i];
        offset = 0;
        size = dataMeta.size;
        pData = (uint8_t *) outputs.GetDataPtr( i );
        do
        {
            rv = recv( m_client, &pData[offset], size - offset, 0 );
            offset += rv;
        } while ( ( rv > 0 ) && ( offset < size ) );
    }

    if ( rv <= 0 )
    {
        QC_ERROR( "recv socket failed, errno = %d", rv );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        TRACE_EVENT( meta.Id );
        m_pub.Publish( outputs );
    }

    return ret;
}

QCStatus_e SampleDataOnline::ReplyModelInfo()
{
    QCStatus_e ret = QC_STATUS_OK;
    int rv;
    std::vector<uint8_t> payload;

    if ( false == m_bHasInfInfo )
    {
        ret = m_modelInOutInfoSub.Receive( m_modelInOutInfo );
        if ( QC_STATUS_OK == ret )
        {
            m_bHasInfInfo = true;
        }
    }

    if ( false == m_bHasInfInfo )
    {
        QC_ERROR( "no inference info" );
        Meta meta;
        memset( &meta, 0, sizeof( meta ) );
        meta.command = MetaCommand::META_COMMAND_MODEL_INFO;
        rv = send( m_client, &meta, sizeof( meta ), 0 );
        if ( rv != sizeof( meta ) )
        {
            QC_ERROR( "send failed: %d", rv );
            ret = QC_STATUS_FAIL;
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
            pTensorMeta->dataType = QC_BUFFER_TYPE_TENSOR;
            pTensorMeta->tensorProps = input.properties;
            pTensorMeta->quantScale = input.quantScale;
            pTensorMeta->quantOffset = input.quantOffset;
            snprintf( pTensorMeta->name, sizeof( pTensorMeta->name ), "%s", input.name.c_str() );
            pTensorMeta++;
        }
        for ( size_t i = 0; i < m_modelInOutInfo.outputs.size(); i++ )
        {
            auto &output = m_modelInOutInfo.outputs[i];
            pTensorMeta->dataType = QC_BUFFER_TYPE_TENSOR;
            pTensorMeta->tensorProps = output.properties;
            pTensorMeta->quantScale = output.quantScale;
            pTensorMeta->quantOffset = output.quantOffset;
            snprintf( pTensorMeta->name, sizeof( pTensorMeta->name ), "%s", output.name.c_str() );
            pTensorMeta++;
        }
        rv = send( m_client, payload.data(), payloadSize, 0 );
        if ( rv != payloadSize )
        {
            QC_ERROR( "send failed: %d", rv );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e SampleDataOnline::ReceiveMain()
{
    QCStatus_e ret = QC_STATUS_OK;
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
        QC_ERROR( "recv socket failed, errno = %d", r );
        ret = QC_STATUS_FAIL;
    }

    return ret;
}

void SampleDataOnline::ThreadPubMain()
{
    QCStatus_e ret = QC_STATUS_OK;
    while ( false == m_stop )
    {
        struct sockaddr_in clientAddr;
        socklen_t socklen = sizeof( clientAddr );
        m_client = accept( m_server, (struct sockaddr *) &clientAddr, &socklen );
        if ( m_client < 0 )
        {
            QC_ERROR( "accept failed, try again" );
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = m_timeout;
        timeout.tv_usec = 0;
        int rv = setsockopt( m_client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof( timeout ) );
        if ( 0 != rv )
        {
            QC_WARN( "Failed to set socket %u s timeout, errno = %d", m_timeout, rv );
        }

        QC_INFO( "remote client online from port %d", m_port );
        m_bOnline = true;

        while ( false == m_stop )
        {
            ret = ReceiveMain();
            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
        close( m_client );
        m_client = -1;
        QC_INFO( "remote client offline" );
        m_bOnline = false;
    }
}


void SampleDataOnline::ThreadSubMain()
{
    QCStatus_e ret = QC_STATUS_OK;
    int rv;
    size_t payloadSize;
    uint8_t *pData;

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( ( QC_STATUS_OK == ret ) && ( m_bOnline ) )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            payloadSize = sizeof( DataMeta ) * frames.frames.size();
            for ( auto &frame : frames.frames )
            {
                payloadSize += frame.GetDataSize();
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
                if ( QC_BUFFER_TYPE_IMAGE == frame.GetBufferType() )
                {
                    DataImageMeta *pImageMeta = (DataImageMeta *) pDataMeta;
                    pImageMeta->dataType = QC_BUFFER_TYPE_IMAGE;
                    pImageMeta->size = frame.GetDataSize();
                    pImageMeta->imageProps = frame.GetImageProps();
                }
                else
                {
                    DataTensorMeta *pTensorMeta = (DataTensorMeta *) pDataMeta;
                    pTensorMeta->dataType = QC_BUFFER_TYPE_TENSOR;
                    pTensorMeta->size = frame.GetDataSize();
                    pTensorMeta->tensorProps = frame.GetTensorProps();
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
                memcpy( pData, frame.GetDataPtr(), frame.GetDataSize() );
                pData += frame.GetDataSize();
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
                    QC_ERROR( "send failed: %d", rv );
                    break;
                }
            } while ( offset < totalSize );
        }
    }
}

QCStatus_e SampleDataOnline::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

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

QCStatus_e SampleDataOnline::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_bufferPools.clear();

    return ret;
}

REGISTER_SAMPLE( DataOnline, SampleDataOnline );

}   // namespace sample
}   // namespace QC
