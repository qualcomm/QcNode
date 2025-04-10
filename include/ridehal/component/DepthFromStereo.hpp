// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef RIDEHAL_DEPTH_FROM_STEREO_HPP
#define RIDEHAL_DEPTH_FROM_STEREO_HPP

#include <cinttypes>
#include <inttypes.h>
#include <map>
#include <memory>
#include <unistd.h>

#include "ridehal/component/ComponentIF.hpp"

#include <eva_dfs.h>
#include <eva_mem.h>
#include <eva_session.h>
#include <eva_utils.h>

using namespace ridehal::common;

namespace ridehal
{
namespace component
{

/** @brief DepthFromStereo component configuration */
typedef struct DepthFromStereo_Config
{
    RideHal_ImageFormat_e format; /**< the input image format */
    uint32_t width;               /**< the input image width */
    uint32_t height;              /**< the input image height */
    uint32_t frameRate;           /**< the input stream frame rate */
    bool bHolefillEnable;
    EvaDFSHoleFillConfig_t holeFillConfig;
    bool bAmFilterEnable;
    uint8_t amFilterConfThreshold;
    EvaDFSP1Config_t p1Config;
    EvaDFSP2Config_t p2Config;
    EvaDFSCensusConfig_t censusConfig;
    EvaDFSSearchDir_e dfsSearchDir;
    uint8_t occlusionConf;
    uint8_t mvVarConf;
    uint8_t mvVarThresh;
    uint8_t flatnessConf;
    uint8_t flatnessThresh;
    bool bFlatnessOverride;
    float rectificationErrorTolerance;
    
public:
    DepthFromStereo_Config();
    DepthFromStereo_Config( const DepthFromStereo_Config &rhs );
    DepthFromStereo_Config &operator=( const DepthFromStereo_Config &rhs );
} DepthFromStereo_Config_t;

class DepthFromStereo : public ComponentIF
{
    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    DepthFromStereo();
    ~DepthFromStereo();

    /**
     * @brief Initialize the DepthFromStereo pipeline
     * @param[in] pName the DepthFromStereo unique instance name
     * @param[in] pConfig the DepthFromStereo configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const DepthFromStereo_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for DepthFromStereo pipeline
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Register buffers for input and output data. This step could be done by user or skipped.
     * If skipped, all the buffers will be registered at execute step.
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Start the DepthFromStereo pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief execute the DepthFromStereo pipeline
     * @param[in] pPriImage the primary image
     * @param[in] pAuxImage the auxiliary image
     * @param[out] pDispMap the disparity map buffer
     * @param[out] pConfMap the disparity confidence map buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pPriImage,
                            const RideHal_SharedBuffer_t *pAuxImage,
                            const RideHal_SharedBuffer_t *pDispMap,
                            const RideHal_SharedBuffer_t *pConfMap );

    /**
     * @brief Stop the DepthFromStereo pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief Deregister buffers for DepthFromStereo pipeline
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Deregister buffers for input and output data. This step could be done by user or
     * skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the DepthFromStereo pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

private:
    static void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent,
                                           void *pSessionUserData );
    void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent );

    RideHalError_e SetInitialFrameConfig();
    RideHalError_e UpdateIconfig( EvaConfigList_t *pConfigList );

    RideHalError_e RegisterEvaMem( const RideHal_SharedBuffer_t *pBuffer, EvaMem_t **pEvaMem );

    RideHalError_e ValidateConfig( const DepthFromStereo_Config_t *pConfig );
    RideHalError_e ValidateImageBuffer( const RideHal_SharedBuffer_t *pImage );

    EvaColorFormat_e GetEvaColorFormat( RideHal_ImageFormat_e colorFormat );

private:
    DepthFromStereo_Config_t m_config;
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;

    EvaSession_t m_hDFSSession = nullptr;
    EvaHandle_t m_hEvaDFS = nullptr;

    EvaImageInfo_t m_imageInfo;
    EvaDFSOutBuffReq_t m_outBufInfo;

    std::map<void *, EvaMem_t> m_evaMemMap;
};   // class DepthFromStereo

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_DEPTH_FROM_STEREO_HPP
