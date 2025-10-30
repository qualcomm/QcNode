
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QCNODE_TRACE_IFS_HPP
#define QCNODE_TRACE_IFS_HPP

#include <cinttypes>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace QC
{

#ifdef QC_ENABLE_NODETRACE
#define QC_TRACE_IF( condition, ... )                                                              \
    if ( condition )                                                                               \
    {                                                                                              \
        __VA_ARGS__;                                                                               \
    }

#define QC_TRACE_INIT( ... )                                                                       \
    do                                                                                             \
    {                                                                                              \
        m_trace.Init( __VA_ARGS__ );                                                               \
    } while ( 0 )

#define QC_TRACE_BEGIN( name, ... )                                                                \
    do                                                                                             \
    {                                                                                              \
        m_trace.Trace( name, QCNODE_TRACE_TYPE_BEGIN, __VA_ARGS__ );                               \
    } while ( 0 )

#define QC_TRACE_END( name, ... )                                                                  \
    do                                                                                             \
    {                                                                                              \
        m_trace.Trace( name, QCNODE_TRACE_TYPE_END, __VA_ARGS__ );                                 \
    } while ( 0 )

#define QC_TRACE_EVENT( name, ... )                                                                \
    do                                                                                             \
    {                                                                                              \
        m_trace.Trace( name, QCNODE_TRACE_TYPE_EVENT, __VA_ARGS__ );                               \
    } while ( 0 )

#define QC_TRACE_COUNTER( name, ... )                                                              \
    do                                                                                             \
    {                                                                                              \
        m_trace.Trace( name, QCNODE_TRACE_TYPE_COUNTER, __VA_ARGS__ );                             \
    } while ( 0 )

#else
#define QC_TRACE_IF( condition, ... )
#define QC_TRACE_INIT( ... )
#define QC_TRACE_BEGIN( name, ... )
#define QC_TRACE_END( name, ... )
#define QC_TRACE_EVENT( name, ... )
#define QC_TRACE_COUNTER( name, ... )
#endif


/**
 * @example
 *   class QCNode {
 *    QCStatus_e Init( QCNodeInit_t &config ) {
 *        ...
 *        QC_TRACE_INIT( R"({"name": "QNN0", "processor": "htp0", "coreIds": [0, 1, 2, 3]})" );
 *        QC_TRACE_BEGIN( "Init", {} );
 *        ...
 *        QC_TRACE_END( "Init", {} );
 *        return status;
 *    }
 *    QCStatus_e Start( QCNodeInit_t &config ) {
 *        ...
 *        QC_TRACE_BEGIN( "Start", {} );
 *        ...
 *        QC_TRACE_END( "Start", {} );
 *        return status;
 *    }
 *    QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc ) {
 *        ...
 *        QC_TRACE_BEGIN( "Execute", { QCNodeTraceArg( "frameId", frameId ) } );
 *        ...
 *        QC_TRACE_END( "Execute", { QCNodeTraceArg( "frameId", frameId ) } );
 *        return status;
 *   }
 *   private:
 *      // declare an instance "m_trace" of QCNodeTraceIfs for the QC Node
 *      QC_DECLARE_NODETRACE();
 *   }
 */

// Enum representing different types of trace events
typedef enum
{
    QCNODE_TRACE_TYPE_BEGIN,     // Marks the beginning of a traceable operation
    QCNODE_TRACE_TYPE_END,       // Marks the end of a traceable operation
    QCNODE_TRACE_TYPE_EVENT,     // Represents a generic event
    QCNODE_TRACE_TYPE_COUNTER,   // Represents a counter-type trace
    QCNODE_TRACE_TYPE_MAX        // Sentinel value for bounds checking
} QCNodeTraceType_e;

// Enum representing supported argument types for trace events
typedef enum
{
    QCNODE_TRACE_ARG_TYPE_STRING,
    QCNODE_TRACE_ARG_TYPE_DOUBLE,
    QCNODE_TRACE_ARG_TYPE_FLOAT,
    QCNODE_TRACE_ARG_TYPE_UINT64,
    QCNODE_TRACE_ARG_TYPE_UINT32,
    QCNODE_TRACE_ARG_TYPE_UINT16,
    QCNODE_TRACE_ARG_TYPE_UINT8,
    QCNODE_TRACE_ARG_TYPE_INT64,
    QCNODE_TRACE_ARG_TYPE_INT32,
    QCNODE_TRACE_ARG_TYPE_INT16,
    QCNODE_TRACE_ARG_TYPE_INT8,
} QCNodeTraceArgType_e;

// Struct representing a single argument passed to a trace event
typedef struct QCNodeTraceArg
{
    // Constructors for each supported argument type
    QCNodeTraceArg( std::string name, std::string strV )
        : type( QCNODE_TRACE_ARG_TYPE_STRING ),
          name( name ),
          strV( strV )
    {}
    QCNodeTraceArg( std::string name, double doubleV )
        : type( QCNODE_TRACE_ARG_TYPE_DOUBLE ),
          name( name ),
          doubleV( doubleV )
    {}
    QCNodeTraceArg( std::string name, float floatV )
        : type( QCNODE_TRACE_ARG_TYPE_FLOAT ),
          name( name ),
          floatV( floatV )
    {}
    QCNodeTraceArg( std::string name, uint64_t u64V )
        : type( QCNODE_TRACE_ARG_TYPE_UINT64 ),
          name( name ),
          u64V( u64V )
    {}
    QCNodeTraceArg( std::string name, uint32_t u32V )
        : type( QCNODE_TRACE_ARG_TYPE_UINT32 ),
          name( name ),
          u32V( u32V )
    {}
    QCNodeTraceArg( std::string name, uint16_t u16V )
        : type( QCNODE_TRACE_ARG_TYPE_UINT16 ),
          name( name ),
          u16V( u16V )
    {}
    QCNodeTraceArg( std::string name, uint8_t u8V )
        : type( QCNODE_TRACE_ARG_TYPE_UINT8 ),
          name( name ),
          u8V( u8V )
    {}
    QCNodeTraceArg( std::string name, int64_t i64V )
        : type( QCNODE_TRACE_ARG_TYPE_INT64 ),
          name( name ),
          i64V( i64V )
    {}
    QCNodeTraceArg( std::string name, int32_t i32V )
        : type( QCNODE_TRACE_ARG_TYPE_INT32 ),
          name( name ),
          i32V( i32V )
    {}
    QCNodeTraceArg( std::string name, int16_t i16V )
        : type( QCNODE_TRACE_ARG_TYPE_INT16 ),
          name( name ),
          i16V( i16V )
    {}
    QCNodeTraceArg( std::string name, int8_t i8V )
        : type( QCNODE_TRACE_ARG_TYPE_INT8 ),
          name( name ),
          i8V( i8V )
    {}

    ~QCNodeTraceArg() {}

    QCNodeTraceArgType_e type;   // Type of the argument
    std::string name;            // Name of the argument
    std::string strV;
    // Union holding the actual value of the argument
    union
    {
        double doubleV;
        float floatV;
        uint64_t u64V;
        uint32_t u32V;
        uint16_t u16V;
        uint8_t u8V;
        int64_t i64V;
        int32_t i32V;
        int16_t i16V;
        int8_t i8V;
    };
} QCNodeTraceArg_t;

/**
 * @class QCNodeTraceIfs
 * @brief Abstract interface for tracing QCNode events.
 */
class QCNodeTraceIfs
{
public:
    /**
     * @brief Initializes the trace system with a configuration.
     *
     * This pure virtual method must be implemented by derived classes to configure
     * the tracing backend (e.g., file logger, telemetry system, etc.) using a JSON-formatted
     * configuration string. The configuration typically includes trace metadata such as
     * the trace name, target processor, and core IDs to monitor.
     *
     * @param[in] config A JSON-formatted string containing trace configuration parameters.
     *
     * @details
     * The configuration string should follow a structure similar to:
     * {
     *   "name": "<trace_name>",
     *   "processor": "<processor_id>",
     *   "coreIds": [<core_id_0>, <core_id_1>, ...]
     * }
     *
     * @note
     * This method is intended to be flexible and backend-agnostic. Implementations may
     * parse the JSON string manually or use a JSON library depending on project constraints.
     *
     * @example
     * // Static configuration using a raw JSON string
     * QC_TRACE_INIT(R"({"name": "QNN0", "processor": "htp0", "coreIds": [0, 1, 2, 3]})");
     *
     * @example
     * // Dynamic configuration using a lambda function to construct the JSON string
     * QC_TRACE_INIT([&]() {
     *     std::ostringstream oss;
     *     oss << "{"; // Begin JSON object
     *     oss << "\"name\": \"" << m_name << "\", "; // Add trace name
     *     oss << "\"processor\": \"" << m_processor << "\", "; // Add processor ID
     *     oss << "\"coreIds\": ["; // Begin core ID array
     *     for (size_t i = 0; i < m_coreIds.size(); ++i) {
     *         oss << m_coreIds[i]; // Add core ID
     *         if (i != m_coreIds.size() - 1) {
     *             oss << ", "; // Separate IDs with commas
     *         }
     *     }
     *     oss << "]}"; // Close array and JSON object
     *     return oss.str(); // Return the constructed JSON string
     * }());
     */
    virtual void Init( std::string config ) = 0;

    /**
     * @brief Records a trace event.
     *
     * This pure virtual method must be implemented by derived classes to log or emit
     * a trace event with the specified name, type, and contextual arguments. It enables
     * fine-grained instrumentation of system behavior for debugging, profiling, or telemetry.
     *
     * Supported trace event types typically include:
     * - `BEGIN`: Marks the start of a scoped operation or activity.
     * - `END`: Marks the end of a previously started operation.
     * - `EVENT`: Represents a standalone, instantaneous event.
     * - `COUNTER`: Captures a numeric value at a specific point in time (e.g., memory usage).
     *
     * @param[in] name  A descriptive label for the trace event (e.g., "Init", "Start", "Stop",
     * "Execute", "Deinit").
     * @param[in] type  The classification of the trace event, defined by the `QCNodeTraceType_e`
     * enum.
     * @param[in] args  A list of typed key-value argument pairs (`QCNodeTraceArg_t`) providing
     *                   additional metadata or context for the event (e.g., frameID, latency).
     *                   Implementations may modify the list (e.g., to enrich or filter arguments).
     *
     * @note
     * Implementers should ensure thread safety and minimal performance overhead when recording
     * events. Depending on the backend, events may be written to binary files or logs, sent to a
     * telemetry system, or visualized in a trace viewer.
     *
     * @example
     * // Example usage to trace the start/stop of an inference operation:
     * QC_TRACE_BEGIN("Execute", { QCNodeTraceArg( "frameId", frameId ) } );
     * ...
     * QC_TRACE_END("Execute", { QCNodeTraceArg( "frameId", frameId ) } );
     */
    virtual void Trace( std::string name, QCNodeTraceType_e type,
                        std::vector<QCNodeTraceArg_t> args ) = 0;
};

}   // namespace QC

#endif   // QCNODE_TRACE_IFS_HPP