// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <cstring>
#include <stdint.h>
#include <thread>

#include "Voxelization.cl.h"
#include "VoxelizationImpl.hpp"

extern const size_t VOXELIZATION_PILLAR_COORDS_DIM =
        sizeof( FadasVM_PointPillar_t ) / sizeof( float );

namespace QC
{
namespace Node
{

VoxelizationImpl::VoxelizationImpl( QCNodeID_t &nodeId, Logger &logger )
    : m_nodeId( nodeId ),
      m_logger( logger ),
      m_state( QC_OBJECT_STATE_INITIAL )
{}

VoxelizationImpl::~VoxelizationImpl() {}

QCStatus_e
VoxelizationImpl::Initialize( QCNodeEventCallBack_t callback,
                              std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    bool bFadasInitOK = false;
    bool bOpenclInitOK = false;
    m_bufferRegisterOK = false;

    QC_TRACE_INIT( [&]() {
        std::ostringstream oss;
        std::string processor = "unknown";
        switch ( m_config.voxelConfig.processor )
        {
            case QC_PROCESSOR_HTP0:
                processor = "htp0";
                break;
            case QC_PROCESSOR_HTP1:
                processor = "htp1";
                break;
            case QC_PROCESSOR_CPU:
                processor = "cpu";
                break;
            case QC_PROCESSOR_GPU:
                processor = "gpu";
                break;
        }
        oss << "{";
        oss << "\"name\": \"" << m_nodeId.name << "\", ";
        oss << "\"processor\": \"" << processor << "\"";
        oss << "}";
        return oss.str();
    }() );
    QC_TRACE_BEGIN( "Init", {} );

    if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Voxelization not in initial state!" );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        m_inputMode = GetInputMode( m_config.voxelConfig.inputMode );
        m_processor = m_config.voxelConfig.processor;
        m_gridXSize = ceil( ( m_config.voxelConfig.maxXRange - m_config.voxelConfig.minXRange ) /
                            m_config.voxelConfig.pillarXSize );
        m_gridYSize = ceil( ( m_config.voxelConfig.maxYRange - m_config.voxelConfig.minYRange ) /
                            m_config.voxelConfig.pillarYSize );

        if ( QC_PROCESSOR_GPU == m_processor )
        {
            ret = m_openCLSrvObj.Init( m_nodeId.name.c_str(), m_logger.GetLevel() );
            if ( QC_STATUS_OK != ret )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to initialize OpenclSrvObj" );
            }

            if ( QC_STATUS_OK == ret )
            {
                bOpenclInitOK = true;
                ret = m_openCLSrvObj.LoadFromSource( s_pSourceVoxelization );
                if ( QC_STATUS_OK != ret )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to load kernel source" );
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                if ( ( VOXELIZATION_INPUT_MODE_XYZR == m_inputMode ) &&
                     ( 4 == m_config.voxelConfig.numInFeatureDim ) )
                {
                    ret = m_openCLSrvObj.CreateKernel( &m_clusterPointKernel,
                                                       "ClusterPointsFromXYZR" );
                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Failed to create cluster point kernel for INPUT_XYZR mode" );
                    }
                }
                else if ( ( VOXELIZATION_INPUT_MODE_XYZRT == m_inputMode ) &&
                          ( 5 == m_config.voxelConfig.numInFeatureDim ) )
                {
                    ret = m_openCLSrvObj.CreateKernel( &m_clusterPointKernel,
                                                       "ClusterPointsFromXYZRT" );
                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Failed to create cluster point kernel for INPUT_XYZRT mode" );
                    }
                }
                else
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "GPU voxelization mode for cluster point kernel is invalid!" );
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                if ( ( VOXELIZATION_INPUT_MODE_XYZR == m_inputMode ) &&
                     ( 4 == m_config.voxelConfig.numInFeatureDim ) )
                {
                    ret = m_openCLSrvObj.CreateKernel( &m_featGatherKernel,
                                                       "FeatureGatherFromXYZR" );
                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Failed to create feature gather kernel for INPUT_XYZR mode" );
                    }
                }
                else if ( ( VOXELIZATION_INPUT_MODE_XYZRT == m_inputMode ) &&
                          ( 5 == m_config.voxelConfig.numInFeatureDim ) )
                {
                    ret = m_openCLSrvObj.CreateKernel( &m_featGatherKernel,
                                                       "FeatureGatherFromXYZRT" );
                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Failed to create feature gather kernel for INPUT_XYZRT mode" );
                    }
                }
                else
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "GPU voxelization mode for feature gather kernel is invalid!" );
                }
            }
        }
        else
        {
            if ( VOXELIZATION_INPUT_MODE_XYZR != m_inputMode )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Fadas only support 4 dimensions point cloud input!" );
            }
            else
            {
                ret = m_plrPre.Init( m_processor, m_nodeId.name.c_str(), m_logger.GetLevel() );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to init FadasPlrPre" );
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                bFadasInitOK = true;
                ret = m_plrPre.SetParams(
                        m_config.voxelConfig.pillarXSize, m_config.voxelConfig.pillarYSize,
                        m_config.voxelConfig.pillarZSize, m_config.voxelConfig.minXRange,
                        m_config.voxelConfig.minYRange, m_config.voxelConfig.minZRange,
                        m_config.voxelConfig.maxXRange, m_config.voxelConfig.maxYRange,
                        m_config.voxelConfig.maxZRange, m_config.voxelConfig.maxNumInPts,
                        m_config.voxelConfig.numInFeatureDim, m_config.voxelConfig.maxNumPlrs,
                        m_config.voxelConfig.maxNumPtsPerPlr,
                        m_config.voxelConfig.numOutFeatureDim );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to set parameters for FadasPlrPre" );
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                ret = m_plrPre.CreatePreProc();
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to create PreProc for FadasPlrPre" );
                }
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_READY;
        ret = SetupGlobalBufferIdMap();
    }

    if ( QC_STATUS_OK == ret )
    {
        // Register output pillar buffers
        m_outputPlrBufferNum = m_config.outputPlrBufferIds.size();
        for ( uint32_t bufferId : m_config.outputPlrBufferIds )
        {
            if ( bufferId < buffers.size() )
            {
                ret = RegisterBuffer( buffers[bufferId], bufferId, FADAS_BUF_TYPE_OUT );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to register output pillar buffer for buffer id %u",
                              bufferId );
                }
            }
            else
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Output pillar buffer id %u out of range", bufferId );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        // Register output feature buffers
        m_outputFeatureBufferNum = m_config.outputFeatureBufferIds.size();
        for ( uint32_t bufferId : m_config.outputFeatureBufferIds )
        {
            if ( bufferId < buffers.size() )
            {
                ret = RegisterBuffer( buffers[bufferId], bufferId, FADAS_BUF_TYPE_OUT );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to register output feature buffer for buffer id %u",
                              bufferId );
                }
            }
            else
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Output feature buffer id %u out of range", bufferId );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        // Register internal pillar points buffer for OpenCL
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            uint32_t plrPointsBufferId = m_config.plrPointsBufferId;
            if ( plrPointsBufferId < buffers.size() )
            {
                QCBufferDescriptorBase_t &bufDesc = buffers[plrPointsBufferId];
                TensorDescriptor_t *pPlrPointsBuffer =
                        dynamic_cast<TensorDescriptor_t *>( &bufDesc );
                if ( pPlrPointsBuffer != nullptr )
                {
                    m_plrPointsTensor = *pPlrPointsBuffer;
                    ret = m_openCLSrvObj.RegBufferDesc( bufDesc, m_clPlrPointsBuffer );
                    if ( QC_STATUS_OK == ret )
                    {
                        // Mark buffer as registered for cleanup tracking
                        m_clBufferDescMap[m_plrPointsTensor.dmaHandle] = m_clPlrPointsBuffer;
                    }
                    else
                    {
                        QC_ERROR( "Failed to register internal pillar points buffer" );
                    }
                }
                else
                {
                    ret = QC_STATUS_NULL_PTR;
                    QC_ERROR( "Failed to cast pointer for plrPointsBuffer" );
                }
            }
            else
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Internal pillar points buffer id %u out of range", plrPointsBufferId );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        // Register internal coordinate to pillar index buffer
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            uint32_t coordToPlrIdxBufferId = m_config.coordToPlrIdxBufferId;
            if ( coordToPlrIdxBufferId < buffers.size() )
            {
                QCBufferDescriptorBase_t &bufDesc = buffers[coordToPlrIdxBufferId];
                TensorDescriptor_t *pCoordToPlrIdxBuffer =
                        dynamic_cast<TensorDescriptor_t *>( &bufDesc );
                if ( pCoordToPlrIdxBuffer != nullptr )
                {
                    m_coordToPlrIdxTensor = *pCoordToPlrIdxBuffer;
                    ret = m_openCLSrvObj.RegBufferDesc( bufDesc, m_clCoordToPlrIdxBuffer );
                    if ( QC_STATUS_OK == ret )
                    {
                        // Mark buffer as registered for cleanup tracking
                        m_clBufferDescMap[m_coordToPlrIdxTensor.dmaHandle] =
                                m_clCoordToPlrIdxBuffer;
                    }
                    else
                    {
                        QC_ERROR( "Failed to register internal coordinate to pillar index buffer" );
                    }
                }
                else
                {
                    ret = QC_STATUS_NULL_PTR;
                    QC_ERROR( "Failed to cast pointer for coordToPlrIdxBuffer" );
                }
            }
            else
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Internal coordinate to pillar index buffer id %u out of range",
                          coordToPlrIdxBufferId );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_bufferRegisterOK = true;
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            InitOpenCLArgs();
        }
        QC_INFO( "Initialize Voxelization successfully" );
    }
    else
    {
        m_state = QC_OBJECT_STATE_INITIAL;

        if ( bFadasInitOK )
        {
            (void) m_plrPre.Deinit();
        }
        if ( bOpenclInitOK )
        {
            (void) m_openCLSrvObj.Deinit();
        }
        if ( m_bufferRegisterOK )
        {
            for ( uint32_t i = 0; i < buffers.size(); i++ )
            {
                if ( m_clBufferDescMap.find( buffers[i].get().dmaHandle ) !=
                     m_clBufferDescMap.end() )
                {
                    (void) DeRegisterBuffer( buffers[i], i );
                }
            }
            if ( QC_PROCESSOR_GPU == m_processor )
            {
                if ( m_clBufferDescMap.find( m_plrPointsTensor.dmaHandle ) !=
                     m_clBufferDescMap.end() )
                {
                    QCBufferDescriptorBase_t &bufDesc = m_plrPointsTensor;
                    (void) m_openCLSrvObj.DeregBufferDesc( bufDesc );
                    m_clBufferDescMap.erase( m_plrPointsTensor.dmaHandle );
                }
                if ( m_clBufferDescMap.find( m_coordToPlrIdxTensor.dmaHandle ) !=
                     m_clBufferDescMap.end() )
                {
                    QCBufferDescriptorBase_t &bufDesc = m_coordToPlrIdxTensor;
                    (void) m_openCLSrvObj.DeregBufferDesc( bufDesc );
                    m_clBufferDescMap.erase( m_coordToPlrIdxTensor.dmaHandle );
                }
            }
            m_bufferRegisterOK = false;
        }
        QC_ERROR( "Failed to initialize Voxelization, ret = %u!", ret );
    }

    QC_TRACE_END( "Init", {} );

    return ret;
}

QCStatus_e VoxelizationImpl::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Start", {} );

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Voxelization not in ready state: %d", m_state );
    }

    QC_TRACE_END( "Start", {} );

    return ret;
}

QCStatus_e VoxelizationImpl::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;

    TensorDescriptor_t *pInputTensor = nullptr;
    TensorDescriptor_t *pOutputPlrTensor = nullptr;
    TensorDescriptor_t *pOutputFeatTensor = nullptr;

    QC_TRACE_BEGIN( "Execute", { QCNodeTraceArg( "frameId", [&]() {
                        uint64_t frameId = 0;
                        const BufferDescriptor_t *pBufDesc =
                                dynamic_cast<BufferDescriptor_t *>( &frameDesc.GetBuffer(
                                        m_config.globalBufferIdMap[0].globalBufferId ) );
                        if ( nullptr != pBufDesc )
                        {
                            frameId = pBufDesc->id;
                        }
                        return frameId;
                    }() ) } );

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Voxelization node not in running state" );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCBufferDescriptorBase_t &inputBufDesc =
                frameDesc.GetBuffer( m_config.globalBufferIdMap[0].globalBufferId );
        pInputTensor = dynamic_cast<TensorDescriptor_t *>( &inputBufDesc );
        if ( nullptr == pInputTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Input tensor is empty" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QCBufferDescriptorBase_t &outputPlrBufDesc =
                frameDesc.GetBuffer( m_config.globalBufferIdMap[1].globalBufferId );
        pOutputPlrTensor = dynamic_cast<TensorDescriptor_t *>( &outputPlrBufDesc );
        if ( nullptr == pOutputPlrTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Output pillar tensor is empty" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QCBufferDescriptorBase_t &outputFeatBufDesc =
                frameDesc.GetBuffer( m_config.globalBufferIdMap[2].globalBufferId );
        pOutputFeatTensor = dynamic_cast<TensorDescriptor_t *>( &outputFeatBufDesc );
        if ( nullptr == pOutputFeatTensor )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Output feature tensor is empty" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( nullptr == pInputTensor->pBuf ) || ( 2 != pInputTensor->numDims ) ||
             ( QC_TENSOR_TYPE_FLOAT_32 != pInputTensor->tensorType ) ||
             ( m_config.voxelConfig.numInFeatureDim != pInputTensor->dims[1] ) )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Input tensor is invalid!" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( VOXELIZATION_INPUT_MODE_XYZR == m_inputMode )
        {
            if ( ( nullptr == pOutputPlrTensor->pBuf ) || ( 2 != pOutputPlrTensor->numDims ) ||
                 ( QC_TENSOR_TYPE_FLOAT_32 != pOutputPlrTensor->tensorType ) ||
                 ( m_config.voxelConfig.maxNumPlrs != pOutputPlrTensor->dims[0] ) ||
                 ( VOXELIZATION_PILLAR_COORDS_DIM != pOutputPlrTensor->dims[1] ) )
            {
                ret = QC_STATUS_INVALID_BUF;
                QC_ERROR( "Output pillar tensor is invalid for XYZR pointcloud input!" );
            }
        }
        else if ( ( VOXELIZATION_INPUT_MODE_XYZRT == m_inputMode ) )
        {
            if ( ( nullptr == pOutputPlrTensor->pBuf ) || ( 2 != pOutputPlrTensor->numDims ) ||
                 ( QC_TENSOR_TYPE_INT_32 != pOutputPlrTensor->tensorType ) ||
                 ( m_config.voxelConfig.maxNumPlrs != pOutputPlrTensor->dims[0] ) ||
                 ( 2 != pOutputPlrTensor->dims[1] ) )
            {
                ret = QC_STATUS_INVALID_BUF;
                QC_ERROR( "Output pillar tensor is invalid for XYZRT pointcloud input!" );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( nullptr == pOutputFeatTensor->pBuf ) || ( 3 != pOutputFeatTensor->numDims ) ||
             ( QC_TENSOR_TYPE_FLOAT_32 != pOutputFeatTensor->tensorType ) ||
             ( m_config.voxelConfig.maxNumPlrs != pOutputFeatTensor->dims[0] ) ||
             ( m_config.voxelConfig.maxNumPtsPerPlr != pOutputFeatTensor->dims[1] ) ||
             ( m_config.voxelConfig.numOutFeatureDim != pOutputFeatTensor->dims[2] ) )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Output feature tensor is invalid!" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            ret = ProcessCL( *pInputTensor, *pOutputPlrTensor, *pOutputFeatTensor );
        }
        else
        {
            ret = m_plrPre.PointPillarRun( *pInputTensor, *pOutputPlrTensor, *pOutputFeatTensor );
        }
    }

    QC_TRACE_END( "Execute", {} );

    return ret;
}

QCStatus_e VoxelizationImpl::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_TRACE_BEGIN( "Stop", {} );

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Voxelization not in running state: %d", m_state );
    }

    QC_TRACE_END( "Stop", {} );

    return ret;
}

QCStatus_e VoxelizationImpl::DeInitialize()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_TRACE_BEGIN( "DeInit", {} );

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Voxelization not in ready state: %d", m_state );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_PROCESSOR_GPU == m_processor )
        {
            ret = m_openCLSrvObj.Deinit();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Release OpenclSrvObj resources failed: %u", ret );
            }
        }
        else
        {
            ret = m_plrPre.DestroyPreProc();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "PlrPre Deinit failed: %u", ret );
            }
        }

        m_clBufferDescMap.clear();
    }

    QC_TRACE_END( "DeInit", {} );

    return ret;
}

QCObjectState_e VoxelizationImpl::GetState()
{
    return m_state;
}

QCStatus_e VoxelizationImpl::ProcessCL( TensorDescriptor_t &inputTensorDesc,
                                        TensorDescriptor_t &outputPlrTensorDesc,
                                        TensorDescriptor_t &outputFeatTensorDesc )
{
    QCStatus_e ret = QC_STATUS_OK;

    uint64_t inputBufferHandle = inputTensorDesc.dmaHandle;
    uint64_t outputPlrBufferHandle = outputPlrTensorDesc.dmaHandle;
    uint64_t outputFeatBufferHandle = outputFeatTensorDesc.dmaHandle;

    cl_mem clInputBufferMem;
    cl_mem clOutputPlrBufferMem;
    cl_mem clOutputFeatBufferMem;

    if ( m_bufferRegisterOK != true )
    {
        ret = QC_STATUS_INVALID_BUF;
        QC_ERROR( "Buffers are not registered" );
    }

    if ( ret == QC_STATUS_OK )
    {
        auto inputIt = m_clBufferDescMap.find( inputBufferHandle );
        if ( inputIt != m_clBufferDescMap.end() )
        {
            clInputBufferMem = inputIt->second;
        }
        else
        {
            ret = m_openCLSrvObj.RegBufferDesc( inputTensorDesc, clInputBufferMem );
            if ( QC_STATUS_OK == ret )
            {
                m_clBufferDescMap[inputBufferHandle] = clInputBufferMem;
            }
            else
            {
                QC_ERROR( "Failed to register input buffer for OpenCL" );
            }
        }
    }

    if ( ret == QC_STATUS_OK )
    {
        auto outputPlrIt = m_clBufferDescMap.find( outputPlrBufferHandle );
        if ( outputPlrIt != m_clBufferDescMap.end() )
        {
            clOutputPlrBufferMem = outputPlrIt->second;
        }
        else
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "outputPlrBuffer is not registered" );
        }
    }

    if ( ret == QC_STATUS_OK )
    {
        auto outputFeatIt = m_clBufferDescMap.find( outputFeatBufferHandle );
        if ( outputFeatIt != m_clBufferDescMap.end() )
        {
            clOutputFeatBufferMem = outputFeatIt->second;
        }
        else
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "outputFeatBuffer is not registered" );
        }
    }

    if ( ret == QC_STATUS_OK )
    {
        if ( nullptr == m_plrPointsTensor.pBuf || m_plrPointsTensor.size == 0 )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Invalid plrPointsTensor buffer" );
        }
        else if ( nullptr == m_coordToPlrIdxTensor.pBuf || m_coordToPlrIdxTensor.size == 0 )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Invalid coordToPlrIdxTensor buffer" );
        }
        else
        {
            (void) memset( m_plrPointsTensor.pBuf, 0, m_plrPointsTensor.size );
            (void) memset( m_coordToPlrIdxTensor.pBuf, -1, m_coordToPlrIdxTensor.size );
        }
    }

    if ( ret == QC_STATUS_OK )
    {
        m_openCLArgsClusterPoint[0].pArg = (void *) &clInputBufferMem;
        m_openCLArgsClusterPoint[0].argSize = sizeof( cl_mem );
        m_openCLArgsClusterPoint[1].pArg = (void *) &clOutputPlrBufferMem;
        m_openCLArgsClusterPoint[1].argSize = sizeof( cl_mem );
        m_openCLArgsClusterPoint[2].pArg = (void *) &clOutputFeatBufferMem;
        m_openCLArgsClusterPoint[2].argSize = sizeof( cl_mem );
        m_openCLArgsClusterPoint[3].pArg = (void *) &m_clCoordToPlrIdxBuffer;
        m_openCLArgsClusterPoint[3].argSize = sizeof( cl_mem );

        OpenclIface_WorkParams_t OpenclWorkParams1;
        OpenclWorkParams1.workDim = 1;
        size_t numPts = inputTensorDesc.dims[0];
        size_t globalWorkSize1[1] = { numPts };
        OpenclWorkParams1.pGlobalWorkSize = globalWorkSize1;
        size_t globalWorkOffset1[1] = { 0 };
        OpenclWorkParams1.pGlobalWorkOffset = globalWorkOffset1;
        /*set local work size to NULL, device would choose optimal size automatically*/
        OpenclWorkParams1.pLocalWorkSize = NULL;

        ret = m_openCLSrvObj.Execute( &m_clusterPointKernel, m_openCLArgsClusterPoint, m_numOfArgs1,
                                      &OpenclWorkParams1 );

        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to execute FeatureGather OpenCL kernel!" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_openCLArgsFeatGather[0].pArg = (void *) &clOutputPlrBufferMem;
        m_openCLArgsFeatGather[0].argSize = sizeof( cl_mem );
        m_openCLArgsFeatGather[1].pArg = (void *) &clOutputFeatBufferMem;
        m_openCLArgsFeatGather[1].argSize = sizeof( cl_mem );

        OpenclIface_WorkParams_t OpenclWorkParams2;
        OpenclWorkParams2.workDim = 1;
        size_t globalWorkSize2[1] = { m_config.voxelConfig.maxNumPlrs };
        OpenclWorkParams2.pGlobalWorkSize = globalWorkSize2;
        size_t globalWorkOffset2[1] = { 0 };
        OpenclWorkParams2.pGlobalWorkOffset = globalWorkOffset2;
        /*set local work size to NULL, device would choose optimal size automatically*/
        OpenclWorkParams2.pLocalWorkSize = NULL;

        ret = m_openCLSrvObj.Execute( &m_featGatherKernel, m_openCLArgsFeatGather, m_numOfArgs2,
                                      &OpenclWorkParams2 );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to execute FeatureGather OpenCL kernel!" );
        }
    }

    return ret;
}

QCStatus_e VoxelizationImpl::RegisterBuffer( QCBufferDescriptorBase_t &buffer, uint32_t bufferId,
                                             FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint64_t bufferHandle = buffer.dmaHandle;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Voxelization not in ready state: %d", m_state );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_BUFFER_TYPE_TENSOR == buffer.type )
        {
            if ( QC_PROCESSOR_GPU == m_processor )
            {
                if ( m_clBufferDescMap.find( bufferHandle ) == m_clBufferDescMap.end() )
                {
                    cl_mem bufferCL;
                    ret = m_openCLSrvObj.RegBufferDesc( buffer, bufferCL );
                    if ( QC_STATUS_OK == ret )
                    {
                        m_clBufferDescMap[bufferHandle] = bufferCL;
                    }
                    else
                    {
                        QC_ERROR( "Failed to register buffer[%u] for OpenCL", bufferId );
                    }
                }
            }
            else
            {
                int32_t fd = m_plrPre.RegBuf( buffer, bufferType );
                if ( 0 > fd )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "Failed to register buffer[%u] for fadas", bufferId );
                }
            }
        }
        else
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Buffer[%u] is not tensor type", bufferId );
        }
    }

    return ret;
}

QCStatus_e VoxelizationImpl::DeRegisterBuffer( QCBufferDescriptorBase_t &buffer, uint32_t bufferId )
{
    QCStatus_e ret = QC_STATUS_OK;
    uint64_t bufferHandle = buffer.dmaHandle;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Voxelization not in ready or running state: %d", m_state );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_BUFFER_TYPE_TENSOR == buffer.type )
        {
            if ( QC_PROCESSOR_GPU == m_processor )
            {
                ret = m_openCLSrvObj.DeregBufferDesc( buffer );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to deregister buffer[%u] for OpenCL", bufferId );
                }
                if ( m_clBufferDescMap.find( bufferHandle ) != m_clBufferDescMap.end() )
                {
                    m_clBufferDescMap.erase( bufferHandle );
                }
            }
            else
            {
                m_plrPre.DeregBuf( buffer.pBuf );
            }
        }
        else
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "buffer[%u] is not tensor type", bufferId );
        }
    }

    return ret;
}


QCStatus_e VoxelizationImpl::SetupGlobalBufferIdMap()
{
    QCStatus_e ret = QC_STATUS_OK;

    uint32_t bufferNum = 3;

    if ( 0 < m_config.globalBufferIdMap.size() )
    {
        if ( bufferNum != m_config.globalBufferIdMap.size() )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "global buffer map size is not correct: expect %" PRIu32, bufferNum );
        }
    }
    else
    {
        m_config.globalBufferIdMap.resize( bufferNum );

        m_config.globalBufferIdMap[0].name = "inputPcd";
        m_config.globalBufferIdMap[0].globalBufferId = 0;

        m_config.globalBufferIdMap[1].name = "OutputPlr";
        m_config.globalBufferIdMap[1].globalBufferId = 1;

        m_config.globalBufferIdMap[2].name = "OutputFeat";
        m_config.globalBufferIdMap[2].globalBufferId = 2;
    }

    return ret;
}

Voxelization_InputMode_e VoxelizationImpl::GetInputMode( std::string &mode )
{
    Voxelization_InputMode_e inputMode;
    if ( mode == "xyzr" )
    {
        inputMode = VOXELIZATION_INPUT_MODE_XYZR;
    }
    else if ( mode == "xyzrt" )
    {
        inputMode = VOXELIZATION_INPUT_MODE_XYZRT;
    }
    else
    {
        inputMode = VOXELIZATION_INPUT_MODE_MAX;
    }

    return inputMode;
}

void VoxelizationImpl::InitOpenCLArgs()
{
    uint32_t maxPlrNum = m_config.voxelConfig.maxNumPlrs;

    m_openCLArgsClusterPoint[3].pArg = (void *) &m_clCoordToPlrIdxBuffer;
    m_openCLArgsClusterPoint[3].argSize = sizeof( cl_mem );
    m_openCLArgsClusterPoint[4].pArg = (void *) &m_clPlrPointsBuffer;
    m_openCLArgsClusterPoint[4].argSize = sizeof( cl_mem );
    m_openCLArgsClusterPoint[5].pArg = (void *) &m_config.voxelConfig.minXRange;
    m_openCLArgsClusterPoint[5].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[6].pArg = (void *) &m_config.voxelConfig.minYRange;
    m_openCLArgsClusterPoint[6].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[7].pArg = (void *) &m_config.voxelConfig.minZRange;
    m_openCLArgsClusterPoint[7].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[8].pArg = (void *) &m_config.voxelConfig.maxXRange;
    m_openCLArgsClusterPoint[8].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[9].pArg = (void *) &m_config.voxelConfig.maxYRange;
    m_openCLArgsClusterPoint[9].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[10].pArg = (void *) &m_config.voxelConfig.maxZRange;
    m_openCLArgsClusterPoint[10].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[11].pArg = (void *) &m_config.voxelConfig.pillarXSize;
    m_openCLArgsClusterPoint[11].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[12].pArg = (void *) &m_config.voxelConfig.pillarYSize;
    m_openCLArgsClusterPoint[12].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[13].pArg = (void *) &m_config.voxelConfig.pillarZSize;
    m_openCLArgsClusterPoint[13].argSize = sizeof( cl_float );
    m_openCLArgsClusterPoint[14].pArg = (void *) &m_gridXSize;
    m_openCLArgsClusterPoint[14].argSize = sizeof( cl_int );
    m_openCLArgsClusterPoint[15].pArg = (void *) &m_gridYSize;
    m_openCLArgsClusterPoint[15].argSize = sizeof( cl_int );
    m_openCLArgsClusterPoint[16].pArg = (void *) &m_config.voxelConfig.maxNumPlrs;
    m_openCLArgsClusterPoint[16].argSize = sizeof( cl_int );
    m_openCLArgsClusterPoint[17].pArg = (void *) &m_config.voxelConfig.maxNumPtsPerPlr;
    m_openCLArgsClusterPoint[17].argSize = sizeof( cl_int );
    m_openCLArgsClusterPoint[18].pArg = (void *) &m_config.voxelConfig.numOutFeatureDim;
    m_openCLArgsClusterPoint[18].argSize = sizeof( cl_int );

    m_openCLArgsFeatGather[2].pArg = (void *) &m_clPlrPointsBuffer;
    m_openCLArgsFeatGather[2].argSize = sizeof( cl_mem );
    m_openCLArgsFeatGather[3].pArg = (void *) &m_config.voxelConfig.minXRange;
    m_openCLArgsFeatGather[3].argSize = sizeof( cl_float );
    m_openCLArgsFeatGather[4].pArg = (void *) &m_config.voxelConfig.minYRange;
    m_openCLArgsFeatGather[4].argSize = sizeof( cl_float );
    m_openCLArgsFeatGather[5].pArg = (void *) &m_config.voxelConfig.minZRange;
    m_openCLArgsFeatGather[5].argSize = sizeof( cl_float );
    m_openCLArgsFeatGather[6].pArg = (void *) &m_config.voxelConfig.pillarXSize;
    m_openCLArgsFeatGather[6].argSize = sizeof( cl_float );
    m_openCLArgsFeatGather[7].pArg = (void *) &m_config.voxelConfig.pillarYSize;
    m_openCLArgsFeatGather[7].argSize = sizeof( cl_float );
    m_openCLArgsFeatGather[8].pArg = (void *) &m_config.voxelConfig.pillarZSize;
    m_openCLArgsFeatGather[8].argSize = sizeof( cl_float );
    m_openCLArgsFeatGather[9].pArg = (void *) &m_config.voxelConfig.maxNumPlrs;
    m_openCLArgsFeatGather[9].argSize = sizeof( cl_int );
    m_openCLArgsFeatGather[10].pArg = (void *) &m_config.voxelConfig.maxNumPtsPerPlr;
    m_openCLArgsFeatGather[10].argSize = sizeof( cl_int );
    m_openCLArgsFeatGather[11].pArg = (void *) &m_config.voxelConfig.numOutFeatureDim;
    m_openCLArgsFeatGather[11].argSize = sizeof( cl_int );
    int numOfPillar = ( (int *) m_plrPointsTensor.pBuf )[maxPlrNum];
    m_openCLArgsFeatGather[12].pArg = (void *) &numOfPillar;
    m_openCLArgsFeatGather[12].argSize = sizeof( cl_int );
}

}   // namespace Node
}   // namespace QC

