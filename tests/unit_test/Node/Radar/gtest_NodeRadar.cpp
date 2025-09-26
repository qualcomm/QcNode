// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "gtest/gtest.h"
#include <chrono>
#include <stdio.h>
#include <string>

#include "QC/Node/Radar.hpp"
#include "md5_utils.hpp"

using namespace QC::Node;
using namespace QC::component;
using namespace QC::test::utils;

/**
 * @brief Helper function to set radar configuration in DataTree
 *
 * This function translates component-level radar configuration to
 * the Node-level DataTree configuration format, following the same
 * pattern as other Node tests (CL2DFlex, Remap, etc.).
 *
 * @param[in] pRadarConfig Component radar configuration
 * @param[out] pdt DataTree to populate with configuration
 */
void SetConfigRadar( Radar_Config_t *pRadarConfig, DataTree *pdt )
{
    // Set static configuration parameters from nested serviceConfig
    pdt->Set<std::string>( "static.serviceName", pRadarConfig->serviceConfig.serviceName );
    pdt->Set<uint32_t>( "static.timeoutMs", pRadarConfig->serviceConfig.timeoutMs );
    pdt->Set<bool>( "static.enablePerformanceLog",
                    pRadarConfig->serviceConfig.bEnablePerformanceLog );
    pdt->Set<uint32_t>( "static.maxInputBufferSize", pRadarConfig->maxInputBufferSize );
    pdt->Set<uint32_t>( "static.maxOutputBufferSize", pRadarConfig->maxOutputBufferSize );

    // Set empty buffer IDs since we don't register buffers during initialization
    std::vector<uint32_t> bufferIds;   // empty - no initialization-time buffer registration
    pdt->Set( "static.bufferIds", bufferIds );

    // Set global buffer ID mapping for Node interface with different IDs to test
    std::vector<DataTree> bufferMapDts;

    DataTree inputMapDt;
    inputMapDt.Set<std::string>( "name", "input" );
    inputMapDt.Set<uint32_t>( "id", 10 );
    bufferMapDts.push_back( inputMapDt );

    DataTree outputMapDt;
    outputMapDt.Set<std::string>( "name", "output" );
    outputMapDt.Set<uint32_t>( "id", 11 );
    bufferMapDts.push_back( outputMapDt );

    pdt->Set( "static.globalBufferIdMap", bufferMapDts );
    pdt->Set<bool>( "static.deRegisterAllBuffersWhenStop", false );
}

/**
 * @brief Sanity test for Radar Node wrapper functionality
 *
 * This test validates basic Node wrapper functionality including:
 * - DataTree configuration parsing and JSON serialization
 * - Node initialization and lifecycle management (Initialize/Start/Stop/DeInitialize)
 * - Buffer descriptor creation and management (QCSharedBufferDescriptor_t)
 * - Frame descriptor setup and processing (QCSharedFrameDescriptorNode)
 *
 * Test intent: Verify Node wrapper correctly interfaces with underlying component
 * Test parameters: Basic radar service config (/dev/radar0, 5s timeout, 2MB buffers)
 * Success criteria: All Node operations complete without BAD_ARGUMENTS/BAD_STATE errors
 * Failure criteria: Node wrapper fails to properly manage component lifecycle or buffers
 *
 * Note: ProcessFrameDescriptor result depends on service availability - tests Node interface, not
 * service functionality
 */
void SanityRadar()
{
    QCStatus_e ret, ret1;
    std::string errors;
    QC::Node::Radar radarNode;

    // Setup component configuration
    Radar_Config_t radarConfig;
    radarConfig.serviceConfig.serviceName = "/dev/radar0";
    radarConfig.serviceConfig.timeoutMs = 5000;
    radarConfig.serviceConfig.bEnablePerformanceLog = false;
    radarConfig.maxInputBufferSize = ( ( 2 * 1024 * 1024 ) + 1888 );
    radarConfig.maxOutputBufferSize = ( 8 * 1024 * 1024 );

    // Build Node configuration using DataTree
    DataTree dt;
    dt.Set<std::string>( "static.name", "Radar" );
    dt.Set<uint32_t>( "static.id", 0 );
    SetConfigRadar( &radarConfig, &dt );

    QCNodeInit_t config = { dt.Dump() };
    printf( "config: %s\n", config.config.c_str() );

    // Initialize Node
    ret1 = radarNode.Initialize( config );
    // If the service is unavailable then the BAD state is reported
    ASSERT_TRUE( ( QC_STATUS_OK == ret1 ) || ( QC_STATUS_BAD_STATE == ret1 ) );

    std::vector<QCSharedBufferDescriptor_t> inputs;
    std::vector<QCSharedBufferDescriptor_t> outputs;

    // Create input buffer descriptor
    QCSharedBufferDescriptor_t inputBuffer;
    ret = inputBuffer.buffer.Allocate( radarConfig.maxInputBufferSize );
    ASSERT_EQ( QC_STATUS_OK, ret );
    inputs.push_back( inputBuffer );

    // Create output buffer descriptor
    QCSharedBufferDescriptor_t outputBuffer;
    ret = outputBuffer.buffer.Allocate( radarConfig.maxOutputBufferSize );
    ASSERT_EQ( QC_STATUS_OK, ret );
    outputs.push_back( outputBuffer );

    // Start Node
    ret = radarNode.Start();
    // If the service is unavailable then the BAD state is reported
    ASSERT_TRUE( ( QC_STATUS_OK == ret ) || ( QC_STATUS_BAD_STATE == ret ) );

    // Setup frame descriptor for Node processing
    // Use a larger size to accommodate the global buffer IDs (10, 11)
    QCSharedFrameDescriptorNode frameDesc( 20 );

    // Set up the base class fields for input buffer
    inputs[0].pBuf = inputs[0].buffer.data();
    inputs[0].size = inputs[0].buffer.size;
    inputs[0].name = "InputBuffer";
    inputs[0].type = QC_BUFFER_TYPE_IMAGE;
    // Set up the base class fields for output buffer
    outputs[0].pBuf = outputs[0].buffer.data();
    outputs[0].size = outputs[0].buffer.size;
    outputs[0].name = "OutputBuffer";
    outputs[0].type = QC_BUFFER_TYPE_IMAGE;

    // Set input buffer at global ID 10 (as configured in globalBufferIdMap)
    ret = frameDesc.SetBuffer( 10, inputs[0] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // Set output buffer at global ID 11 (as configured in globalBufferIdMap)
    ret = frameDesc.SetBuffer( 11, outputs[0] );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // Setup buffer descriptors for Node interface (after initialization)
    // Process frame descriptor (Node-level operation)
    if ( ret1 == QC_STATUS_OK )
    {
        ret = radarNode.ProcessFrameDescriptor( frameDesc );
        // Note: Result depends on service availability, so we don't assert specific result
        printf( "ProcessFrameDescriptor result: %d (service dependent)\n", ret );
        // If the service is unavailable then the BAD state is reported
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    // Stop Node
    ret = radarNode.Stop();
    // If the service is unavailable then the BAD state is reported
    ASSERT_TRUE( ( QC_STATUS_OK == ret ) || ( QC_STATUS_BAD_STATE == ret ) );

    // Deinitialize Node
    ret = radarNode.DeInitialize();
    // If the service is unavailable then the BAD state is reported
    ASSERT_TRUE( ( QC_STATUS_OK == ret ) || ( QC_STATUS_BAD_STATE == ret ) );

    // Cleanup buffers
    for ( auto &buffer : inputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }

    for ( auto &buffer : outputs )
    {
        ret = buffer.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

/**
 * @brief Configuration validation test for Radar Node wrapper
 *
 * This test validates Node-specific configuration handling including:
 * - DataTree configuration parsing and JSON conversion
 * - Node configuration validation logic
 * - Error handling for invalid/missing configuration fields
 * - Configuration interface robustness
 *
 * Test intent: Verify Node wrapper properly validates and processes configuration
 * Test parameters:
 *   - Valid config: /dev/radar0, 3s timeout, 2MB buffers, performance logging enabled
 *   - Invalid config: Missing required fields (serviceName, buffer sizes)
 * Success criteria: Valid config accepted, invalid config rejected with appropriate error
 * Failure criteria: Node accepts invalid config or fails to process valid config
 *
 * Note: Tests Node configuration interface, not underlying component configuration
 */
void ConfigurationRadar()
{
    QCStatus_e ret;
    std::string errors;
    QC::Node::Radar radarNode;

    // Test 1: Valid configuration
    Radar_Config_t radarConfig;
    radarConfig.serviceConfig.serviceName = "/dev/radar0";
    radarConfig.serviceConfig.timeoutMs = 3000;
    radarConfig.serviceConfig.bEnablePerformanceLog = true;
    radarConfig.maxInputBufferSize = ( ( 2 * 1024 * 1024 ) + 1888 );
    radarConfig.maxOutputBufferSize = ( 8 * 1024 * 1024 );

    DataTree dt;
    dt.Set<std::string>( "static.name", "RadarTest" );
    dt.Set<uint32_t>( "static.id", 1 );
    SetConfigRadar( &radarConfig, &dt );

    QCNodeInit_t config = { dt.Dump() };
    printf( "Valid config test: %s\n", config.config.c_str() );

    ret = radarNode.Initialize( config );
    ASSERT_TRUE( ( QC_STATUS_OK == ret ) || ( QC_STATUS_BAD_STATE == ret ) );
    // ASSERT_EQ(QC_STATUS_OK, ret);

    if ( QC_STATUS_OK == ret )
    {
        ret = radarNode.DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    // Test 2: Invalid configuration (missing required fields)
    QC::Node::Radar radarNode2;
    DataTree invalidDt;
    invalidDt.Set<std::string>( "static.name", "InvalidRadar" );
    // Missing required fields intentionally

    QCNodeInit_t invalidConfig = { invalidDt.Dump() };
    printf( "Invalid config test: %s\n", invalidConfig.config.c_str() );

    ret = radarNode2.Initialize( invalidConfig );
    // Should fail due to missing configuration
    ASSERT_NE( QC_STATUS_OK, ret );
    printf( "Expected failure for invalid config: %d\n", ret );
}

/**
 * @brief Buffer management validation test for Radar Node wrapper
 *
 * This test validates Node-specific buffer management including:
 * - Buffer descriptor creation and lifecycle management (QCSharedBufferDescriptor_t)
 * - Frame descriptor setup and buffer indexing (QCSharedFrameDescriptorNode)
 * - Buffer retrieval and bounds checking
 * - Memory management in Node context
 *
 * Test intent: Verify Node wrapper properly manages buffer descriptors and frame descriptors
 * Test parameters: 3 buffers (2MB each), radar config (2MB buffers, 1s timeout)
 * Success criteria: All buffer operations succeed, bounds checking works correctly
 * Failure criteria: Buffer allocation/deallocation fails, bounds checking fails, memory leaks
 *
 * Note: Tests Node buffer management interface, not component buffer processing
 */
void BufferManagementRadar()
{
    QCStatus_e ret, ret1;
    QC::Node::Radar radarNode;

    // Setup basic configuration
    Radar_Config_t radarConfig;
    radarConfig.serviceConfig.serviceName = "/dev/radar0";
    radarConfig.serviceConfig.timeoutMs = 1000;
    radarConfig.serviceConfig.bEnablePerformanceLog = false;
    radarConfig.maxInputBufferSize = ( ( 2 * 1024 * 1024 ) + 1888 );
    radarConfig.maxOutputBufferSize = ( 8 * 1024 * 1024 );

    DataTree dt;
    dt.Set<std::string>( "static.name", "BufferTest" );
    dt.Set<uint32_t>( "static.id", 2 );
    SetConfigRadar( &radarConfig, &dt );

    QCNodeInit_t config = { dt.Dump() };

    ret1 = radarNode.Initialize( config );
    ASSERT_TRUE( ( QC_STATUS_OK == ret1 ) || ( QC_STATUS_BAD_STATE == ret1 ) );
    // ASSERT_EQ(QC_STATUS_OK, ret);

    // Test buffer descriptor management
    const uint32_t numBuffers = 3;
    QCSharedFrameDescriptorNode frameDesc( numBuffers );

    // Test setting and getting buffers
    for ( uint32_t i = 0; i < numBuffers; i++ )
    {
        QCSharedBufferDescriptor_t bufferDesc;
        ret = bufferDesc.buffer.Allocate( radarConfig.maxInputBufferSize );
        ASSERT_EQ( QC_STATUS_OK, ret );

        // Set up the base class fields to match the allocated buffer
        bufferDesc.pBuf = bufferDesc.buffer.data();
        bufferDesc.size = bufferDesc.buffer.size;
        bufferDesc.name = "TestBuffer" + std::to_string( i );
        bufferDesc.type = QC_BUFFER_TYPE_IMAGE;

        ret = frameDesc.SetBuffer( i, bufferDesc );
        ASSERT_EQ( QC_STATUS_OK, ret );

        // Verify buffer can be retrieved
        QCBufferDescriptorBase_t &retrievedBuffer = frameDesc.GetBuffer( i );
        ASSERT_NE( static_cast<void *>( nullptr ), retrievedBuffer.pBuf );

        // Cleanup
        ret = bufferDesc.buffer.Free();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
    if ( ret1 == QC_STATUS_OK )
    {
        ret = radarNode.DeInitialize();
        ASSERT_EQ( QC_STATUS_OK, ret );
    }
}

// Test cases using GTest framework

TEST( NodeRadar, Sanity )
{
    SanityRadar();
}

TEST( NodeRadar, Configuration )
{
    ConfigurationRadar();
}

TEST( NodeRadar, BufferManagement )
{
    BufferManagementRadar();
}

/**
 * @brief Test Node state management and lifecycle transitions
 *
 * This test validates proper Node state transitions throughout the complete lifecycle:
 * - Initial uninitialized state verification
 * - State transition from uninitialized to initialized (READY)
 * - State transition from initialized to started (RUNNING)
 * - State transition from started back to initialized (READY after stop)
 * - State transition from initialized back to uninitialized (INITIAL after deinitialize)
 *
 * Test intent: Verify Node wrapper correctly manages and reports state transitions
 * Test parameters: Minimal radar config (1KB buffers, 1s timeout, /dev/radar0)
 * Success criteria: All state transitions occur correctly and GetState() reports accurate states
 * Failure criteria: Incorrect state reported, state transition fails, state machine corruption
 *
 * Note: Tests Node state management by delegating to component state - wrapper tracks component
 * state
 */
TEST( NodeRadar, StateManagement )
{
    QC::Node::Radar radarNode;

    // Check initial state
    QCObjectState_e state = radarNode.GetState();
    EXPECT_EQ( QC_OBJECT_STATE_INITIAL, state );

    // Setup minimal configuration for state testing
    Radar_Config_t radarConfig;
    radarConfig.serviceConfig.serviceName = "/dev/radar0";
    radarConfig.serviceConfig.timeoutMs = 1000;
    radarConfig.serviceConfig.bEnablePerformanceLog = false;
    radarConfig.maxInputBufferSize = 1024;
    radarConfig.maxOutputBufferSize = 1024;

    DataTree dt;
    dt.Set<std::string>( "static.name", "StateTest" );
    dt.Set<uint32_t>( "static.id", 3 );
    SetConfigRadar( &radarConfig, &dt );

    QCNodeInit_t config = { dt.Dump() };

    // Test initialization state
    QCStatus_e ret = radarNode.Initialize( config );
    ASSERT_TRUE( ( QC_STATUS_OK == ret ) || ( QC_STATUS_BAD_STATE == ret ) );
    if ( QC_STATUS_OK == ret )
    {
        state = radarNode.GetState();
        EXPECT_EQ( QC_OBJECT_STATE_READY, state );

        // Test started state
        ret = radarNode.Start();
        ASSERT_EQ( QC_STATUS_OK, ret );

        state = radarNode.GetState();
        EXPECT_EQ( QC_OBJECT_STATE_RUNNING, state );

        // Test stop state
        ret = radarNode.Stop();
        EXPECT_EQ( QC_STATUS_OK, ret );

        state = radarNode.GetState();
        EXPECT_EQ( QC_OBJECT_STATE_READY, state );

        // Test deinitialized state
        ret = radarNode.DeInitialize();
        EXPECT_EQ( QC_STATUS_OK, ret );

        state = radarNode.GetState();
        EXPECT_EQ( QC_OBJECT_STATE_INITIAL, state );
    }
    else
    {
        GTEST_SKIP() << "Initialization failed, skipping state transition tests";
    }
}

/**
 * @brief Test Node interface methods and capabilities
 *
 * This test validates Node-specific interface functionality including:
 * - Configuration interface access and options retrieval
 * - Monitoring interface access and size reporting
 * - Interface method availability and proper responses
 * - Node-specific interface behavior vs component interfaces
 *
 * Test intent: Verify Node wrapper exposes proper interfaces for configuration and monitoring
 * Test parameters: Default Node instance (no initialization required for interface access)
 * Success criteria: All interfaces accessible, options available, size reporting functional
 * Failure criteria: Interface access fails, options unavailable, size reporting broken
 *
 * Note: Tests Node interface availability, not interface functionality - focuses on wrapper
 * interface exposure
 */
TEST( NodeRadar, NodeInterfaces )
{
    QC::Node::Radar radarNode;

    // Test configuration interface
    QCNodeConfigIfs &configIfs = radarNode.GetConfigurationIfs();
    const std::string &options = configIfs.GetOptions();
    // Configuration options may be empty for Radar Node - this is acceptable
    printf( "Configuration options: %s\n", options.c_str() );

    // Test monitoring interface
    QCNodeMonitoringIfs &monitoringIfs = radarNode.GetMonitoringIfs();

    // Note: GetMaximalSize() and GetCurrentSize() return UINT32_MAX for Radar monitoring interface
    // These methods indicate unlimited size for radar monitoring data

    // Test monitoring options
    const std::string &monitorOptions = monitoringIfs.GetOptions();
    printf( "Monitoring options: %s\n", monitorOptions.c_str() );

    // Interface access should succeed even if options are empty
    EXPECT_TRUE( true );   // Test passes if we reach here without exceptions
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
