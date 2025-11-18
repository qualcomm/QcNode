// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SamplePlrPre.hpp"

extern const size_t VOXELIZATION_PILLAR_COORDS_DIM;

namespace QC
{
namespace sample
{

SamplePlrPre::SamplePlrPre() {}
SamplePlrPre::~SamplePlrPre() {}

QCStatus_e SamplePlrPre::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_config.Set<std::string>( "name", m_name );
    m_config.Set<uint32_t>( "id", 0 );
    m_config.Set<std::string>( "processorType", Get( config, "processor", "cpu" ) );
    m_config.Set<float>( "Xsize", Get( config, "pillar_size_x", 0.16f ) );
    m_config.Set<float>( "Ysize", Get( config, "pillar_size_y", 0.16f ) );
    m_config.Set<float>( "Zsize", Get( config, "pillar_size_z", 4.0f ) );
    m_config.Set<float>( "Xmin", Get( config, "min_x", 0.0f ) );
    m_config.Set<float>( "Ymin", Get( config, "min_y", -39.68f ) );
    m_config.Set<float>( "Zmin", Get( config, "min_z", -3.0f ) );
    m_config.Set<float>( "Xmax", Get( config, "max_x", 69.12f ) );
    m_config.Set<float>( "Ymax", Get( config, "max_y", 39.68f ) );
    m_config.Set<float>( "Zmax", Get( config, "max_z", 1.0f ) );
    m_config.Set<uint32_t>( "maxPointNum", Get( config, "max_points", 300000 ) );
    m_config.Set<uint32_t>( "maxPlrNum", Get( config, "max_pillars", 12000 ) );
    m_config.Set<uint32_t>( "maxPointNumPerPlr", Get( config, "max_points_per_pillar", 32 ) );
    m_config.Set<std::string>( "inputMode", Get( config, "input_mode", "xyzr" ) );
    m_config.Set<uint32_t>( "outputFeatureDimNum", Get( config, "out_feature_dim", 10 ) );
    m_config.Set<bool>( "deRegisterAllBuffersWhenStop",
                        Get( config, "deRegisterAllBuffersWhenStop", false ) );

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    std::vector<uint32_t> outputPlrBufferIds;
    std::vector<uint32_t> outputFeatureBufferIds;
    uint32_t plrPointsBufferId;
    uint32_t coordToPlrIdxBufferId;

    uint32_t bufferIdx = 0;
    for ( uint32_t i = 0; i < m_poolSize; i++ )
    {
        outputPlrBufferIds.push_back( bufferIdx );
        bufferIdx++;
    }
    for ( uint32_t i = 0; i < m_poolSize; i++ )
    {
        outputFeatureBufferIds.push_back( bufferIdx );
        bufferIdx++;
    }
    plrPointsBufferId = bufferIdx++;
    coordToPlrIdxBufferId = bufferIdx;

    m_config.Set( "outputPlrBufferIds", outputPlrBufferIds );
    m_config.Set( "outputFeatureBufferIds", outputFeatureBufferIds );
    m_config.Set<uint32_t>( "plrPointsBufferId", plrPointsBufferId );
    m_config.Set<uint32_t>( "coordToPlrIdxBufferId", coordToPlrIdxBufferId );
    m_dataTree.Set( "static", m_config );

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

    m_processor = m_config.GetProcessorType( "processorType", QC_PROCESSOR_CPU );

    return ret;
}

QCStatus_e SamplePlrPre::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCNodeInit_t nodeConfig;

    float Xsize = 0;
    float Ysize = 0;
    float Zsize = 0;
    float Xmin = 0;
    float Ymin = 0;
    float Zmin = 0;
    float Xmax = 0;
    float Ymax = 0;
    float Zmax = 0;
    std::string inputMode;

    uint32_t maxPointNum = 0;
    uint32_t maxPlrNum = 0;
    uint32_t maxPointNumPerPlr = 0;
    uint32_t inputFeatureDimNum = 0;
    uint32_t outputFeatureDimNum = 0;

    QCTensorProps_t inputTensorProp;
    QCTensorProps_t outputPlrTensorProp;
    QCTensorProps_t outputFeatureTensorProp;
    TensorProps_t plrPointsTensorProp;
    TensorProps_t coordToPlrIdxTensorProp;

    ret = SampleIF::Init( name, QC_NODE_TYPE_VOXEL );

    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        nodeConfig.config = m_dataTree.Dump();
        QC_INFO( "config: %s", nodeConfig.config.c_str() );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_pBufMgr = BufferManager::Get( m_nodeId, LOGGER_LEVEL_INFO );
        if ( nullptr == m_pBufMgr )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "Failed to get buffer manager for %s %d: %s!", m_nodeId.name.c_str(),
                      m_nodeId.id, name.c_str() );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        Xsize = m_config.Get<float>( "Xsize", 0 );
        Ysize = m_config.Get<float>( "Ysize", 0 );
        Zsize = m_config.Get<float>( "Zsize", 0 );
        Xmin = m_config.Get<float>( "Xmin", 0 );
        Ymin = m_config.Get<float>( "Ymin", 0 );
        Zmin = m_config.Get<float>( "Zmin", 0 );
        Xmax = m_config.Get<float>( "Xmax", 0 );
        Ymax = m_config.Get<float>( "Ymax", 0 );
        Zmax = m_config.Get<float>( "Zmax", 0 );
        maxPointNum = m_config.Get<uint32_t>( "maxPointNum", 0 );
        if ( 0 == maxPointNum )
        {
            QC_ERROR( "invalid maxPointNum" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        maxPlrNum = m_config.Get<uint32_t>( "maxPlrNum", 0 );
        if ( 0 == maxPlrNum )
        {
            QC_ERROR( "invalid maxPlrNum" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        maxPointNumPerPlr = m_config.Get<uint32_t>( "maxPointNumPerPlr", 0 );
        if ( 0 == maxPointNumPerPlr )
        {
            QC_ERROR( "invalid maxPointNumPerPlr" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        inputMode = m_config.Get<std::string>( "inputMode", "" );
        if ( inputMode == "xyzr" )
        {
            inputFeatureDimNum = 4;
        }
        else if ( inputMode == "xyzrt" )
        {
            inputFeatureDimNum = 5;
        }

        outputFeatureDimNum = m_config.Get<uint32_t>( "outputFeatureDimNum", 0 );
        if ( 0 == outputFeatureDimNum )
        {
            QC_ERROR( "invalid outputFeatureDimNum" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        inputMode = m_config.Get<std::string>( "inputMode", "" );
        if ( ( "xyzr" != inputMode ) && ( "xyzrt" != inputMode ) )
        {
            QC_ERROR( "invalid inputMode" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( inputMode == "xyzr" )
        {
            outputPlrTensorProp.type = QC_TENSOR_TYPE_FLOAT_32;
            outputPlrTensorProp.dims[0] = maxPlrNum;
            outputPlrTensorProp.dims[1] = (uint32_t) VOXELIZATION_PILLAR_COORDS_DIM;
            outputPlrTensorProp.dims[2] = 0;
            outputPlrTensorProp.numDims = 2;
        }
        else if ( inputMode == "xyzrt" )
        {
            outputPlrTensorProp.type = QC_TENSOR_TYPE_INT_32;
            outputPlrTensorProp.dims[0] = maxPlrNum;
            outputPlrTensorProp.dims[1] = 2;
            outputPlrTensorProp.dims[2] = 0;
            outputPlrTensorProp.numDims = 2;
        }
        ret = m_outputPlrBufferPool.Init( name + ".plrs", m_nodeId, LOGGER_LEVEL_INFO, m_poolSize,
                                          outputPlrTensorProp, QC_MEMORY_ALLOCATOR_DMA_HTP );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to init buffer pool for output pillar buffers" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_outputPlrBufferPool.GetBuffers( nodeConfig.buffers );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to get output pillar buffers for config" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        outputFeatureTensorProp.type = QC_TENSOR_TYPE_FLOAT_32;
        outputFeatureTensorProp.dims[0] = maxPlrNum;
        outputFeatureTensorProp.dims[1] = maxPointNumPerPlr;
        outputFeatureTensorProp.dims[2] = outputFeatureDimNum;
        outputFeatureTensorProp.dims[3] = 0;
        outputFeatureTensorProp.numDims = 3;
        ret = m_outputFeatureBufferPool.Init( name + ".features", m_nodeId, LOGGER_LEVEL_INFO,
                                              m_poolSize, outputFeatureTensorProp,
                                              QC_MEMORY_ALLOCATOR_DMA_HTP );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to init buffer pool for output feature buffers" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_outputFeatureBufferPool.GetBuffers( nodeConfig.buffers );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to get output feature buffers for config" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            plrPointsTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
            plrPointsTensorProp.dims[0] = maxPlrNum + 1;
            plrPointsTensorProp.dims[1] = 0;
            plrPointsTensorProp.numDims = 1;

            ret = m_pBufMgr->Allocate( plrPointsTensorProp, m_plrPointsTensor );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to allocate buffe for m_plrPointsTensor" );
            }
            else
            {
                nodeConfig.buffers.push_back( m_plrPointsTensor );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            size_t gridXSize = ceil( ( Xmax - Xmin ) / Xsize );
            size_t gridYSize = ceil( ( Ymax - Ymin ) / Ysize );

            coordToPlrIdxTensorProp.tensorType = QC_TENSOR_TYPE_INT_32;
            coordToPlrIdxTensorProp.dims[0] = (uint32_t) ( gridXSize * gridYSize * 2 );
            coordToPlrIdxTensorProp.dims[1] = 0;
            coordToPlrIdxTensorProp.numDims = 1;

            ret = m_pBufMgr->Allocate( coordToPlrIdxTensorProp, m_coordToPlrIdxTensor );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to allocate buffe for m_coordToPlrIdxTensor" );
            }
            else
            {
                nodeConfig.buffers.push_back( m_coordToPlrIdxTensor );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Init( m_processor );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_voxel.Initialize( nodeConfig );
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

QCStatus_e SamplePlrPre::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = m_voxel.Start();

    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SamplePlrPre::ThreadMain, this );
    }

    return ret;
}

void SamplePlrPre::ThreadMain()
{
    QCStatus_e ret = QC_STATUS_OK;
    NodeFrameDescriptor frameDesc( 5 );

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );

            std::shared_ptr<SharedBuffer_t> pOutputPlrBuffer = m_outputPlrBufferPool.Get();
            std::shared_ptr<SharedBuffer_t> pOutputFeatBuffer = m_outputFeatureBufferPool.Get();
            if ( ( nullptr != pOutputPlrBuffer ) && ( nullptr != pOutputFeatBuffer ) )
            {
                QCBufferDescriptorBase_t &buffer = frames.GetBuffer( 0 );
                ret = frameDesc.SetBuffer( 0, buffer );

                if ( QC_STATUS_OK == ret )
                {
                    ret = frameDesc.SetBuffer( 1, pOutputPlrBuffer->GetBuffer() );
                }

                if ( QC_STATUS_OK == ret )
                {
                    ret = frameDesc.SetBuffer( 2, pOutputFeatBuffer->GetBuffer() );
                }

                if ( QC_STATUS_OK == ret )
                {
                    ret = SampleIF::Lock();
                }

                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_BEGIN();
                    ret = m_voxel.ProcessFrameDescriptor( frameDesc );
                }

                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_END();
                    DataFrames_t outFrames;
                    DataFrame_t frame;
                    frame.buffer = pOutputPlrBuffer;
                    frame.frameId = frames.FrameId( 0 );
                    frame.timestamp = frames.Timestamp( 0 );
                    outFrames.Add( frame );
                    frame.buffer = pOutputFeatBuffer;
                    outFrames.Add( frame );
                    m_pub.Publish( outFrames );
                }
                else
                {
                    QC_ERROR( "Pillarize failed for %" PRIu64 " : %d", frames.FrameId( 0 ), ret );
                }
                (void) SampleIF::Unlock();
            }
        }
    }
}

QCStatus_e SamplePlrPre::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }
    ret = m_voxel.Stop();

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SamplePlrPre::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = m_voxel.DeInitialize();
    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Deinit();
    }

    BufferManager::Put( m_pBufMgr );
    m_pBufMgr = nullptr;

    return ret;
}

REGISTER_SAMPLE( PlrPre, SamplePlrPre );

}   // namespace sample
}   // namespace QC

