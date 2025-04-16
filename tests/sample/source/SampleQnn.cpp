// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/SampleQnn.hpp"
#include "QnnSampleAppUtils.hpp"


namespace QC
{
namespace sample
{

static void SampleQnn_OutputCallback( void *pAppPriv, void *pOutputPriv )
{
    std::condition_variable *pCondVar = (std::condition_variable *) pAppPriv;
    uint64_t *pAsyncResult = (uint64_t *) pOutputPriv;
    *pAsyncResult = 0;
    pCondVar->notify_one();
}

static void SampleQnn_ErrorCallback( void *pAppPriv, void *pOutputPriv,
                                     Qnn_NotifyStatus_t notifyStatus )
{
    std::condition_variable *pCondVar = (std::condition_variable *) pAppPriv;
    uint64_t *pAsyncResult = (uint64_t *) pOutputPriv;
    *pAsyncResult = notifyStatus.error;
    pCondVar->notify_one();
}

SampleQnn::SampleQnn() {}
SampleQnn::~SampleQnn() {}

QCStatus_e SampleQnn::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_modelPath = Get( config, "model_path", "" );
    m_config.modelPath = m_modelPath.c_str();
    if ( "" == m_config.modelPath )
    {
        QC_ERROR( "invalid modelPath\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

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

    m_config.processorType = Get( config, "processor", QC_PROCESSOR_HTP0 );
    if ( QC_PROCESSOR_MAX == m_config.processorType )
    {
        QC_ERROR( "invalid processor %s\n", Get( config, "processor", "" ).c_str() );
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

    std::string opPackagePathsStr = Get( config, "udo", "" );
    if ( "" != opPackagePathsStr )
    {
        std::vector<std::string> opPackagePaths;
        split( opPackagePaths, opPackagePathsStr, ',' );
        m_udoPkgs.resize( opPackagePaths.size() );
        m_opPackagePaths.resize( opPackagePaths.size() );
        for ( int i = 0; i < opPackagePaths.size(); ++i )
        {
            static std::vector<std::string> opPackage;
            split( opPackage, opPackagePaths[i], ':' );
            if ( opPackage.size() != 2 )
            {
                QC_ERROR( "invalid opPackage params: %s\n", opPackagePaths[i].c_str() );
                ret = QC_STATUS_BAD_ARGUMENTS;
                break;
            }
            m_udoPkgs[i].udoLibPath = opPackage[0];
            m_udoPkgs[i].interfaceProvider = opPackage[1];
            m_opPackagePaths[i].udoLibPath = m_udoPkgs[i].udoLibPath.c_str();
            m_opPackagePaths[i].interfaceProvider = m_udoPkgs[i].interfaceProvider.c_str();
            QC_INFO( "opPackage params %d, udoLibPath: %s, interfaceProvider: %s\n", i,
                     m_opPackagePaths[i].udoLibPath, m_opPackagePaths[i].interfaceProvider );
        }
        m_config.numOfUdoPackages = m_opPackagePaths.size();
        m_config.pUdoPackages = &m_opPackagePaths[0];
    }

    return ret;
}

QCStatus_e SampleQnn::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Init( m_config.processorType );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_qnn.Init( name.c_str(), &m_config );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( ( QC_STATUS_OK == ret ) && ( true == m_bAsync ) )
    {
        ret = m_qnn.RegisterCallback( SampleQnn_OutputCallback, SampleQnn_ErrorCallback,
                                      &m_condVar );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_qnn.GetInputInfo( &m_inputInfoList );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_qnn.GetOutputInfo( &m_outputInfoList );
    }

    const size_t outputNum = m_outputInfoList.num;

    if ( QC_STATUS_OK == ret )
    {
        m_tensorPools.resize( outputNum );

        size_t index = 0;
        for ( int i = 0; i < outputNum; ++i )
        {
            ret = m_tensorPools[index].Init(
                    "Qnn." + name + "." + std::to_string( index ), LOGGER_LEVEL_INFO, m_poolSize,
                    m_outputInfoList.pInfo[i].properties, QC_BUFFER_USAGE_HTP );
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
    QnnRuntime_TensorInfoList_t inputInfoList;
    QnnRuntime_TensorInfoList_t outputInfoList;
    ModelInOutInfo_t ioInfo;
    if ( "" != m_modelInOutInfoTopicName )
    {
        ret = m_qnn.GetInputInfo( &inputInfoList );
        if ( QC_STATUS_OK == ret )
        {
            for ( uint32_t i = 0; i < inputInfoList.num; i++ )
            {
                TensorInfo_t ts;
                ts.name = inputInfoList.pInfo[i].pName;
                ts.properties = inputInfoList.pInfo[i].properties;
                ts.quantScale = inputInfoList.pInfo[i].quantScale;
                ts.quantOffset = inputInfoList.pInfo[i].quantOffset;
                ioInfo.inputs.push_back( ts );
            }
            ret = m_qnn.GetOutputInfo( &outputInfoList );
        }

        if ( QC_STATUS_OK == ret )
        {
            for ( uint32_t i = 0; i < outputInfoList.num; i++ )
            {
                TensorInfo_t ts;
                ts.name = outputInfoList.pInfo[i].pName;
                ts.properties = outputInfoList.pInfo[i].properties;
                ts.quantScale = outputInfoList.pInfo[i].quantScale;
                ts.quantOffset = outputInfoList.pInfo[i].quantOffset;
                ioInfo.outputs.push_back( ts );
            }
            m_modelInOutInfoPub.Publish( ioInfo );
        }
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
    uint64_t asyncResult = 0;

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

            for ( size_t i = 0; ( i < m_outputInfoList.num ) && ( QC_STATUS_OK == ret ); i++ )
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

            if ( QC_STATUS_OK == ret )
            {
                ret = SampleIF::Lock();
                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_BEGIN();
                    TRACE_BEGIN( frames.FrameId( 0 ) );
                    if ( true == m_bAsync )
                    {
                        asyncResult = 0xdeadbeef;
                        ret = m_qnn.Execute( inputs.data(), inputs.size(), outputs.data(),
                                             outputs.size(), &asyncResult );
                        if ( QC_STATUS_OK == ret )
                        {
                            std::unique_lock<std::mutex> lock( mtx );
                            (void) m_condVar.wait_for( lock, std::chrono::milliseconds( 1000 ) );
                            if ( 0 != asyncResult )
                            {
                                QC_ERROR( "QNN Async Execute failed for %" PRIu64 " : %" PRIu64,
                                          frames.FrameId( 0 ), asyncResult );
                                ret = QC_STATUS_FAIL;
                            }
                        }
                    }
                    else
                    {
                        ret = m_qnn.Execute( inputs.data(), inputs.size(), outputs.data(),
                                             outputs.size() );
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
                    tensor.name = m_outputInfoList.pInfo[index].pName;
                    tensor.quantScale = m_outputInfoList.pInfo[index].quantScale;
                    tensor.quantOffset = m_outputInfoList.pInfo[index].quantOffset;
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
    ret = m_qnn.Deinit();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( Qnn, SampleQnn );

}   // namespace sample
}   // namespace QC
