// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/sample/SamplePlrPost.hpp"
#include <cmath>
#include <math.h>

namespace QC
{
namespace sample
{

SamplePlrPost::SamplePlrPost() : m_plrPost( m_logger ) {}
SamplePlrPost::~SamplePlrPost() {}

QCStatus_e SamplePlrPost::ParseConfig( SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    m_config.processor = Get( config, "processor", QC_PROCESSOR_HTP0 );
    if ( QC_PROCESSOR_MAX == m_config.processor )
    {
        QC_ERROR( "invalid processor %s\n", Get( config, "processor", "" ).c_str() );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_config.pillarXSize = Get( config, "pillar_size_x", 0.16f );
    m_config.pillarYSize = Get( config, "pillar_size_y", 0.16f );
    m_config.minXRange = Get( config, "min_x", 0.0f );
    m_config.minYRange = Get( config, "min_y", -39.68f );
    m_config.maxXRange = Get( config, "max_x", 69.12f );
    m_config.maxYRange = Get( config, "max_y", 39.68f );
    m_config.numClass = Get( config, "num_class", 3 );
    m_config.maxNumInPts = Get( config, "max_points", 300000 );
    m_config.numInFeatureDim = Get( config, "in_feature_dim", 4 );
    m_config.maxNumDetOut = Get( config, "max_det_out", 500 );
    m_config.stride = Get( config, "stride", 2 );
    m_config.threshScore = Get( config, "thresh_score", 0.4f );
    m_config.threshIOU = Get( config, "thresh_iou", 0.4f );
    m_config.bMapPtsToBBox = Get( config, "map_points_to_bbox", false );
    m_config.bBBoxFilter = false;

    m_config.nodeId = m_nodeId.id;

    m_offsetX = Get( config, "offset_x", 514 );
    m_offsetY = Get( config, "offset_y", 0 );
    m_ratioW = Get( config, "ratio_w", 12.903225806451614f );
    m_ratioH = Get( config, "ratio_h", 12.903225806451614f );

    m_bDebug = Get( config, "debug", false );

    m_indexs = Get( config, "output_indexs", m_indexs );
    QC_INFO( "output indexs = [%u %u %u %u %u]", m_indexs[0], m_indexs[1], m_indexs[2], m_indexs[3],
             m_indexs[4] );

    m_poolSize = Get( config, "pool_size", 4 );
    if ( 0 == m_poolSize )
    {
        QC_ERROR( "invalid pool_size = %d\n", m_poolSize );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

    m_inputLidarTopicName = Get( config, "input_lidar_topic", "" );
    if ( "" == m_inputLidarTopicName )
    {
        QC_ERROR( "no input lidar topic\n" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }

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

    return ret;
}

QCStatus_e SamplePlrPost::Init( std::string name, SampleConfig_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = SampleIF::Init( name );
    if ( QC_STATUS_OK == ret )
    {
        ret = ParseConfig( config );
    }

    if ( QC_STATUS_OK == ret )
    {
        QCTensorProps_t detTsProp = {
                QC_TENSOR_TYPE_FLOAT_32,
                { m_config.maxNumDetOut, POSTCENTERPOINT_OBJECT_3D_DIM },
                2,
        };

        ret = m_objsPool.Init( name, m_nodeId, LOGGER_LEVEL_INFO, m_poolSize, detTsProp,
                               QC_MEMORY_ALLOCATOR_DMA_HTP );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Init( m_config.processor );
    }

    if ( QC_STATUS_OK == ret )
    {
        TRACE_BEGIN( SYSTRACE_TASK_INIT );
        ret = m_plrPost.Init( name.c_str(), &m_config );
        TRACE_END( SYSTRACE_TASK_INIT );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_lidarSub.Init( name, m_inputLidarTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_infSub.Init( name, m_inputTopicName );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = m_pub.Init( name, m_outputTopicName );
    }

    return ret;
}

QCStatus_e SamplePlrPost::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_START );
    ret = m_plrPost.Start();
    TRACE_END( SYSTRACE_TASK_START );
    if ( QC_STATUS_OK == ret )
    {
        m_stop = false;
        m_thread = std::thread( &SamplePlrPost::ThreadMain, this );
    }

    return ret;
}

/*
 *  pt0                pt1
 *  +-------------------+
 *  |                   |
 *  |        *(center)  |
 *  |                   |
 *  +-------------------+
 *  pt3                pt2
 *
 *                                    Y
 *                                    ^
 * ------+----------------------------|---------------------------------+----> x
 *    ^  |          +-----------------|-----------------+ maxY          |
 *    |  |          |                 |                 |               |
 *    |  |          |                 |                 |               |
 *    |  |          |                 |                 |               |
 *    |  |          |                 |                 |               |
 *    |  |          |                 +-----------------------------------------> X
 *    H  |     minX |                                   |maxX           |
 *    |  |          |                                   |               |
 *    |  |          |                                   |               |
 *    |  |          |                                   |               |
 *    V  |          +-----------------------------------+ minY          |
 * ------+----------------------------|---------------------------------+
 *       |<-------------------------- W ------------------------------->|
 *       V
 *       y
 */
Point2D_t SamplePlrPost::ProjectToImage( Point2D_t &pt, Point2D_t &center, float yaw )
{
    Point2D_t imgPt;
    float yaw_ = yaw - M_PI / 2;

    imgPt.x = cos( yaw_ ) * pt.x + sin( yaw_ ) * pt.y + center.x;
    imgPt.y = -sin( yaw_ ) * pt.x + cos( yaw_ ) * pt.y + center.y;

    imgPt.x = m_offsetX + m_ratioW * ( imgPt.x - m_config.minXRange );
    imgPt.y = m_offsetY + m_ratioH * ( m_config.maxYRange - imgPt.y );

    imgPt.x = std::round( imgPt.x );
    imgPt.y = std::round( imgPt.y );

    return imgPt;
}

void SamplePlrPost::ThreadMain()
{
    QCStatus_e ret;
    while ( false == m_stop )
    {
        DataFrames_t lidarFrames;
        ret = m_lidarSub.Receive( lidarFrames );
        if ( QC_STATUS_OK == ret )
        {
            QC_DEBUG( "receive lidar frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                      lidarFrames.FrameId( 0 ), lidarFrames.Timestamp( 0 ) );
            DataFrames_t infFrames;
            /* lidar frame inference generally in 10 fps */
            ret = m_infSub.Receive( infFrames, 500 );
            if ( QC_STATUS_OK == ret )
            {
                QC_DEBUG( "receive inference frameId %" PRIu64 ", timestamp %" PRIu64 "\n",
                          infFrames.FrameId( 0 ), infFrames.Timestamp( 0 ) );
                std::shared_ptr<SharedBuffer_t> detOut = m_objsPool.Get();
                if ( nullptr != detOut )
                {
                    TensorDescriptor_t *pDetOutDesc =
                            dynamic_cast<TensorDescriptor_t *>( &detOut->GetBuffer() );
                    TensorDescriptor_t *pInPts =
                            static_cast<TensorDescriptor_t *>( &lidarFrames.GetBuffer( 0 ) );
                    TensorDescriptor_t *pHeatmap = static_cast<TensorDescriptor_t *>(
                            &infFrames.GetBuffer( m_indexs[0] ) );
                    TensorDescriptor_t *pXY = static_cast<TensorDescriptor_t *>(
                            &infFrames.GetBuffer( m_indexs[1] ) );
                    TensorDescriptor_t *pZ = static_cast<TensorDescriptor_t *>(
                            &infFrames.GetBuffer( m_indexs[2] ) );
                    TensorDescriptor_t *pSize = static_cast<TensorDescriptor_t *>(
                            &infFrames.GetBuffer( m_indexs[3] ) );
                    TensorDescriptor_t *pTheta = static_cast<TensorDescriptor_t *>(
                            &infFrames.GetBuffer( m_indexs[4] ) );

                    ret = SampleIF::Lock();
                    if ( QC_STATUS_OK == ret )
                    {
                        PROFILER_BEGIN();
                        TRACE_BEGIN( infFrames.FrameId( 0 ) );
                        pDetOutDesc->dims[0] = m_config.maxNumDetOut;
                        ret = m_plrPost.Execute( pHeatmap, pXY, pZ, pSize, pTheta, pInPts,
                                                 pDetOutDesc );
                        if ( QC_STATUS_OK == ret )
                        {
                            PROFILER_END();
                            TRACE_END( infFrames.FrameId( 0 ) );
                            Road2DObjects_t objs;
                            PostCenterPoint_Object3D_t *pObj =
                                    (PostCenterPoint_Object3D_t *) pDetOutDesc->GetDataPtr();
                            if ( m_bDebug )
                            {
                                printf( "lidar frameId %" PRIu64 ", number of detections %" PRIu32
                                        "\n",
                                        lidarFrames.FrameId( 0 ), pDetOutDesc->dims[0] );
                            }
                            for ( uint32_t i = 0; i < pDetOutDesc->dims[0]; i++ )
                            {
                                Road2DObject_t obj;
                                obj.classId = pObj->label;
                                obj.prob = pObj->score;
                                Point2D_t center{ pObj->x, pObj->y };
                                Point2D_t pt0{ -pObj->length / 2, pObj->width / 2 };
                                Point2D_t pt1{ pObj->length / 2, pObj->width / 2 };
                                Point2D_t pt2{ pObj->length / 2, -pObj->width / 2 };
                                Point2D_t pt3{ -pObj->length / 2, -pObj->width / 2 };

                                obj.points[0] = ProjectToImage( pt0, center, pObj->theta );
                                obj.points[1] = ProjectToImage( pt1, center, pObj->theta );
                                obj.points[2] = ProjectToImage( pt2, center, pObj->theta );
                                obj.points[3] = ProjectToImage( pt3, center, pObj->theta );
                                objs.objs.push_back( obj );
                                if ( m_bDebug )
                                {
                                    printf( "  [%.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, %.3f, "
                                            "%d],\n",
                                            pObj->x, pObj->y, pObj->z, pObj->length, pObj->width,
                                            pObj->height, pObj->theta, pObj->score, pObj->label );
                                }
                                pObj++;
                            }
                            objs.frameId = infFrames.FrameId( 0 );
                            objs.timestamp = infFrames.Timestamp( 0 );
                            m_pub.Publish( objs );
                        }
                        else
                        {
                            QC_ERROR( "Extract BBox failed for %" PRIu64 " : %d",
                                      infFrames.FrameId( 0 ), ret );
                        }
                        (void) SampleIF::Unlock();
                    }
                }
            }
        }
    }
}

QCStatus_e SamplePlrPost::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    m_stop = true;
    if ( m_thread.joinable() )
    {
        m_thread.join();
    }

    PROFILER_SHOW();

    TRACE_BEGIN( SYSTRACE_TASK_STOP );
    ret = m_plrPost.Stop();
    TRACE_END( SYSTRACE_TASK_STOP );

    return ret;
}

QCStatus_e SamplePlrPost::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    TRACE_BEGIN( SYSTRACE_TASK_DEINIT );
    ret = m_plrPost.Deinit();
    if ( QC_STATUS_OK == ret )
    {
        ret = SampleIF::Deinit();
    }
    TRACE_END( SYSTRACE_TASK_DEINIT );

    return ret;
}

REGISTER_SAMPLE( PlrPost, SamplePlrPost );

}   // namespace sample
}   // namespace QC
