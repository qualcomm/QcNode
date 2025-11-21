// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_SAMPLE_RESMON_HPP
#define QC_SAMPLE_RESMON_HPP

#include <iostream>
#include <string>
#include <unistd.h>

#include "QC/Infras/NodeTrace/NodeTrace.hpp"
#include "QC/sample/SampleIF.hpp"
#include "ResMonitor.hpp"

namespace QC
{
namespace sample
{

#define MAX_METRIC_NUM 8
#define MAX_SAMPLE_INTERVAL 100000
#if ( QC_TARGET_SOC == 8797 )
#define MIN_SAMPLE_INTERVAL 100
#else
#define MIN_SAMPLE_INTERVAL 1000
#endif

class SampleResMon : public SampleIF
{
public:
    SampleResMon();
    ~SampleResMon();

    /// @brief Initialize the ResMon
    /// @param name the sample unique instance name
    /// @param config the sample config key value map
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Init( std::string name, SampleConfig_t &config );

    /// @brief Start the ResMon
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Start();

    /// @brief Stop the ResMon
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Stop();

    /// @brief deinitialize the ResMon
    /// @return QC_STATUS_OK on success, others on failure
    QCStatus_e Deinit();

private:
    QCStatus_e ParseConfig( SampleConfig_t &config );
    void ThreadMain();

    QCStatus_e GetCpuUtil();
    QCStatus_e GetGpuUtil();
    QCStatus_e GetNspUtil();
    QCStatus_e GetDdrBW();

    ResMon_Metrics_e GetResMonMetric( std::string metricName );

private:
    bool m_loopMode;
    uint32_t m_gpuId;
    uint64_t m_totalTime;
    uint64_t m_sampleInterval;

    bool m_bCpuMetricEnable = false;
    bool m_bGpuMetricEnable = false;
    bool m_bNsp0MetricEnable = false;
    bool m_bNsp1MetricEnable = false;
    bool m_bDdrMetricEnable = false;
    std::vector<std::string> m_metricNames;
    ResMon_CPU_MetricData_t m_cpuMetricData;
    ResMon_GPU_MetricData_t m_gpuMetricData;
    ResMon_NSP_MetricData_t m_nspMetricData;
    ResMon_DDR_MetricData_t m_ddrMetricData;

    ResMonitor m_resMonitor;

    std::vector<ResMon_CPU_MetricData_t> m_cpuDataArray;

    std::thread m_thread;
    bool m_stop;

    QC_DECLARE_NODETRACE();
};   // class SampleResMon
}   // namespace sample
}   // namespace QC

#endif   // QC_SAMPLE_RESMON_HPP

