// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef _QC_SAMPLE_RADAR_HPP_
#define _QC_SAMPLE_RADAR_HPP_

#include "QC/Common/DataTree.hpp"
#include "QC/Node/Radar.hpp"
#include "QC/sample/SampleIF.hpp"
#include "QC/sample/SharedBufferPool.hpp"

#include <atomic>
#include <chrono>
#include <thread>

namespace QC
{
namespace sample
{

/**
 * @brief Sample Radar component for radar data processing pipeline
 *
 * This class demonstrates how to use the QC Radar Node component within the
 * sample application framework. It provides a complete radar processing pipeline
 * that can read radar data from input topics, process it using hardware-accelerated
 * radar algorithms, and publish the results to output topics.
 *
 * The component follows the established QC Sample Application patterns:
 * - Configuration-driven setup using key-value pairs
 * - DataBroker pub/sub messaging for inter-component communication
 * - SharedBufferPool for efficient memory management
 * - Node integration for hardware acceleration
 * - Standard lifecycle management (Init/Start/Stop/Deinit)
 *
 * Key Features:
 * - Hardware-accelerated radar processing via QC Radar Node
 * - Configurable buffer sizes and processing parameters
 * - Zero-copy buffer sharing between pipeline components
 * - Performance monitoring and logging capabilities
 * - Robust error handling and recovery mechanisms
 * - Thread-safe operation with dedicated processing thread
 *
 * Typical Usage:
 * DataReader → SampleRadar → Recorder/TinyViz
 *
 * Configuration Parameters:
 * - max_input_buffer_size: Maximum input buffer size in bytes
 * - max_output_buffer_size: Maximum output buffer size in bytes
 * - service_name: Radar service device path (e.g., "/dev/radar0")
 * - timeout_ms: Processing timeout in milliseconds
 * - enable_performance_log: Enable/disable performance logging
 * - pool_size: Number of buffers in output buffer pool
 * - cache: Enable/disable cached memory allocation
 * - input_topic: Input topic name for radar data
 * - output_topic: Output topic name for processed results
 */
class SampleRadar : public SampleIF
{
public:
    /**
     * @brief Default constructor
     *
     * Initializes the SampleRadar instance with default values.
     * All actual initialization is performed in the Init() method.
     */
    SampleRadar();

    /**
     * @brief Destructor
     *
     * Ensures proper cleanup of resources. The actual cleanup
     * is performed in the Deinit() method following RAII principles.
     */
    ~SampleRadar();

    /**
     * @brief Initialize the SampleRadar component
     *
     * Performs complete initialization of the radar processing pipeline including:
     * - Configuration parsing and validation
     * - Radar Node initialization with hardware service setup
     * - Buffer pool allocation for input/output data
     * - DataBroker publisher/subscriber setup
     * - Performance monitoring initialization
     *
     * @param[in] name Unique instance name for this component
     * @param[in] config Configuration key-value map containing all parameters
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Initialization completed successfully
     * @retval QC_STATUS_BAD_ARGUMENTS Invalid configuration parameters
     * @retval QC_STATUS_FAIL Radar Node initialization failed
     * @retval QC_STATUS_NOMEM Memory allocation failed
     *
     * Success Conditions:
     * - All required configuration parameters are valid
     * - Radar service is accessible and responsive
     * - Buffer pools are successfully allocated
     * - DataBroker topics are properly configured
     *
     * Failure Conditions:
     * - Missing or invalid configuration parameters
     * - Radar service unavailable or unresponsive
     * - Insufficient memory for buffer allocation
     * - Topic name conflicts in DataBroker
     */
    QCStatus_e Init( std::string name, SampleConfig_t &config ) override;

    /**
     * @brief Start the radar processing pipeline
     *
     * Activates the radar processing pipeline by:
     * - Starting the underlying Radar Node component
     * - Launching the dedicated processing thread
     * - Beginning message subscription and processing
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Pipeline started successfully
     * @retval QC_STATUS_BAD_STATE Component not properly initialized
     * @retval QC_STATUS_FAIL Radar Node start failed
     *
     * Success Conditions:
     * - Component is in initialized state
     * - Radar Node starts successfully
     * - Processing thread launches without errors
     *
     * Failure Conditions:
     * - Component not initialized
     * - Radar Node fails to start
     * - Thread creation fails
     */
    QCStatus_e Start() override;

    /**
     * @brief Stop the radar processing pipeline
     *
     * Gracefully stops the radar processing pipeline by:
     * - Signaling the processing thread to terminate
     * - Waiting for thread completion (blocking join)
     * - Stopping the underlying Radar Node component
     * - Displaying performance statistics if enabled
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Pipeline stopped successfully
     * @retval QC_STATUS_FAIL Thread join failed or Radar Node stop failed
     *
     * Success Conditions:
     * - Processing thread terminates gracefully
     * - Radar Node stops successfully
     * - Performance statistics are displayed if enabled
     *
     * Failure Conditions:
     * - Thread join operation fails
     * - Radar Node stop operation fails
     */
    QCStatus_e Stop() override;

    /**
     * @brief Deinitialize the SampleRadar component
     *
     * Performs complete cleanup of the radar processing pipeline:
     * - Deinitializing the Radar Node component
     * - Automatic cleanup of buffer pools via destructors
     * - Automatic cleanup of DataBroker subscriptions via destructors
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Deinitialization completed successfully
     * @retval QC_STATUS_FAIL Radar Node deinitialization failed
     *
     * Success Conditions:
     * - Radar Node is properly deinitialized
     * - All resources are automatically cleaned up
     *
     * Failure Conditions:
     * - Radar Node deinitialization fails
     *
     * Note: Buffer pools and DataBroker subscriptions are automatically
     * cleaned up by their destructors, no explicit cleanup required.
     */
    QCStatus_e Deinit() override;

private:
    /**
     * @brief Parse and validate configuration parameters
     *
     * Extracts and validates all configuration parameters from the provided
     * configuration map. Sets up default values for optional parameters and
     * performs comprehensive validation of all settings.
     *
     * Parsed Configuration Parameters:
     * - Buffer size limits for input/output data
     * - Radar service configuration (device path, timeout)
     * - Performance and logging settings
     * - Memory allocation preferences (cached/uncached)
     * - DataBroker topic names for messaging
     *
     * @param[in] config Configuration key-value map
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Configuration parsed and validated successfully
     * @retval QC_STATUS_BAD_ARGUMENTS Invalid or missing required parameters
     *
     * Success Conditions:
     * - All required parameters are present and valid
     * - Buffer sizes are within reasonable limits
     * - Topic names are properly formatted
     * - Service configuration is accessible
     *
     * Failure Conditions:
     * - Required parameters are missing
     * - Parameter values are out of valid ranges
     * - Topic names are empty or malformed
     * - Service configuration is invalid
     */
    QCStatus_e ParseConfig( SampleConfig_t &config );

    /**
     * @brief Main processing thread function
     *
     * Implements the core radar data processing loop that:
     * - Continuously receives radar data from input topic
     * - Processes data through the Radar Node component
     * - Publishes processed results to output topic
     * - Handles errors and performance monitoring
     *
     * Processing Flow:
     * 1. Wait for input data on subscribed topic
     * 2. Acquire output buffer from pool
     * 3. Set up frame descriptor for Node processing
     * 4. Execute radar processing via Node component
     * 5. Publish results to output topic
     * 6. Handle errors and update performance metrics
     *
     * Thread Safety:
     * - Uses atomic variables for thread synchronization
     * - Employs proper locking for shared resources
     * - Handles graceful shutdown on stop signal
     *
     * Error Handling:
     * - Continues processing on recoverable errors
     * - Logs detailed error information for debugging
     * - Maintains processing statistics for monitoring
     *
     * Performance Monitoring:
     * - Tracks processing latency and throughput
     * - Monitors buffer pool utilization
     * - Records error rates and recovery statistics
     */
    void ThreadMain();

    /**
     * @brief Setup Radar Node configuration from parsed parameters
     *
     * Converts the parsed sample configuration into the JSON format
     * required by the Radar Node component. Creates a comprehensive
     * configuration tree that includes all radar processing parameters.
     *
     * Configuration Mapping:
     * - Sample config → DataTree structure → Node JSON
     * - Service parameters → Node service configuration
     * - Buffer limits → Node buffer management settings
     * - Performance settings → Node optimization parameters
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Configuration setup completed successfully
     * @retval QC_STATUS_BAD_ARGUMENTS Configuration conversion failed
     *
     * Success Conditions:
     * - All parameters are successfully mapped
     * - JSON configuration is valid and complete
     * - Node accepts the configuration
     *
     * Failure Conditions:
     * - Parameter mapping fails
     * - JSON generation produces invalid format
     * - Node rejects the configuration
     */
    QCStatus_e SetupNodeConfiguration();

    /**
     * @brief Process a single radar data frame
     *
     * Handles the processing of one radar data frame through the complete
     * pipeline including input validation, Node processing, and result
     * publication. Implements comprehensive error handling and performance
     * monitoring for each processing operation.
     *
     * Processing Steps:
     * 1. Validate input frame data and format
     * 2. Acquire output buffer from pool
     * 3. Set up frame descriptor for Node processing
     * 4. Execute radar processing via Node component
     * 5. Validate and publish processing results
     * 6. Update performance metrics and statistics
     *
     * @param[in] inputFrames Input radar data frames from DataBroker
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Frame processed successfully
     * @retval QC_STATUS_BAD_ARGUMENTS Invalid input frame data
     * @retval QC_STATUS_NOMEM Output buffer allocation failed
     * @retval QC_STATUS_FAIL Radar processing failed
     *
     * Success Conditions:
     * - Input frame data is valid and complete
     * - Output buffer is successfully acquired
     * - Radar processing completes without errors
     * - Results are successfully published
     *
     * Failure Conditions:
     * - Input frame data is corrupted or invalid
     * - No output buffers available in pool
     * - Radar processing encounters errors
     * - Result publication fails
     */
    QCStatus_e ProcessFrame( const DataFrames_t &inputFrames );

    /**
     * @brief Validate radar input data format and content
     *
     * Performs comprehensive validation of incoming radar data to ensure
     * it meets the requirements for processing. Checks data format,
     * size constraints, and content validity.
     *
     * Validation Checks:
     * - Buffer type compatibility (RAW, TENSOR, IMAGE)
     * - Data size within configured limits
     * - Buffer integrity and accessibility
     * - Format-specific validation rules
     *
     * @param[in] frame Input radar data frame to validate
     *
     * @return QCStatus_e
     * @retval QC_STATUS_OK Input data is valid and ready for processing
     * @retval QC_STATUS_BAD_ARGUMENTS Invalid data format or content
     * @retval QC_STATUS_INVALID_BUF Buffer integrity check failed
     *
     * Success Conditions:
     * - Data format matches expected radar input format
     * - Buffer size is within configured limits
     * - Buffer data is accessible and uncorrupted
     *
     * Failure Conditions:
     * - Unsupported or invalid data format
     * - Buffer size exceeds maximum limits
     * - Buffer data is corrupted or inaccessible
     */
    QCStatus_e ValidateInputData( const DataFrame_t &frame );

    /**
     * @brief Update performance metrics and statistics
     *
     * Collects and updates various performance metrics for monitoring
     * and optimization purposes. Tracks processing latency, throughput,
     * error rates, and provides periodic logging updates.
     *
     * Tracked Metrics:
     * - Frame processing latency (min/max/total)
     * - Total frame count and error count
     * - Error rates and success statistics
     * - Periodic performance logging (every 100 frames)
     *
     * Thread Safety:
     * - Uses mutex locking for thread-safe metric updates
     * - Atomic operations for frame and error counters
     *
     * @param[in] processingTimeMs Time taken to process the current frame (ignored for failed
     * frames)
     * @param[in] success Whether the processing was successful
     */
    void UpdatePerformanceMetrics( double processingTimeMs, bool success );

private:
    // Configuration parameters
    uint32_t m_maxInputBufferSize;    ///< Maximum input buffer size in bytes
    uint32_t m_maxOutputBufferSize;   ///< Maximum output buffer size in bytes
    std::string m_serviceName;        ///< Radar service device path
    uint32_t m_timeoutMs;             ///< Processing timeout in milliseconds
    bool m_enablePerformanceLog;      ///< Enable performance logging
    uint32_t m_poolSize;              ///< Output buffer pool size
    uint32_t m_bufferFlags;           ///< Buffer allocation flags (cached/uncached)
    std::string m_inputTopicName;     ///< Input topic name for DataBroker
    std::string m_outputTopicName;    ///< Output topic name for DataBroker

    // Node integration
    QC::Node::Radar m_radar;   ///< Radar Node component for processing
    DataTree m_dataTree;       ///< Configuration tree for Node setup

    // Buffer management
    SharedBufferPool m_outputPool;   ///< Output buffer pool for processed data

    // Messaging infrastructure
    DataSubscriber<DataFrames_t> m_sub;   ///< Subscriber for input radar data
    DataPublisher<DataFrames_t> m_pub;    ///< Publisher for processed results

    // Threading and synchronization
    std::thread m_thread;       ///< Dedicated processing thread
    std::atomic<bool> m_stop;   ///< Thread termination signal

    // Performance monitoring
    std::chrono::high_resolution_clock::time_point m_startTime;   ///< Processing start time
    uint64_t m_frameCount;                                        ///< Total processed frame count
    uint64_t m_errorCount;                                        ///< Total error count
    double m_totalProcessingTime;                                 ///< Cumulative processing time
    double m_minProcessingTime;                                   ///< Minimum processing time
    double m_maxProcessingTime;                                   ///< Maximum processing time
    std::mutex m_metricsMutex;   ///< Mutex for thread-safe metrics updates
    QCNodeID_t m_nodeId;
};

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_RADAR_HPP_
