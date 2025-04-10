// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_VOXELIZATION_HPP
#define RIDEHAL_VOXELIZATION_HPP

#include <cinttypes>
#include <cmath>
#include <inttypes.h>
#include <memory>
#include <string.h>
#include <unistd.h>

#include "FadasPlr.hpp"
#include "OpenclIface.hpp"
#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::FadasIface;
using namespace ridehal::libs::OpenclIface;

namespace ridehal
{
namespace component
{

#define VOXELIZATION_PILLAR_COORDS_DIM ( sizeof( FadasVM_PointPillar_t ) / sizeof( float ) )

/**< voxelization input pointclouds type used for OpenCL implementation.
 * for VOXELIZATION_INPUT_XYZR, the input pcd file have 4 dimensions [x,y,z,r],
 * and the 10 output points in pillar features are
 * [x,y,z,r,x-xMean,y-yMean,z-zMean,x-xPillar,y-yPillar,z-zPillar].
 * for VOXELIZATION_INPUT_XYZRT, the input pcd file have 5 dimensions [x,y,z,r,t],
 * and the 10 output points in pillar features are
 * [x,y,z,r,t,x-xMean,y-yMean,z-zMean,x-xPillar,y-yPillar]. */
typedef enum
{
    VOXELIZATION_INPUT_XYZR,
    VOXELIZATION_INPUT_XYZRT,
} Voxelization_InputMode_e;

/** @brief Voxelization component configuration */
typedef struct
{
    RideHal_ProcessorType_e processor; /**< processor type */
    float pillarXSize;                 /**< Pillar size in x direction in meters. */
    float pillarYSize;                 /**< Pillar size in y direction in meters. */
    float pillarZSize;                 /**< Pillar size in z direction in meters. */
    float minXRange;                   /**< Minimum range value in x direction. */
    float minYRange;                   /**< Minimum range value in y direction. */
    float minZRange;                   /**< Minimum range value in z direction. */
    float maxXRange;                   /**< Maximum range value in x direction.  */
    float maxYRange;                   /**< Maximum range value in y direction. */
    float maxZRange;                   /**< Maximum range value in z direction. */
    uint32_t maxNumInPts;              /**< Maximum number of points in input point cloud. */
    uint32_t numInFeatureDim;  /**< Number of features for each point in the input point cloud data.
                                * For e.g., if point cloud data contains (x, y, z, r) features for
                                * each point, then numInFeatureDim is 4. */
    uint32_t maxNumPlrs;       /**< Maximum number of point pillars that can be created. */
    uint32_t maxNumPtsPerPlr;  /**< Maximum number of points to map to each pillar. */
    uint32_t numOutFeatureDim; /**< Number of features for each point in point pillars. */
    Voxelization_InputMode_e inputMode =
            VOXELIZATION_INPUT_XYZR; /**< voxelization input pointclouds type, default value is
                                        VOXELIZATION_INPUT_XYZR. */
} Voxelization_Config_t;

/**
 * @brief Voxelization
 * Component Voxelization that creates point pillars from point cloud data.
 */
class Voxelization : public ComponentIF
{

public:
    Voxelization();
    ~Voxelization();

    /**
     * @brief Initialize the voxelization pipeline
     * @param[in] pName the voxelization unique instance name
     * @param[in] pConfig the voxelization configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const Voxelization_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for voxelization
     * @param[in] pBuffers a list of buffers to be registered
     * @param[in] numBuffers number of buffers
     * @param[in] bufferType buffer type, could be IN, OUT, INOUT
     * @note Calling this API to register all input/output buffers for voxelization during the
     * initialization phase is recommended. However, if the input buffers are not known at
     * initialization, it’s also acceptable to skip this step. The Execute API will handle
     * registration once, in case the buffer hasn’t been registered previously.
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers,
                                    FadasBufType_e bufferType );

    /**
     * @brief Start the voxelization pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief Execute the voxelization pipeline
     * @param[in] pInPts The input point cloud where size in bytes
     *                 is maxNumInPts x numInFeatureDim x sizeof(float32_t).
     * @param[out] pOutPlrs The output pillar index tensor where memory (in bytes)
     *                 for each pillar is maxNumPlrs x 4 x sizeof(float32_t)
     * @param[out] pOutFeature The output stacked pillar tensor where
     *                 memory size in bytes for all pillars is
     *                 maxNumPlrs * maxNumPtsPerPlr x numOutFeatureDim x sizeof(float32_t)
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInPts,
                            const RideHal_SharedBuffer_t *pOutPlrs,
                            const RideHal_SharedBuffer_t *pOutFeature );

    /**
     * @brief Stop the voxelization pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief Deregister buffers for voxelization
     * @param[in] pBuffers a list of buffers to be deregistered
     * @param[in] numBuffers number of buffers
     * @note Although it is recommended to call this API to deregister all input/output buffers
     * before invoking the Deinit API to release resources, doing so is optional. If this API is not
     * called, the Deinit API will automatically handle deregistering any input/output buffers that
     * were previously registered.
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the voxelization pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

private:
    Voxelization_Config_t m_config;
    FadasPlrPreProc m_plrPre;
    OpenclSrv m_OpenclSrvObj;
    cl_kernel m_kernel1;                   /*OpenCL kernel for ClusterPoints*/
    cl_kernel m_kernel2;                   /*OpenCL kernel for FeatureGather*/
    RideHal_SharedBuffer_t m_numOfPts;     /*internal buffer used by OpenCL kernel*/
    RideHal_SharedBuffer_t m_coorToPlrIdx; /*internal buffer used by OpenCL kernel*/

private:
    RideHalError_e ExecuteCL( const RideHal_SharedBuffer_t *pInPts,
                              const RideHal_SharedBuffer_t *pOutPlrs,
                              const RideHal_SharedBuffer_t *pOutFeature );

};   // class Voxelization

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_VOXELIZATION_HPP
