
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QCNODE_TRACE_HPP
#define QCNODE_TRACE_HPP

#include <chrono>
#include <cinttypes>
#include <mutex>
#include <stdio.h>
#include <string>

#include "QC/Infras/NodeTrace/Ifs/QCNodeTraceIfs.hpp"

namespace QC
{
namespace Node
{

#ifdef QC_ENABLE_NODETRACE
#define QC_DECLARE_NODETRACE() NodeTrace m_trace
#else
#define QC_DECLARE_NODETRACE()
#endif

class NodeTrace : public QCNodeTraceIfs
{
public:
    NodeTrace();
    ~NodeTrace();

    void Init( std::string config );

    void Trace( std::string name, QCNodeTraceType_e type, std::vector<QCNodeTraceArg_t> args );

private:
    uint64_t Timestamp();

private:
    static void NodeTrace_CloseFile( void );

private:
    std::string m_name;
    std::string m_processor;
    uint32_t m_coreIdsMask;
    std::vector<uint8_t> m_record;

    static std::mutex s_lock;
    static FILE *s_pTraceFile;
};

}   // namespace Node
}   // namespace QC

#endif   // QCNODE_TRACE_HPP