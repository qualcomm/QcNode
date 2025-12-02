// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/SampleResMon.hpp"

namespace QC
{
namespace sample
{

SampleResMon::SampleResMon()
    : m_gpuId( 0 ),
      m_loopMode( true ),
      m_totalTime( 0 ),
      m_sampleInterval( 5000 ),
      m_stop( false )
{}

SampleResMon::~SampleResMon() {}

QCStatus_e SampleResMon::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;
    ResMon_Metrics_e metric = RESMON_METRIC_MAX;

    m_metricNames = Get( config, "metrics", m_metricNames );
    if ( 0 == m_metricNames.size() )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "Invalid metric names" );
    }
    else if ( MAX_METRIC_NUM < m_metricNames.size() )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "Metric num exceeds maximum limit" );
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < m_metricNames.size(); i++ )
        {
            metric = GetResMonMetric( m_metricNames[i] );
            if ( RESMON_METRIC_CPU_UTIL == metric )
            {
                m_bCpuMetricEnable = true;
                printf( "CPU util metric enabled\n" );
            }
            else if ( RESMON_METRIC_GPU_UTIL == metric )
            {
                m_bGpuMetricEnable = true;
                printf( "GPU util metric enabled\n" );
            }
            else if ( RESMON_METRIC_NSP0_UTIL == metric )
            {
                m_bNsp0MetricEnable = true;
                printf( "NSP0 util metric enabled\n" );
            }
            else if ( RESMON_METRIC_NSP1_UTIL == metric )
            {
                m_bNsp1MetricEnable = true;
                printf( "NSP1 util metric enabled\n" );
            }
            else if ( RESMON_METRIC_DDR_BW == metric )
            {
                m_bDdrMetricEnable = true;
                printf( "DDR BW metric enabled\n" );
            }
        }
    }

    m_gpuId = Get( config, "gpu_id", 0 );
    if ( true == m_bGpuMetricEnable )
    {
        m_gpuMetricData.gpuId = m_gpuId;
    }

    m_loopMode = Get( config, "loop_mode", true );

    m_sampleInterval = Get( config, "sample_interval", 5000 );
    if ( ( MIN_SAMPLE_INTERVAL > m_sampleInterval ) || ( MAX_SAMPLE_INTERVAL < m_sampleInterval ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "ResMon sample time interval must be in range of [%u-%u] ms", MIN_SAMPLE_INTERVAL,
                  MAX_SAMPLE_INTERVAL );
    }

    m_totalTime = Get( config, "total_time", 0 );
    if ( ( m_sampleInterval > m_totalTime ) && ( m_loopMode == false ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "ResMon sample time must be larger than sample time interval" );
    }

    return ret;
}

QCStatus_e SampleResMon::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_TRACE_INIT( [&]() {
        std::ostringstream oss;
        oss << "{";
        oss << "\"name\": \"" << name << "\"";
        oss << "}";
        return oss.str();
    }() );

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_resMonitor.Init();
    }

    if ( QC_STATUS_OK == ret )
    {
        m_cpuMetricData = { 0 };
        m_cpuMetricData.sampleTimeData.sampleTime = m_sampleInterval;
    }

    return ret;
}

QCStatus_e SampleResMon::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SampleResMon::ThreadMain, this );
    }

    return ret;
}

void SampleResMon::ThreadMain()
{
    QCStatus_e ret = QC_STATUS_OK;

    uint64_t sampleIdx = 0;
    uint64_t sampleTime = 0;
    uint64_t sampleNum = 0;
    if ( m_sampleInterval > 0 )
    {
        sampleNum = m_totalTime / m_sampleInterval;
    }

    while ( false == m_stop )
    {
        printf( "########## ResMon sample time: %lu ms ##########\n", sampleTime );
        if ( true == m_bCpuMetricEnable )
        {
            ret = GetCpuUtil();
            if ( QC_STATUS_OK == ret )
            {
                QC_TRACE_COUNTER(
                        "CPU Metrics",
                        { QCNodeTraceArg( "CPU total (%)", m_cpuMetricData.totalCpuUtil ) } );
                printf( "CPU total utilization: %lf\n", m_cpuMetricData.totalCpuUtil );
                for ( uint32_t i = 0; i < m_cpuMetricData.cpuNum; i++ )
                {
                    std::stringstream coreMetric;
                    coreMetric << "CPU[" << std::to_string( i ) << "]" << " (%)";
                    QC_TRACE_COUNTER( "CPU Metrics",
                                      { QCNodeTraceArg( coreMetric.str(),
                                                        m_cpuMetricData.perCpuCoreUtil[i] ) } );
                    printf( "CPU core[%u] utilization (%): %lf\n", i,
                            m_cpuMetricData.perCpuCoreUtil[i] );
                }
            }
        }

        if ( true == m_bGpuMetricEnable )
        {
            ret = GetGpuUtil();
            if ( QC_STATUS_OK == ret )
            {
                printf( "GPU %u activate process num: %u\n", m_gpuId, m_gpuMetricData.processNum );
                for ( uint32_t i = 0; i < m_gpuMetricData.processNum; i++ )
                {
                    QC_TRACE_COUNTER(
                            "GPU Metrics",
                            { QCNodeTraceArg( "GPU process id", m_gpuMetricData.processId[i] ),
                              QCNodeTraceArg( "GPU process name", m_gpuMetricData.processName[i] ),
                              QCNodeTraceArg( "GPU busy percentage (%)",
                                              m_gpuMetricData.busyPercentage[i] ) } );
                    printf( "GPU process id: %u\n", m_gpuMetricData.processId[i] );
                    printf( "GPU process name: %s\n", m_gpuMetricData.processName[i] );
                    printf( "GPU busy percentage (%): %lf\n", m_gpuMetricData.busyPercentage[i] );
                }
            }
        }

        if ( true == m_bNsp0MetricEnable )
        {
            m_nspMetricData.nspId = 0;
            ret = GetNspUtil();
            if ( QC_STATUS_OK == ret )
            {
                QC_TRACE_COUNTER( "NSP0 Metrics", { QCNodeTraceArg( "NSP0 Q6 load (kcps)",
                                                                    m_nspMetricData.q6Load ) } );
                QC_TRACE_COUNTER( "NSP0 Metrics", { QCNodeTraceArg( "NSP0 HVX util (%)",
                                                                    m_nspMetricData.hvxUtil ) } );
                QC_TRACE_COUNTER( "NSP0 Metrics", { QCNodeTraceArg( "NSP0 HMX util (%)",
                                                                    m_nspMetricData.hmxUtil ) } );
                printf( "NSP0 Q6 load (kcps): %u\n", m_nspMetricData.q6Load );
                printf( "NSP0 HVX util (%): %u\n", m_nspMetricData.hvxUtil );
                printf( "NSP0 HMX util (%): %u\n", m_nspMetricData.hmxUtil );
            }
        }

        if ( true == m_bNsp1MetricEnable )
        {
            m_nspMetricData.nspId = 1;
            ret = GetNspUtil();
            if ( QC_STATUS_OK == ret )
            {
                QC_TRACE_COUNTER( "NSP1 Metrics", { QCNodeTraceArg( "NSP1 Q6 load (kcps)",
                                                                    m_nspMetricData.q6Load ) } );
                QC_TRACE_COUNTER( "NSP1 Metrics", { QCNodeTraceArg( "NSP1 HVX util (%)",
                                                                    m_nspMetricData.hvxUtil ) } );
                QC_TRACE_COUNTER( "NSP1 Metrics", { QCNodeTraceArg( "NSP1 HMX util (%)",
                                                                    m_nspMetricData.hmxUtil ) } );
                printf( "NSP1 Q6 load (kcps): %u\n", m_nspMetricData.q6Load );
                printf( "NSP1 HVX util (%): %u\n", m_nspMetricData.hvxUtil );
                printf( "NSP1 HMX util (%): %u\n", m_nspMetricData.hmxUtil );
            }
        }

        if ( true == m_bDdrMetricEnable )
        {
            ret = GetDdrBW();
            if ( QC_STATUS_OK == ret )
            {
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR total bw (MB/s)",
                                                                   m_ddrMetricData.ddrTotalBw ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR read bw (MB/s)",
                                                                   m_ddrMetricData.ddrReadBw ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR write bw (MB/s)",
                                                                   m_ddrMetricData.ddrWriteBw ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR LLC total bw (MB/s)",
                                                                   m_ddrMetricData.llcTotalBw ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR LLC read bw (MB/s)",
                                                                   m_ddrMetricData.llcReadBw ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR LLC write bw (MB/s)",
                                                                   m_ddrMetricData.llcWriteBw ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR apps0 noc bw (MB/s)",
                                                                   m_ddrMetricData.nocBwApps0 ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR apps1 noc bw (MB/s)",
                                                                   m_ddrMetricData.nocBwApps1 ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR gpu0 noc bw (MB/s)",
                                                                   m_ddrMetricData.nocBwGpu0 ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR nsp0 noc bw (MB/s)",
                                                                   m_ddrMetricData.nocBwNsp0 ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR nsp1 noc bw (MB/s)",
                                                                   m_ddrMetricData.nocBwNsp1 ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR nsp2 noc bw (MB/s)",
                                                                   m_ddrMetricData.nocBwNsp2 ) } );
                QC_TRACE_COUNTER( "DDR Metrics", { QCNodeTraceArg( "DDR nsp3 noc bw (MB/s)",
                                                                   m_ddrMetricData.nocBwNsp3 ) } );
                printf( "DDR total bw (MB/s): %ld\n", m_ddrMetricData.ddrTotalBw );
                printf( "DDR read bw (MB/s): %ld\n", m_ddrMetricData.ddrReadBw );
                printf( "DDR write bw (MB/s): %ld\n", m_ddrMetricData.ddrWriteBw );
                printf( "DDR LLC total bw (MB/s): %ld\n", m_ddrMetricData.llcTotalBw );
                printf( "DDR LLC read bw (MB/s): %ld\n", m_ddrMetricData.llcReadBw );
                printf( "DDR LLC write bw (MB/s): %ld\n", m_ddrMetricData.llcWriteBw );
                printf( "DDR apps0 noc bw (MB/s): %ld\n", m_ddrMetricData.nocBwApps0 );
                printf( "DDR apps1 noc bw (MB/s): %ld\n", m_ddrMetricData.nocBwApps1 );
                printf( "DDR gpu0 noc bw (MB/s): %ld\n", m_ddrMetricData.nocBwGpu0 );
                printf( "DDR nsp0 noc bw (MB/s): %ld\n", m_ddrMetricData.nocBwNsp0 );
                printf( "DDR nsp1 noc bw (MB/s): %ld\n", m_ddrMetricData.nocBwNsp1 );
                printf( "DDR nsp2 noc bw (MB/s): %ld\n", m_ddrMetricData.nocBwNsp2 );
                printf( "DDR nsp3 noc bw (MB/s): %ld\n", m_ddrMetricData.nocBwNsp3 );
            }
        }

        if ( false == m_loopMode )
        {
            if ( sampleIdx >= sampleNum )
            {
                break;
            }
        }
        sampleIdx++;
        sampleTime += m_sampleInterval;
    }
}

QCStatus_e SampleResMon::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    return ret;
}

QCStatus_e SampleResMon::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_STATUS_OK == ret )
    {
        ret = m_resMonitor.DeInit();
    }

    return ret;
}

QCStatus_e SampleResMon::GetCpuUtil()
{
    return m_resMonitor.GetCpuUtil( m_cpuMetricData );
}

QCStatus_e SampleResMon::GetGpuUtil()
{
    return m_resMonitor.GetGpuUtil( m_gpuMetricData );
}

QCStatus_e SampleResMon::GetNspUtil()
{
    return m_resMonitor.GetNspUtil( m_nspMetricData );
}

QCStatus_e SampleResMon::GetDdrBW()
{
    return m_resMonitor.GetDdrBW( m_ddrMetricData );
}

ResMon_Metrics_e SampleResMon::GetResMonMetric( std::string metricName )
{
    ResMon_Metrics_e metric = RESMON_METRIC_MAX;

    if ( "cpu_util" == metricName )
    {
        metric = RESMON_METRIC_CPU_UTIL;
    }
    else if ( "gpu_util" == metricName )
    {
        metric = RESMON_METRIC_GPU_UTIL;
    }
    else if ( "nsp0_util" == metricName )
    {
        metric = RESMON_METRIC_NSP0_UTIL;
    }
    else if ( "nsp1_util" == metricName )
    {
        metric = RESMON_METRIC_NSP1_UTIL;
    }
    else if ( "ddr_bw" == metricName )
    {
        metric = RESMON_METRIC_DDR_BW;
    }
    else
    {
        QC_ERROR( "Invalid metric name" );
    }

    return metric;
}


REGISTER_SAMPLE( ResMon, SampleResMon );

}   // namespace sample
}   // namespace QC

