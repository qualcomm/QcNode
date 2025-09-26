// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "QC/component/Voxelization.hpp"
#include "Voxelization.cl.h"

namespace QC
{
namespace component
{

Voxelization::Voxelization() {}

Voxelization::~Voxelization() {}

QCStatus_e Voxelization::Init( const char *pName, const Voxelization_Config_t *pConfig,
                               Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;
    bool bIFInitOK = false;
    bool bFadasInitOK = false;
    bool bOpenclInitOK = false;
    bool bcoorAllocOK = false;

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK == ret )
    {
        bIFInitOK = true;
        if ( nullptr == pConfig )
        {
            QC_ERROR( "pConfig is nullptr!" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_PROCESSOR_GPU == pConfig->processor )
        {
            ret = m_OpenclSrvObj.Init( pName, level );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Init OpenclSrvObj1 failed!" );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                bOpenclInitOK = true;
                ret = m_OpenclSrvObj.LoadFromSource( s_pSourceVoxelization );
            }

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Load kernel from source s_pSourceVoxelization failed!" );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                if ( ( VOXELIZATION_INPUT_XYZR == pConfig->inputMode ) &&
                     ( 4 == pConfig->numInFeatureDim ) )
                {
                    ret = m_OpenclSrvObj.CreateKernel( &m_kernel1, "ClusterPointsFromXYZR" );
                }
                else if ( ( VOXELIZATION_INPUT_XYZRT == pConfig->inputMode ) &&
                          ( 5 == pConfig->numInFeatureDim ) )
                {
                    ret = m_OpenclSrvObj.CreateKernel( &m_kernel1, "ClusterPointsFromXYZRT" );
                }
                else
                {
                    QC_ERROR( "GPU voxelization mode is invalid!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
            }

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Create kernel1 for ClusterPoints failed!" );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                if ( ( VOXELIZATION_INPUT_XYZR == pConfig->inputMode ) &&
                     ( 4 == pConfig->numInFeatureDim ) )
                {
                    ret = m_OpenclSrvObj.CreateKernel( &m_kernel2, "FeatureGatherFromXYZR" );
                }
                else if ( ( VOXELIZATION_INPUT_XYZRT == pConfig->inputMode ) &&
                          ( 5 == pConfig->numInFeatureDim ) )
                {
                    ret = m_OpenclSrvObj.CreateKernel( &m_kernel2, "FeatureGatherFromXYZRT" );
                }
                else
                {
                    QC_ERROR( "GPU voxelization mode is invalid!" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
            }

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Create kernel2 for FeatureGather failed!" );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                size_t gridXSize =
                        ceil( ( pConfig->maxXRange - pConfig->minXRange ) / pConfig->pillarXSize );
                size_t gridYSize =
                        ceil( ( pConfig->maxYRange - pConfig->minYRange ) / pConfig->pillarYSize );
                QCTensorProps_t coorToPlrIdxProp = {
                        QC_TENSOR_TYPE_INT_32,
                        { ( uint32_t )( gridXSize * gridYSize * 2 ), 0 },
                        1,
                };
                ret = m_coorToPlrIdx.Allocate( &coorToPlrIdxProp );
            }

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to allocate coorToPlrIdx buffer!" );
            }
            else
            {
                bcoorAllocOK = true;
                QCTensorProps_t outPlrsProp = {
                        QC_TENSOR_TYPE_INT_32,
                        { pConfig->maxNumPlrs + 1,
                          0 }, /* add one more place to record pillar number */
                        1,
                };
                ret = m_numOfPts.Allocate( &outPlrsProp );
            }
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to allocate numOfPts buffer!" );
            }
        }
        else
        {
            if ( VOXELIZATION_INPUT_XYZR != pConfig->inputMode )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Fadas only support 4 dimensions point cloud input!" );
            }
            else
            {
                ret = m_plrPre.Init( pConfig->processor, pName, level );
            }

            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to init FadasPlrPre!" );
            }
            else
            {
                bFadasInitOK = true;
                ret = m_plrPre.SetParams(
                        pConfig->pillarXSize, pConfig->pillarYSize, pConfig->pillarZSize,
                        pConfig->minXRange, pConfig->minYRange, pConfig->minZRange,
                        pConfig->maxXRange, pConfig->maxYRange, pConfig->maxZRange,
                        pConfig->maxNumInPts, pConfig->numInFeatureDim, pConfig->maxNumPlrs,
                        pConfig->maxNumPtsPerPlr, pConfig->numOutFeatureDim );
            }

            if ( QC_STATUS_OK == ret )
            {
                ret = m_plrPre.CreatePreProc();
            }
        }
    }

    if ( ret != QC_STATUS_OK )
    { /* do error clean up */
        QC_ERROR( "PlrPre Init failed: %d!", ret );

        if ( bFadasInitOK )
        {
            (void) m_plrPre.Deinit();
        }
        if ( bOpenclInitOK )
        {
            (void) m_OpenclSrvObj.Deinit();
        }
        if ( bcoorAllocOK )
        {
            (void) m_coorToPlrIdx.Free();
        }
        if ( bIFInitOK )
        {
            (void) ComponentIF::Deinit();
        }
    }
    else
    {
        m_state = QC_OBJECT_STATE_READY;
        m_config = *pConfig;
    }

    return ret;
}

QCStatus_e Voxelization::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "Start is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Voxelization::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "Stop is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Voxelization::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Deinit is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        QCStatus_e ret2;
        if ( QC_PROCESSOR_GPU == m_config.processor )
        {
            ret2 = m_OpenclSrvObj.Deinit();
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "Release OpenclSrvObj resources failed!" );
                ret = ret2;
            }

            ret2 = m_numOfPts.Free();
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "Failed to free numOfPts buffer!" );
                ret = ret2;
            }

            ret2 = m_coorToPlrIdx.Free();
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "Failed to free coorToPlrIdx buffer!" );
                ret = ret2;
            }
        }
        else
        {
            ret2 = m_plrPre.DestroyPreProc();
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "PlrPre DestroyPreProc failed: %d!", ret2 );
                ret = ret2;
            }

            ret2 = m_plrPre.Deinit();
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "PlrPre Deinit failed: %d!", ret2 );
                ret = ret2;
            }
        }

        ret2 = ComponentIF::Deinit();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "Deinit ComponentIF failed!" );
            ret = ret2;
        }
    }

    return ret;
}

QCStatus_e Voxelization::RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers,
                                          FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "PlrPre component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            const QCSharedBuffer_t *pBuf = &pBuffers[i];
            if ( QC_BUFFER_TYPE_TENSOR == pBuf->type )
            {
                if ( QC_PROCESSOR_GPU == m_config.processor )
                {
                    cl_mem bufferCL;
                    ret = m_OpenclSrvObj.RegBuf( &( pBuffers[i].buffer ), &bufferCL );
                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Failed to register buffer[%d] for GPU!", i );
                    }
                }
                else
                {
                    int32_t fd = m_plrPre.RegBuf( pBuf, bufferType );
                    if ( 0 > fd )
                    {
                        QC_ERROR( "Failed to register buffer[%d]!", i );
                        ret = QC_STATUS_FAIL;
                    }
                }
            }
            else
            {
                QC_ERROR( "buffer[%d] is not tensor!", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
    }

    return ret;
}

QCStatus_e Voxelization::DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "PlrPre component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            const QCSharedBuffer_t *pBuf = &pBuffers[i];
            if ( QC_BUFFER_TYPE_TENSOR == pBuf->type )
            {
                if ( QC_PROCESSOR_GPU == m_config.processor )
                {
                    ret = m_OpenclSrvObj.DeregBuf( &( pBuffers[i].buffer ) );
                    if ( QC_STATUS_OK != ret )
                    {
                        QC_ERROR( "Failed to deregister buffer[%d] for GPU!", i );
                    }
                }
                else
                {
                    m_plrPre.DeregBuf( pBuf->data() );
                }
            }
            else
            {
                QC_ERROR( "buffer[%d] is not tensor!", i );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }

            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
    }

    return ret;
}

QCStatus_e Voxelization::ExecuteCL( const QCSharedBuffer_t *pInPts,
                                    const QCSharedBuffer_t *pOutPlrs,
                                    const QCSharedBuffer_t *pOutFeature )
{
    QCStatus_e ret = QC_STATUS_OK;

    bool bRegOK = true;

    cl_mem bufferSrc;
    ret = m_OpenclSrvObj.RegBuf( &( pInPts->buffer ), &bufferSrc );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register input points buffer!" );
        bRegOK = false;
    }

    cl_mem bufferDst1;
    ret = m_OpenclSrvObj.RegBuf( &( pOutPlrs->buffer ), &bufferDst1 );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register output pillars buffer!" );
        bRegOK = false;
    }

    cl_mem bufferDst2;
    ret = m_OpenclSrvObj.RegBuf( &( pOutFeature->buffer ), &bufferDst2 );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register output features buffer!" );
        bRegOK = false;
    }

    /* initialize points number in pillar, pillar number to 0 */
    (void) memset( m_numOfPts.data(), 0, m_numOfPts.size );
    cl_mem bufferNumOfPts;
    ret = m_OpenclSrvObj.RegBuf( &( m_numOfPts.buffer ), &bufferNumOfPts );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register numOfPts buffer!" );
        bRegOK = false;
    }

    size_t gridXSize = ceil( ( m_config.maxXRange - m_config.minXRange ) / m_config.pillarXSize );
    size_t gridYSize = ceil( ( m_config.maxYRange - m_config.minYRange ) / m_config.pillarYSize );
    /* initialize pillar coordinate to -1 */
    (void) memset( m_coorToPlrIdx.data(), -1, m_coorToPlrIdx.size );
    cl_mem bufferCoorToPlr;
    ret = m_OpenclSrvObj.RegBuf( &( m_coorToPlrIdx.buffer ), &bufferCoorToPlr );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to register coorToPlrIdx buffer!" );
        bRegOK = false;
    }

    if ( true == bRegOK )
    {
        size_t numOfArgs1 = 19;
        OpenclIfcae_Arg_t OpenclArgs1[19];
        OpenclArgs1[0].pArg = (void *) &bufferSrc;
        OpenclArgs1[0].argSize = sizeof( cl_mem );
        OpenclArgs1[1].pArg = (void *) &bufferDst1;
        OpenclArgs1[1].argSize = sizeof( cl_mem );
        OpenclArgs1[2].pArg = (void *) &bufferDst2;
        OpenclArgs1[2].argSize = sizeof( cl_mem );
        OpenclArgs1[3].pArg = (void *) &bufferCoorToPlr;
        OpenclArgs1[3].argSize = sizeof( cl_mem );
        OpenclArgs1[4].pArg = (void *) &bufferNumOfPts;
        OpenclArgs1[4].argSize = sizeof( cl_mem );
        OpenclArgs1[5].pArg = (void *) &m_config.minXRange;
        OpenclArgs1[5].argSize = sizeof( cl_float );
        OpenclArgs1[6].pArg = (void *) &m_config.minYRange;
        OpenclArgs1[6].argSize = sizeof( cl_float );
        OpenclArgs1[7].pArg = (void *) &m_config.minZRange;
        OpenclArgs1[7].argSize = sizeof( cl_float );
        OpenclArgs1[8].pArg = (void *) &m_config.maxXRange;
        OpenclArgs1[8].argSize = sizeof( cl_float );
        OpenclArgs1[9].pArg = (void *) &m_config.maxYRange;
        OpenclArgs1[9].argSize = sizeof( cl_float );
        OpenclArgs1[10].pArg = (void *) &m_config.maxZRange;
        OpenclArgs1[10].argSize = sizeof( cl_float );
        OpenclArgs1[11].pArg = (void *) &m_config.pillarXSize;
        OpenclArgs1[11].argSize = sizeof( cl_float );
        OpenclArgs1[12].pArg = (void *) &m_config.pillarYSize;
        OpenclArgs1[12].argSize = sizeof( cl_float );
        OpenclArgs1[13].pArg = (void *) &m_config.pillarZSize;
        OpenclArgs1[13].argSize = sizeof( cl_float );
        OpenclArgs1[14].pArg = (void *) &gridXSize;
        OpenclArgs1[14].argSize = sizeof( cl_int );
        OpenclArgs1[15].pArg = (void *) &gridYSize;
        OpenclArgs1[15].argSize = sizeof( cl_int );
        OpenclArgs1[16].pArg = (void *) &m_config.maxNumPlrs;
        OpenclArgs1[16].argSize = sizeof( cl_int );
        OpenclArgs1[17].pArg = (void *) &m_config.maxNumPtsPerPlr;
        OpenclArgs1[17].argSize = sizeof( cl_int );
        OpenclArgs1[18].pArg = (void *) &m_config.numOutFeatureDim;
        OpenclArgs1[18].argSize = sizeof( cl_int );

        OpenclIface_WorkParams_t OpenclWorkParams1;
        OpenclWorkParams1.workDim = 1;
        size_t numPts = pInPts->tensorProps.dims[0];
        size_t globalWorkSize1[1] = { numPts };
        OpenclWorkParams1.pGlobalWorkSize = globalWorkSize1;
        size_t globalWorkOffset1[1] = { 0 };
        OpenclWorkParams1.pGlobalWorkOffset = globalWorkOffset1;
        /*set local work size to NULL, device would choose optimal size automatically*/
        OpenclWorkParams1.pLocalWorkSize = NULL;

        ret = m_OpenclSrvObj.Execute( &m_kernel1, OpenclArgs1, numOfArgs1, &OpenclWorkParams1 );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to execute ClusterPoints OpenCL kernel!" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            size_t numOfArgs2 = 13;
            OpenclIfcae_Arg_t OpenclArgs2[13];
            OpenclArgs2[0].pArg = (void *) &bufferDst1;
            OpenclArgs2[0].argSize = sizeof( cl_mem );
            OpenclArgs2[1].pArg = (void *) &bufferDst2;
            OpenclArgs2[1].argSize = sizeof( cl_mem );
            OpenclArgs2[2].pArg = (void *) &bufferNumOfPts;
            OpenclArgs2[2].argSize = sizeof( cl_mem );
            OpenclArgs2[3].pArg = (void *) &m_config.minXRange;
            OpenclArgs2[3].argSize = sizeof( cl_float );
            OpenclArgs2[4].pArg = (void *) &m_config.minYRange;
            OpenclArgs2[4].argSize = sizeof( cl_float );
            OpenclArgs2[5].pArg = (void *) &m_config.minZRange;
            OpenclArgs2[5].argSize = sizeof( cl_float );
            OpenclArgs2[6].pArg = (void *) &m_config.pillarXSize;
            OpenclArgs2[6].argSize = sizeof( cl_float );
            OpenclArgs2[7].pArg = (void *) &m_config.pillarYSize;
            OpenclArgs2[7].argSize = sizeof( cl_float );
            OpenclArgs2[8].pArg = (void *) &m_config.pillarZSize;
            OpenclArgs2[8].argSize = sizeof( cl_float );
            OpenclArgs2[9].pArg = (void *) &m_config.maxNumPlrs;
            OpenclArgs2[9].argSize = sizeof( cl_int );
            OpenclArgs2[10].pArg = (void *) &m_config.maxNumPtsPerPlr;
            OpenclArgs2[10].argSize = sizeof( cl_int );
            OpenclArgs2[11].pArg = (void *) &m_config.numOutFeatureDim;
            OpenclArgs2[11].argSize = sizeof( cl_int );
            int numOfPillar = ( (int *) m_numOfPts.data() )[m_config.maxNumPlrs];
            OpenclArgs2[12].pArg = (void *) &numOfPillar;
            OpenclArgs2[12].argSize = sizeof( cl_int );

            OpenclIface_WorkParams_t OpenclWorkParams2;
            OpenclWorkParams2.workDim = 1;
            size_t globalWorkSize2[1] = { m_config.maxNumPlrs };
            OpenclWorkParams2.pGlobalWorkSize = globalWorkSize2;
            size_t globalWorkOffset2[1] = { 0 };
            OpenclWorkParams2.pGlobalWorkOffset = globalWorkOffset2;
            /*set local work size to NULL, device would choose optimal size automatically*/
            OpenclWorkParams2.pLocalWorkSize = NULL;

            ret = m_OpenclSrvObj.Execute( &m_kernel2, OpenclArgs2, numOfArgs2, &OpenclWorkParams2 );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to execute FeatureGather OpenCL kernel!" );
                ret = QC_STATUS_FAIL;
            }
        }
    }

    return ret;
}

QCStatus_e Voxelization::Execute( const QCSharedBuffer_t *pInPts, const QCSharedBuffer_t *pOutPlrs,
                                  const QCSharedBuffer_t *pOutFeature )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Execute is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pInPts )
    {
        QC_ERROR( "pInPts is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pInPts->type ) || ( nullptr == pInPts->buffer.pData ) ||
              ( 2 != pInPts->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pInPts->tensorProps.type ) ||
              ( m_config.numInFeatureDim != pInPts->tensorProps.dims[1] ) )
    {
        QC_ERROR( "pInPts is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pOutPlrs )
    {
        QC_ERROR( "pOutPlrs is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( VOXELIZATION_INPUT_XYZR == m_config.inputMode ) &&
              ( ( QC_BUFFER_TYPE_TENSOR != pOutPlrs->type ) ||
                ( nullptr == pOutPlrs->buffer.pData ) || ( 2 != pOutPlrs->tensorProps.numDims ) ||
                ( QC_TENSOR_TYPE_FLOAT_32 != pOutPlrs->tensorProps.type ) ||
                ( m_config.maxNumPlrs != pOutPlrs->tensorProps.dims[0] ) ||
                ( VOXELIZATION_PILLAR_COORDS_DIM != pOutPlrs->tensorProps.dims[1] ) ) )
    {
        QC_ERROR( "pOutPlrs is invalid for XYZR pointcloud input!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( ( VOXELIZATION_INPUT_XYZRT == m_config.inputMode ) &&
              ( ( QC_BUFFER_TYPE_TENSOR != pOutPlrs->type ) ||
                ( nullptr == pOutPlrs->buffer.pData ) || ( 2 != pOutPlrs->tensorProps.numDims ) ||
                ( QC_TENSOR_TYPE_INT_32 != pOutPlrs->tensorProps.type ) ||
                ( m_config.maxNumPlrs != pOutPlrs->tensorProps.dims[0] ) ||
                ( 2 != pOutPlrs->tensorProps.dims[1] ) ) )
    {
        QC_ERROR( "pOutPlrs is invalid for XYZRT pointcloud input!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pOutFeature )
    {
        QC_ERROR( "pOutFeature is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pOutFeature->type ) ||
              ( nullptr == pOutFeature->buffer.pData ) ||
              ( 3 != pOutFeature->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_FLOAT_32 != pOutFeature->tensorProps.type ) ||
              ( m_config.maxNumPlrs != pOutFeature->tensorProps.dims[0] ) ||
              ( m_config.maxNumPtsPerPlr != pOutFeature->tensorProps.dims[1] ) ||
              ( m_config.numOutFeatureDim != pOutFeature->tensorProps.dims[2] ) )
    {
        QC_ERROR( "pOutFeature is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        if ( QC_PROCESSOR_GPU == m_config.processor )
        {
            ret = ExecuteCL( pInPts, pOutPlrs, pOutFeature );
        }
        else
        {
            ret = m_plrPre.PointPillarRun( pInPts, pOutPlrs, pOutFeature );
        }
    }

    return ret;
}

}   // namespace component
}   // namespace QC
