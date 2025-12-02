// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/SampleC2C.hpp"

#define C2C_MAX_PIO_MSG_SIZE 256

namespace QC
{
namespace sample
{

typedef enum
{
    C2C_MSG_DMA_INIT = 1,
    C2C_MSG_DMA_SETUP = 2,
    C2C_MSG_DMA_READY = 3,
    C2C_MSG_DMA_STOP = 4,
} C2C_MsgType_t;

typedef struct
{
    C2C_MsgType_t type;
    uint32_t bufIdx;
    uint64_t frameId;
    uint64_t timestamp;
    union
    {
        /* data structure for connecting DMA channels between two devices */
        struct
        {
            uint64_t bufId;   /* Buffer ID to open a DMA connection and logical frame buffer */
            uint32_t bufSize; /* Receiver buffer size */
        } dma;
    };
} C2C_PioMsg_t;


SampleC2C::SampleC2C() {}
SampleC2C::~SampleC2C() {}

QCStatus_e SampleC2C::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    auto typeStr = Get( config, "type", "pub" );
    if ( "pub" == typeStr )
    {
        m_bIsPub = true;
    }
    else if ( "sub" == typeStr )
    {
        m_bIsPub = false;
    }
    else
    {
        QC_ERROR( "invalid type <%s>\n", typeStr.c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( true == m_bIsPub )
    {
        m_buffersName = Get( config, "buffers_name", "" );
        if ( "" == m_buffersName )
        {
            QC_ERROR( "no buffers_name\n" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        QC_ERROR( "no topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_queueDepth = Get( config, "queue_depth", 2u );
    m_width = Get( config, "width", 0u );
    if ( 0 == m_width )
    {
        QC_ERROR( "invalid width" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_height = Get( config, "height", 0u );
    if ( 0 == m_height )
    {
        QC_ERROR( "invalid height" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_format = Get( config, "format", QC_IMAGE_FORMAT_NV12 );
    if ( QC_IMAGE_FORMAT_MAX == m_format )
    {
        QC_ERROR( "invalid format\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_poolSize = Get( config, "pool_size", 4u );
    m_channleId = Get( config, "channel", 0u );

    return ret;
}

QCStatus_e SampleC2C::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    TRACE_BEGIN( SYSTRACE_TASK_INIT );

    if ( QC_STATUS_OK == ret )
    {
        m_buffers.reserve( m_poolSize );
        m_dmaInfos.resize( m_poolSize );
        if ( m_bIsPub )
        {
            ret = m_sub.Init( name, m_topicName, m_queueDepth );
            if ( QC_STATUS_OK == ret )
            {
                ret = SampleIF::GetBuffers( m_buffersName, m_buffers );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Can't find input buffers: %s", m_buffersName.c_str() );
                }
            }
        }
        else
        {
            ret = m_pub.Init( name, m_topicName );
            if ( QC_STATUS_OK == ret )
            {
                ret = m_imagePool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, m_width,
                                        m_height, m_format );
                if ( QC_STATUS_OK == ret )
                {
                    ret = m_imagePool.GetBuffers( m_buffers );
                    /* release it, as just need the shared buffer image properties */
                    (void) m_imagePool.Deinit();
                }
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_c2cFd = C2C_Open_Channel( m_channleId, C2C_MAX_PIO_MSG_SIZE );
        if ( m_c2cFd < 0 )
        {
            QC_ERROR( "C2C open %u failed: %d", m_channleId, m_c2cFd );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( ( QC_STATUS_OK == ret ) && ( true == m_bIsPub ) )
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            QCBufferDescriptorBase_t &buffer = m_buffers[i];
            DmaInfo &dmaInfo = m_dmaInfos[i];
            m_dmaIndexMap[buffer.dmaHandle] = i;
            dmaInfo.dmaBufId = i;
            dmaInfo.dmaFd =
                    C2C_Register_DMABuffer( &dmaInfo.dmaBufId, (pmem_handle_t) buffer.dmaHandle,
                                            buffer.pBuf, buffer.size, C2C_LOCAL_BUFFER );
            if ( dmaInfo.dmaFd < 0 )
            {
                QC_ERROR( "C2C register DMA %u failed: %d", i, dmaInfo.dmaFd );
                ret = QC_STATUS_FAIL;
                break;
            }
        }
    }

    if ( ( QC_STATUS_OK == ret ) && ( false == m_bIsPub ) )
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            QCBufferDescriptorBase_t &buffer = m_buffers[i];
            DmaInfo &dmaInfo = m_dmaInfos[i];
            m_dmaIndexMap[buffer.dmaHandle] = i;
            dmaInfo.dmaBufId = i;
            dmaInfo.dmaFd = C2C_Request_DMABuffer( &dmaInfo.dmaBufId, &dmaInfo.pData, buffer.size );
            if ( dmaInfo.dmaFd < 0 )
            {
                QC_ERROR( "C2C request DMA %u failed: %d", i, dmaInfo.dmaFd );
                ret = QC_STATUS_FAIL;
                break;
            }
            else
            { /* update the virtual address */
                buffer.pBuf = dmaInfo.pData;
                buffer.dmaHandle = 0; /* not known */
            }
        }
    }

    TRACE_END( SYSTRACE_TASK_INIT );

    return ret;
}


int SampleC2C::WaitC2cMsg( void *buff, size_t size )
{
    int rc = C2C_SUCCESS;
    QC_DEBUG( "RECV BEGIN" );
    while ( true )
    {
        rc = C2C_MsgRecv( m_c2cFd, buff, size );
        if ( C2C_RESOURCE_UNAVAILABLE != rc )
        {
            break;
        }
        QC_DEBUG( "RECV return with no data, waiting again" );
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    }
    QC_DEBUG( "RECV END: rc=%d", rc );

    return rc;
}

QCStatus_e SampleC2C::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    C2C_PioMsg_t msg = { (C2C_MsgType_t) 0 };
    int rc;

    TRACE_BEGIN( SYSTRACE_TASK_START );

    if ( true == m_bIsPub )
    {
        /* notifier the receiver that we have initialised */
        msg.type = C2C_MSG_DMA_INIT;
        rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
        if ( C2C_SUCCESS != rc )
        {
            QC_ERROR( "Send INIT failed: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "Send INIT DONE" );
        }

        if ( QC_STATUS_OK == ret )
        {
            for ( uint32_t i; i < m_poolSize; i++ )
            {
                QCBufferDescriptorBase_t &buffer = m_buffers[i];
                DmaInfo &dmaInfo = m_dmaInfos[i];
                rc = WaitC2cMsg( (void *) &msg, sizeof( msg ) );
                if ( C2C_SUCCESS != rc )
                {
                    QC_ERROR( "Recv SETUP for buffer %u failed: %d", i, rc );
                    ret = QC_STATUS_FAIL;
                }
                else if ( C2C_MSG_DMA_SETUP != msg.type )
                {
                    QC_ERROR( "Recv SETUP for buffer %u with invalid type: %d", i, msg.type );
                    ret = QC_STATUS_FAIL;
                }
                else if ( msg.bufIdx != i )
                {
                    QC_ERROR( "Recv SETUP for buffer %u with invalid idx: %u", i, msg.bufIdx );
                    ret = QC_STATUS_FAIL;
                }
                else if ( buffer.size != msg.dma.bufSize )
                {
                    QC_ERROR( "Recv SETUP for buffer %u with invalid size: %" PRIu64 " != %" PRIu64,
                              i, buffer.size, msg.dma.bufSize );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    QC_DEBUG( "Recv SETUP for buffer %u", i );
                    rc = C2C_Connect_DMABuffer( dmaInfo.dmaFd, msg.dma.bufId, msg.dma.bufSize );
                    if ( C2C_SUCCESS != rc )
                    {
                        QC_ERROR( "Connect DMA %u failed: %d", i, rc );
                        ret = QC_STATUS_FAIL;
                    }
                    else
                    {
                        QC_DEBUG( "Connect DMA %u Done", i );
                    }
                }

                if ( QC_STATUS_OK != ret )
                {
                    break;
                }
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleC2C::ThreadPubMain, this );
        }
    }
    else
    {
        /* wating for the initialization massasge from the sender */
        QC_DEBUG( "Wait INIT" );
        rc = WaitC2cMsg( (void *) &msg, sizeof( msg ) );
        if ( ( C2C_SUCCESS != rc ) || ( C2C_MSG_DMA_INIT != msg.type ) )
        {
            QC_ERROR( "Recv INIT failed: %d, type: %d", rc, msg.type );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "Recv INIT DONE" );
        }

        if ( QC_STATUS_OK == ret )
        {
            for ( uint32_t i; i < m_poolSize; i++ )
            {
                QCBufferDescriptorBase_t &buffer = m_buffers[i];
                DmaInfo &dmaInfo = m_dmaInfos[i];
                msg.type = C2C_MSG_DMA_SETUP;
                msg.bufIdx = i;
                msg.dma.bufId = dmaInfo.dmaBufId;
                msg.dma.bufSize = buffer.size;
                rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
                if ( C2C_SUCCESS != rc )
                {
                    QC_ERROR( "Send SETUP for buffer %u failed: %d", i, rc );
                    ret = QC_STATUS_FAIL;
                    break;
                }
                else
                {
                    QC_DEBUG( "Send SETUP for buffer %u", i );
                }
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_stop = false;
            m_thread = std::thread( &SampleC2C::ThreadSubMain, this );
        }
    }
    TRACE_END( SYSTRACE_TASK_START );

    return ret;
}

QCStatus_e SampleC2C::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;
    C2C_PioMsg_t msg = { (C2C_MsgType_t) 0 };
    int rc;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    if ( true == m_bIsPub )
    {
        msg.type = C2C_MSG_DMA_STOP;
        rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
        if ( C2C_SUCCESS != rc )
        {
            QC_ERROR( "Send STOP for failed: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &dmaInfo = m_dmaInfos[i];
            rc = C2C_Disconnect_DMABuffer( dmaInfo.dmaFd );
            if ( C2C_SUCCESS != rc )
            {
                QC_ERROR( "Disconnect DMA %u failed: %d", i, rc );
                ret = QC_STATUS_FAIL;
            }
        }
    }
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleC2C::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    int rc;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    if ( true == m_bIsPub )
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &dmaInfo = m_dmaInfos[i];
            rc = C2C_Free_DMABuffer( dmaInfo.dmaFd );
            if ( C2C_SUCCESS != rc )
            {
                QC_ERROR( "Free DMA %u failed: %d", i, rc );
                ret = QC_STATUS_FAIL;
            }
        }
    }
    else
    {
        for ( uint32_t i; i < m_poolSize; i++ )
        {
            auto &dmaInfo = m_dmaInfos[i];
            rc = C2C_Release_DMABuffer( dmaInfo.dmaFd );
            if ( C2C_SUCCESS != rc )
            {
                QC_ERROR( "Free DMA %u failed: %d", i, rc );
                ret = QC_STATUS_FAIL;
            }
        }
    }
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}


void SampleC2C::ThreadPubMain()
{
    QCStatus_e ret;
    C2C_PioMsg_t msg = { (C2C_MsgType_t) C2C_MSG_DMA_READY };
    int rc;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            PROFILER_BEGIN();
            TRACE_BEGIN( frames.FrameId( 0 ) );
            QCBufferDescriptorBase_t &buffer = frames.GetBuffer( 0 );
            auto it = m_dmaIndexMap.find( buffer.dmaHandle );
            if ( it != m_dmaIndexMap.end() )
            {
                DmaInfo &dmaInfo = m_dmaInfos[it->second];
                rc = C2C_ScheduleDMA( dmaInfo.dmaFd, 0, /* local offset */
                                      0,                /* remote offset */
                                      buffer.size );
                if ( C2C_SUCCESS != rc )
                {
                    QC_ERROR( "ScheduleDMA for buffer %u failed: %d", it->second, rc );
                }
                else
                {
                    msg.bufIdx = it->second;
                    msg.dma.bufId = dmaInfo.dmaBufId;
                    msg.dma.bufSize = buffer.size;
                    msg.frameId = frames.FrameId( 0 );
                    msg.timestamp = frames.Timestamp( 0 );
                    rc = C2C_MsgSend( m_c2cFd, (void *) &msg, sizeof( msg ) );
                    if ( C2C_SUCCESS != rc )
                    {
                        QC_ERROR( "Send READY for buffer %u failed: %d", it->second, rc );
                        ret = QC_STATUS_FAIL;
                    }
                }
                PROFILER_END();
                TRACE_END( frames.FrameId( 0 ) );
            }
            else
            {
                QC_ERROR( "Failed to publish for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
            }
        }
    }
}

void SampleC2C::ThreadSubMain()
{
    QCStatus_e ret;
    int rc;
    while ( false == m_stop )
    {
        C2C_PioMsg_t msg = { (C2C_MsgType_t) 0 };
        rc = WaitC2cMsg( (void *) &msg, sizeof( msg ) );
        if ( ( C2C_SUCCESS == rc ) && ( C2C_MSG_DMA_STOP == msg.type ) )
        {
            QC_DEBUG( "Recv STOP" );
            break;
        }
        if ( ( C2C_SUCCESS != rc ) || ( C2C_MSG_DMA_READY != msg.type ) ||
             ( msg.bufIdx >= m_buffers.size() ) )
        {
            QC_ERROR( "Recv READY failed: %d", rc );
        }
        else
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", msg.frameId,
                      msg.timestamp );
            PROFILER_BEGIN();
            TRACE_BEGIN( msg.frameId );
            QCBufferDescriptorBase_t &bufferDesc = m_buffers[msg.bufIdx];
            SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;
            pSharedBuffer->imgDesc = bufferDesc;
            pSharedBuffer->buffer = pSharedBuffer->imgDesc;
            pSharedBuffer->pubHandle = msg.bufIdx;
            std::shared_ptr<SharedBuffer_t> buffer(
                    pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) { delete pSharedBuffer; } );
            DataFrames_t frames;
            DataFrame_t frame;
            frame.frameId = msg.frameId;
            frame.buffer = buffer;
            frame.timestamp = msg.timestamp;
            frames.Add( frame );
            ret = m_pub.Publish( frames );
            if ( QC_STATUS_OK == ret )
            {
                PROFILER_END();
                TRACE_END( msg.frameId );
            }
            else
            {
                QC_ERROR( "Failed to publish for %" PRIu64 " : %d", msg.frameId, ret );
            }
        }
    }
}

REGISTER_SAMPLE( C2C, SampleC2C );

}   // namespace sample
}   // namespace QC
