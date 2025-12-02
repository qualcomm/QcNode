// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_SYSTRACE_HPP
#define QC_SAMPLE_SYSTRACE_HPP

#include "QC/Common/Types.hpp"
#include "QC/Infras/NodeTrace/NodeTrace.hpp"

using namespace QC;

#include <chrono>
#include <cinttypes>
#include <mutex>
#include <stdio.h>
#include <string>

namespace QC
{
namespace sample
{

using namespace QC::Node;

#ifdef QC_ENABLE_NODETRACE
#define TRACE_ON( processor )                                                                      \
    do                                                                                             \
    {                                                                                              \
        m_systrace.Init( SYSTRACE_PROCESSOR_##processor );                                         \
    } while ( 0 )

#define TRACE_BEGIN( id_or_cat )                                                                   \
    do                                                                                             \
    {                                                                                              \
        m_systrace.Begin( id_or_cat );                                                             \
    } while ( 0 )

#define TRACE_END( id_or_cat )                                                                     \
    do                                                                                             \
    {                                                                                              \
        m_systrace.End( id_or_cat );                                                               \
    } while ( 0 )

#define TRACE_EVENT( id_or_cat )                                                                   \
    do                                                                                             \
    {                                                                                              \
        m_systrace.Event( id_or_cat );                                                             \
    } while ( 0 )

#define TRACE_CAMERA_EVENT( streamId, id )                                                         \
    do                                                                                             \
    {                                                                                              \
        m_systrace.Event( streamId, id );                                                          \
    } while ( 0 )
#else
#define TRACE_ON( processor )
#define TRACE_BEGIN( id_or_cat )
#define TRACE_END( id_or_cat )
#define TRACE_EVENT( id_or_cat )
#define TRACE_CAMERA_EVENT( streamId, id )
#endif

typedef enum
{
    SYSTRACE_PROCESSOR_HTP0 = QC_PROCESSOR_HTP0,
    SYSTRACE_PROCESSOR_HTP1 = QC_PROCESSOR_HTP1,
    SYSTRACE_PROCESSOR_CPU = QC_PROCESSOR_CPU,
    SYSTRACE_PROCESSOR_GPU = QC_PROCESSOR_GPU,
    SYSTRACE_PROCESSOR_HTP0_CORE1,
    SYSTRACE_PROCESSOR_HTP0_CORE2,
    SYSTRACE_PROCESSOR_HTP0_CORE3,
    SYSTRACE_PROCESSOR_HTP2,
    SYSTRACE_PROCESSOR_HTP3,
    SYSTRACE_PROCESSOR_CAMERA,
    SYSTRACE_PROCESSOR_VPU,
    SYSTRACE_PROCESSOR_DATA_READER,
    SYSTRACE_PROCESSOR_DATA_ONLINE,
    SYSTRACE_PROCESSOR_MAX = 0xFFFFFFFF,
} SysTrace_ProcessorType_e;

typedef enum
{
    SYSTRACE_TASK_INIT,
    SYSTRACE_TASK_START,
    SYSTRACE_TASK_EXECUTE,
    SYSTRACE_TASK_STOP,
    SYSTRACE_TASK_DEINIT,
    SYSTRACE_EVENT_FRAME_READY,
    SYSTRACE_EVENT_VENC_INPUT_DONE,
    SYSTRACE_EVENT_VENC_OUTPUT_WITH_2ND_FRAME,
    SYSTRACE_EVENT_VDEC_INPUT_DONE,
    SYSTRACE_EVENT_VDEC_OUTPUT_WITH_2ND_FRAME,
} SysTrace_Category_e;

class SysTrace
{
public:
    SysTrace();
    ~SysTrace();

    void Init( std::string name );
    void Init( SysTrace_ProcessorType_e processor );

    /**
     * @brief Begin of the Task Execute with a frame ID
     * @param[in] id the frame ID
     * @return void
     */
    void Begin( uint64_t id );

    /**
     * @brief End of the Task Execute with a frame ID
     * @param[in] id the frame ID
     * @return void
     */
    void End( uint64_t id );

    /**
     * @brief Begin of a specific task, Init, Start, Stop or Deinit
     * @param[in] cat the task category
     * @return void
     */
    void Begin( SysTrace_Category_e cat );

    /**
     * @brief End of a specific task, Init, Start, Stop or Deinit
     * @param[in] cat the task category
     * @return void
     */
    void End( SysTrace_Category_e cat );

    /**
     * @brief A Event that represent a frame with id is ready
     * @param[in] id the frame ID
     * @return void
     */
    void Event( uint64_t id );


    /**
     * @brief A Event that represent a frame with id is ready for the camera stream identified by
     * the streamId
     * @param[in] streamId the camera streamId
     * @param[in] id the frame ID
     * @return void
     */
    void Event( uint32_t streamId, uint64_t id );

    /**
     * @brief A event identified by the category happens
     * @param[in] cat the event type category
     * @return void
     */
    void Event( SysTrace_Category_e cat );

private:
    QC_DECLARE_NODETRACE();
};

}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_SYSTRACE_HPP
