// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_POST_CENTERPOINT_HPP
#define QC_POST_CENTERPOINT_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "FadasPlrPostProc.hpp"
#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/sample/BufferManager.hpp"

namespace QC
{
namespace sample
{
using namespace QC::libs::FadasIface;
using namespace QC::Memory;

typedef struct
{
    uint32_t maxNumFilter; /**< Maximum number of filtered boxes allowed after enabling BBox
                            * filter. */
    float minCentreX;      /**< Min values of x used for range check for centre of detection. */
    float minCentreY;      /**< Min values of y used for range check for centre of detection. */
    float minCentreZ;      /**< Min values of z used for range check for centre of detection. */
    float maxCentreX;      /**< Max values of x used for range check for centre of detection. */
    float maxCentreY;      /**< Max values of y used for range check for centre of detection. */
    float maxCentreZ;      /**< Max values of z used for range check for centre of detection. */
    bool *labelSelect;     /**< Pointer to buffer providing selection/exclusion status of labels. */
} PostCenterPoint_3DBBoxFilterParams_t;

/** @brief PostCenterPoint component configuration */
typedef struct
{
    QCProcessorType_e processor; /**< processor type */
    float pillarXSize;           /**< Pillar size in x direction in meters. */
    float pillarYSize;           /**< Pillar size in y direction in meters. */
    float minXRange;             /**< Minimum range value in x direction. */
    float minYRange;             /**< Minimum range value in y direction. */
    float maxXRange;             /**< Maximum range value in x direction.  */
    float maxYRange;             /**< Maximum range value in y direction. */
    uint32_t numClass;           /**< Number of classes detected by the network. */
    uint32_t maxNumInPts;        /**< Maximum number of points in the input point cloud data. */
    uint32_t numInFeatureDim; /**< Number of features for each point in the input point cloud data.
                               * For e.g., if point cloud data contains (x, y, z, r) features for
                               * each point, then numInFeatureDim is 4. */
    uint32_t maxNumDetOut;    /**< Maximum number of 3D bounding boxes expected in the output. */

    uint32_t stride; /**< Downsample ratio of the center head */

    float32_t threshScore; /**< Confidence score threshold. */
    float32_t threshIOU;   /**< Overlap threshold. */

    PostCenterPoint_3DBBoxFilterParams_t filterParams; /* Filtering parameters */

    /**< Flag to enable/disable mapping of point cloud points to 3D bounding box. */
    bool bMapPtsToBBox;

    /**< Flag to enable/disable filtering of 3D bounding boxes based on min/max
     * ranges provided for centre of box, distance from LiDAR & specified
     * class labels before mapping point cloud points to 3D bounding box.
     * NOTE: This flag will be used only if bMapPtsToBBox is set to true. */
    bool bBBoxFilter;

    uint32_t nodeId; /**< a unique instance ID used by buffer manager */
} PostCenterPoint_Config_t;

typedef struct
{
    int label;       /**< The object label ID */
    float score;     /**< The object confidence score */
    float x;         /**< The x co-ordinate of centre point */
    float y;         /**< The y co-ordinate of centre point */
    float z;         /**< The z co-ordinate of centre point */
    float length;    /**< The length of the 3D box */
    float width;     /**< The width of the 3D box */
    float height;    /**< The height of the 3D box */
    float theta;     /**< The orientation of the 3D box relative to the viewing angle */
    float meanPtX;   /**< The mean values of x coordinates of all points inside this bounding box */
    float meanPtY;   /**< The mean values of y coordinates of all points inside this bounding box */
    float meanPtZ;   /**< The mean values of z coordinates of all points inside this bounding box */
    uint32_t numPts; /**< Number of 3D points located inside this bounding box */
    float meanIntensity; /**< Mean of intensities of all points inside the bounding box */
} PostCenterPoint_Object3D_t;

#define POSTCENTERPOINT_OBJECT_3D_DIM ( sizeof( PostCenterPoint_Object3D_t ) / sizeof( float ) )

/**
 * @brief PostCenterPoint
 * Extracts and filters bounding boxes from the center point network output.
 */
class PostCenterPoint
{

public:
    PostCenterPoint( Logger &logger );
    ~PostCenterPoint();

    /**
     * @brief Initialize the center point post processing pipeline
     * @param[in] pName the PostCenterPoint unique instance name
     * @param[in] pConfig the PostCenterPoint configuration paramaters
     * @param[in] level the logger message level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const PostCenterPoint_Config_t *pConfig );

    /**
     * @brief Register buffers for PostCenterPoint
     * @param[in] pBuffers a list of buffers to be registered
     * @param[in] numBuffers number of buffers
     * @param[in] bufferType buffer type, could be IN, OUT, INOUT
     * @note Calling this API to register all input/output buffers for the PostCenterPoint during
     * the initialization phase is recommended. However, if the input buffers are not known at
     * initialization, it’s also acceptable to skip this step. The Execute API will handle
     * registration once, in case the buffer hasn’t been registered before.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterBuffers( const TensorDescriptor_t *pBuffers, uint32_t numBuffers,
                                FadasBufType_e bufferType );

    /**
     * @brief Start the PostCenterPoint pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief Execute the PostCenterPoint pipeline
     * @param[in] pHeatmap Pointer to heatmap buffer
     *            it's float32 tensor with shape [1, height, width, numClass]
     * @param[in] pXY Pointer to buffer containing x,y co-ordinates of center point
     *            it's float32 tensor with shape [1, height, width, 2]
     * @param[in] pZ Pointer to buffer containing z co-ordinate of center point
     *            it's float32 tensor with shape [1, height, width, 1]
     * @param[in] pSize Pointer to buffer containing length, width, height for each detection
     *            it's float32 tensor with shape [1, height, width, 3]
     * @param[in] pTheta Pointer to buffer containing orientation of each detection
     *            it's float32 tensor with shape [1, height, width, 2]
     * @param[in] pInPts The input point cloud where size in bytes
     *            it's float32 tensor with shape [numPts, numInFeatureDim]
     *             NOTE: pInPts is nullptr if bMapPtsToBBox is true.
     * @param[out] pDetections Pointer to buffer that represent 3D bounding box
     *            it's float32 tensor with shape [N, PostCenterPoint_Object3D_t]
     *                   [N, ( label,   score,   x,      y,       z,      length,
     *                         width,   height,  theta,  meanPtX, meanPtY, meanPtZ,
     *                         numPts,  meanIntensity )]
     * @note For the width/height of the pHeatmap/pXY/pZ/pSize/pTheta, it can be calculated
     * with below formula:
     *     height = ( maxYRange - minYRange ) / pillarYSize / stride
     *     width  = ( maxXRange - minXRange ) / pillarXSize / stride
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( const TensorDescriptor_t *pHeatmap, const TensorDescriptor_t *pXY,
                        const TensorDescriptor_t *pZ, const TensorDescriptor_t *pSize,
                        const TensorDescriptor_t *pTheta, const TensorDescriptor_t *pInPts,
                        TensorDescriptor_t *pDetections );

    /**
     * @brief Stop the PostCenterPoint pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief Deregister buffers for PostCenterPoint
     * @param[in] pBuffers a list of buffers to be deregistered
     * @param[in] numBuffers number of buffers
     * @note Although it is recommended to call this API to deregister all input/output buffers
     * before invoking the Deinit API to release resources, doing so is optional. If this API is not
     * called, the Deinit API will automatically handle deregistering any input/output buffers that
     * were previously registered.
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeRegisterBuffers( const TensorDescriptor_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the PostCenterPoint pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

private:
    QCStatus_e RegisterBuffersToFadas( const TensorDescriptor_t *pBuffers, uint32_t numBuffers,
                                       FadasBufType_e bufferType );

    QCStatus_e DeRegisterBuffersToFadas( const TensorDescriptor_t *pBuffers, uint32_t numBuffers );

private:
    PostCenterPoint_Config_t m_config;

    FadasPlrPostProc m_plrPost;

    uint32_t m_height;
    uint32_t m_width;

    TensorDescriptor_t m_BBoxList;
    TensorDescriptor_t m_labels;
    TensorDescriptor_t m_scores;
    TensorDescriptor_t m_metadata;

    BufferManager *m_pBufMgr = nullptr;
    Logger &m_logger;
    QCObjectState_e m_state = QC_OBJECT_STATE_INITIAL;

};   // class PostCenterPoint

}   // namespace sample
}   // namespace QC

#endif   // QC_POST_CENTERPOINT_HPP
