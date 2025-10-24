// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/sample/SysTrace.hpp"
#include <stdlib.h>

namespace QC
{
namespace sample
{

static const std::string s_catToString[] = {
        "Init",                     // SYSTRACE_TASK_INIT
        "Start",                    // SYSTRACE_TASK_START
        "Execute",                  // SYSTRACE_TASK_EXECUTE
        "Stop",                     // SYSTRACE_TASK_STOP
        "Deinit",                   // SYSTRACE_TASK_DEINIT
        "FrameReady",               // SYSTRACE_EVENT_FRAME_READY
        "VencInputDone",            // SYSTRACE_EVENT_VENC_INPUT_DONE
        "VencOutputWith2ndFrame",   // SYSTRACE_EVENT_VENC_OUTPUT_WITH_2ND_FRAME
        "VdecInputDone",            // SYSTRACE_EVENT_VDEC_INPUT_DONE
        "VdecOutputWith2ndFrame"    // SYSTRACE_EVENT_VDEC_OUTPUT_WITH_2ND_FRAME
};

SysTrace::SysTrace() {}
SysTrace::~SysTrace() {}

void SysTrace::Init( std::string name )
{
    QC_TRACE_INIT( [&]() {
        std::ostringstream oss;
        oss << "{";
        oss << "\"name\": \"" << name << "\"";
        oss << "}";
        return oss.str();
    }() );
}

void SysTrace::Init( SysTrace_ProcessorType_e processor )
{
    QC_TRACE_INIT( [&]() {
        std::ostringstream oss;
        std::string pname = "default";
        switch ( processor )
        {
            case SYSTRACE_PROCESSOR_HTP0:
                pname = "htp0";
                break;
            case SYSTRACE_PROCESSOR_HTP1:
                pname = "htp1";
                break;
            case SYSTRACE_PROCESSOR_CPU:
                pname = "cpu";
                break;
            case SYSTRACE_PROCESSOR_GPU:
                pname = "gpu";
                break;
            case SYSTRACE_PROCESSOR_HTP2:
                pname = "htp2";
                break;
            case SYSTRACE_PROCESSOR_HTP3:
                pname = "htp3";
                break;
            SYSTRACE_PROCESSOR_CAMERA:
                pname = "camera";
                break;
            case SYSTRACE_PROCESSOR_VPU:
                pname = "vpu";
                break;
            case SYSTRACE_PROCESSOR_DATA_READER:
                pname = "data_reader";
                break;
            case SYSTRACE_PROCESSOR_DATA_ONLINE:
                pname = "data_online";
                break;
            default:
                break;
        }
        oss << "{";
        oss << "\"processor\": \"" << pname << "\"";
        oss << "}";
        return oss.str();
    }() );
}

void SysTrace::Begin( uint64_t id )
{
    QC_TRACE_BEGIN( "Execute", { QCNodeTraceArg( "frameId", id ) } );
}

void SysTrace::End( uint64_t id )
{
    QC_TRACE_END( "Execute", { QCNodeTraceArg( "frameId", id ) } );
}

void SysTrace::Event( uint64_t id )
{
    QC_TRACE_EVENT( "FrameReady", { QCNodeTraceArg( "frameId", id ) } );
}

void SysTrace::Event( uint32_t streamId, uint64_t id )
{
    QC_TRACE_EVENT( "FrameReady",
                    { QCNodeTraceArg( "frameId", id ), QCNodeTraceArg( "streamId", streamId ) } );
}

void SysTrace::Begin( SysTrace_Category_e cat )
{
    QC_TRACE_BEGIN( s_catToString[cat], {} );
}

void SysTrace::End( SysTrace_Category_e cat )
{
    QC_TRACE_END( s_catToString[cat], {} );
}

void SysTrace::Event( SysTrace_Category_e cat )
{
    QC_TRACE_EVENT( s_catToString[cat], {} );
}

}   // namespace sample
}   // namespace QC
