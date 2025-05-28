// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SampleCL2DFlex.hpp"

namespace QC
{
namespace sample
{

CL2DFlex_Work_Mode_e SampleCL2DFlex::GetMode( SampleConfig_t &config, std::string key,
                                              CL2DFlex_Work_Mode_e defaultV )
{
    CL2DFlex_Work_Mode_e ret = defaultV;

    auto it = config.find( key );
    if ( it != config.end() )
    {
        std::string mode = it->second;
        if ( "convert" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_CONVERT;
        }
        else if ( "resize_nearest" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_RESIZE_NEAREST;
        }
        else if ( "resize_bilinear" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_RESIZE_BILINEAR;
        }
        else if ( "letterbox_nearest" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST;
        }
        else if ( "letterbox_nearest_multiple" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE;
        }
        else if ( "resize_nearest_multiple" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE;
        }
        else if ( "convert_ubwc" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_CONVERT_UBWC;
        }
        else if ( "remap_nearest" == mode )
        {
            ret = CL2DFLEX_WORK_MODE_REMAP_NEAREST;
        }
        else
        {
            ret = CL2DFLEX_WORK_MODE_MAX;
        }
    }

    QC_DEBUG( "Get config %s = %d\n", key.c_str(), ret );

    return ret;
}

SampleCL2DFlex::SampleCL2DFlex() {}
SampleCL2DFlex::~SampleCL2DFlex() {}

QCStatus_e SampleCL2DFlex::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    CL2DFlex_Work_Mode_e workModes[QC_MAX_INPUTS];
    QCImageFormat_e inputFormats[QC_MAX_INPUTS];
    uint32_t inputWidths[QC_MAX_INPUTS];
    uint32_t inputHeights[QC_MAX_INPUTS];
    CL2DFlex_ROIConfig_t ROIs[QC_MAX_INPUTS];

    m_dataTree.Set<std::string>( "static.name", m_name );
    m_dataTree.Set<uint32_t>( "static.id", 0 );

    m_bNoPadding = Get( config, "no_padding", false );

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

        workModes[i] = GetMode( config, "work_mode" + std::to_string( i ),
                                CL2DFLEX_WORK_MODE_RESIZE_NEAREST );
        if ( CL2DFLEX_WORK_MODE_MAX == workModes[i] )
        {
            QC_ERROR( "invalid work_mode%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        std::string workMode = Get( config, "work_mode" + std::to_string( i ), "resize_nearest" );
        inputDt.Set<std::string>( "workMode", workMode );

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

        ROIs[i].width = Get( config, "roi_width" + std::to_string( i ), inputWidths[i] );
        if ( 0 == ROIs[i].width )
        {
            QC_ERROR( "invalid roi_width%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "roiWidth", ROIs[i].width );

        ROIs[i].height = Get( config, "roi_height" + std::to_string( i ), inputHeights[i] );
        if ( 0 == ROIs[i].height )
        {
            QC_ERROR( "invalid roi_height%u\n", i );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        inputDt.Set<uint32_t>( "roiHeight", ROIs[i].height );

        if ( true == m_bEnableUndistortion )
        {
            bool bAllocateOK = true;
            QCTensorProps_t mapXProp;
            mapXProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { m_outputHeight * m_outputWidth, 0 },
                    1,
            };
            ret = m_mapXBufferDesc[i].buffer.Allocate( &mapXProp );
            if ( QC_STATUS_OK != ret )
            {
                bAllocateOK = false;
                QC_ERROR( "failed to allocate mapX%u!\n", i );
            }
            QCTensorProps_t mapYProp;
            mapYProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { m_outputHeight * m_outputWidth, 0 },
                    1,
            };
            ret = m_mapYBufferDesc[i].buffer.Allocate( &mapYProp );
            if ( QC_STATUS_OK != ret )
            {
                bAllocateOK = false;
                QC_ERROR( "failed to allocate mapY%u!\n", i );
            }

            if ( true == bAllocateOK )
            {
                bool bReadOK = true;
                std::string mapXPath = Get( config, "mapX_path" + std::to_string( i ),
                                            "./data/test/CL2DFlex/mapX.raw" );
                std::string mapYPath = Get( config, "mapY_path" + std::to_string( i ),
                                            "./data/test/CL2DFlex/mapY.raw" );
                ret = LoadFile( m_mapXBufferDesc[i].buffer, mapXPath );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "failed to read mapX table for input %u!\n", i );
                    bReadOK = false;
                }
                ret = LoadFile( m_mapYBufferDesc[i].buffer, mapYPath );
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
                    inputDt.Set<uint32_t>( "mapX", i * 2 );
                    inputDt.Set<uint32_t>( "mapY", i * 2 + 1 );
                }
            }
        }
        inputDts.push_back( inputDt );
    }
    m_dataTree.Set( "static.inputs", inputDts );

    if ( ( 1 == m_numOfInputs ) &&
         ( ( CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE == workModes[0] ) ||
           ( CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE == workModes[0] ) ) )
    {
        m_bExecuteWithROIs = true;
        m_roiNumber = Get( config, "roi_number", 1 );
        if ( 0 == m_roiNumber )
        {
            QC_ERROR( "invalid roi_number\n" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        for ( uint32_t i = 0; i < m_roiNumber; i++ )
        {
            m_ROIs[i].x = Get( config, "roi_x" + std::to_string( i ), 0 );
            if ( m_ROIs[i].x >= inputWidths[0] )
            {
                QC_ERROR( "invalid roi_x%u\n", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_ROIs[i].y = Get( config, "roi_y" + std::to_string( i ), 0 );
            if ( m_ROIs[i].y >= inputHeights[0] )
            {
                QC_ERROR( "invalid roi_y%u\n", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_ROIs[i].width = Get( config, "roi_width" + std::to_string( i ), inputWidths[0] );
            if ( 0 == m_ROIs[i].width )
            {
                QC_ERROR( "invalid roi_width%u\n", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            m_ROIs[i].height = Get( config, "roi_height" + std::to_string( i ), inputHeights[0] );
            if ( 0 == m_ROIs[i].height )
            {
                QC_ERROR( "invalid roi_height%u\n", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
        }
    }

    if ( true == m_bExecuteWithROIs )
    {
        std::vector<DataTree> roiDts;
        for ( int i = 0; i < m_roiNumber; i++ )
        {
            DataTree roiDt;
            roiDt.Set<uint32_t>( "roiX", m_ROIs[i].x );
            roiDt.Set<uint32_t>( "roiY", m_ROIs[i].y );
            roiDt.Set<uint32_t>( "roiWidth", m_ROIs[i].width );
            roiDt.Set<uint32_t>( "roiHeight", m_ROIs[i].height );
            roiDts.push_back( roiDt );
        }
        m_dataTree.Set( "static.ROIs", roiDts );
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
        m_bufferFlags = 0;
    }
    else
    {
        m_bufferFlags = QC_BUFFER_FLAGS_CACHE_WB_WA;
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

QCStatus_e SampleCL2DFlex::Init( std::string name, SampleConfig_t &config )
{
    QCNodeInit_t nodeCfg;
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        TRACE_ON( GPU );
        ret = ParseConfig( config );
    }
    if ( QC_STATUS_OK == ret )
    {
        QCImageProps_t imgProp;
        if ( m_bExecuteWithROIs == true )
        {
            imgProp.batchSize = m_roiNumber;
        }
        else
        {
            imgProp.batchSize = m_numOfInputs;
        }
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
            ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                    QC_BUFFER_USAGE_GPU, m_bufferFlags );
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
                    ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                            QC_BUFFER_USAGE_GPU, m_bufferFlags );
                }
                else if ( QC_IMAGE_FORMAT_UYVY == m_outputFormat )
                {
                    imgProp.numPlanes = 1;
                    imgProp.format = m_outputFormat;
                    imgProp.stride[0] = m_outputWidth * 2;
                    imgProp.actualHeight[0] = m_outputHeight;
                    imgProp.planeBufSize[0] = 0;
                    ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp,
                                            QC_BUFFER_USAGE_GPU, m_bufferFlags );
                }
                else
                {
                    QC_ERROR( "no padding for format %d is not supported", m_outputFormat );
                    ret = QC_STATUS_FAIL;
                }
            }
            else
            {
                ret = m_imagePool.Init( name, LOGGER_LEVEL_INFO, m_poolSize, imgProp.batchSize,
                                        m_outputWidth, m_outputHeight, m_outputFormat,
                                        QC_BUFFER_USAGE_GPU, m_bufferFlags );
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
                if ( nullptr != m_mapXBufferDesc[i].buffer.data() )
                {
                    nodeCfg.buffers.push_back( m_mapXBufferDesc[i] );
                }
                else
                {
                    QC_INFO( "m_mapXBufferDesc[%d] is nullptr\n", i );
                }
                if ( nullptr != m_mapYBufferDesc[i].buffer.data() )
                {
                    nodeCfg.buffers.push_back( m_mapYBufferDesc[i] );
                }
                else
                {
                    QC_INFO( "m_mapYBufferDesc[%d] is nullptr\n", i );
                }
            }
        }
        ret = m_cl2d.Initialize( nodeCfg );
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

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < m_numOfInputs; i++ )
        {
            if ( true == m_bEnableUndistortion )
            {
                (void) m_mapXBufferDesc[i].buffer.Free();
                (void) m_mapYBufferDesc[i].buffer.Free();
            }
        }
    }

    return ret;
}

QCStatus_e SampleCL2DFlex::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_cl2d.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleCL2DFlex::ThreadMain, this );
    }

    return ret;
}

void SampleCL2DFlex::ThreadMain()
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
                std::vector<QCSharedBuffer_t> inputs;
                for ( auto &frame : frames.frames )
                {
                    inputs.push_back( frame.buffer->sharedBuffer );
                }

                PROFILER_BEGIN();
                TRACE_BEGIN( frames.FrameId( 0 ) );

                std::vector<QCSharedBufferDescriptor_t> bufferDescs;
                bufferDescs.resize( inputs.size() + 1 );
                frameDesc.Clear();
                uint32_t globalIdx = 0;

                if ( QC_STATUS_OK == ret )
                {
                    for ( auto &buffer : inputs )
                    {
                        bufferDescs[globalIdx].buffer = buffer;
                        ret = frameDesc.SetBuffer( globalIdx, bufferDescs[globalIdx] );
                        if ( QC_STATUS_OK != ret )
                        {
                            break;
                        }
                        globalIdx++;
                    }
                }

                if ( QC_STATUS_OK == ret )
                {
                    bufferDescs[globalIdx].buffer = bufferOutput->sharedBuffer;
                    ret = frameDesc.SetBuffer( globalIdx, bufferDescs[globalIdx] );
                    if ( QC_STATUS_OK != ret )
                    {
                        break;
                    }
                    globalIdx++;
                }

                if ( QC_STATUS_OK == ret )
                {
                    ret = m_cl2d.ProcessFrameDescriptor( frameDesc );
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
                    QC_ERROR( "CL2D execute failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                              ret );
                }
            }
        }
    }
}

QCStatus_e SampleCL2DFlex::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_cl2d.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleCL2DFlex::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_cl2d.DeInitialize();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( CL2DFlex, SampleCL2DFlex );

}   // namespace sample
}   // namespace QC
