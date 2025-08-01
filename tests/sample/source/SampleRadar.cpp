// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/sample/SampleRadar.hpp"

#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace QC
{
namespace sample
{

SampleRadar::SampleRadar()
    : m_maxInputBufferSize( 0 ),
      m_maxOutputBufferSize( 0 ),
      m_serviceName( "" ),
      m_timeoutMs( 0 ),
      m_enablePerformanceLog( false ),
      m_poolSize( 0 ),
      m_bufferFlags( QC_BUFFER_FLAGS_CACHE_WB_WA ),
      m_inputTopicName( "" ),
      m_outputTopicName( "" ),
      m_stop( false ),
      m_frameCount( 0 ),
      m_errorCount( 0 ),
      m_totalProcessingTime( 0.0 ),
      m_minProcessingTime( std::numeric_limits<double>::max() ),
      m_maxProcessingTime( 0.0 )
{
    QC_DEBUG( "SampleRadar constructor called" );
}

SampleRadar::~SampleRadar()
{
    QC_DEBUG( "SampleRadar destructor called" );
    // Ensure proper cleanup if not already done
    if ( m_thread.joinable() )
    {
        m_stop = true;
        m_thread.join();
    }
}

QCStatus_e SampleRadar::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    // Initialize base class with component name
    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "SampleIF::Init failed with error: %d", ret );
        return ret;
    }

    QC_INFO( "Initializing SampleRadar component: %s", name.c_str() );

    // Enable system tracing for performance monitoring
    // Note: SYSTRACE_PROCESSOR_RADAR not defined, using generic tracing
    // TRACE_ON( RADAR );

    // Parse and validate configuration parameters
    ret = ParseConfig( config );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Configuration parsing failed with error: %d", ret );
        return ret;
    }

    // Setup Node configuration from parsed parameters
    ret = SetupNodeConfiguration();
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Node configuration setup failed with error: %d", ret );
        return ret;
    }
    QC_LOG_DEBUG( "Node configuration setup completed" );
    // Initialize output buffer pool for processed radar data
    // Use tensor allocation for raw radar output data
    QCTensorProps_t outputTensorProps = { QC_TENSOR_TYPE_UINT_8, { m_maxOutputBufferSize }, 1 };
    ret = m_outputPool.Init( name + "_output", m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, outputTensorProps);
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Output buffer pool initialization failed with error: %d", ret );
        return ret;
    }

    QC_LOG_DEBUG( "Output buffer also initialized" );
    // Initialize Radar Node component with configuration
    TRACE_BEGIN( SYSTRACE_TASK_INIT );
    QCNodeInit_t nodeCfg;
    nodeCfg.config = m_dataTree.Dump();
    ret = m_radar.Initialize( nodeCfg );
    TRACE_END( SYSTRACE_TASK_INIT );

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Radar Node initialization failed with error: %d", ret );
        return ret;
    }

    // Initialize DataBroker subscriber for input radar data
    ret = m_sub.Init( name, m_inputTopicName );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Input subscriber initialization failed for topic: %s, error: %d",
                  m_inputTopicName.c_str(), ret );
        return ret;
    }

    // Initialize DataBroker publisher for processed results
    ret = m_pub.Init( name, m_outputTopicName );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Output publisher initialization failed for topic: %s, error: %d",
                  m_outputTopicName.c_str(), ret );
        return ret;
    }

    // Initialize performance monitoring
    m_frameCount = 0;
    m_errorCount = 0;
    m_totalProcessingTime = 0.0;
    m_minProcessingTime = std::numeric_limits<double>::max();
    m_maxProcessingTime = 0.0;
    m_startTime = std::chrono::high_resolution_clock::now();

    QC_INFO( "SampleRadar component initialized successfully" );
    QC_INFO( "Configuration: input_topic=%s, output_topic=%s, service=%s, timeout=%ums",
             m_inputTopicName.c_str(), m_outputTopicName.c_str(), m_serviceName.c_str(),
             m_timeoutMs );

    return ret;
}

QCStatus_e SampleRadar::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_INFO( "Starting SampleRadar component" );

    // Start the underlying Radar Node component
    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_radar.Start();
    TRACE_END( SYSTRACE_TASK_START );

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Radar Node start failed with error: %d", ret );
        return ret;
    }

    // Launch dedicated processing thread
    m_stop = false;
    try
    {
        m_thread = std::thread( &SampleRadar::ThreadMain, this );
        QC_INFO( "Processing thread launched successfully" );
    }
    catch ( const std::exception &e )
    {
        QC_ERROR( "Failed to launch processing thread: %s", e.what() );
        ret = QC_STATUS_FAIL;

        // Cleanup on thread creation failure
        m_radar.Stop();
        return ret;
    }

    QC_INFO( "SampleRadar component started successfully" );
    return ret;
}

QCStatus_e SampleRadar::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_INFO( "Stopping SampleRadar component" );

    // Signal processing thread to terminate
    m_stop = true;

    // Wait for thread completion with timeout
    if ( m_thread.joinable() )
    {
        try
        {
            m_thread.join();
            QC_INFO( "Processing thread terminated successfully" );
        }
        catch ( const std::exception &e )
        {
            QC_ERROR( "Error joining processing thread: %s", e.what() );
            ret = QC_STATUS_FAIL;
        }
    }

    // Stop the underlying Radar Node component
    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    QCStatus_e nodeRet = m_radar.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );

    if ( QC_STATUS_OK != nodeRet )
    {
        QC_ERROR( "Radar Node stop failed with error: %d", nodeRet );
        ret = nodeRet;
    }

    // Display performance statistics if enabled
    if ( m_enablePerformanceLog && m_frameCount > 0 )
    {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalTime = std::chrono::duration<double>( endTime - m_startTime ).count();
        double avgProcessingTime = m_totalProcessingTime / m_frameCount;
        double throughput = m_frameCount / totalTime;

        QC_INFO( "=== SampleRadar Performance Statistics ===" );
        QC_INFO( "Total frames processed: %lu", m_frameCount );
        QC_INFO( "Total errors: %lu", m_errorCount );
        QC_INFO( "Error rate: %.2f%%", ( m_errorCount * 100.0 ) / m_frameCount );
        QC_INFO( "Processing time - Min: %.2fms, Max: %.2fms, Avg: %.2fms", m_minProcessingTime,
                 m_maxProcessingTime, avgProcessingTime );
        QC_INFO( "Throughput: %.2f FPS", throughput );
        QC_INFO( "Total runtime: %.2f seconds", totalTime );
    }

    PROFILER_SHOW();

    QC_INFO( "SampleRadar component stopped successfully" );
    return ret;
}

QCStatus_e SampleRadar::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_INFO( "Deinitializing SampleRadar component" );

    // Deinitialize the Radar Node component
    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    QCStatus_e nodeRet = m_radar.DeInitialize();
    TRACE_END( SYSTRACE_TASK_DEINIT );

    if ( QC_STATUS_OK != nodeRet )
    {
        QC_ERROR( "Radar Node deinitialization failed with error: %d", nodeRet );
        ret = nodeRet;
    }

    // Buffer pools are automatically cleaned up by their destructors
    // DataBroker subscriptions are automatically cleaned up by their destructors

    QC_INFO( "SampleRadar component deinitialized successfully" );
    return ret;
}

QCStatus_e SampleRadar::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_DEBUG( "Parsing SampleRadar configuration" );

    // Parse buffer size limits with validation
    m_maxInputBufferSize = Get( config, "max_input_buffer_size", 1048576U );       // 1MB default
    if ( m_maxInputBufferSize == 0 || m_maxInputBufferSize > 100 * 1024 * 1024 )   // Max 100MB
    {
        QC_ERROR( "Invalid max_input_buffer_size: %u (must be 1-104857600)", m_maxInputBufferSize );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    m_maxOutputBufferSize = Get( config, "max_output_buffer_size", 1048576U );       // 1MB default
    if ( m_maxOutputBufferSize == 0 || m_maxOutputBufferSize > 100 * 1024 * 1024 )   // Max 100MB
    {
        QC_ERROR( "Invalid max_output_buffer_size: %u (must be 1-104857600)",
                  m_maxOutputBufferSize );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    // Parse radar service configuration
    m_serviceName = Get( config, "service_name", "/dev/radar0" );
    if ( m_serviceName.empty() )
    {
        QC_ERROR( "Service name cannot be empty" );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    // Parse timeout with validation
    m_timeoutMs = Get( config, "timeout_ms", 5000U );   // 5 second default
    if ( m_timeoutMs == 0 || m_timeoutMs > 60000 )      // Max 60 seconds
    {
        QC_ERROR( "Invalid timeout_ms: %u (must be 1-60000)", m_timeoutMs );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    // Parse performance logging setting
    m_enablePerformanceLog = Get( config, "enable_performance_log", false );

    // Parse buffer pool configuration
    m_poolSize = Get( config, "pool_size", 4U );
    if ( m_poolSize == 0 || m_poolSize > 32 )   // Max 32 buffers
    {
        QC_ERROR( "Invalid pool_size: %u (must be 1-32)", m_poolSize );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    // Parse memory allocation preferences
    bool bCache = Get( config, "cache", true );
    m_bufferFlags = bCache ? QC_BUFFER_FLAGS_CACHE_WB_WA : 0;

    // Parse required topic names
    m_inputTopicName = Get( config, "input_topic", "" );
    if ( m_inputTopicName.empty() )
    {
        QC_ERROR( "Input topic name is required" );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( m_outputTopicName.empty() )
    {
        QC_ERROR( "Output topic name is required" );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    // Validate topic names are different to prevent loops
    if ( m_inputTopicName == m_outputTopicName )
    {
        QC_ERROR( "Input and output topic names must be different" );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    QC_DEBUG( "Configuration parsed successfully" );
    QC_DEBUG( "Buffer sizes: input=%u, output=%u", m_maxInputBufferSize, m_maxOutputBufferSize );
    QC_DEBUG( "Service: %s, timeout: %ums", m_serviceName.c_str(), m_timeoutMs );
    QC_DEBUG( "Topics: input=%s, output=%s", m_inputTopicName.c_str(), m_outputTopicName.c_str() );

    return ret;
}

QCStatus_e SampleRadar::SetupNodeConfiguration()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_DEBUG( "Setting up Radar Node configuration" );

    // Create DataTree configuration for Radar Node
    // This follows the Node configuration pattern established in tests
    DataTree staticConfig;
    staticConfig.Set<std::string>( "name", m_name );
    staticConfig.Set<uint32_t>( "id", 0 );
    // Set service configuration at top level (as expected by VerifyStaticConfig)
    staticConfig.Set<std::string>( "serviceName", m_serviceName );
    staticConfig.Set<uint32_t>( "timeoutMs", m_timeoutMs );
    staticConfig.Set<bool>( "enablePerformanceLog", m_enablePerformanceLog );
    // Set buffer size limits at top level
    staticConfig.Set<uint32_t>( "maxInputBufferSize", m_maxInputBufferSize );
    staticConfig.Set<uint32_t>( "maxOutputBufferSize", m_maxOutputBufferSize );

    // Set empty buffer IDs since we don't register buffers during initialization
    std::vector<uint32_t> bufferIds;   // empty - no initialization-time buffer registration
    staticConfig.Set( "bufferIds", bufferIds );

    // Set global buffer ID mapping for Node interface
    std::vector<DataTree> bufferMapDts;

    DataTree inputMapDt;
    inputMapDt.Set<std::string>( "name", "input" );
    inputMapDt.Set<uint32_t>( "id", 0 );   // Input at global ID 0
    bufferMapDts.push_back( inputMapDt );

    DataTree outputMapDt;
    outputMapDt.Set<std::string>( "name", "output" );
    outputMapDt.Set<uint32_t>( "id", 1 );   // Output at global ID 1
    bufferMapDts.push_back( outputMapDt );

    staticConfig.Set( "globalBufferIdMap", bufferMapDts );
    staticConfig.Set<bool>( "deRegisterAllBuffersWhenStop", false );

    // Set the complete static configuration
    m_dataTree.Set( "static", staticConfig );

    // Generate JSON configuration for Node
    std::string jsonConfig = m_dataTree.Dump();
    QC_DEBUG( "Node configuration JSON: %s", jsonConfig.c_str() );

    return ret;
}

void SampleRadar::ThreadMain()
{
    QCStatus_e ret = QC_STATUS_OK;
    QC::Node::QCSharedFrameDescriptorNode frameDesc( 2 );   // Input + Output buffers

    QC_INFO( "Radar processing thread started" );

    // Main processing loop - continues until stop signal received
    while ( !m_stop )
    {
        DataFrames_t inputFrames;

        // Receive radar data from input topic
        ret = m_sub.Receive( inputFrames );

        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                      inputFrames.FrameId( 0 ), inputFrames.Timestamp( 0 ) );

            // Get output buffer from pool
            std::shared_ptr<SharedBuffer_t> outputBuffer = m_outputPool.Get();
            if ( nullptr != outputBuffer )
            {
                auto processingStart = std::chrono::high_resolution_clock::now();

                // Setup frame descriptor following SampleRemap pattern
                std::vector<QC::Node::QCSharedBufferDescriptor_t> bufferDescs;
                bufferDescs.resize( 2 );
                frameDesc.Clear();

                PROFILER_BEGIN();
                TRACE_BEGIN( inputFrames.FrameId( 0 ) );

                // Set input buffer
                bufferDescs[0].buffer = inputFrames.frames[0].buffer->sharedBuffer;
                ret = frameDesc.SetBuffer( 0, bufferDescs[0] );

                if ( QC_STATUS_OK == ret )
                {
                    // Set output buffer
                    bufferDescs[1].buffer = outputBuffer->sharedBuffer;
                    ret = frameDesc.SetBuffer( 1, bufferDescs[1] );
                }
                if ( QC_STATUS_OK == ret )
                {
                    // Execute radar processing
                    ret = m_radar.ProcessFrameDescriptor( frameDesc );
                }
                if ( QC_STATUS_OK == ret )
                {
                    PROFILER_END();
                    TRACE_END( inputFrames.FrameId( 0 ) );
                    // Publish output
                    DataFrames_t outputFrames;
                    DataFrame_t outputFrame;
                    outputFrame.buffer = outputBuffer;
                    outputFrame.frameId = inputFrames.FrameId( 0 );
                    outputFrame.timestamp = inputFrames.Timestamp( 0 );
                    outputFrames.Add( outputFrame );
                    m_pub.Publish( outputFrames );

                    // Update performance metrics
                    auto processingEnd = std::chrono::high_resolution_clock::now();
                    double processingTimeMs = std::chrono::duration<double, std::milli>(
                                                      processingEnd - processingStart )
                                                      .count();
                    UpdatePerformanceMetrics( processingTimeMs, true );
                }
                else
                {
                    QC_ERROR( "Radar processing failed for frameId %" PRIu64 ": %d",
                              inputFrames.FrameId( 0 ), ret );
                    UpdatePerformanceMetrics( 0.0, false );
                }
            }
            else
            {
                QC_WARN( "Failed to get output buffer from pool" );
            }
        }
    }

    QC_INFO( "Radar processing thread terminated" );
}

QCStatus_e SampleRadar::ProcessFrame( const DataFrames_t &inputFrames )
{
    QCStatus_e ret = QC_STATUS_OK;
    auto processingStart = std::chrono::high_resolution_clock::now();

    // Validate input frame data
    if ( inputFrames.frames.empty() )
    {
        QC_ERROR( "No input frames received" );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    const DataFrame_t &inputFrame = inputFrames.frames[0];

    QC_DEBUG( "Processing radar frame: frameId=%lu, timestamp=%lu, size=%u", inputFrame.frameId,
              inputFrame.timestamp, inputFrame.buffer->sharedBuffer.size );

    // Validate input data format and content
    ret = ValidateInputData( inputFrame );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Input data validation failed: %d", ret );
        return ret;
    }

    // Acquire output buffer from pool
    std::shared_ptr<SharedBuffer_t> outputBuffer = m_outputPool.Get();
    if ( nullptr == outputBuffer )
    {
        QC_ERROR( "Failed to acquire output buffer from pool" );
        return QC_STATUS_NOMEM;
    }

    // Setup frame descriptor for Node processing
    // This maps input and output buffers for the Node component
    QC::Node::QCSharedFrameDescriptorNode frameDesc( 2 );
    std::vector<QC::Node::QCSharedBufferDescriptor_t> bufferDescs( 2 );

    frameDesc.Clear();

    // Set input buffer descriptor with proper base class initialization
    bufferDescs[0].buffer = inputFrame.buffer->sharedBuffer;
    bufferDescs[0].pBuf = bufferDescs[0].buffer.data();
    bufferDescs[0].size = bufferDescs[0].buffer.size;
    bufferDescs[0].name = "InputBuffer";
    bufferDescs[0].type = QC_BUFFER_TYPE_TENSOR;
    ret = frameDesc.SetBuffer( 0, bufferDescs[0] );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to set input buffer descriptor: %d", ret );
        return ret;
    }

    // Set output buffer descriptor with proper base class initialization
    bufferDescs[1].buffer = outputBuffer->sharedBuffer;
    bufferDescs[1].pBuf = bufferDescs[1].buffer.data();
    bufferDescs[1].size = bufferDescs[1].buffer.size;
    bufferDescs[1].name = "OutputBuffer";
    bufferDescs[1].type = QC_BUFFER_TYPE_TENSOR;
    ret = frameDesc.SetBuffer( 1, bufferDescs[1] );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to set output buffer descriptor: %d", ret );
        return ret;
    }

    // Execute radar processing through Node component
    PROFILER_BEGIN();
    TRACE_BEGIN( inputFrame.frameId );

    ret = m_radar.ProcessFrameDescriptor( frameDesc );

    TRACE_END( inputFrame.frameId );
    PROFILER_END();

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Radar Node processing failed: %d", ret );
        return ret;
    }

    // Publish processed results to output topic
    DataFrames_t outputFrames;
    DataFrame_t outputFrame;
    outputFrame.buffer = outputBuffer;
    outputFrame.frameId = inputFrame.frameId;
    outputFrame.timestamp = inputFrame.timestamp;
    outputFrames.Add( outputFrame );

    ret = m_pub.Publish( outputFrames );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to publish output frame: %d", ret );
        return ret;
    }

    // Update performance metrics
    auto processingEnd = std::chrono::high_resolution_clock::now();
    double processingTimeMs =
            std::chrono::duration<double, std::milli>( processingEnd - processingStart ).count();

    UpdatePerformanceMetrics( processingTimeMs, true );

    QC_DEBUG( "Frame processed successfully: frameId=%lu, processing_time=%.2fms",
              inputFrame.frameId, processingTimeMs );

    return ret;
}

QCStatus_e SampleRadar::ValidateInputData( const DataFrame_t &frame )
{
    QCStatus_e ret = QC_STATUS_OK;

    // Check buffer pointer validity
    if ( nullptr == frame.buffer )
    {
        QC_ERROR( "Input frame buffer is null" );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    // Check buffer data accessibility
    if ( nullptr == frame.buffer->sharedBuffer.data() )
    {
        QC_ERROR( "Input buffer data pointer is null" );
        return QC_STATUS_INVALID_BUF;
    }

    // Check buffer size constraints
    if ( frame.buffer->sharedBuffer.size == 0 )
    {
        QC_ERROR( "Input buffer size is zero" );
        return QC_STATUS_INVALID_BUF;
    }

    if ( frame.buffer->sharedBuffer.size > m_maxInputBufferSize )
    {
        QC_ERROR( "Input buffer size (%u) exceeds maximum (%u)", frame.buffer->sharedBuffer.size,
                  m_maxInputBufferSize );
        return QC_STATUS_INVALID_BUF;
    }

    // Check DMA handle validity
    if ( frame.buffer->sharedBuffer.buffer.dmaHandle == 0 )
    {
        QC_ERROR( "Invalid DMA handle in input buffer" );
        return QC_STATUS_INVALID_BUF;
    }

    // Validate buffer type compatibility
    QCBufferType_e bufferType = frame.buffer->sharedBuffer.type;
    if ( bufferType != QC_BUFFER_TYPE_RAW && bufferType != QC_BUFFER_TYPE_TENSOR &&
         bufferType != QC_BUFFER_TYPE_IMAGE )
    {
        QC_ERROR( "Unsupported buffer type: %d", bufferType );
        return QC_STATUS_BAD_ARGUMENTS;
    }

    QC_DEBUG( "Input data validation passed: type=%d, size=%u", bufferType,
              frame.buffer->sharedBuffer.size );

    return ret;
}

void SampleRadar::UpdatePerformanceMetrics( double processingTimeMs, bool success )
{
    std::lock_guard<std::mutex> lock( m_metricsMutex );

    m_frameCount++;

    if ( success )
    {
        m_totalProcessingTime += processingTimeMs;

        if ( processingTimeMs < m_minProcessingTime )
        {
            m_minProcessingTime = processingTimeMs;
        }

        if ( processingTimeMs > m_maxProcessingTime )
        {
            m_maxProcessingTime = processingTimeMs;
        }
    }
    else
    {
        m_errorCount++;
    }

    // Log periodic performance updates if enabled
    if ( m_enablePerformanceLog && ( m_frameCount % 100 == 0 ) )
    {
        double avgProcessingTime = m_totalProcessingTime / ( m_frameCount - m_errorCount );
        double errorRate = ( m_errorCount * 100.0 ) / m_frameCount;

        QC_INFO( "Performance update: frames=%lu, errors=%lu (%.1f%%), avg_time=%.2fms",
                 m_frameCount, m_errorCount, errorRate, avgProcessingTime );
    }
}

// Register the SampleRadar component with the factory system
// This enables dynamic creation via the command-line interface
REGISTER_SAMPLE( Radar, SampleRadar );

}   // namespace sample
}   // namespace QC
