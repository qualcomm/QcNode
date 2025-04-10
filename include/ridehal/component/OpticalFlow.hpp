// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef RIDEHAL_OPTICALFLOW_HPP
#define RIDEHAL_OPTICALFLOW_HPP

#include <cinttypes>
#include <inttypes.h>
#include <map>
#include <memory>
#include <unistd.h>

#include "ridehal/component/ComponentIF.hpp"

#include <eva_mem.h>
#include <eva_of.h>
#include <eva_session.h>
#include <eva_utils.h>

using namespace ridehal::common;

namespace ridehal
{
namespace component
{

/** @brief OpticalFlow component configuration */
typedef struct OpticalFlow_Config
{
    RideHal_ImageFormat_e format; /**< the input image format */
    uint32_t width;               /**< the input image width */
    uint32_t height;              /**< the input image height */
    uint32_t frameRate;           /**< the input stream frame rate */
    EvaOFAmFilterConfig_t amFilter;
    EvaOFMvPackFormat_e mvPacketFormat;
    EvaOFDirection_e direction;
    EvaOFQualityLevel_e qualityLevel;
    EvaOFFilterOperationMode_e filterOperationMode;

    bool bAmFilterEnable;
    bool bHolefillEnable;
    EvaOFHoleFillConfig_t holeFillConfig;
    EvaOFP1Config_t p1Config;
    EvaOFP2Config_t p2Config;
    EvaOFCensusConfig_t censusConfig;
    EvaOFConfWeight_t confWeight;
    uint8_t nDsThresh;
    uint8_t nFlatnessThresh;
    bool bFlatnessOverride;
    uint8_t nMvVarThresh;
    EvaOFMvCandPrun_e mvCandPrun;
    bool bDs4MaxOverwrite;
    EvaOFTimeOption_e timeOption;

public:
    OpticalFlow_Config();
    OpticalFlow_Config( const OpticalFlow_Config &rhs );
    OpticalFlow_Config &operator=( const OpticalFlow_Config &rhs );
} OpticalFlow_Config_t;

class OpticalFlow : public ComponentIF
{
    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    OpticalFlow();
    ~OpticalFlow();

    /**
     * @brief Initialize the OpticalFlow pipeline
     * @param[in] pName the OpticalFlow unique instance name
     * @param[in] pConfig the OpticalFlow configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const OpticalFlow_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for OpticalFlow pipeline
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Register buffers for input and output data. This step could be done by user or skipped.
     * If skipped, all the buffers will be registered at execute step.
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Start the OpticalFlow pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief execute the OpticalFlow pipeline
     * @param[in] pRefImage the reference image buffer
     * @param[in] pCurImage the current image buffer
     * @param[out] pOutMvBuf the motion vector map buffer
     * @param[out] pConfBuf the motion vector confidence map buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pRefImage,
                            const RideHal_SharedBuffer_t *pCurImage,
                            const RideHal_SharedBuffer_t *pOutMvBuf,
                            const RideHal_SharedBuffer_t *pConfBuf );

    /**
     * @brief Stop the OpticalFlow pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief Deregister buffers for OpticalFlow pipeline
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Deregister buffers for input and output data. This step could be done by user or
     * skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the OpticalFlow pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

private:
    static void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent,
                                           void *pSessionUserData );
    void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent );

    RideHalError_e SetInitialFrameConfig();
    RideHalError_e UpdateIconfigOF( EvaConfigList_t *pConfigList );

    RideHalError_e RegisterEvaMem( const RideHal_SharedBuffer_t *pBuffer, EvaMem_t **pEvaMem );

    RideHalError_e ValidateConfig( const OpticalFlow_Config_t *pConfig );
    RideHalError_e ValidateImageBuffer( const RideHal_SharedBuffer_t *pImage );

    EvaColorFormat_e GetEvaColorFormat( RideHal_ImageFormat_e colorFormat );

private:
    OpticalFlow_Config_t m_config;
    uint32_t m_outputWidth;
    uint32_t m_outputHeight;

    EvaSession_t m_hOFSession = nullptr;
    EvaHandle_t m_hEvaOF = nullptr;

    EvaImageInfo_t m_imageInfo;
    EvaOFOutBuffReq_t m_outBufInfo;

    std::map<void *, EvaMem_t> m_evaMemMap;
};   // class OpticalFlow

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_OPTICALFLOW_HPP
