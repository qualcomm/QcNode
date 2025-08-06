// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SamplePlrPre.hpp"

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

    m_processor = m_config.GetProcessorType( "processorType", QC_PROCESSOR_HTP0 );

    return ret;
}

QCStatus_e SamplePlrPre::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = SampleIF::Init( name );

    uint32_t maxPlrNum;
    uint32_t maxPointNumPerPlr;
    uint32_t outputFeatureDimNum;
    std::string inputMode;

    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
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
        QCTensorProps_t outPlrsTsProp;
        if ( inputMode == "xyzr" )
        {
            outPlrsTsProp = {
                    QC_TENSOR_TYPE_FLOAT_32,
                    { maxPlrNum, VOXELIZATION_PILLAR_COORDS_DIM, 0 },
                    2,
            };
        }
        else if ( inputMode == "xyzrt" )
        {
            outPlrsTsProp = {
                    QC_TENSOR_TYPE_INT_32,
                    { maxPlrNum, 2, 0 },
                    2,
            };
        }
        ret = m_coordsPool.Init( name + ".coords", LOGGER_LEVEL_INFO, m_poolSize, outPlrsTsProp,
                                 QC_MEMORY_ALLOCATOR_DMA_HTP );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t outFeatureTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { maxPlrNum, maxPointNumPerPlr, outputFeatureDimNum, 0 },
                3,
        };
        ret = m_featuresPool.Init( name + ".features", LOGGER_LEVEL_INFO, m_poolSize,
                                   outFeatureTsProp, QC_MEMORY_ALLOCATOR_DMA_HTP );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Init( m_processor );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCNodeInit_t nodeConfig = { .config = m_dataTree.Dump() };
        std::cout << "config: " << nodeConfig.config << std::endl;

        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_voxel.Initialize( nodeConfig );
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

QCStatus_e SamplePlrPre::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_voxel.Start();
    TRACE_END( SYSTRACE_TASK_START );

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
    QCSharedFrameDescriptorNode frameDesc( 3 );
    QCSharedBufferDescriptor_t inputBuffer;
    QCSharedBufferDescriptor_t outputPlrBuffer;
    QCSharedBufferDescriptor_t outputFeatureBuffer;

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            std::shared_ptr<SharedBuffer_t> coords = m_coordsPool.Get();
            std::shared_ptr<SharedBuffer_t> features = m_featuresPool.Get();
            if ( ( nullptr != coords ) && ( nullptr != features ) )
            {
                inputBuffer.buffer = frames.SharedBuffer( 0 );
                outputPlrBuffer.buffer = coords->sharedBuffer;
                outputFeatureBuffer.buffer = features->sharedBuffer;

                ret = frameDesc.SetBuffer( 0, inputBuffer );

                if ( QC_STATUS_OK == ret )
                {
                    ret = frameDesc.SetBuffer( 1, outputPlrBuffer );
                }

                if ( QC_STATUS_OK == ret )
                {
                    ret = frameDesc.SetBuffer( 2, outputFeatureBuffer );
                }

                if ( QC_STATUS_OK == ret )
                {
                    ret = SampleIF::Lock();
                }

                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_BEGIN();
                    TRACE_BEGIN( frames.FrameId( 0 ) );
                    ret = m_voxel.ProcessFrameDescriptor( frameDesc );
                }

                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_END();
                    TRACE_END( frames.FrameId( 0 ) );
                    DataFrames_t outFrames;
                    DataFrame_t frame;
                    frame.buffer = coords;
                    frame.frameId = frames.FrameId( 0 );
                    frame.timestamp = frames.Timestamp( 0 );
                    outFrames.Add( frame );
                    frame.buffer = features;
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
    PROFILER_SHOW();
    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_voxel.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );

    return ret;
}

QCStatus_e SamplePlrPre::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_voxel.DeInitialize();
    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Deinit();
    }
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( PlrPre, SamplePlrPre );

}   // namespace sample
}   // namespace QC

