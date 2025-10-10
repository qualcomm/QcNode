// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/sample/SampleRemap.hpp"

namespace QC
{
namespace sample
{

SampleRemap::SampleRemap() {}
SampleRemap::~SampleRemap() {}

QCStatus_e SampleRemap::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    QCImageFormat_e inputFormats[QC_MAX_INPUTS];
    uint32_t inputWidths[QC_MAX_INPUTS];
    uint32_t inputHeights[QC_MAX_INPUTS];
    uint32_t mapWidths[QC_MAX_INPUTS];
    uint32_t mapHeights[QC_MAX_INPUTS];
    FadasROI_t ROIs[QC_MAX_INPUTS];
    std::vector<uint32_t> bufferIds;

    m_dataTree.Set<std::string>( "static.name", m_name );
    m_dataTree.Set<uint32_t>( "static.id", 0 );

    m_bNoPadding = Get( config, "no_padding", false );

    QCProcessorType_e processor = Get( config, "processor", QC_PROCESSOR_HTP0 );
    if ( QC_PROCESSOR_MAX == processor )
    {
        QC_ERROR( "invalid processor %s\n", Get( config, "processor", "" ).c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_dataTree.SetProcessorType( "static.processorType", processor );

    m_outputWidth = Get( config, "output_width", 1920 );
    if ( 0 == m_outputWidth )
    {
        QC_ERROR( "invalid output_width\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_dataTree.Set<uint32_t>( "static.outputWidth", m_outputWidth );

    m_outputHeight = Get( config, "output_height", 1024 );
    if ( 0 == m_outputHeight )
    {
        QC_ERROR( "invalid output_height\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_dataTree.Set<uint32_t>( "static.outputHeight", m_outputHeight );

    m_outputFormat = Get( config, "output_format", QC_IMAGE_FORMAT_RGB888 );
    if ( QC_IMAGE_FORMAT_MAX == m_outputFormat )
    {
        QC_ERROR( "invalid output_format\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_dataTree.SetImageFormat( "static.outputFormat", m_outputFormat );

    m_numOfInputs = Get( config, "batch_size", 1 );
    if ( 0 == m_numOfInputs )
    {
        QC_ERROR( "invalid batch_size\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_bEnableUndistortion = Get( config, "map_table", false );
    m_dataTree.Set<bool>( "static.bEnableUndistortion", m_bEnableUndistortion );

    bool bEnableNormalize = Get( config, "normalize", true );
    m_dataTree.Set<bool>( "static.bEnableNormalize", bEnableNormalize );
    if ( true == bEnableNormalize )
    {
        float quantScale = Get( config, "quant_scale", 0.0186584480106831f );
        int32_t quantOffset = Get( config, "quant_offset", 114 );
        float Rsub = Get( config, "Rsub", 123.675f );
        float Rmul = Get( config, "Rmul", 0.0171f );
        float Radd = Get( config, "Radd", 0.0f );
        float Gsub = Get( config, "Gsub", 116.28f );
        float Gmul = Get( config, "Gmul", 0.0175f );
        float Gadd = Get( config, "Gadd", 0.0f );
        float Bsub = Get( config, "Bsub", 103.53f );
        float Bmul = Get( config, "Bmul", 0.0174f );
        float Badd = Get( config, "Badd", 0.0f );
        Radd = Radd / quantScale + quantOffset;
        Rmul = Rmul / quantScale;
        Gadd = Gadd / quantScale + quantOffset;
        Gmul = Gmul / quantScale;
        Badd = Badd / quantScale + quantOffset;
        Bmul = Bmul / quantScale;

        m_dataTree.Set<float>( "static.RSub", Rsub );
        m_dataTree.Set<float>( "static.RMul", Rmul );
        m_dataTree.Set<float>( "static.RAdd", Radd );
        m_dataTree.Set<float>( "static.GSub", Gsub );
        m_dataTree.Set<float>( "static.GMul", Gmul );
        m_dataTree.Set<float>( "static.GAdd", Gadd );
        m_dataTree.Set<float>( "static.BSub", Bsub );
        m_dataTree.Set<float>( "static.BMul", Bmul );
        m_dataTree.Set<float>( "static.BAdd", Badd );
    }

    std::vector<DataTree> inputDts;
    for ( uint32_t i = 0; i < m_numOfInputs; i++ )
    {
        DataTree inputDt;

        inputWidths[i] = Get( config, "input_width" + std::to_string( i ), 1920 );
        if ( 0 == inputWidths[i] )
        {
            QC_ERROR( "invalid input_width%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "inputWidth", inputWidths[i] );

        inputHeights[i] = Get( config, "input_height" + std::to_string( i ), 1024 );
        if ( 0 == inputHeights[i] )
        {
            QC_ERROR( "invalid input_height%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "inputHeight", inputHeights[i] );

        inputFormats[i] = Get( config, "input_format" + std::to_string( i ), QC_IMAGE_FORMAT_NV12 );
        if ( QC_IMAGE_FORMAT_MAX == inputFormats[i] )
        {
            QC_ERROR( "invalid input_format%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.SetImageFormat( "inputFormat", inputFormats[i] );

        mapWidths[i] = Get( config, "map_width" + std::to_string( i ), m_outputWidth );
        if ( 0 == inputWidths[i] )
        {
            QC_ERROR( "invalid map_width%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "mapWidth", mapWidths[i] );

        mapHeights[i] = Get( config, "map_height" + std::to_string( i ), m_outputHeight );
        if ( 0 == inputHeights[i] )
        {
            QC_ERROR( "invalid map_height%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "mapHeight", mapHeights[i] );

        ROIs[i].x = Get( config, "roi_x" + std::to_string( i ), 0 );
        if ( ROIs[i].x >= inputWidths[i] )
        {
            QC_ERROR( "invalid roi_x%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "roiX", ROIs[i].x );

        ROIs[i].y = Get( config, "roi_y" + std::to_string( i ), 0 );
        if ( ROIs[i].y >= inputHeights[i] )
        {
            QC_ERROR( "invalid roi_y%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "roiY", ROIs[i].y );

        ROIs[i].width = Get( config, "roi_width" + std::to_string( i ), m_outputWidth );
        if ( 0 == ROIs[i].width )
        {
            QC_ERROR( "invalid roi_width%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "roiWidth", ROIs[i].width );

        ROIs[i].height = Get( config, "roi_height" + std::to_string( i ), m_outputHeight );
        if ( 0 == ROIs[i].height )
        {
            QC_ERROR( "invalid roi_height%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "roiHeight", ROIs[i].height );

        if ( true == m_bEnableUndistortion )
        {
            bool bAllocateOK = true;
            TensorProps_t mapXProp;
            mapXProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { mapWidths[i], mapHeights[i] },
            };
            ret = m_pBufMgr->Allocate( mapXProp, m_mapXBufferDesc[i] );
            if ( QC_STATUS_OK != ret )
            {
                bAllocateOK = false;
                QC_ERROR( "failed to allocate mapX%u!\n", i );
            }
            TensorProps_t mapYProp;
            mapYProp = { QC_TENSOR_TYPE_FLOAT_32, { mapWidths[i], mapHeights[i] } };
            ret = m_pBufMgr->Allocate( mapYProp, m_mapYBufferDesc[i] );
            if ( QC_STATUS_OK != ret )
            {
                bAllocateOK = false;
                QC_ERROR( "failed to allocate mapY%u!\n", i );
            }

            if ( true == bAllocateOK )
            {
                bool bReadOK = true;
                std::string mapXPath = Get( config, "mapX_path" + std::to_string( i ),
                                            "./data/test/remap/mapX.raw" );
                std::string mapYPath = Get( config, "mapY_path" + std::to_string( i ),
                                            "./data/test/remap/mapY.raw" );
                ret = LoadFile( dynamic_cast<QCBufferDescriptorBase_t &>( m_mapXBufferDesc[i] ),
                                mapXPath );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "failed to read mapX table for input %u!\n", i );
                    bReadOK = false;
                }
                ret = LoadFile( dynamic_cast<QCBufferDescriptorBase_t &>( m_mapYBufferDesc[i] ),
                                mapYPath );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "failed to read mapY table for input %u!\n", i );
                    bReadOK = false;
                }
                if ( false == bReadOK )
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
                else
                {
                    inputDt.Set<uint32_t>( "mapXBufferId", i * 2 );
                    bufferIds.push_back( i * 2 );
                    inputDt.Set<uint32_t>( "mapYBufferId", i * 2 + 1 );
                    bufferIds.push_back( i * 2 + 1 );
                }
            }
        }
        inputDts.push_back( inputDt );
    }
    m_dataTree.Set( "static.inputs", inputDts );

    if ( 0 != bufferIds.size() )
    {
        m_dataTree.Set<uint32_t>( "static.bufferIds", bufferIds );
    }
    QCNodeInit_t data = { m_dataTree.Dump() };
    QC_INFO( "config data: %s\n", data.config.c_str() );

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    bool bCache = Get( config, "cache", true );
    if ( false == bCache )
    {
        m_bufferCache = QC_CACHEABLE_NON;
    }
    else
    {
        m_bufferCache = QC_CACHEABLE;
    }

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

    return ret;
}

QCStatus_e SampleRemap::Init( std::string name, SampleConfig_t &config )
{
    QCNodeInit_t nodeCfg;
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        m_pBufMgr = BufferManager::Get( m_nodeId, LOGGER_LEVEL_INFO );
        if ( nullptr == m_pBufMgr )
        {
            QC_ERROR( "Failed to get buffer manager for %s %d: %s!", m_nodeId.name.c_str(),
                      m_nodeId.id, name.c_str() );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( GPU );
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCImageProps_t imgProp;
        imgProp.batchSize = m_numOfInputs;
        imgProp.width = m_outputWidth;
        imgProp.height = m_outputHeight;
        if ( ( QC_IMAGE_FORMAT_RGB888 == m_outputFormat ) ||
             ( QC_IMAGE_FORMAT_BGR888 == m_outputFormat ) )
        {
            imgProp.numPlanes = 1;
            imgProp.format = m_outputFormat;
            imgProp.stride[0] = m_outputWidth * 3;
            imgProp.actualHeight[0] = m_outputHeight;
            imgProp.planeBufSize[0] = 0;
            ret = m_imagePool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                    QC_MEMORY_ALLOCATOR_DMA_GPU, m_bufferCache );
        }
        else
        {
            if ( true == m_bNoPadding )
            {
                if ( ( QC_IMAGE_FORMAT_NV12 == m_outputFormat ) ||
                     ( QC_IMAGE_FORMAT_P010 == m_outputFormat ) )
                {
                    uint32_t bpp = 1;
                    if ( QC_IMAGE_FORMAT_P010 == m_outputFormat )
                    {
                        bpp = 2;
                    }
                    imgProp.numPlanes = 2;
                    imgProp.format = m_outputFormat;
                    imgProp.stride[0] = m_outputWidth * bpp;
                    imgProp.actualHeight[0] = m_outputHeight;
                    imgProp.planeBufSize[0] = 0;
                    imgProp.stride[1] = m_outputWidth * bpp;
                    imgProp.actualHeight[1] = m_outputHeight / 2;
                    imgProp.planeBufSize[1] = 0;
                    ret = m_imagePool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                            QC_MEMORY_ALLOCATOR_DMA_GPU, m_bufferCache );
                }
                else if ( QC_IMAGE_FORMAT_UYVY == m_outputFormat )
                {
                    imgProp.numPlanes = 1;
                    imgProp.format = m_outputFormat;
                    imgProp.stride[0] = m_outputWidth * 2;
                    imgProp.actualHeight[0] = m_outputHeight;
                    imgProp.planeBufSize[0] = 0;
                    ret = m_imagePool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                            QC_MEMORY_ALLOCATOR_DMA_GPU, m_bufferCache );
                }
                else
                {
                    QC_ERROR( "no padding for format %d is not supported", m_outputFormat );
                    ret = QC_STATUS_FAIL;
                }
            }
            else
            {
                ret = m_imagePool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize,
                                        imgProp.batchSize, m_outputWidth, m_outputHeight,
                                        m_outputFormat, QC_MEMORY_ALLOCATOR_DMA_GPU,
                                        m_bufferCache );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        nodeCfg.config = m_dataTree.Dump();
        for ( uint32_t i = 0; i < m_numOfInputs; i++ )
        {
            if ( true == m_bEnableUndistortion )
            {
                if ( nullptr != m_mapXBufferDesc[i].pBuf )
                {
                    nodeCfg.buffers.push_back( m_mapXBufferDesc[i] );
                }
                else
                {
                    QC_INFO( "m_mapXBufferDesc[%d] is nullptr\n", i );
                }
                if ( nullptr != m_mapYBufferDesc[i].pBuf )
                {
                    nodeCfg.buffers.push_back( m_mapYBufferDesc[i] );
                }
                else
                {
                    QC_INFO( "m_mapYBufferDesc[%d] is nullptr\n", i );
                }
            }
        }
        ret = m_remap.Initialize( nodeCfg );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

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

QCStatus_e SampleRemap::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_remap.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleRemap::ThreadMain, this );
    }

    return ret;
}

void SampleRemap::ThreadMain()
{
    QCStatus_e ret;
    QCSharedFrameDescriptorNode frameDesc( m_numOfInputs + 1 );
    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            std::shared_ptr<SharedBuffer_t> bufferOutput = m_imagePool.Get();
            if ( nullptr != bufferOutput )
            {
                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );
                frameDesc.Clear();
                uint32_t globalIdx = 0;

                for ( auto &frame : frames.frames )
                {
                    std::shared_ptr<SharedBuffer_t> sbuf = frame.buffer;
                    QCBufferDescriptorBase_t &buffer = frame.GetBuffer();
                    ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &buffer );
                    ret = frameDesc.SetBuffer( globalIdx, *pImage );
                    if ( QC_STATUS_OK != ret )
                    {
                        break;
                    }
                    globalIdx++;
                }

                if ( QC_STATUS_OK == ret )
                {
                    ret = frameDesc.SetBuffer( globalIdx, bufferOutput->imgDesc );
                    if ( QC_STATUS_OK != ret )
                    {
                        break;
                    }
                    globalIdx++;
                }

                if ( QC_STATUS_OK == ret )
                {
                    ret = m_remap.ProcessFrameDescriptor( frameDesc );
                }

                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_END();
                    TRACE_END( frames.FrameId( 0 ) );
                    DataFrames_t outFrames;
                    DataFrame_t frame;
                    frame.buffer = bufferOutput;
                    frame.frameId = frames.FrameId( 0 );
                    frame.timestamp = frames.Timestamp( 0 );
                    outFrames.Add( frame );
                    m_pub.Publish( outFrames );
                }
                else
                {
                    QC_ERROR( "Remap execute failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                              ret );
                }
            }
        }
    }
}

QCStatus_e SampleRemap::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_remap.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleRemap::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_remap.DeInitialize();
    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Deinit();
    }
    TRACE_END( SYSTRACE_TASK_DEINIT );

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < m_numOfInputs; i++ )
        {
            if ( true == m_bEnableUndistortion )
            {
                (void) m_pBufMgr->Free( m_mapXBufferDesc[i] );
                (void) m_pBufMgr->Free( m_mapYBufferDesc[i] );
            }
        }
    }

    BufferManager::Put( m_pBufMgr );
    m_pBufMgr = nullptr;

    return ret;
}

REGISTER_SAMPLE( Remap, SampleRemap );

}   // namespace sample
}   // namespace QC
