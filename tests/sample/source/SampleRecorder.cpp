// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "QC/sample/SampleRecorder.hpp"


namespace QC
{
namespace sample
{

SampleRecorder::SampleRecorder() {}
SampleRecorder::~SampleRecorder() {}

QCStatus_e SampleRecorder::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_maxImages = Get( config, "max", 1000 );
    if ( 0 == m_maxImages )
    {
        QC_ERROR( "invalid max = %d\n", m_maxImages );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_topicName = Get( config, "topic", "" );
    if ( "" == m_topicName )
    {
        QC_ERROR( "no topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}

QCStatus_e SampleRecorder::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_topicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        std::string path = "/tmp/" + name + ".raw";
        m_file = fopen( path.c_str(), "wb" );
        if ( nullptr == m_file )
        {
            QC_ERROR( "can't create file %s", path.c_str() );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        std::string path = "/tmp/" + name + ".meta";
        m_meta = fopen( path.c_str(), "wb" );
        if ( nullptr == m_meta )
        {
            QC_ERROR( "can't create meta file %s", path.c_str() );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e SampleRecorder::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = false;
    m_thread = std::thread( &SampleRecorder::ThreadMain, this );

    return ret;
}

void SampleRecorder::ThreadMain()
{
    QCStatus_e ret;
    uint32_t num = 0;
    while ( false == m_stop )
    {
        DataFrames_t frames;
        DataFrame_t frame;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            frame = frames.frames[0];
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frame.frameId,
                      frame.timestamp );
            if ( num < m_maxImages )
            {
                PROFILER_BEGIN();
                QCBufferDescriptorBase_t &bufDesc = frame.GetBuffer();
                ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
                if ( nullptr != pImage )
                { /* for Image, dump the first one only */
                    if ( pImage->format < QC_IMAGE_FORMAT_MAX )
                    {
                        uint32_t sizeOne = pImage->GetDataSize() / pImage->batchSize;
                        for ( uint32_t i = 0; i < pImage->batchSize; i++ )
                        {
                            std::string path = "/tmp/" + m_name + "_" + std::to_string( num ) +
                                               "_" + std::to_string( i ) + ".raw";
                            uint8_t *ptr = (uint8_t *) pImage->GetDataPtr() + sizeOne * i;
                            FILE *fp = fopen( path.c_str(), "wb" );
                            if ( nullptr != fp )
                            {
                                fwrite( ptr, sizeOne, 1, fp );
                                fclose( fp );
                            }
                            else
                            {
                                QC_ERROR( "failed to create file: %s", path.c_str() );
                            }
                        }
                        fprintf( m_meta,
                                 "%u: frameId %" PRIu64 " timestamp %" PRIu64
                                 ": batch=%u resolution=%ux%u stride=%u actual_height=%u "
                                 "format=%d\n",
                                 num, frame.frameId, frame.timestamp, pImage->batchSize,
                                 pImage->width, pImage->height, pImage->stride[0],
                                 pImage->actualHeight[0], pImage->format );
                    }
                    else
                    { /* compressed image */
                        fwrite( pImage->GetDataPtr(), pImage->GetDataSize(), 1, m_file );
                        fprintf( m_meta,
                                 "%u: frameId %" PRIu64 " timestamp %" PRIu64
                                 ": resolution=%ux%u size=%" PRIu64 " format=%d\n",
                                 num, frame.frameId, frame.timestamp, pImage->width, pImage->height,
                                 pImage->size, pImage->format );
                    }
                }
                else
                { /* for Tensor, dump all */
                    for ( size_t i = 0; i < frames.frames.size(); i++ )
                    {
                        QCBufferDescriptorBase_t &bufDesc = frames.GetBuffer( i );
                        std::string path = "/tmp/" + m_name + "_" + std::to_string( num ) + "_" +
                                           std::to_string( i ) + ".raw";
                        FILE *fp = fopen( path.c_str(), "wb" );
                        if ( nullptr != fp )
                        {
                            fwrite( bufDesc.GetDataPtr(), bufDesc.GetDataSize(), 1, fp );
                            fclose( fp );
                        }
                        else
                        {
                            QC_ERROR( "failed to create file: %s", path.c_str() );
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
                QC_INFO( "recording done!" );
                printf( "recording done!\n" );
            }
            else
            {
            }
        }
    }
}

QCStatus_e SampleRecorder::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

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
        QC_INFO( "recording done!" );
        printf( "recording done!\n" );
    }

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleRecorder::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    return ret;
}

REGISTER_SAMPLE( Recorder, SampleRecorder );

}   // namespace sample
}   // namespace QC
