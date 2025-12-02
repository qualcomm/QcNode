// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear
#include "QC/sample/SampleGenie.hpp"
#include <fstream>

namespace QC
{
namespace sample
{

static void GenieLog_Callback( const GenieLog_Handle_t handle, const char *fmt,
                               GenieLog_Level_t level, uint64_t timestamp, va_list args )
{
#ifndef DISABLE_QC_LOG
    switch ( level )
    {
        case GENIE_LOG_LEVEL_VERBOSE:
            Logger::GetDefault().Log( LOGGER_LEVEL_VERBOSE, fmt, args );
            break;
        case GENIE_LOG_LEVEL_INFO:
            Logger::GetDefault().Log( LOGGER_LEVEL_INFO, fmt, args );
            break;
        case GENIE_LOG_LEVEL_WARN:
            Logger::GetDefault().Log( LOGGER_LEVEL_WARN, fmt, args );
            break;
        case GENIE_LOG_LEVEL_ERROR:
            Logger::GetDefault().Log( LOGGER_LEVEL_ERROR, fmt, args );
            break;
        default:
            Logger::GetDefault().Log( LOGGER_LEVEL_VERBOSE, fmt, args );
            break;
    }
#endif
}

SampleGenie::SampleGenie() {}
SampleGenie::~SampleGenie() {}

QCStatus_e SampleGenie::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

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

    m_genieConfig = Get( config, "config", "" );
    if ( "" == m_genieConfig )
    {
        QC_ERROR( "the json genie config file is not specified\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_embeddingTable = Get( config, "embedding_table", "" );

    m_lutDataType = Get( config, "lut_data_type", QC_TENSOR_TYPE_FLOAT_32 );
    m_lutScale = Get( config, "lut_scale", 1.0f );
    m_lutOffset = Get( config, "lut_offset", 0 );

    m_inputDataType = Get( config, "input_data_type", QC_TENSOR_TYPE_FLOAT_32 );
    m_inputScale = Get( config, "input_scale", 1.0f );
    m_inputOffset = Get( config, "input_offset", 0 );

    return ret;
}


GenieLog_Level_t SampleGenie::GetGenieLogLevel()
{
    Logger_Level_e level = m_logger.GetLevel();
    GenieLog_Level_t logLevelGenie = GENIE_LOG_LEVEL_ERROR;
    switch ( level )
    {
        case LOGGER_LEVEL_VERBOSE:
            logLevelGenie = GENIE_LOG_LEVEL_VERBOSE;
            break;
        case LOGGER_LEVEL_DEBUG:
            logLevelGenie = GENIE_LOG_LEVEL_VERBOSE;
            break;
        case LOGGER_LEVEL_INFO:
            logLevelGenie = GENIE_LOG_LEVEL_INFO;
            break;
        case LOGGER_LEVEL_WARN:
            logLevelGenie = GENIE_LOG_LEVEL_WARN;
            break;
        case LOGGER_LEVEL_ERROR:
            logLevelGenie = GENIE_LOG_LEVEL_ERROR;
            break;
        default: /* warn level */
            break;
    }
    return logLevelGenie;
}

QCStatus_e SampleGenie::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;
    int32_t status;

    ret = SampleIF::Init( name, QC_NODE_TYPE_QNN );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_TRACE_INIT( [&]() {
            DataTree dt;
            dt.Set<std::string>( "name", name );
            dt.Set<std::string>( "processor", "Genie" );
            return dt.Dump();
        }() );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( "" != m_embeddingTable )
        {
            FILE *file = fopen( m_embeddingTable.c_str(), "rb" );
            if ( nullptr != file )
            {
                fseek( file, 0, SEEK_END );
                size_t fileSize = (size_t) ftell( file );
                fseek( file, 0, SEEK_SET );

                m_embeddingLut = std::shared_ptr<void>( new int8_t[fileSize] );
                m_embeddingLutSize = fileSize;

                if ( nullptr != m_embeddingLut )
                {
                    auto r = fread( m_embeddingLut.get(), 1, fileSize, file );
                    if ( fileSize != r )
                    {
                        QC_ERROR( "failed to read embedding table %s", m_embeddingTable.c_str() );
                        ret = QC_STATUS_FAIL;
                    }
                }
                else
                {
                    QC_ERROR( "Failed to allocate memory for %s", m_embeddingTable.c_str() );
                    ret = QC_STATUS_NOMEM;
                }
                fclose( file );
            }
            else
            {
                QC_ERROR( "Can't load embedding table %s", m_embeddingTable.c_str() );
                ret = QC_STATUS_FAIL;
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_t2eCallbacks[QC_TENSOR_TYPE_FLOAT_32][QC_TENSOR_TYPE_FLOAT_32] = TokenToEmbedCallback;
        m_t2eCallbacks[QC_TENSOR_TYPE_SFIXED_POINT_8][QC_TENSOR_TYPE_SFIXED_POINT_16] =
                TokenToEmbedRequantCallback<int8_t, int8_t>;
        m_t2eCallbacks[QC_TENSOR_TYPE_SFIXED_POINT_8][QC_TENSOR_TYPE_SFIXED_POINT_16] =
                TokenToEmbedRequantCallback<int8_t, int16_t>;
        m_t2eCallbacks[QC_TENSOR_TYPE_UFIXED_POINT_8][QC_TENSOR_TYPE_UFIXED_POINT_8] =
                TokenToEmbedRequantCallback<uint8_t, uint8_t>;
        m_t2eCallbacks[QC_TENSOR_TYPE_UFIXED_POINT_8][QC_TENSOR_TYPE_UFIXED_POINT_16] =
                TokenToEmbedRequantCallback<uint8_t, uint16_t>;
        m_t2eCallbacks[QC_TENSOR_TYPE_SFIXED_POINT_16][QC_TENSOR_TYPE_SFIXED_POINT_8] =
                TokenToEmbedRequantCallback<int16_t, int8_t>;
        m_t2eCallbacks[QC_TENSOR_TYPE_SFIXED_POINT_16][QC_TENSOR_TYPE_SFIXED_POINT_16] =
                TokenToEmbedRequantCallback<int16_t, int16_t>;
        m_t2eCallbacks[QC_TENSOR_TYPE_UFIXED_POINT_16][QC_TENSOR_TYPE_UFIXED_POINT_8] =
                TokenToEmbedRequantCallback<uint16_t, uint8_t>;
        m_t2eCallbacks[QC_TENSOR_TYPE_UFIXED_POINT_16][QC_TENSOR_TYPE_UFIXED_POINT_16] =
                TokenToEmbedRequantCallback<uint16_t, uint16_t>;
        if ( m_embeddingLutSize > 0 )
        {
            m_requantScale = m_lutScale / m_inputScale;
            m_requantOffset = m_requantScale * m_lutOffset - m_inputOffset;
            if ( m_t2eCallbacks.end() != m_t2eCallbacks.find( m_lutDataType ) )
            {
                if ( m_t2eCallbacks[m_lutDataType].end() !=
                     m_t2eCallbacks[m_lutDataType].find( m_inputDataType ) )
                {
                    m_tokenToEmbeddingCallback = m_t2eCallbacks[m_lutDataType][m_inputDataType];
                }
                else
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "input data type %d not supported", m_inputDataType );
                }
            }
            else
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "lut data type %d not supported", m_lutDataType );
            }
        }
    }

    QC_TRACE_BEGIN( "Init", {} );
    if ( QC_STATUS_OK == ret )
    {
        std::ifstream configStream = std::ifstream( m_genieConfig );
        std::string config;
        std::getline( configStream, config, '\0' );
        status = GenieDialogConfig_createFromJson( config.c_str(), &m_genieConfigHandle );
        if ( ( GENIE_STATUS_SUCCESS != status ) || ( nullptr == m_genieConfigHandle ) )
        {
            QC_ERROR( "Failed to create the dialog config from %s: %d.", m_genieConfig.c_str(),
                      status );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        status = GenieLog_create( nullptr, GenieLog_Callback, GetGenieLogLevel(), &m_logHandle );
        if ( ( GENIE_STATUS_SUCCESS != status ) || ( nullptr == m_logHandle ) )
        {
            QC_ERROR( "Failed to create the log handle: %d.", status );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        status = GenieDialogConfig_bindLogger( m_genieConfigHandle, m_logHandle );
        if ( GENIE_STATUS_SUCCESS != status )
        {
            QC_ERROR( "Failed to bind the log handle with the dialog config: %d.", status );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        status = GenieDialog_create( m_genieConfigHandle, &m_dialogHandle );
        if ( ( GENIE_STATUS_SUCCESS != status ) || ( nullptr == m_dialogHandle ) )
        {
            QC_ERROR( "Failed to create the dialog: %d.", status );
            ret = QC_STATUS_FAIL;
        }
    }
    QC_TRACE_END( "Init", {} );

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

QCStatus_e SampleGenie::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Start", {} );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleGenie::ThreadMain, this );
    }
    QC_TRACE_END( "Start", {} );

    return ret;
}

void SampleGenie::TokenToEmbedCallback( const int32_t token, void *embedding,
                                        const uint32_t embeddingSize, const void *pPriv )
{
    SampleGenie *pSelf = (SampleGenie *) pPriv;
    pSelf->TokenToEmbedCallback( token, embedding, embeddingSize );
}

void SampleGenie::TokenToEmbedCallback( const int32_t token, void *embedding,
                                        const uint32_t embeddingSize )
{
    QC_TRACE_EVENT( "T2E", { QCNodeTraceArg( "token", token ) } );
    const size_t lutIndex = token * embeddingSize;
    if ( ( lutIndex + embeddingSize ) <= m_embeddingLutSize )
    {
        int8_t *embeddingSrc = static_cast<int8_t *>( m_embeddingLut.get() ) + lutIndex;
        int8_t *embeddingDst = static_cast<int8_t *>( embedding );
        std::copy( embeddingSrc, embeddingSrc + embeddingSize, embeddingDst );
    }
    else
    {
        QC_ERROR( "Error: T2E conversion overflow." );
    }
}

void SampleGenie::QueryCallback( const char *responseStr,
                                 const GenieDialog_SentenceCode_t sentenceCode, const void *pPriv )
{
    SampleGenie *pSelf = (SampleGenie *) pPriv;
    pSelf->QueryCallback( responseStr, sentenceCode );
}

void SampleGenie::QueryCallback( const char *responseStr,
                                 const GenieDialog_SentenceCode_t sentenceCode )
{
    QC_TRACE_EVENT( "Query",
                    { QCNodeTraceArg( "sentence", static_cast<int32_t>( sentenceCode ) ),
                      QCNodeTraceArg( "str", std::string( responseStr ? responseStr : "" ) ) } );
    switch ( sentenceCode )
    {
        case GENIE_DIALOG_SENTENCE_COMPLETE:
            std::cout << "[COMPLETE]: " << std::flush;
            break;
        case GENIE_DIALOG_SENTENCE_BEGIN:
            std::cout << "[BEGIN]: " << std::flush;
            break;
        case GENIE_DIALOG_SENTENCE_RESUME:
            std::cout << "[RESUME]: " << std::flush;
            break;
        case GENIE_DIALOG_SENTENCE_CONTINUE:
            break;
        case GENIE_DIALOG_SENTENCE_END:
            std::cout << "[END]" << std::flush << std::endl;
            break;
        case GENIE_DIALOG_SENTENCE_ABORT:
            std::cout << "[ABORT]: " << std::flush;
            break;
        default:
            std::cout << "[UNKNOWN]: " << std::flush;
            break;
    }
    if ( responseStr )
    {
        std::cout << responseStr << std::flush;
    }
}

void SampleGenie::ThreadMain()
{
    QCStatus_e ret;

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            PROFILER_BEGIN();
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            QCBufferDescriptorBase_t &bufDesc = frames.GetBuffer( 0 );
            QC_TRACE_BEGIN( "Execute", { QCNodeTraceArg( "frameId", frames.FrameId( 0 ) ) } );
            int32_t status = GenieDialog_embeddingQuery(
                    m_dialogHandle, bufDesc.pBuf, bufDesc.size,
                    GenieDialog_SentenceCode_t::GENIE_DIALOG_SENTENCE_COMPLETE,
                    m_tokenToEmbeddingCallback, QueryCallback, this );
            if ( GENIE_STATUS_SUCCESS != status )
            {
                QC_ERROR( "Failed to embedding query: %d.", status );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                PROFILER_END();
            }
            (void) GenieDialog_reset( m_dialogHandle );
            QC_TRACE_END( "Execute", { QCNodeTraceArg( "frameId", frames.FrameId( 0 ) ) } );
        }
    }
}

QCStatus_e SampleGenie::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Stop", {} );
    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }
    QC_TRACE_END( "Stop", {} );

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleGenie::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Deinit", {} );
    if ( nullptr != m_dialogHandle )
    {
        int32_t status = GenieDialog_free( m_dialogHandle );
        if ( GENIE_STATUS_SUCCESS != status )
        {
            QC_ERROR( "Failed to free the diaglog handle: %d.", status );
            ret = QC_STATUS_FAIL;
        }
        m_dialogHandle = nullptr;
    }

    if ( nullptr != m_logHandle )
    {
        int32_t status = GenieLog_free( m_logHandle );
        if ( GENIE_STATUS_SUCCESS != status )
        {
            QC_ERROR( "Failed to free the log handle: %d.", status );
            ret = QC_STATUS_FAIL;
        }
        m_logHandle = nullptr;
    }

    if ( nullptr != m_genieConfigHandle )
    {
        int32_t status = GenieDialogConfig_free( m_genieConfigHandle );
        if ( GENIE_STATUS_SUCCESS != status )
        {
            QC_ERROR( "Failed to free the dialog config: %d.", status );
            ret = QC_STATUS_FAIL;
        }
        m_genieConfigHandle = nullptr;
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Deinit();
    }
    QC_TRACE_END( "Deinit", {} );

    return ret;
}

REGISTER_SAMPLE( Genie, SampleGenie );

}   // namespace sample
}   // namespace QC
