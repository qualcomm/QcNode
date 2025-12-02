// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "ResMonitor.hpp"

namespace QC
{
namespace sample
{

ResMonitor::ResMonitor() {}
ResMonitor::~ResMonitor() {}

QCStatus_e ResMonitor::GetCpuUtil( ResMon_CPU_MetricData_t &cpuMetricData )
{
    QCStatus_e ret = QC_STATUS_OK;
    CompResMonRet_e resMonRet = COMPRESMON_RET_SUCCESS;

    uint64_t startTime = 0;
    uint64_t endTime = 0;
#if defined( __QNXNTO__ )
    cpuUtils_t cpuStats = { 0 };
#else
    CpuUtilStats_t cpuStats = { 0 };
#endif

    uint64_t sampleTime = cpuMetricData.sampleTimeData.sampleTime;

    auto t0 = std::chrono::high_resolution_clock::now();
    resMonRet = CompResmonQueryCpuUtil( sampleTime, &cpuStats );
    auto t1 = std::chrono::high_resolution_clock::now();

    startTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t0.time_since_epoch() ).count();
    endTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t1.time_since_epoch() ).count();


    if ( COMPRESMON_RET_SUCCESS == resMonRet )
    {
        cpuMetricData.sampleTimeData.startTime = startTime;
        cpuMetricData.sampleTimeData.endTime = endTime;
        cpuMetricData.cpuNum = cpuStats.num_cpus;
        cpuMetricData.totalCpuUtil = cpuStats.totalUtilization;
        for ( uint32_t i = 0; i < cpuStats.num_cpus; i++ )
        {
            cpuMetricData.perCpuCoreUtil[i] = cpuStats.perCoreUtilization[i];
        }
    }
    else if ( COMPRESMON_RET_ENVLD == resMonRet )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Invalid CPU Sampling time: %u (Expected range: %u - %u)", sampleTime,
                      COMPRESMON_CPU_MIN_SAMPLING_TIME_MS, COMPRESMON_CPU_MAX_SAMPLING_TIME_MS );
    }
    else
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Failed to get CPU utilization, status = %u", resMonRet );
    }

    return ret;
}

QCStatus_e ResMonitor::GetGpuUtil( ResMon_GPU_MetricData_t &gpuMetricData )
{
    QCStatus_e ret = QC_STATUS_OK;
    CompResMonRet_e resMonRet = COMPRESMON_RET_SUCCESS;

    uint64_t startTime = 0;
    uint64_t endTime = 0;
    GpuUtilStats_t gpuStats = { 0 };
    GpuId_e gpuId = (GpuId_e) gpuMetricData.gpuId;

    auto t0 = std::chrono::high_resolution_clock::now();
    resMonRet = CompResmonQueryGpuUtil( gpuId, &gpuStats );
    auto t1 = std::chrono::high_resolution_clock::now();

    startTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t0.time_since_epoch() ).count();
    endTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t1.time_since_epoch() ).count();

    if ( COMPRESMON_RET_SUCCESS == resMonRet )
    {
        gpuMetricData.sampleTimeData.startTime = startTime;
        gpuMetricData.sampleTimeData.endTime = endTime;
        gpuMetricData.processNum = gpuStats.processNum;
        for ( uint32_t i = 0; i < gpuStats.processNum; i++ )
        {
#if defined( __QNXNTO__ )
            gpuMetricData.processId[i] = gpuStats.processId[i];
#else
            gpuMetricData.processId[i] = gpuStats.processID[i];
#endif
            gpuMetricData.busyPercentage[i] = gpuStats.busyPercentage[i];
            memcpy( gpuMetricData.processName[i], gpuStats.processName[i],
                    sizeof( gpuStats.processName[i] ) );
        }
    }
    else if ( COMPRESMON_RET_ENVLD == resMonRet )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Invalid GPU ID %u", gpuMetricData.gpuId );
    }
    else
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Failed to get GPU utilization for gpu %u, status = %u", gpuMetricData.gpuId,
                      resMonRet );
    }

    return ret;
}

QCStatus_e ResMonitor::GetNspUtil( ResMon_NSP_MetricData_t &nspMetricData )
{
    QCStatus_e ret = QC_STATUS_OK;
    CompResMonRet_e resMonRet = COMPRESMON_RET_SUCCESS;

    uint64_t startTime = 0;
    uint64_t endTime = 0;
    NspUtilStats_t nspStats = { 0 };
    NspId_e nspId = (NspId_e) nspMetricData.nspId;

    auto t0 = std::chrono::high_resolution_clock::now();
    resMonRet = CompResmonQueryNspUtil( nspId, &nspStats );
    auto t1 = std::chrono::high_resolution_clock::now();

    startTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t0.time_since_epoch() ).count();
    endTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t1.time_since_epoch() ).count();

    if ( COMPRESMON_RET_SUCCESS == resMonRet )
    {
        nspMetricData.sampleTimeData.startTime = startTime;
        nspMetricData.sampleTimeData.endTime = endTime;
        nspMetricData.hvxUtil = nspStats.hvxUtilPct;
        nspMetricData.hmxUtil = nspStats.hmxUtilPct;
        nspMetricData.q6Load = nspStats.q6LoadKcps;
    }
    else if ( COMPRESMON_RET_ENVLD == resMonRet )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Invalid NSP ID %u", nspMetricData.nspId );
    }
    else
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Failed to get NSP utilization for nsp %u, status = %u", nspMetricData.nspId,
                      resMonRet );
    }

    return ret;
}

QCStatus_e ResMonitor::GetDdrBW( ResMon_DDR_MetricData_t &ddrMetricData )
{
    QCStatus_e ret = QC_STATUS_OK;
    CompResMonRet_e resMonRet = COMPRESMON_RET_SUCCESS;

    uint64_t startTime = 0;
    uint64_t endTime = 0;
    DdrBwStats_t ddrStats = { 0 };

    auto t0 = std::chrono::high_resolution_clock::now();
    resMonRet = CompResmonQueryDdrBwUtil( &ddrStats );
    auto t1 = std::chrono::high_resolution_clock::now();

    startTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t0.time_since_epoch() ).count();
    endTime =
            std::chrono::duration_cast<std::chrono::microseconds>( t1.time_since_epoch() ).count();

    if ( COMPRESMON_RET_SUCCESS == resMonRet )
    {
        ddrMetricData.sampleTimeData.startTime = startTime;
        ddrMetricData.sampleTimeData.endTime = endTime;
        ddrMetricData.ddrTotalBw = ddrStats.ddrTotalBw;
        ddrMetricData.ddrReadBw = ddrStats.ddrReadBw;
        ddrMetricData.ddrWriteBw = ddrStats.ddrWriteBw;
        ddrMetricData.llcTotalBw = ddrStats.llcTotalBw;
        ddrMetricData.llcReadBw = ddrStats.llcReadBw;
        ddrMetricData.llcWriteBw = ddrStats.llcWriteBw;
        ddrMetricData.nocBwApps0 = ddrStats.nocBwApps0;
        ddrMetricData.nocBwApps1 = ddrStats.nocBwApps1;
        ddrMetricData.nocBwGpu0 = ddrStats.nocBwGpu0;
        ddrMetricData.nocBwNsp0 = ddrStats.nocBwNsp0;
        ddrMetricData.nocBwNsp1 = ddrStats.nocBwNsp1;
        ddrMetricData.nocBwNsp2 = ddrStats.nocBwNsp2;
        ddrMetricData.nocBwNsp3 = ddrStats.nocBwNsp3;
    }
    else if ( COMPRESMON_RET_ENVLD == resMonRet )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Invalid DDR BW Type" );
    }
    else
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Failed to get DDR BW, status = %u", resMonRet );
    }

    return ret;
}

QCStatus_e ResMonitor::Init()
{
    QCStatus_e ret = QC_STATUS_OK;
    CompResMonRet_e resMonRet = COMPRESMON_RET_SUCCESS;

    resMonRet = CompResmonRegister();
    if ( COMPRESMON_RET_SUCCESS != resMonRet )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Failed to register Resource Monitor, status = %u", resMonRet );
    }

    return ret;
}

QCStatus_e ResMonitor::DeInit()
{
    QCStatus_e ret = QC_STATUS_OK;
    CompResMonRet_e resMonRet = COMPRESMON_RET_SUCCESS;

    resMonRet = CompResmonUnregister();
    if ( COMPRESMON_RET_SUCCESS != resMonRet )
    {
        ret = QC_STATUS_FAIL;
        QC_LOG_ERROR( "Failed to deregister Resource Monitor, status = %u", resMonRet );
    }

    return ret;
}

}   // namespace sample
}   // namespace QC

