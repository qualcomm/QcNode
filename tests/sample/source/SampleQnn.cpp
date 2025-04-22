// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QnnSampleAppUtils.hpp"
// keep QnnSampleAppUtils include firstly to avoid a strange build error for linux, TODO: why
#include "QC/sample/SampleQnn.hpp"


namespace QC
{
namespace sample
{

void SampleQnn::EventCallback( const QCNodeEventInfo_t &info )
{
    if ( QC_STATUS_OK == info.status )
    {
        m_asyncResult = 0;
        m_condVar.notify_one();
    }
    else
    {
        m_asyncResult = 0xdeadbeef; /* TODO: */
        m_condVar.notify_one();
    }
}

SampleQnn::SampleQnn() {}
SampleQnn::~SampleQnn() {}

QCStatus_e SampleQnn::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    auto imageConvertTypeStr = Get( config, "image_convert", "default" );
    if ( "default" == imageConvertTypeStr )
    {
        m_imageConvertType = SAMPLE_QNN_IMAGE_CONVERT_DEFAULT;
    }
    else if ( "gray" == imageConvertTypeStr )
    {
        m_imageConvertType = SAMPLE_QNN_IMAGE_CONVERT_GRAY;
    }
    else if ( "chroma_first" == imageConvertTypeStr )
    {
        m_imageConvertType = SAMPLE_QNN_IMAGE_CONVERT_CHROMA_FIRST;
    }
    else
    {
        QC_ERROR( "invalid image_convert\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_bAsync = Get( config, "async", false );

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

    m_modelInOutInfoTopicName = Get( config, "model_io_info_topic", "" );

    std::vector<DataTree> udoPkgs;
    std::string opPackagePathsStr = Get( config, "udo", "" );
    if ( "" != opPackagePathsStr )
    {
        std::vector<std::string> opPackagePaths;
        split( opPackagePaths, opPackagePathsStr, ',' );
        m_opPackagePaths.resize( opPackagePaths.size() );
        for ( int i = 0; i < opPackagePaths.size(); ++i )
        {
            DataTree dt;
            std::vector<std::string> opPackage;
            split( opPackage, opPackagePaths[i], ':' );
            if ( opPackage.size() != 2 )
            {
                QC_ERROR( "invalid opPackage params: %s\n", opPackagePaths[i].c_str() );
                ret = QC_STATUS_BAD_ARGUMENTS;
                break;
            }
            m_opPackagePaths[i].udoLibPath = opPackage[0].c_str();
            m_opPackagePaths[i].interfaceProvider = opPackage[1].c_str();
            QC_INFO( "opPackage params %d, udoLibPath: %s, interfaceProvider: %s\n", i,
                     m_opPackagePaths[i].udoLibPath, m_opPackagePaths[i].interfaceProvider );
            dt.Set<std::string>( "udoLibPath", opPackage[0] );
            dt.Set<std::string>( "interfaceProvider", opPackage[1] );
            udoPkgs.push_back( dt );
        }
    }

    DataTree dt;
    dt.Set<std::string>( "name", m_name );
    dt.Set<uint32_t>( "id", 0 ); /* TODO: */
    dt.Set<std::string>( "modelPath", Get( config, "model_path", "" ) );
    dt.Set<std::string>( "loadType", "binary" );
    dt.Set<std::string>( "processorType", Get( config, "processor", "htp0" ) );
    dt.Set( "udoPackages", udoPkgs );
    m_dataTree.Set( "static", dt );
    m_processor = Get( config, "processor", QC_PROCESSOR_HTP0 );

    return ret;
}

QCStatus_e SampleQnn::ConvertDtToInfo( DataTree &dt, TensorInfo_t &info )
{
    QCStatus_e ret = QC_STATUS_OK;
    std::vector<uint32_t> dims = dt.Get<uint32_t>( "dims", std::vector<uint32_t>( {} ) );

    info.name = dt.Get<std::string>( "name", "" );
    info.quantOffset = dt.Get<int32_t>( "quantOffset", 0 );
    info.quantScale = dt.Get<float>( "quantScale", 1.0f );

    if ( dims.size() <= QC_NUM_TENSOR_DIMS )
    {
        info.properties.numDims = dims.size();
        std::copy( dims.begin(), dims.end(), info.properties.dims );
    }
    else
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    info.properties.type = dt.GetTensorType( "type", QC_TENSOR_TYPE_MAX );
    if ( QC_TENSOR_TYPE_MAX == info.properties.type )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    return ret;
}


QCStatus_e SampleQnn::Init( std::string name, SampleConfig_t &config )
{
    QCNodeInit_t nodeCfg;
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Init( m_processor );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        nodeCfg.config = m_dataTree.Dump();
        if ( true == m_bAsync )
        {
            using std::placeholders::_1;
            nodeCfg.callback = std::bind( &SampleQnn::EventCallback, this, _1 );
        }
        ret = m_qnn.Initialize( nodeCfg );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCNodeConfigIfs &cfgIfs = m_qnn.GetConfigurationIfs();
        const std::string &options = cfgIfs.GetOptions();
        DataTree optionsDt;
        std::vector<DataTree> inputDts;
        std::vector<DataTree> outputDts;
        std::string errors;
        ret = optionsDt.Load( options, errors );
        if ( QC_STATUS_OK == ret )
        {
            ret = optionsDt.Get( "model.inputs", inputDts );
        }
        if ( QC_STATUS_OK == ret )
        {
            ret = optionsDt.Get( "model.outputs", outputDts );
        }
        if ( QC_STATUS_OK == ret )
        {
            for ( auto &inDt : inputDts )
            {
                TensorInfo_t info;
                ret = ConvertDtToInfo( inDt, info );
                if ( QC_STATUS_OK == ret )
                {
                    m_inputsInfo.push_back( info );
                }
                else
                {
                    break;
                }
            }
        }

        for ( auto &outDt : outputDts )
        {
            TensorInfo_t info;
            ret = ConvertDtToInfo( outDt, info );
            if ( QC_STATUS_OK == ret )
            {
                m_ouputsInfo.push_back( info );
            }
            else
            {
                break;
            }
        }
    }

    const size_t outputNum = m_ouputsInfo.size();

    if ( QC_STATUS_OK == ret )
    {
        m_tensorPools.resize( outputNum );

        size_t index = 0;
        for ( int i = 0; i < outputNum; ++i )
        {
            ret = m_tensorPools[index].Init( "Qnn." + name + "." + std::to_string( index ),
                                             LOGGER_LEVEL_INFO, m_poolSize,
                                             m_ouputsInfo[i].properties, QC_BUFFER_USAGE_HTP );
            index += 1;
            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
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
        if ( "" != m_modelInOutInfoTopicName )
        {
            ret = m_modelInOutInfoPub.Init( name, m_modelInOutInfoTopicName );
        }
    }

    return ret;
}

QCStatus_e SampleQnn::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    ModelInOutInfo_t ioInfo;
    if ( "" != m_modelInOutInfoTopicName )
    {
        ioInfo.inputs = m_inputsInfo;
        ioInfo.outputs = m_ouputsInfo;
        m_modelInOutInfoPub.Publish( ioInfo );
    }

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_qnn.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleQnn::ThreadMain, this );
    }

    return ret;
}

void SampleQnn::ThreadMain()
{
    QCStatus_e ret;
    std::mutex mtx;

    QCSharedFrameDescriptorNode frameDesc( m_inputsInfo.size() + m_ouputsInfo.size() + 1 );

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            std::vector<QCSharedBuffer_t> inputs;
            std::vector<QCSharedBuffer_t> outputs;
            std::vector<std::shared_ptr<SharedBuffer_t>> outputBuffers;
            for ( auto &frame : frames.frames )
            {
                if ( QC_BUFFER_TYPE_IMAGE == frame.BufferType() )
                {
                    if ( ( QC_IMAGE_FORMAT_NV12 == frame.SharedBuffer().imgProps.format ) ||
                         ( QC_IMAGE_FORMAT_P010 == frame.SharedBuffer().imgProps.format ) )
                    {
                        QCSharedBuffer luma;
                        QCSharedBuffer chroma;
                        ret = frame.SharedBuffer().ImageToTensor( &luma, &chroma );
                        if ( QC_STATUS_OK == ret )
                        {
                            if ( SAMPLE_QNN_IMAGE_CONVERT_DEFAULT == m_imageConvertType )
                            {
                                inputs.push_back( luma );
                                inputs.push_back( chroma );
                            }
                            else if ( SAMPLE_QNN_IMAGE_CONVERT_GRAY == m_imageConvertType )
                            {
                                inputs.push_back( luma );
                            }
                            else
                            {
                                inputs.push_back( chroma );
                                inputs.push_back( luma );
                            }
                        }
                    }
                    else
                    {
                        QCSharedBuffer_t sharedBuffer;
                        ret = frame.SharedBuffer().ImageToTensor( &sharedBuffer );
                        if ( QC_STATUS_OK == ret )
                        {
                            inputs.push_back( sharedBuffer );
                        }
                    }
                }
                else
                {
                    inputs.push_back( frame.SharedBuffer() );
                }
                if ( QC_STATUS_OK != ret )
                { /* only possible has error for image */
                    QC_ERROR( "QNN failed to do image to tensor convert for frameId %" PRIu64
                              ": ret = %d",
                              frames.FrameId( 0 ), ret );
                    break;
                }
            }

            for ( size_t i = 0; ( i < m_ouputsInfo.size() ) && ( QC_STATUS_OK == ret ); i++ )
            {
                std::shared_ptr<SharedBuffer_t> buffer = m_tensorPools[i].Get();
                if ( nullptr != buffer )
                {
                    outputs.push_back( buffer->sharedBuffer );
                    outputBuffers.push_back( buffer );
                }
                else
                {
                    ret = QC_STATUS_NOMEM;
                }
            }

            /* below code is ugly not friendly as because of the design, how to make better? */
            std::vector<QCSharedBufferDescriptor_t> bufferDescs;
            bufferDescs.resize( inputs.size() + outputs.size() );
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
                for ( auto &buffer : outputs )
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
                ret = SampleIF::Lock();
                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_BEGIN();
                    TRACE_BEGIN( frames.FrameId( 0 ) );
                    if ( true == m_bAsync )
                    {
                        m_asyncResult = 0xdeadbeef;
                        ret = m_qnn.ProcessFrameDescriptor( frameDesc );
                        if ( QC_STATUS_OK == ret )
                        {
                            std::unique_lock<std::mutex> lock( mtx );
                            (void) m_condVar.wait_for( lock, std::chrono::milliseconds( 1000 ) );
                            if ( 0 != m_asyncResult )
                            {
                                QC_ERROR( "QNN Async Execute failed for %" PRIu64 " : %" PRIu64,
                                          frames.FrameId( 0 ), m_asyncResult );
                                ret = QC_STATUS_FAIL;
                            }
                        }
                    }
                    else
                    {
                        ret = m_qnn.ProcessFrameDescriptor( frameDesc );
                    }
                    if ( QC_STATUS_OK == ret )
                    {
                        PROFILER_END();
                        TRACE_END( frames.FrameId( 0 ) );
                    }
                    else
                    {
                        QC_ERROR( "QNN Execute failed for %" PRIu64 " : %d", frames.FrameId( 0 ),
                                  ret );
                    }
                    (void) SampleIF::Unlock();
                }
            }
            if ( QC_STATUS_OK == ret )
            {
                DataFrames_t outTensors;
                size_t index = 0;
                for ( auto &buffer : outputBuffers )
                {
                    DataFrame_t tensor;
                    tensor.buffer = buffer;
                    tensor.frameId = frames.FrameId( 0 );
                    tensor.timestamp = frames.Timestamp( 0 );
                    tensor.name = m_ouputsInfo[index].name;
                    tensor.quantScale = m_ouputsInfo[index].quantScale;
                    tensor.quantOffset = m_ouputsInfo[index].quantOffset;
                    outTensors.Add( tensor );
                    index++;
                }
                m_pub.Publish( outTensors );
            }
            else
            {
                QC_ERROR( "QNN Execute failed for frameId %" PRIu64 ": ret = %d",
                          frames.FrameId( 0 ), ret );
            }
        }
    }
}

QCStatus_e SampleQnn::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_qnn.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );
    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleQnn::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_qnn.DeInitialize();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( Qnn, SampleQnn );

}   // namespace sample
}   // namespace QC
