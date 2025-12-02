// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/sample/SampleTemporal.hpp"
#include <algorithm>
#include <cmath>
#include <math.h>

namespace QC
{
namespace sample
{

SampleTemporal::SampleTemporal() {}
SampleTemporal::~SampleTemporal() {}

QCStatus_e SampleTemporal::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_inputTopicName = Get( config, "input_topic", "" );
    if ( "" == m_inputTopicName )
    {
        QC_ERROR( "no input topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_outputTopicName = Get( config, "output_topic", "" );
    if ( "" == m_outputTopicName )
    {
        QC_ERROR( "no output topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_temporalTsProps.tensorType =
            Get( config, "temporal_tensor_type", QC_TENSOR_TYPE_UFIXED_POINT_8 );
    if ( QC_TENSOR_TYPE_MAX == m_temporalTsProps.tensorType )
    {
        QC_ERROR( "invalid temporal_tensor_type\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    std::vector<uint32_t> dimsTemp;
    dimsTemp = Get( config, "temporal_tensor_dims", dimsTemp );
    if ( 0 == dimsTemp.size() )
    {
        QC_ERROR( "invalid temporal_tensor_dims\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    m_temporalTsProps.numDims = dimsTemp.size();
    for ( size_t i = 0; i < dimsTemp.size(); i++ )
    {
        m_temporalTsProps.dims[i] = dimsTemp[i];
    }
    m_temporalQuantScale = Get( config, "temporal_quant_scale", 1.0f );
    m_temporalQuantOffset = Get( config, "temporal_quant_offset", 0 );
    m_temporalIndex = Get( config, "temporal_index", 0u );

    m_useFlagTsProps.tensorType = Get( config, "use_flag_tensor_type", QC_TENSOR_TYPE_MAX );
    if ( QC_TENSOR_TYPE_MAX == m_useFlagTsProps.tensorType )
    {
        QC_INFO( "temporal use flag was not used\n" );
        m_bHasUseFlag = false;
    }
    else
    {
        m_bHasUseFlag = true;
        std::vector<uint32_t> dimsFlag = { 1 };
        dimsFlag = Get( config, "use_flag_tensor_dims", dimsFlag );
        if ( 0 == dimsFlag.size() )
        {
            QC_ERROR( "invalid use_flag_tensor_dims\n" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        m_useFlagTsProps.numDims = dimsFlag.size();
        for ( size_t i = 0; i < dimsFlag.size(); i++ )
        {
            m_useFlagTsProps.dims[i] = dimsFlag[i];
        }
        m_useFlagQuantScale = Get( config, "use_flag_quant_scale", 1.0f );
        m_useFlagQuantOffset = Get( config, "use_flag_quant_offset", 0 );
    }

    m_windowMs = Get( config, "window", 200u );

    return ret;
}

QCStatus_e SampleTemporal::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_sub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_pBufMgr = BufferManager::Get( m_nodeId, m_logger.GetLevel() );
        if ( nullptr == m_pBufMgr )
        {
            QC_ERROR( "Failed to create buffer manager!" );
            ret = QC_STATUS_NOMEM;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pBufMgr->Allocate( m_temporalTsProps, m_initTempTs );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to allocal temporal init tensor!" );
            ret = QC_STATUS_NOMEM;
        }
        else
        {
            m_temporal = std::make_shared<SharedBuffer_t>();
            m_temporal->SetBuffer( m_initTempTs );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( true == m_bHasUseFlag )
        {
            ret = m_pBufMgr->Allocate( m_useFlagTsProps, m_useFlagTs );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to allocal use flag tensor!" );
                ret = QC_STATUS_NOMEM;
            }
            else
            {
                m_useFlag = std::make_shared<SharedBuffer_t>();
                m_useFlag->SetBuffer( m_useFlagTs );

                ret = FillTensor( m_useFlagTs, m_useFlagQuantScale, m_useFlagQuantOffset, 0.0f );
            }
        }
        else
        { /* if without use flag tensor, init temporal with 0 */
            ret = FillTensor( m_initTempTs, m_temporalQuantScale, m_temporalQuantOffset, 0.0f );
        }
    }

    return ret;
}

QCStatus_e SampleTemporal::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = false;
    m_thread = std::thread( &SampleTemporal::ThreadMain, this );


    return ret;
}

QCStatus_e SampleTemporal::FillTensor( TensorDescriptor_t &tensorDesc, float scale, int32_t offset,
                                       float value )
{
    QCStatus_e ret = QC_STATUS_OK;

    switch ( tensorDesc.tensorType )
    {
        case QC_TENSOR_TYPE_FLOAT_32:
        {
            float *pF32 = static_cast<float *>( tensorDesc.GetDataPtr() );
            uint32_t num = tensorDesc.GetDataSize() / sizeof( float );
            std::fill( pF32, pF32 + num, value );
            break;
        }
        case QC_TENSOR_TYPE_UFIXED_POINT_8:
        {
            uint8_t *pU8 = static_cast<uint8_t *>( tensorDesc.GetDataPtr() );
            uint32_t num = tensorDesc.GetDataSize() / sizeof( uint8_t );
            uint8_t quantU8 = (uint8_t) std::min(
                    std::max( std::round( value / scale - offset ), 0.0f ), 255.0f );
            std::fill( pU8, pU8 + num, quantU8 );
            break;
        }
        case QC_TENSOR_TYPE_UFIXED_POINT_16:
        {
            uint16_t *pU16 = static_cast<uint16_t *>( tensorDesc.GetDataPtr() );
            uint32_t num = tensorDesc.GetDataSize() / sizeof( uint16_t );
            uint16_t quantU16 = (uint16_t) std::min(
                    std::max( std::round( value / scale - offset ), 0.0f ), 65535.0f );
            std::fill( pU16, pU16 + num, quantU16 );
            break;
        }
        default:
            QC_ERROR( "tensor with type %d is not supported", tensorDesc.tensorType );
            ret = QC_STATUS_BAD_ARGUMENTS;
            break;
    }

    return ret;
}

void SampleTemporal::ThreadMain()
{
    QCStatus_e ret;
    uint64_t frameId = 0;

    { /* publish the 1st frame */
        DataFrames_t frames;
        {
            DataFrame_t frame;
            frame.buffer = m_temporal;
            frame.frameId = frameId;
            frames.Add( frame );
        }
        if ( true == m_bHasUseFlag )
        {
            (void) FillTensor( m_useFlagTs, m_useFlagQuantScale, m_useFlagQuantOffset, 0.0f );
            DataFrame_t frame;
            frame.buffer = m_useFlag;
            frame.frameId = frameId;
            frames.Add( frame );
        }
        frameId++;
        m_pub.Publish( frames );
    }

    while ( false == m_stop )
    {
        DataFrames_t frames;
        ret = m_sub.Receive( frames, m_windowMs );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive frameId %" PRIu64 ", timestamp %" PRIu64 "\n", frames.FrameId( 0 ),
                      frames.Timestamp( 0 ) );
            if ( m_temporalIndex < frames.frames.size() )
            {
                m_temporal = frames.frames[m_temporalIndex].buffer;
            }
            else
            {
                QC_ERROR( "temporal index %u out of range.", m_temporalIndex );
                break; /* exit this thread as wrong configuration */
            }
        }
        else
        {
            QC_WARN( "reach deadline, publish history data instead." );
        }
        DataFrames_t framesOut;
        {
            DataFrame_t frame;
            frame.buffer = m_temporal;
            frame.frameId = frameId;
            framesOut.Add( frame );
        }
        if ( true == m_bHasUseFlag )
        {
            (void) FillTensor( m_useFlagTs, m_useFlagQuantScale, m_useFlagQuantOffset, 1.0f );
            DataFrame_t frame;
            frame.buffer = m_useFlag;
            frame.frameId = frameId;
            framesOut.Add( frame );
        }
        frameId++;
        m_pub.Publish( framesOut );
    }
}

QCStatus_e SampleTemporal::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    return ret;
}

QCStatus_e SampleTemporal::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( nullptr != m_pBufMgr )
    {
        BufferManager::Put( m_pBufMgr );
        m_pBufMgr = nullptr;
    }
    return ret;
}

REGISTER_SAMPLE( Temporal, SampleTemporal );

}   // namespace sample
}   // namespace QC
