// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#ifndef _QC_SAMPLE_PROFILER_HPP_
#define _QC_SAMPLE_PROFILER_HPP_

#include <chrono>
#include <cinttypes>
#include <stdio.h>
#include <string>

namespace QC
{
namespace sample
{
#define __ENABLE_PROFILER__
#ifdef __ENABLE_PROFILER__
#define PROFILER_BEGIN()                                                                           \
    do                                                                                             \
    {                                                                                              \
        m_profiler.Begin();                                                                        \
    } while ( 0 )

#define PROFILER_END()                                                                             \
    do                                                                                             \
    {                                                                                              \
        m_profiler.End();                                                                          \
    } while ( 0 )

#define PROFILER_SHOW()                                                                            \
    do                                                                                             \
    {                                                                                              \
        m_profiler.Show();                                                                         \
    } while ( 0 )
#else
#define PROFILER_BEGIN()
#define PROFILER_END()
#define PROFILER_SHOW()
#endif

#define PROFILER_GET_MS( end, begin )                                                              \
    ( (double) std::chrono::duration_cast<std::chrono::microseconds>( end - begin ).count() /      \
      1000.d )

#define PROFILER_GET_SECONDS( end, begin )                                                         \
    ( (double) std::chrono::duration_cast<std::chrono::milliseconds>( end - begin ).count() /      \
      1000.d )

#ifndef PROFILER_SHOW_PERIOD_SECONDS
#define PROFILER_SHOW_PERIOD_SECONDS 5
#endif

class Profiler
{
public:
    Profiler() {}
    ~Profiler() {}


    void Show()
    {
        if ( m_count > 0 )
        {
            printf( "%-16s: FPS=%-7.3f AVG=%-7.3f MIN=%-7.3f MAX=%-7.3f COUNT=%" PRIu64 "\n",
                    m_name.c_str(), GetFPS(), m_avg, m_min, m_max, m_count );
            m_prevShow = std::chrono::high_resolution_clock::now();
        }
    }

    void Init( std::string name )
    {
        m_name = name;
        m_count = 0;
        m_begin = std::chrono::high_resolution_clock::now();
        m_prevShow = std::chrono::high_resolution_clock::now();
    }

    void Begin()
    {
        if ( 0 == m_count )
        { /* update begin if count is 0 to make the FPS more accurate */
            m_begin = std::chrono::high_resolution_clock::now();
            m_prevShow = std::chrono::high_resolution_clock::now();
        }
        m_prev = std::chrono::high_resolution_clock::now();
    }

    void End()
    {
        /* NOTE: no consideration of overflow as uint64 and double can hold large value */
        m_count++;
        m_end = std::chrono::high_resolution_clock::now();
        float durationMs = (float) PROFILER_GET_MS( m_end, m_prev );
        if ( durationMs > m_max )
        {
            m_max = durationMs;
        }

        if ( durationMs < m_min )
        {
            m_min = durationMs;
        }

        m_totalCost += durationMs;
        m_avg = m_totalCost / m_count;

        double elapsedS = PROFILER_GET_SECONDS( m_end, m_prevShow );
        if ( elapsedS > PROFILER_SHOW_PERIOD_SECONDS )
        {
            Show();
        }
    }

    float GetFPS()
    {
        float fps = 0.f;

        if ( m_count > 0.f )
        {
            double durationS = PROFILER_GET_SECONDS( m_end, m_begin );
            fps = (float) ( (double) m_count / durationS );
        }

        return fps;
    }

    float GetAvg() { return m_avg; }
    float GetMin() { return m_min; }
    float GetMax() { return m_max; }

private:
    std::string m_name;
    uint64_t m_count = 0LLU;
    double m_totalCost = 0.d;
    float m_min = std::numeric_limits<float>::max();
    float m_avg = 0.f;
    float m_max = 0.f;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_begin;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_prev;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_prevShow;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_end;
};

}   // namespace sample
}   // namespace QC

#endif   // _QC_SAMPLE_PROFILER_HPP_
