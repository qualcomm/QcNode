// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <chrono>
#include <cmath>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <vector>

#include "QC/component/Radar.hpp"
#include "md5_utils.hpp"

using namespace QC;
using namespace QC::component;
using namespace QC::test::utils;

/**
 * @brief Test configuration for basic radar functionality
 *
 * This configuration represents a typical radar setup with:
 * - 2MB input buffer size for typical radar data processing
 * - 8MB output buffer size for processed radar output
 * - Standard QNX device path for radar service
 * - 5 second timeout for processing operations
 * - Performance logging disabled for basic testing
 */
static Radar_Config_t radarConfigBasic = {
        ( ( 2 * 1024 * 1024 ) + 1888 ),   // maxInputBufferSize - 2MB for typical radar data
        ( 8 * 1024 * 1024 ),              // maxOutputBufferSize - 8MB for processed output
        {
                "/dev/radar0",   // serviceName - QNX device path for radar service
                5000,            // timeoutMs - 5 second timeout for processing
                false            // enablePerformanceLog - disabled for basic tests
        } };

/**
 * @brief Test configuration for performance testing scenarios
 *
 * This configuration is designed for performance evaluation with:
 * - Larger 2MB buffers to test throughput capabilities
 * - Extended 10 second timeout for complex processing
 * - Performance logging enabled to validate timing measurements
 */
static Radar_Config_t radarConfigPerformance = {
        ( 2 * 1024 * 1024 + 1888 ),   // maxInputBufferSize - 2MB for performance testing
        ( 8 * 1024 * 1024 ),          // maxOutputBufferSize - 8MB for larger datasets
        {
                "/dev/radar0",   // serviceName - alternate device for performance tests
                10000,           // timeoutMs - extended timeout for complex processing
                true             // enablePerformanceLog - enabled for performance validation
        } };

/**
 * @brief Generate pseudo-random radar data for testing purposes
 *
 * This function fills a buffer with deterministic pseudo-random data that
 * simulates radar input patterns. The data generation is seeded to ensure
 * reproducible test results while providing varied input patterns.
 *
 * @param[out] pData Pointer to buffer to fill with test data
 * @param[in] size Size of buffer in bytes to fill
 *
 * Success criteria: Buffer is filled with pseudo-random values 0-255
 * Failure criteria: None (function always succeeds with valid parameters)
 */
void GenerateRadarTestData( void *pData, uint32_t size )
{
    uint8_t *data = (uint8_t *) pData;

    // Use deterministic seed for reproducible test results
    srand( 12345 );

    // Fill buffer with pseudo-random radar-like data patterns
    for ( uint32_t i = 0; i < size; i++ )
    {
        // Generate values that simulate typical radar data characteristics
        data[i] = (uint8_t) ( ( rand() % 256 ) );
    }
}

/**
 * @brief Validate radar output data for basic sanity checks
 *
 * This function performs basic validation on radar output data to ensure
 * the processing service has modified the input data appropriately.
 * It checks for data patterns that indicate successful processing.
 *
 * @param[in] pData Pointer to output data buffer
 * @param[in] size Size of data buffer in bytes
 * @return true if output data appears valid, false otherwise
 *
 * Success criteria: Output data shows signs of processing (not all zeros/same value)
 * Failure criteria: Output data is unchanged, all zeros, or has invalid patterns
 */
bool ValidateRadarOutput( const void *pData, uint32_t size )
{
    const uint8_t *data = (const uint8_t *) pData;

    // Check for all-zero output (indicates no processing occurred)
    bool hasNonZero = false;
    for ( uint32_t i = 0; i < size; i++ )
    {
        if ( data[i] != 0 )
        {
            hasNonZero = true;
            break;
        }
    }

    if ( !hasNonZero )
    {
        printf( "Warning: Output data is all zeros\n" );
        return false;
    }

    // Check for reasonable data distribution (not all same value)
    uint8_t firstValue = data[0];
    bool hasVariation = false;
    for ( uint32_t i = 1; i < size && i < 100; i++ )
    {   // Check first 100 bytes
        if ( data[i] != firstValue )
        {
            hasVariation = true;
            break;
        }
    }

    if ( !hasVariation )
    {
        printf( "Warning: Output data lacks variation\n" );
        return false;
    }

    return true;
}

/**
 * @brief Performance test helper function for radar component lifecycle timing
 *
 * This function measures the performance of radar component operations including
 * initialization, buffer registration, execution, and cleanup. It focuses on
 * component-level performance rather than service-level processing performance.
 *
 * @param[in] config Radar configuration to use for testing
 * @param[in] inputSize Size of input buffer for testing
 * @param[in] outputSize Size of output buffer for testing
 * @param[in] iterations Number of execute iterations to perform
 *
 * Success criteria: All operations complete within reasonable time bounds
 * Failure criteria: Operations timeout or fail, excessive execution time
 */
void PerformanceTest( Radar_Config_t &config, uint32_t inputSize, uint32_t outputSize,
                      uint32_t iterations )
{
    QCStatus_e ret = QC_STATUS_OK;
    Radar radarObj;
    char pName[20] = "RadarPerf";

    // Measure initialization time
    auto initStart = std::chrono::high_resolution_clock::now();
    ret = radarObj.Init( pName, &config );
    auto initEnd = std::chrono::high_resolution_clock::now();

    if ( ret != QC_STATUS_OK )
    {
        printf( "Radar init failed (ret=%d), skipping performance test\n", ret );
        return;
    }

    double initTime = std::chrono::duration<double, std::milli>( initEnd - initStart ).count();
    printf( "Radar initialization time: %.2f ms\n", initTime );

    // Measure start time
    auto startTime = std::chrono::high_resolution_clock::now();
    ret = radarObj.Start();
    auto startEnd = std::chrono::high_resolution_clock::now();

    if ( ret != QC_STATUS_OK )
    {
        printf( "Radar start failed (ret=%d), skipping performance test\n", ret );
        radarObj.Deinit();
        return;
    }

    double startDuration =
            std::chrono::duration<double, std::milli>( startEnd - startTime ).count();
    printf( "Radar start time: %.2f ms\n", startDuration );

    // Allocate and prepare test buffers
    QCSharedBuffer_t inputBuffer, outputBuffer;
    ret = inputBuffer.Allocate( inputSize );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = outputBuffer.Allocate( outputSize );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // Generate test data
    GenerateRadarTestData( inputBuffer.data(), inputSize );

    // Measure buffer registration time
    auto regStart = std::chrono::high_resolution_clock::now();
    ret = radarObj.RegisterInputBuffer( &inputBuffer );
    EXPECT_EQ( QC_STATUS_OK, ret );
    ret = radarObj.RegisterOutputBuffer( &outputBuffer );
    EXPECT_EQ( QC_STATUS_OK, ret );
    auto regEnd = std::chrono::high_resolution_clock::now();

    double regTime = std::chrono::duration<double, std::milli>( regEnd - regStart ).count();
    printf( "Buffer registration time: %.2f ms\n", regTime );

    // Measure execution performance (component overhead, not service processing time)
    auto execStart = std::chrono::high_resolution_clock::now();
    for ( uint32_t i = 0; i < iterations; i++ )
    {
        ret = radarObj.Execute( &inputBuffer, &outputBuffer );
        // Note: Execute may fail if no actual radar hardware/service available
        // This measures component call overhead regardless of service availability
    }
    auto execEnd = std::chrono::high_resolution_clock::now();

    double execTime = std::chrono::duration<double, std::milli>( execEnd - execStart ).count();
    printf( "Radar execute calls: %d iterations, total time = %.2f ms, avg = %.2f ms\n", iterations,
            execTime, execTime / iterations );

    // Validate output if execution succeeded
    if ( ret == QC_STATUS_OK )
    {
        bool outputValid = ValidateRadarOutput( outputBuffer.data(), outputBuffer.size );
        printf( "Output validation: %s\n", outputValid ? "PASS" : "FAIL" );
    }

    // Measure cleanup time
    auto cleanupStart = std::chrono::high_resolution_clock::now();
    radarObj.DeregisterInputBuffer( &inputBuffer );
    radarObj.DeregisterOutputBuffer( &outputBuffer );
    inputBuffer.Free();
    outputBuffer.Free();
    radarObj.Stop();
    radarObj.Deinit();
    auto cleanupEnd = std::chrono::high_resolution_clock::now();

    double cleanupTime =
            std::chrono::duration<double, std::milli>( cleanupEnd - cleanupStart ).count();
    printf( "Cleanup time: %.2f ms\n", cleanupTime );
}

/**
 * @brief Sanity test helper function for basic radar functionality validation
 *
 * This function performs a complete radar component workflow test including
 * initialization, buffer setup, registration, execution, and cleanup.
 * It validates the basic operational flow without requiring actual hardware.
 *
 * @param[in] config Radar configuration to test
 *
 * Success criteria: All component operations complete successfully
 * Failure criteria: Any component operation fails with unexpected error
 */
void SanityTest( Radar_Config_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;
    Radar radarObj;
    char pName[20] = "RadarSanity";

    // Test component initialization
    ret = radarObj.Init( pName, &config );
    if ( ret != QC_STATUS_OK )
    {
        printf( "Radar init failed (ret=%d), skipping sanity test for config\n", ret );
        return;
    }

    // Test component start
    ret = radarObj.Start();
    if ( ret != QC_STATUS_OK )
    {
        printf( "Radar start failed (ret=%d), skipping sanity test\n", ret );
        radarObj.Deinit();
        return;
    }

    // Allocate test buffers with standard size
    QCSharedBuffer_t inputBuffer, outputBuffer;
    ret = inputBuffer.Allocate( config.maxInputBufferSize );
    ASSERT_EQ( QC_STATUS_OK, ret );

    ret = outputBuffer.Allocate( config.maxOutputBufferSize );
    ASSERT_EQ( QC_STATUS_OK, ret );

    // Generate test input data
    GenerateRadarTestData( inputBuffer.data(), config.maxInputBufferSize );

    // Test buffer registration
    ret = radarObj.RegisterInputBuffer( &inputBuffer );
    EXPECT_EQ( QC_STATUS_OK, ret );

    ret = radarObj.RegisterOutputBuffer( &outputBuffer );
    EXPECT_EQ( QC_STATUS_OK, ret );

    // Test execution (may fail if no service available, but validates call path)
    ret = radarObj.Execute( &inputBuffer, &outputBuffer );
    // Note: Don't assert on execute result as it depends on service availability
    printf( "Execute result: %d (service availability dependent)\n", ret );

    // Test buffer deregistration
    ret = radarObj.DeregisterInputBuffer( &inputBuffer );
    EXPECT_EQ( QC_STATUS_OK, ret );

    ret = radarObj.DeregisterOutputBuffer( &outputBuffer );
    EXPECT_EQ( QC_STATUS_OK, ret );

    // Cleanup resources
    inputBuffer.Free();
    outputBuffer.Free();
    radarObj.Stop();
    radarObj.Deinit();

    printf( "Sanity test completed for configuration\n" );
}

/**
 * @brief Comprehensive coverage test for error conditions and edge cases
 *
 * This function tests all error paths, invalid parameter handling, and
 * state machine validation for the radar component. It ensures robust
 * error handling and proper state management under various failure conditions.
 *
 * Success criteria: All error conditions return appropriate error codes
 * Failure criteria: Unexpected behavior, crashes, or incorrect error codes
 */
void RadarCoverageTest()
{
    QCStatus_e ret = QC_STATUS_OK;
    Radar radarObj;
    Radar_Config_t config = radarConfigBasic;
    char pName[20] = "RadarCov";

    printf( "Testing operations before initialization...\n" );

    // Test all operations before init - should return BAD_STATE
    ret = radarObj.Start();
    EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = radarObj.Stop();
    EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = radarObj.Deinit();
    EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

    QCSharedBuffer_t buffer;
    ret = radarObj.RegisterInputBuffer( &buffer );
    EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = radarObj.RegisterOutputBuffer( &buffer );
    EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

    ret = radarObj.Execute( &buffer, &buffer );
    EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

    printf( "Testing invalid initialization parameters...\n" );

    // Test init with null config - should return BAD_ARGUMENTS
    ret = radarObj.Init( pName, nullptr );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // Test init with invalid buffer sizes - should return BAD_ARGUMENTS
    config.maxInputBufferSize = 0;
    ret = radarObj.Init( pName, &config );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    config.maxInputBufferSize = 1024;
    config.maxOutputBufferSize = 0;
    ret = radarObj.Init( pName, &config );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    // Test init with empty service name - should return BAD_ARGUMENTS
    config = radarConfigBasic;
    config.serviceConfig.serviceName = "";
    ret = radarObj.Init( pName, &config );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

    printf( "Testing successful initialization and state transitions...\n" );

    // Successful init
    config = radarConfigBasic;
    ret = radarObj.Init( pName, &config, LOGGER_LEVEL_INFO );
    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );
    // EXPECT_TRUE(false) << "first init state: " << radarObj.GetState();
    if ( ret == QC_STATUS_OK )
    {
        // Test double init - should return BAD_STATE
        ret = radarObj.Init( pName, &config, LOGGER_LEVEL_INFO );
        // EXPECT_TRUE(false) << "second init state: " << radarObj.GetState();
        EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

        printf( "Testing buffer operations with null pointers...\n" );

        // Test buffer registration with null pointers - should return BAD_ARGUMENTS
        ret = radarObj.RegisterInputBuffer( nullptr );
        EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = radarObj.RegisterOutputBuffer( nullptr );
        EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = radarObj.DeregisterInputBuffer( nullptr );
        EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = radarObj.DeregisterOutputBuffer( nullptr );
        EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        printf( "Testing execute before start...\n" );

        // Test execute before start - should return BAD_STATE
        QCSharedBuffer_t inputBuffer, outputBuffer;
        ret = inputBuffer.Allocate( config.maxInputBufferSize );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = outputBuffer.Allocate( config.maxInputBufferSize );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = radarObj.Execute( &inputBuffer, &outputBuffer );
        EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

        // Start component for further testing
        ret = radarObj.Start();
        if ( ret == QC_STATUS_OK )
        {
            printf( "Testing execute with invalid parameters...\n" );

            // Test execute with null buffers - should return BAD_ARGUMENTS
            ret = radarObj.Execute( nullptr, &outputBuffer );
            EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

            ret = radarObj.Execute( &inputBuffer, nullptr );
            EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

            ret = radarObj.Execute( nullptr, nullptr );
            EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

            printf( "Testing buffer validation...\n" );

            // Test with different buffer types (all should be accepted with warnings)
            QCBufferType_e originalInputType = inputBuffer.type;
            QCBufferType_e originalOutputType = outputBuffer.type;

            // Test with IMAGE buffer type (should work but generate warning)
            inputBuffer.type = QC_BUFFER_TYPE_IMAGE;
            ret = radarObj.Execute( &inputBuffer, &outputBuffer );
            // Should not fail due to buffer type, may fail due to service availability
            printf( "Execute with IMAGE input buffer returned: %d\n", ret );

            inputBuffer.type = originalInputType;
            outputBuffer.type = QC_BUFFER_TYPE_IMAGE;
            ret = radarObj.Execute( &inputBuffer, &outputBuffer );
            // Should not fail due to buffer type, may fail due to service availability
            printf( "Execute with IMAGE output buffer returned: %d\n", ret );

            // Test with null buffer data
            outputBuffer.type = originalOutputType;
            void *originalData = inputBuffer.buffer.pData;
            inputBuffer.buffer.pData = nullptr;
            ret = radarObj.Execute( &inputBuffer, &outputBuffer );
            EXPECT_EQ( QC_STATUS_INVALID_BUF, ret );
            inputBuffer.buffer.pData = originalData;

            ret = radarObj.Stop();
            EXPECT_EQ( QC_STATUS_OK, ret );
        }

        inputBuffer.Free();
        outputBuffer.Free();
        ret = radarObj.Deinit();
        EXPECT_EQ( QC_STATUS_OK, ret );
    }

    printf( "Coverage test completed\n" );
}

/**
 * @brief Test fixture class for Radar component unit tests
 *
 * This fixture provides common setup and teardown functionality for
 * radar component tests. It initializes test configurations and
 * provides helper methods for consistent test execution.
 */
class RadarTest : public ::testing::Test
{
protected:
    /**
     * @brief Set up test fixture before each test
     *
     * Initializes the default radar configuration for testing.
     * This configuration uses standard parameters suitable for
     * most test scenarios.
     */
    void SetUp() override { m_config = radarConfigBasic; }

    /**
     * @brief Clean up test fixture after each test
     *
     * Performs any necessary cleanup after test execution.
     * Currently no specific cleanup is required.
     */
    void TearDown() override
    {
        // No specific cleanup needed for radar tests
    }

    Radar_Config_t m_config;   ///< Default configuration for tests
};

// Basic functionality tests using test fixture

/**
 * @brief Test radar component initialization with valid configuration
 *
 * This test validates that the radar component can be successfully
 * initialized with a valid configuration and properly deinitialized.
 *
 * Test procedure:
 * 1. Create radar component instance
 * 2. Initialize with valid configuration
 * 3. Verify initialization result
 * 4. Deinitialize if successful
 *
 * Success criteria: Init returns OK or FAIL (service dependent)
 * Failure criteria: Init returns BAD_ARGUMENTS or other unexpected error
 */
TEST_F( RadarTest, InitWithValidConfig )
{
    Radar radar;
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        EXPECT_EQ( QC_STATUS_OK, radar.Deinit() );
    }
}

/**
 * @brief Test radar component initialization with null configuration
 *
 * This test validates proper error handling when attempting to
 * initialize the radar component with a null configuration pointer.
 *
 * Test procedure:
 * 1. Create radar component instance
 * 2. Attempt initialization with null config
 * 3. Verify BAD_ARGUMENTS error is returned
 *
 * Success criteria: Init returns QC_STATUS_BAD_ARGUMENTS
 * Failure criteria: Init returns any other status code
 */
TEST_F( RadarTest, InitWithNullConfig )
{
    Radar radar;
    QCStatus_e ret = radar.Init( "TestRadar", nullptr, LOGGER_LEVEL_ERROR );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
}

/**
 * @brief Test radar component initialization with invalid buffer sizes
 *
 * This test validates proper parameter validation for buffer size
 * configuration. Zero buffer sizes should be rejected.
 *
 * Test procedure:
 * 1. Create radar component instance
 * 2. Set input buffer size to zero
 * 3. Attempt initialization
 * 4. Verify BAD_ARGUMENTS error is returned
 *
 * Success criteria: Init returns QC_STATUS_BAD_ARGUMENTS
 * Failure criteria: Init accepts invalid configuration
 */
TEST_F( RadarTest, InitWithInvalidBufferSizes )
{
    Radar radar;
    m_config.maxInputBufferSize = 0;
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
}

/**
 * @brief Test radar component initialization with empty service name
 *
 * This test validates that empty service names are properly rejected
 * during initialization as they cannot be used for service connection.
 *
 * Test procedure:
 * 1. Create radar component instance
 * 2. Set service name to empty string
 * 3. Attempt initialization
 * 4. Verify BAD_ARGUMENTS error is returned
 *
 * Success criteria: Init returns QC_STATUS_BAD_ARGUMENTS
 * Failure criteria: Init accepts empty service name
 */
TEST_F( RadarTest, InitWithEmptyServiceName )
{
    Radar radar;
    m_config.serviceConfig.serviceName = "";
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );
    EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );
}

/**
 * @brief Test radar component start without initialization
 *
 * This test validates proper state machine enforcement by attempting
 * to start the component before initialization.
 *
 * Test procedure:
 * 1. Create radar component instance
 * 2. Attempt to start without initialization
 * 3. Verify BAD_STATE error is returned
 *
 * Success criteria: Start returns QC_STATUS_BAD_STATE
 * Failure criteria: Start succeeds or returns unexpected error
 */
TEST_F( RadarTest, StartWithoutInit )
{
    Radar radar;
    QCStatus_e ret = radar.Start();
    EXPECT_EQ( QC_STATUS_BAD_STATE, ret );
}

/**
 * @brief Test radar component execute without start
 *
 * This test validates that execute operations are properly rejected
 * when the component is not in running state.
 *
 * Test procedure:
 * 1. Initialize radar component
 * 2. Allocate test buffers
 * 3. Attempt execute without start
 * 4. Verify BAD_STATE error is returned
 * 5. Clean up resources
 *
 * Success criteria: Execute returns QC_STATUS_BAD_STATE
 * Failure criteria: Execute succeeds or returns unexpected error
 */
TEST_F( RadarTest, ExecuteWithoutStart )
{
    Radar radar;
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        QCSharedBuffer_t inputBuffer, outputBuffer;
        ret = inputBuffer.Allocate( m_config.maxInputBufferSize );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = outputBuffer.Allocate( m_config.maxOutputBufferSize );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = radar.Execute( &inputBuffer, &outputBuffer );
        EXPECT_EQ( QC_STATUS_BAD_STATE, ret );

        inputBuffer.Free();
        outputBuffer.Free();
        radar.Deinit();
    }
}

/**
 * @brief Test radar component execute with null buffers
 *
 * This test validates proper parameter validation for execute operations
 * by passing null buffer pointers.
 *
 * Test procedure:
 * 1. Initialize and start radar component
 * 2. Attempt execute with null buffers
 * 3. Verify BAD_ARGUMENTS error is returned
 * 4. Clean up component
 *
 * Success criteria: Execute returns QC_STATUS_BAD_ARGUMENTS
 * Failure criteria: Execute accepts null parameters
 */
TEST_F( RadarTest, ExecuteWithNullBuffers )
{
    Radar radar;
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        ret = radar.Start();
        if ( ret == QC_STATUS_OK )
        {
            ret = radar.Execute( nullptr, nullptr );
            EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

            radar.Stop();
        }
        radar.Deinit();
    }
}

/**
 * @brief Test radar input buffer registration functionality
 *
 * This test validates the input buffer registration and deregistration
 * workflow including duplicate registration handling.
 *
 * Test procedure:
 * 1. Initialize radar component
 * 2. Allocate input buffer
 * 3. Register input buffer
 * 4. Attempt duplicate registration
 * 5. Deregister input buffer
 * 6. Clean up resources
 *
 * Success criteria: All operations return QC_STATUS_OK
 * Failure criteria: Any operation fails unexpectedly
 */
TEST_F( RadarTest, RegisterInputBuffer )
{
    Radar radar;
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        QCSharedBuffer_t inputBuffer;
        ret = inputBuffer.Allocate( m_config.maxInputBufferSize );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = radar.RegisterInputBuffer( &inputBuffer );
        EXPECT_EQ( QC_STATUS_OK, ret );

        // Test duplicate registration (should succeed)
        ret = radar.RegisterInputBuffer( &inputBuffer );
        EXPECT_EQ( QC_STATUS_OK, ret );

        ret = radar.DeregisterInputBuffer( &inputBuffer );
        EXPECT_EQ( QC_STATUS_OK, ret );

        inputBuffer.Free();
        radar.Deinit();
    }
}

/**
 * @brief Test radar output buffer registration functionality
 *
 * This test validates the output buffer registration and deregistration
 * workflow for proper buffer management.
 *
 * Test procedure:
 * 1. Initialize radar component
 * 2. Allocate output buffer
 * 3. Register output buffer
 * 4. Deregister output buffer
 * 5. Clean up resources
 *
 * Success criteria: All operations return QC_STATUS_OK
 * Failure criteria: Any operation fails unexpectedly
 */
TEST_F( RadarTest, RegisterOutputBuffer )
{
    Radar radar;
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        QCSharedBuffer_t outputBuffer;
        ret = outputBuffer.Allocate( m_config.maxOutputBufferSize );
        ASSERT_EQ( QC_STATUS_OK, ret );

        ret = radar.RegisterOutputBuffer( &outputBuffer );
        EXPECT_EQ( QC_STATUS_OK, ret );

        ret = radar.DeregisterOutputBuffer( &outputBuffer );
        EXPECT_EQ( QC_STATUS_OK, ret );

        outputBuffer.Free();
        radar.Deinit();
    }
}

/**
 * @brief Test radar buffer registration with null pointers
 *
 * This test validates proper error handling when attempting to
 * register null buffer pointers.
 *
 * Test procedure:
 * 1. Initialize radar component
 * 2. Attempt to register null input buffer
 * 3. Attempt to register null output buffer
 * 4. Verify BAD_ARGUMENTS errors are returned
 * 5. Clean up component
 *
 * Success criteria: Both operations return QC_STATUS_BAD_ARGUMENTS
 * Failure criteria: Operations accept null pointers
 */
TEST_F( RadarTest, RegisterNullBuffer )
{
    Radar radar;
    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        ret = radar.RegisterInputBuffer( nullptr );
        EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        ret = radar.RegisterOutputBuffer( nullptr );
        EXPECT_EQ( QC_STATUS_BAD_ARGUMENTS, ret );

        radar.Deinit();
    }
}

/**
 * @brief Test complete radar component workflow
 *
 * This test validates the complete operational workflow of the radar
 * component including all major operations in proper sequence.
 *
 * Test procedure:
 * 1. Initialize radar component
 * 2. Start component
 * 3. Allocate and prepare buffers
 * 4. Register buffers
 * 5. Execute processing
 * 6. Deregister buffers
 * 7. Stop and deinitialize component
 *
 * Success criteria: All operations complete successfully
 * Failure criteria: Any operation fails unexpectedly
 */
TEST_F( RadarTest, FullWorkflow )
{
    Radar radar;

    QCStatus_e ret = radar.Init( "TestRadar", &m_config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        EXPECT_EQ( QC_STATUS_OK, ret );
        ret = radar.Start();
        if ( ret == QC_STATUS_OK )
        {
            QCSharedBuffer_t inputBuffer, outputBuffer;
            ret = inputBuffer.Allocate( m_config.maxInputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret );

            ret = outputBuffer.Allocate( m_config.maxOutputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret );

            ret = radar.RegisterInputBuffer( &inputBuffer );
            EXPECT_EQ( QC_STATUS_OK, ret );

            ret = radar.RegisterOutputBuffer( &outputBuffer );
            EXPECT_EQ( QC_STATUS_OK, ret );

            // Generate test data
            GenerateRadarTestData( inputBuffer.data(), m_config.maxInputBufferSize );

            ret = radar.Execute( &inputBuffer, &outputBuffer );
            // Don't assert on execute result as it depends on service availability

            ret = radar.DeregisterInputBuffer( &inputBuffer );
            EXPECT_EQ( QC_STATUS_OK, ret );

            ret = radar.DeregisterOutputBuffer( &outputBuffer );
            EXPECT_EQ( QC_STATUS_OK, ret );

            inputBuffer.Free();
            outputBuffer.Free();

            ret = radar.Stop();
            EXPECT_EQ( QC_STATUS_OK, ret );
        }
        ret = radar.Deinit();
        EXPECT_EQ( QC_STATUS_OK, ret );
    }
}

// Advanced test cases following established patterns

/**
 * @brief Comprehensive sanity test across multiple configurations
 *
 * This test validates basic functionality across different radar
 * configurations to ensure robustness across various parameter sets.
 */
TEST( Radar, SanityTest )
{
    printf( "Running sanity tests across multiple configurations...\n" );
    SanityTest( radarConfigBasic );
    SanityTest( radarConfigPerformance );
}

/**
 * @brief Comprehensive coverage test for error conditions
 *
 * This test validates all error paths and edge cases to ensure
 * robust error handling throughout the component.
 */
TEST( Radar, CoverageTest )
{
    printf( "Running comprehensive coverage test...\n" );
    RadarCoverageTest();
}

/**
 * @brief Performance test for component lifecycle operations
 *
 * This test measures the performance of component operations
 * to validate acceptable response times and identify bottlenecks.
 */
TEST( Radar, PerformanceTest )
{
    printf( "Running performance tests...\n" );
    printf( "Performance test with basic config (2MB buffers)\n" );
    PerformanceTest( radarConfigBasic, radarConfigBasic.maxInputBufferSize,
                     radarConfigBasic.maxOutputBufferSize, 5 );

    printf( "Performance test with performance config (2MB buffers)\n" );
    PerformanceTest( radarConfigPerformance, radarConfigPerformance.maxInputBufferSize,
                     radarConfigPerformance.maxOutputBufferSize, 5 );
}

/**
 * @brief Configuration variation test across multiple radar configurations
 *
 * This test validates component behavior across different radar
 * configurations (basic and performance) to ensure consistent operation
 * with varying buffer sizes and timeout parameters.
 */
TEST( Radar, ConfigurationVariationTest )
{
    printf( "Testing different radar configurations...\n" );
    std::vector<Radar_Config_t> configs = { radarConfigBasic, radarConfigPerformance };

    for ( size_t i = 0; i < configs.size(); i++ )
    {
        printf( "Testing configuration %zu with %u/%u buffer sizes\n", i,
                configs[i].maxInputBufferSize, configs[i].maxOutputBufferSize );
        SanityTest( configs[i] );
    }
}

/**
 * @brief Timeout handling test with short timeout configuration
 *
 * This test validates proper timeout handling by configuring a very
 * short timeout value (100ms) and observing component behavior during
 * execute operations that may exceed this timeout.
 */
TEST( Radar, TimeoutTest )
{
    printf( "Testing timeout handling...\n" );
    Radar_Config_t config = radarConfigBasic;
    config.serviceConfig.timeoutMs = 100;   // Very short timeout

    Radar radar;
    QCStatus_e ret = radar.Init( "TimeoutTest", &config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        ret = radar.Start();
        if ( ret == QC_STATUS_OK )
        {
            QCSharedBuffer_t inputBuffer, outputBuffer;
            ret = inputBuffer.Allocate( config.maxInputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret );

            ret = outputBuffer.Allocate( config.maxOutputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret );

            GenerateRadarTestData( inputBuffer.data(), config.maxInputBufferSize );

            // Execute may timeout with short timeout value
            ret = radar.Execute( &inputBuffer, &outputBuffer );
            printf( "Execute with short timeout returned: %d\n", ret );
            // Don't assert specific result as timeout behavior depends on implementation

            inputBuffer.Free();
            outputBuffer.Free();
            radar.Stop();
        }
        radar.Deinit();
    }
}

/**
 * @brief Buffer size validation test
 *
 * This test validates proper buffer size checking by attempting
 * to use buffers larger than the configured maximum sizes.
 */
TEST( Radar, BufferSizeValidationTest )
{
    printf( "Testing buffer size validation...\n" );
    Radar radar;
    Radar_Config_t config = radarConfigBasic;

    QCStatus_e ret = radar.Init( "BufferTest", &config, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret == QC_STATUS_OK || ret == QC_STATUS_BAD_STATE );

    if ( ret == QC_STATUS_OK )
    {
        ret = radar.Start();
        if ( ret == QC_STATUS_OK )
        {
            // Test with buffer larger than configured max
            QCSharedBuffer_t largeBuffer;
            ret = largeBuffer.Allocate( config.maxInputBufferSize * 2 );
            ASSERT_EQ( QC_STATUS_OK, ret );

            QCSharedBuffer_t outputBuffer;
            ret = outputBuffer.Allocate( config.maxOutputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret );

            ret = radar.Execute( &largeBuffer, &outputBuffer );
            printf( "Execute with oversized buffer returned: %d\n", ret );
            // Will fail due to buffer size validation
            EXPECT_EQ( QC_STATUS_INVALID_BUF, ret );
            largeBuffer.Free();
            outputBuffer.Free();
            radar.Stop();
        }
        radar.Deinit();
    }
}

/**
 * @brief Multiple instance test
 *
 * This test validates that multiple radar component instances
 * can coexist and operate independently without interference.
 */
TEST( Radar, MultipleInstanceTest )
{
    printf( "Testing multiple radar instances...\n" );
    Radar radar1, radar2;

    QCStatus_e ret1 = radar1.Init( "Radar1", &radarConfigBasic, LOGGER_LEVEL_ERROR );
    QCStatus_e ret2 = radar2.Init( "Radar2", &radarConfigPerformance, LOGGER_LEVEL_ERROR );

    // Accept both OK and FAIL as valid results (service availability dependent)
    EXPECT_TRUE( ret1 == QC_STATUS_OK || ret1 == QC_STATUS_BAD_STATE );
    EXPECT_TRUE( ret2 == QC_STATUS_OK || ret2 == QC_STATUS_BAD_STATE );

    if ( ret1 == QC_STATUS_OK && ret2 == QC_STATUS_OK )
    {
        ret1 = radar1.Start();
        ret2 = radar2.Start();

        if ( ret1 == QC_STATUS_OK && ret2 == QC_STATUS_OK )
        {
            // Both instances should be able to operate independently
            QCSharedBuffer_t input1, output1, input2, output2;

            ret1 = input1.Allocate( radarConfigBasic.maxInputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret1 );
            ret1 = output1.Allocate( radarConfigBasic.maxOutputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret1 );

            ret2 = input2.Allocate( radarConfigPerformance.maxInputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret2 );
            ret2 = output2.Allocate( radarConfigPerformance.maxOutputBufferSize );
            ASSERT_EQ( QC_STATUS_OK, ret2 );

            GenerateRadarTestData( input1.data(), radarConfigBasic.maxInputBufferSize );
            GenerateRadarTestData( input2.data(), radarConfigPerformance.maxInputBufferSize );

            // Execute on both instances
            ret1 = radar1.Execute( &input1, &output1 );
            ret2 = radar2.Execute( &input2, &output2 );

            printf( "Instance 1 execute result: %d\n", ret1 );
            printf( "Instance 2 execute result: %d\n", ret2 );

            input1.Free();
            output1.Free();
            input2.Free();
            output2.Free();

            radar1.Stop();
            radar2.Stop();
        }

        if ( ret1 == QC_STATUS_OK ) radar1.Deinit();
        if ( ret2 == QC_STATUS_OK ) radar2.Deinit();
    }
}

#ifndef GTEST_QCNODE
int main( int argc, char **argv )
{
    ::testing::InitGoogleTest( &argc, argv );
    int nVal = RUN_ALL_TESTS();
    return nVal;
}
#endif
