// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "QC/sample/SysTrace.hpp"
#include <stdlib.h>

namespace QC
{
namespace sample
{

std::mutex SysTrace::s_lock;
FILE *SysTrace::s_pTraceFile = nullptr;

void SysTrace::SysTrace_CloseFile( void )
{
    if ( nullptr != s_pTraceFile )
    {
        fclose( s_pTraceFile );
    }
}

SysTrace::SysTrace() {}
SysTrace::~SysTrace() {}

void SysTrace::Init( std::string name )
{
    m_name = name;
    std::lock_guard<std::mutex> l( s_lock );
    if ( nullptr == s_pTraceFile )
    {
        const char *envValue = getenv( "QC_SYSTRACE" );
        if ( nullptr != envValue )
        {
            s_pTraceFile = fopen( envValue, "wb" );
            if ( nullptr != s_pTraceFile )
            {
                atexit( SysTrace_CloseFile );
                fprintf( stdout, "QC SysTrace File: <%s>.\n", envValue );
            }
            else
            {
                fprintf( stderr, "Failed to create qcnode systrace bin file <%s>.\n", envValue );
            }
        }
    }

    (void) snprintf( m_record.name, sizeof( m_record.name ), "%s", m_name.c_str() );
}

void SysTrace::Init( SysTrace_ProcessorType_e processor )
{
    m_processor = processor;
    m_record.processor = processor;
}

uint64_t SysTrace::Timestamp()
{
    uint64_t timestamp;
    auto now = std::chrono::high_resolution_clock::now();

    timestamp =
            std::chrono::duration_cast<std::chrono::microseconds>( now.time_since_epoch() ).count();

    return timestamp;
}

void SysTrace::Begin( uint64_t id )
{
    if ( nullptr != s_pTraceFile )
    {
        std::lock_guard<std::mutex> l( s_lock );
        m_record.id = id;
        m_record.timestamp = Timestamp();
        m_record.cat = SYSTRACE_TASK_EXECUTE;
        m_record.ph = SYSTRACE_PHASE_BEGIN;
        fwrite( &m_record, sizeof( m_record ), 1, s_pTraceFile );
    }
}

void SysTrace::End( uint64_t id )
{
    if ( nullptr != s_pTraceFile )
    {
        std::lock_guard<std::mutex> l( s_lock );
        m_record.id = id;
        m_record.timestamp = Timestamp();
        m_record.cat = SYSTRACE_TASK_EXECUTE;
        m_record.ph = SYSTRACE_PHASE_END;
        fwrite( &m_record, sizeof( m_record ), 1, s_pTraceFile );
    }
}
void SysTrace::Event( uint64_t id )
{
    if ( nullptr != s_pTraceFile )
    {
        std::lock_guard<std::mutex> l( s_lock );
        m_record.id = id;
        m_record.timestamp = Timestamp();
        m_record.cat = SYSTRACE_EVENT_FRAME_READY;
        m_record.ph = SYSTRACE_PHASE_COMPLETE;
        fwrite( &m_record, sizeof( m_record ), 1, s_pTraceFile );
    }
}

void SysTrace::Event( uint32_t streamId, uint64_t id )
{
    if ( nullptr != s_pTraceFile )
    {
        SysTrace_Record_t record;
        (void) snprintf( record.name, sizeof( record.name ), "%s_%u", m_name.c_str(), streamId );
        std::lock_guard<std::mutex> l( s_lock );
        record.id = id;
        record.processor = SYSTRACE_PROCESSOR_CAMERA;
        record.timestamp = Timestamp();
        record.cat = SYSTRACE_EVENT_FRAME_READY;
        record.ph = SYSTRACE_PHASE_COMPLETE;
        fwrite( &record, sizeof( record ), 1, s_pTraceFile );
    }
}

void SysTrace::Begin( SysTrace_Category_e cat )
{
    if ( nullptr != s_pTraceFile )
    {
        std::lock_guard<std::mutex> l( s_lock );
        m_record.id = 0;
        m_record.timestamp = Timestamp();
        m_record.cat = cat;
        m_record.ph = SYSTRACE_PHASE_BEGIN;
        fwrite( &m_record, sizeof( m_record ), 1, s_pTraceFile );
    }
}

void SysTrace::End( SysTrace_Category_e cat )
{
    if ( nullptr != s_pTraceFile )
    {
        std::lock_guard<std::mutex> l( s_lock );
        m_record.id = 0;
        m_record.timestamp = Timestamp();
        m_record.cat = cat;
        m_record.ph = SYSTRACE_PHASE_END;
        fwrite( &m_record, sizeof( m_record ), 1, s_pTraceFile );
    }
}

void SysTrace::Event( SysTrace_Category_e cat )
{
    if ( nullptr != s_pTraceFile )
    {
        std::lock_guard<std::mutex> l( s_lock );
        m_record.id = 0;
        m_record.timestamp = Timestamp();
        m_record.cat = cat;
        m_record.ph = SYSTRACE_PHASE_COMPLETE;
        fwrite( &m_record, sizeof( m_record ), 1, s_pTraceFile );
    }
}

}   // namespace sample
}   // namespace QC
