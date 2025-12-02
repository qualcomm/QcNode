// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_RESMONITOR_HPP
#define QC_RESMONITOR_HPP

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

#include "QC/Common/Types.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "amss/compresmon.h"

namespace QC
{
namespace sample
{

#define MAX_CPU_NUM 32
#define MAX_PROCESS_NUM 512
#define MAX_PROCESS_NAME_STRENGTH 96

typedef enum
{
    RESMON_METRIC_CPU_UTIL,
    RESMON_METRIC_GPU_UTIL,
    RESMON_METRIC_NSP0_UTIL,
    RESMON_METRIC_NSP1_UTIL,
    RESMON_METRIC_DDR_BW,

    RESMON_METRIC_MAX
} ResMon_Metrics_e;

typedef struct
{
    uint64_t sampleTime;
    uint64_t startTime;
    uint64_t endTime;
} ResMon_SampleTimeData_t;

typedef struct
{
    ResMon_SampleTimeData_t sampleTimeData;
    uint32_t cpuNum;
    double totalCpuUtil;
    double perCpuCoreUtil[MAX_CPU_NUM];
} ResMon_CPU_MetricData_t;

typedef struct
{
    ResMon_SampleTimeData_t sampleTimeData;
    uint32_t gpuId;
    uint32_t processNum;
    uint32_t processId[MAX_PROCESS_NUM];
    char processName[MAX_PROCESS_NUM][MAX_PROCESS_NAME_STRENGTH];
    double busyPercentage[MAX_PROCESS_NUM];
    unsigned long long ts;
} ResMon_GPU_MetricData_t;

typedef struct
{
    ResMon_SampleTimeData_t sampleTimeData;
    uint32_t nspId;
    uint32_t hvxUtil;
    uint32_t hmxUtil;
    uint32_t q6Load;
} ResMon_NSP_MetricData_t;

typedef struct
{
    ResMon_SampleTimeData_t sampleTimeData;
    uint64_t ddrTotalBw;
    uint64_t ddrReadBw;
    uint64_t ddrWriteBw;
    uint64_t llcTotalBw;
    uint64_t llcReadBw;
    uint64_t llcWriteBw;
    uint64_t nocBwApps0;
    uint64_t nocBwApps1;
    uint64_t nocBwGpu0;
    uint64_t nocBwNsp0;
    uint64_t nocBwNsp1;
    uint64_t nocBwNsp2;
    uint64_t nocBwNsp3;
} ResMon_DDR_MetricData_t;

class ResMonitor
{
public:
    ResMonitor();
    ~ResMonitor();

    QCStatus_e Init();
    QCStatus_e DeInit();

    QCStatus_e GetCpuUtil( ResMon_CPU_MetricData_t &cpuMetricData );
    QCStatus_e GetGpuUtil( ResMon_GPU_MetricData_t &gpuMetricData );
    QCStatus_e GetNspUtil( ResMon_NSP_MetricData_t &nspMetricData );
    QCStatus_e GetDdrBW( ResMon_DDR_MetricData_t &ddrMetricData );
};

}   // namespace sample
}   // namespace QC

#endif /* QC_RESMONITOR_HPP */

