// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/sample/SampleCamera.hpp"

namespace QC
{
namespace sample
{

SampleCamera::SampleCamera() {}
SampleCamera ::~SampleCamera() {}

void SampleCamera::ProcessDoneCb( const QCNodeEventInfo_t &eventInfo )
{
    QCStatus_e status = QC_STATUS_OK;
    QCFrameDescriptorNodeIfs &frameDescIfs = eventInfo.frameDesc;
    QCBufferDescriptorBase_t &bufDesc = frameDescIfs.GetBuffer( 0 );
    const CameraFrameDescriptor_t *pCamFrameDesc =
            dynamic_cast<const CameraFrameDescriptor_t *>( &bufDesc );

    if ( nullptr == pCamFrameDesc )
    {
        QC_ERROR( "Frame pointer is empty" );
    }
    else
    {
        if ( ( m_stop == false ) )
        {
            std::unique_lock<std::mutex> lck( m_mutex );
            m_camFrameQueue.push( *pCamFrameDesc );
            m_condVar.notify_one();
        }
        else
        {
            QCSharedFrameDescriptorNode frameDesc( 1 );
            (void) frameDesc.SetBuffer( 0, bufDesc );
            status = m_camera.ProcessFrameDescriptor( frameDesc );
            if ( QC_STATUS_OK != status )
            {
                QC_ERROR( "Failed to process frame descriptor, status=%u", status );
            }
        }
    }
}

QCStatus_e SampleCamera::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    uint32_t numStream = Get( config, "number", 1 );
    m_config.Set<std::string>( "name", m_name );
    m_config.Set<uint32_t>( "id", 0 );
    m_config.Set<uint32_t>( "inputId", Get( config, "input_id", 0 ) );
    m_config.Set<uint32_t>( "srcId", Get( config, "src_id", 0 ) );
    m_config.Set<uint32_t>( "clientId", Get( config, "client_id", 0 ) );
    m_config.Set<uint32_t>( "inputMode", Get( config, "input_mode", 0 ) );
    m_config.Set<uint32_t>( "ispUseCase", Get( config, "isp_use_case", 0 ) );
    m_config.Set<uint32_t>( "camFrameDropPattern", Get( config, "frame_drop_pattern", 0 ) );
    m_config.Set<uint32_t>( "camFrameDropPeriod", Get( config, "frame_drop_period", 0 ) );
    m_config.Set<uint32_t>( "opMode",
                            Get( config, "op_mode", (uint32_t) QCARCAM_OPMODE_OFFLINE_ISP ) );

    uint32_t bufferIdx = 0;
    for ( uint32_t i = 0; i < numStream; i++ )
    {
        DataTree streamConfig;
        std::string suffix = "";

        if ( i > 0 )
        {
            suffix = std::to_string( i );
        }

        uint32_t streamId = Get( config, "stream_id" + suffix, i );

        uint32_t bufCnt = Get( config, "pool_size" + suffix, 4 );
        if ( 0 == bufCnt )
        {
            QC_ERROR( "invalid pool_size for stream %u", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        uint32_t width = Get( config, "width" + suffix, 0 );
        if ( 0 == width )
        {
            QC_ERROR( "invalid width for stream %u", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        uint32_t height = Get( config, "height" + suffix, 0 );
        if ( 0 == height )
        {
            QC_ERROR( "invalid height for stream %u", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        QCImageFormat_e format = Get( config, "format" + suffix, QC_IMAGE_FORMAT_NV12 );
        if ( QC_IMAGE_FORMAT_MAX == format )
        {
            QC_ERROR( "invalid format for stream %u", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        uint32_t submitRequestPattern = Get( config, "submit_request_pattern" + suffix, 0 );
        if ( submitRequestPattern > 10 )
        {
            QC_ERROR( "invalid submitRequestPattern for stream %u", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        if ( QC_STATUS_OK == ret )
        {
            streamConfig.Set<uint32_t>( "streamId", streamId );
            streamConfig.Set<uint32_t>( "bufCnt", bufCnt );
            streamConfig.Set<uint32_t>( "width", width );
            streamConfig.Set<uint32_t>( "height", height );
            streamConfig.SetImageFormat( "format", format );
            streamConfig.Set<uint32_t>( "submitRequestPattern", submitRequestPattern );
            m_streamConfigs.push_back( streamConfig );
        }

        std::string topicName = Get( config, "topic" + suffix, "" );
        if ( "" == topicName )
        {
            QC_ERROR( "no topic for stream %u", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            m_topicNameMap[streamId] = topicName;
        }

        m_frameId[streamId] = 0;
    }
    m_config.Set( "streamConfigs", m_streamConfigs );

    m_config.Set<bool>( "requestMode", Get( config, "request_mode", false ) );
    m_config.Set<bool>( "primary", Get( config, "is_primary", false ) );
    m_config.Set<bool>( "recovery", Get( config, "recovery", false ) );

    m_dataTree.Set( "static", m_config );

    m_bIgnoreError = Get( config, "ignore_error", false );
    m_bImmediateRelease = Get( config, "immediate_release", false );

    return ret;
}

QCStatus_e SampleCamera::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = SampleIF::Init( name );

    std::vector<DataTree> streamConfigs;
    QCImageProps_t imgProp;
    uint32_t streamId = 0;
    uint32_t bufCnt = 0;
    uint32_t bufferId = 0;
    uint32_t numStream = 0;

    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        using std::placeholders::_1;
        m_nodeCfg.config = m_dataTree.Dump();
        m_nodeCfg.callback = std::bind( &SampleCamera::ProcessDoneCb, this, _1 );
        QC_INFO( "config: %s", m_nodeCfg.config );
    }

    if ( QC_STATUS_OK == ret )
    {
        numStream = m_streamConfigs.size();
        m_frameBufferPools.resize( numStream );
        for ( uint32_t i = 0; i < numStream; i++ )
        {
            streamId = m_streamConfigs[i].Get<uint32_t>( "streamId", UINT32_MAX );
            std::string bufPoolName = m_name + std::to_string( i );
            bufCnt = m_streamConfigs[i].Get<uint32_t>( "bufCnt", UINT32_MAX );
            imgProp.format = m_streamConfigs[i].GetImageFormat( "format", QC_IMAGE_FORMAT_MAX );
            imgProp.width = m_streamConfigs[i].Get<uint32_t>( "width", UINT32_MAX );
            imgProp.height = m_streamConfigs[i].Get<uint32_t>( "height", UINT32_MAX );

            if ( ( QC_IMAGE_FORMAT_RGB888 == imgProp.format ) ||
                 ( QC_IMAGE_FORMAT_BGR888 == imgProp.format ) )
            {
                imgProp.batchSize = 1;
                imgProp.stride[0] = QC_ALIGN_SIZE( imgProp.width * 3, 16 );
                imgProp.actualHeight[0] = imgProp.height;
                imgProp.numPlanes = 1;
                imgProp.planeBufSize[0] = 0;

                ret = m_frameBufferPools[i].Init( bufPoolName, m_nodeId, LOGGER_LEVEL_ERROR, bufCnt,
                                                  imgProp, QC_MEMORY_ALLOCATOR_DMA_CAMERA,
                                                  QC_CACHEABLE );
            }
            else
            {
                ret = m_frameBufferPools[i].Init( bufPoolName, m_nodeId, LOGGER_LEVEL_ERROR, bufCnt,
                                                  imgProp.width, imgProp.height, imgProp.format,
                                                  QC_MEMORY_ALLOCATOR_DMA_CAMERA, QC_CACHEABLE );
            }
            if ( QC_STATUS_OK == ret )
            {
                ret = m_frameBufferPools[i].GetBuffers( m_nodeCfg.buffers );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_camera.Initialize( m_nodeCfg );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to Init camera node" );
        if ( m_bIgnoreError )
        {
            ret = QC_STATUS_OK;
            QC_ERROR( "Ignore Initialization Error" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < numStream; i++ )
        {
            std::string topicName = m_topicNameMap[streamId];
            m_pubMap[streamId] = std::make_shared<DataPublisher<DataFrames_t>>();
            ret = m_pubMap[streamId]->Init( name + "." + std::to_string( streamId ), topicName );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Create topic %s for stream %u failed: %d", topicName.c_str(), i, ret );
                break;
            }
        }
    }

    return ret;
}

QCStatus_e SampleCamera::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_camera.Start();
    TRACE_END( SYSTRACE_TASK_START );

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to start camera node" );
        if ( m_bIgnoreError )
        {
            QC_ERROR( "Ignore Starting Error" );
            ret = QC_STATUS_OK;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleCamera::ThreadMain, this );
    }

    return ret;
}

void SampleCamera::ProcessFrame( CameraFrameDescriptor_t *pCamFrameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;

    DataFrames_t frames;
    DataFrame_t frame;

    SharedBuffer_t *pSharedBuffer = new SharedBuffer_t;
    pSharedBuffer->SetBuffer( *pCamFrameDesc );
    pSharedBuffer->pubHandle =
            (uint64_t) pCamFrameDesc->frameIdx + ( (uint64_t) pCamFrameDesc->streamId << 32 );

    std::shared_ptr<SharedBuffer_t> buffer( pSharedBuffer, [&]( SharedBuffer_t *pSharedBuffer ) {
        CameraFrameDescriptor_t camFrameDesc;
        QCSharedFrameDescriptorNode frameDesc( 1 );

        uint32_t frameIdx = pSharedBuffer->pubHandle & 0xFFFFFFFFul;
        camFrameDesc = pSharedBuffer->imgDesc;
        camFrameDesc.frameIdx = frameIdx;
        camFrameDesc.streamId = pSharedBuffer->pubHandle >> 32;
        (void) frameDesc.SetBuffer( 0, camFrameDesc );

        if ( true == m_bImmediateRelease )
        {
            /* do nothing as immediate release in the callback */
        }
        else if ( ( 0 != m_config.Get<uint32_t>( "clientId", UINT32_MAX ) ) &&
                  ( false == m_config.Get<bool>( "primary", true ) ) )
        {
            /* do nothing for multi-client non-primary session */
        }
        else
        {
            m_camera.ProcessFrameDescriptor( frameDesc );
        }

        delete pSharedBuffer;
    } );

    frame.frameId = m_frameId[pCamFrameDesc->streamId]++;
    frame.buffer = buffer;
    frame.timestamp = pCamFrameDesc->timestamp;
    frames.Add( frame );

    uint32_t streamId = m_streamConfigs[0].Get<uint32_t>( "streamId", UINT32_MAX );
    if ( streamId == pCamFrameDesc->streamId )
    {
        /* trace and profiling the first stream */
        PROFILER_BEGIN();
        PROFILER_END();
        TRACE_EVENT( frame.frameId );
    }
    else
    {
        /* profiling the other streams */
        TRACE_CAMERA_EVENT( pCamFrameDesc->streamId, frame.frameId );
    }

    auto it = m_pubMap.find( pCamFrameDesc->streamId );
    if ( m_pubMap.end() != it )
    {
        it->second->Publish( frames );
    }
    else
    {
        QC_ERROR( "no publisher for stream %u", pCamFrameDesc->streamId );
    }

    if ( true == m_bImmediateRelease )
    {
        if ( ( 0 != m_config.Get<uint32_t>( "clientId", UINT32_MAX ) ) &&
             ( false == m_config.Get<bool>( "primary", true ) ) )
        {
            /* do nothing for multi-client non-primary session */
        }
        else
        {
            CameraFrameDescriptor_t camFrameDesc;
            QCSharedFrameDescriptorNode frameDesc( 1 );

            uint32_t framdIdx = pSharedBuffer->pubHandle & 0xFFFFFFFFul;
            camFrameDesc = pSharedBuffer->imgDesc;
            camFrameDesc.frameIdx = framdIdx;
            camFrameDesc.streamId = pSharedBuffer->pubHandle >> 32;
            (void) frameDesc.SetBuffer( 0, camFrameDesc );
            m_camera.ProcessFrameDescriptor( frameDesc );
        }
    }
}

void SampleCamera::ThreadMain()
{
    QCStatus_e ret = QC_STATUS_OK;
    CameraFrameDescriptor_t camFrameDesc;

    while ( !m_stop )
    {
        std::unique_lock<std::mutex> lck( m_mutex );
        (void) m_condVar.wait_for( lck, std::chrono::milliseconds( 10 ) );

        if ( !m_camFrameQueue.empty() )
        {
            camFrameDesc = m_camFrameQueue.front();
            m_camFrameQueue.pop();
            ProcessFrame( &camFrameDesc );
        }
    }
}

QCStatus_e SampleCamera::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    CameraFrameDescriptor_t camFrameDesc;
    while ( !m_camFrameQueue.empty() )
    {
        std::unique_lock<std::mutex> lck( m_mutex );
        camFrameDesc = m_camFrameQueue.front();
        m_camFrameQueue.pop();
        ProcessFrame( &camFrameDesc );
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    QC_DEBUG( "Stopping camera begin" );
    ret = m_camera.Stop();
    QC_DEBUG( "Stopping camera end" );
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleCamera::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_camera.DeInitialize();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( Camera, SampleCamera );

}   // namespace sample
}   // namespace QC

