// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/NodeTrace/NodeTrace.hpp"
#include "QC/Common/DataTree.hpp"
#include <stdlib.h>

namespace QC
{
namespace Node
{

#ifndef NODETRACE_MAX_RECORD_SIZE
#define NODETRACE_MAX_RECORD_SIZE 1024u
#endif

typedef struct
{
    uint64_t timestamp;
    QCNodeTraceType_e traceType;
    uint32_t lenName;
    uint32_t lenProcessor;
    uint32_t lenEventName;
    uint32_t coreIdsMask;
    uint32_t numArgs;
} NodeTrace_EventHeader_t;
/* following by raw string of name */
/* following by raw of processor */
/* following by raw string of event name */

typedef struct
{
    QCNodeTraceArgType_e argType;
    uint32_t lenName;
    uint32_t lenValue;
} NodeTrace_EventArg_t;
/* following by raw string of name */
/* following by raw value of arg */

std::mutex NodeTrace::s_lock;
FILE *NodeTrace::s_pTraceFile = nullptr;

static uint64_t s_id = 0;

static size_t GetArgValueRawPtrAndSize( QCNodeTraceArg_t &arg, const void *&pRaw )
{
    size_t size = 0;
    switch ( arg.type )
    {
        case QCNODE_TRACE_ARG_TYPE_STRING:
            size = arg.strV.size();
            pRaw = static_cast<const void *>( arg.strV.c_str() );
            break;
        case QCNODE_TRACE_ARG_TYPE_DOUBLE:
            size = sizeof( double );
            pRaw = static_cast<void *>( &arg.doubleV );
            break;
        case QCNODE_TRACE_ARG_TYPE_FLOAT:
            size = sizeof( float );
            pRaw = static_cast<void *>( &arg.floatV );
            break;
        case QCNODE_TRACE_ARG_TYPE_UINT64:
            size = sizeof( uint64_t );
            pRaw = static_cast<void *>( &arg.u64V );
            break;
        case QCNODE_TRACE_ARG_TYPE_UINT32:
            size = sizeof( uint32_t );
            pRaw = static_cast<void *>( &arg.u32V );
            break;
        case QCNODE_TRACE_ARG_TYPE_UINT16:
            size = sizeof( uint16_t );
            pRaw = static_cast<void *>( &arg.u16V );
            break;
        case QCNODE_TRACE_ARG_TYPE_UINT8:
            size = sizeof( uint8_t );
            pRaw = static_cast<void *>( &arg.u8V );
            break;
        case QCNODE_TRACE_ARG_TYPE_INT64:
            size = sizeof( int64_t );
            pRaw = static_cast<void *>( &arg.i64V );
            break;
        case QCNODE_TRACE_ARG_TYPE_INT32:
            size = sizeof( int32_t );
            pRaw = static_cast<void *>( &arg.i32V );
            break;
        case QCNODE_TRACE_ARG_TYPE_INT16:
            size = sizeof( int16_t );
            pRaw = static_cast<void *>( &arg.i16V );
            break;
        case QCNODE_TRACE_ARG_TYPE_INT8:
            size = sizeof( int8_t );
            pRaw = static_cast<void *>( &arg.i8V );
            break;
    }

    return size;
}

void NodeTrace::NodeTrace_CloseFile( void )
{
    if ( nullptr != s_pTraceFile )
    {
        fclose( s_pTraceFile );
    }
}

NodeTrace::NodeTrace()
    : m_name( "unknown" + std::to_string( s_id++ ) ),
      m_processor( "default" ),
      m_coreIdsMask( 0u )
{
    m_record.reserve( NODETRACE_MAX_RECORD_SIZE );
}

NodeTrace::~NodeTrace() {}

void NodeTrace::Init( std::string config )
{
    std::lock_guard<std::mutex> l( s_lock );
    if ( nullptr == s_pTraceFile )
    {
        const char *envValue = getenv( "QC_NODETRACE" );
        if ( nullptr != envValue )
        {
            s_pTraceFile = fopen( envValue, "wb" );
            if ( nullptr != s_pTraceFile )
            {
                atexit( NodeTrace_CloseFile );
                fprintf( stdout, "QC NodeTrace File: <%s>.\n", envValue );
            }
            else
            {
                fprintf( stderr, "Failed to create qcnode node trace bin file <%s>.\n", envValue );
            }
        }
    }

    if ( nullptr != s_pTraceFile )
    {
        QCStatus_e status = QC_STATUS_OK;
        std::string errors;
        DataTree dt;
        status = dt.Load( config, errors );
        if ( QC_STATUS_OK == status )
        {
            if ( true == dt.Exists( "name" ) )
            {
                m_name = dt.Get<std::string>( "name", m_name );
            }
            if ( true == dt.Exists( "processor" ) )
            {
                m_processor = dt.Get<std::string>( "processor", "default" );
            }
            if ( true == dt.Exists( "coreIds" ) )
            {
                std::vector<uint32_t> coreIds;
                coreIds = dt.Get<uint32_t>( "coreIds", coreIds );
                for ( uint32_t coreId : coreIds )
                {
                    m_coreIdsMask |= ( 1u << coreId );
                }
            }
        }
        else
        {
            fprintf( stderr, "invalid trace config <%s>.\n", config.c_str() );
        }
    }
    else
    {
        /* do nothing as node trace was not enabled */
    }
}

uint64_t NodeTrace::Timestamp()
{
    uint64_t timestamp;
    auto now = std::chrono::high_resolution_clock::now();

    timestamp =
            std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch() ).count();

    return timestamp;
}


void NodeTrace::Trace( std::string name, QCNodeTraceType_e type,
                       std::vector<QCNodeTraceArg_t> args )
{
    size_t size;
    if ( nullptr != s_pTraceFile )
    {
        std::lock_guard<std::mutex> l( s_lock );

        size_t offset = 0;
        uint64_t timestamp = Timestamp();
        NodeTrace_EventHeader_t eventHeader;
        size = sizeof( eventHeader ) + m_name.size() + m_processor.size() + name.size();
        m_record.resize( size );

        eventHeader.timestamp = timestamp;
        eventHeader.coreIdsMask = m_coreIdsMask;
        eventHeader.numArgs = args.size();
        eventHeader.lenName = m_name.size();
        eventHeader.lenProcessor = m_processor.size();
        eventHeader.lenEventName = name.size();
        eventHeader.traceType = type;
        memcpy( m_record.data(), &eventHeader, sizeof( eventHeader ) );
        offset += sizeof( eventHeader );

        memcpy( &( m_record.data()[offset] ), m_name.c_str(), m_name.size() );
        offset += m_name.size();

        memcpy( &( m_record.data()[offset] ), m_processor.c_str(), m_processor.size() );
        offset += m_processor.size();

        memcpy( &( m_record.data()[offset] ), name.c_str(), name.size() );
        offset += name.size();

        for ( QCNodeTraceArg_t &arg : args )
        {
            NodeTrace_EventArg_t evtArg;
            const void *pRaw;
            size_t lenValue = GetArgValueRawPtrAndSize( arg, pRaw );
            size += sizeof( evtArg ) + arg.name.size() + lenValue;
            m_record.resize( size );

            evtArg.argType = arg.type;
            evtArg.lenName = arg.name.size();
            evtArg.lenValue = lenValue;
            memcpy( &( m_record.data()[offset] ), &evtArg, sizeof( evtArg ) );
            offset += sizeof( evtArg );

            memcpy( &( m_record.data()[offset] ), arg.name.c_str(), arg.name.size() );
            offset += arg.name.size();

            memcpy( &( m_record.data()[offset] ), pRaw, lenValue );
            offset += lenValue;
        }

        fwrite( m_record.data(), offset, 1, s_pTraceFile );
    }
}

}   // namespace Node
}   // namespace QC
