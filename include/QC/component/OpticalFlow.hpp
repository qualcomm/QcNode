// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_OPTICALFLOW_HPP
#define QC_OPTICALFLOW_HPP

#include <cinttypes>
#include <inttypes.h>
#include <map>
#include <memory>
#include <unistd.h>

#include "QC/component/ComponentIF.hpp"

#include <eva_mem.h>
#include <eva_of.h>
#include <eva_session.h>
#include <eva_utils.h>

using namespace QC::common;

namespace QC
{
namespace component
{

/** @brief OpticalFlow component configuration */
typedef struct OpticalFlow_Config
{
    QCImageFormat_e format; /**< the input image format */
    uint32_t width;         /**< the input image width */
    uint32_t height;        /**< the input image height */
    uint32_t frameRate;     /**< the input stream frame rate */
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
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const OpticalFlow_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for OpticalFlow pipeline
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note Register buffers for input and output data. This step could be done by user or skipped.
     * If skipped, all the buffers will be registered at execute step.
     */
    QCStatus_e RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Start the OpticalFlow pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief execute the OpticalFlow pipeline
     * @param[in] pRefImage the reference image buffer
     * @param[in] pCurImage the current image buffer
     * @param[out] pOutMvBuf the motion vector map buffer
     * @param[out] pConfBuf the motion vector confidence map buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( const QCSharedBuffer_t *pRefImage, const QCSharedBuffer_t *pCurImage,
                        const QCSharedBuffer_t *pOutMvBuf, const QCSharedBuffer_t *pConfBuf );

    /**
     * @brief Stop the OpticalFlow pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief Deregister buffers for OpticalFlow pipeline
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note Deregister buffers for input and output data. This step could be done by user or
     * skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    QCStatus_e DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the OpticalFlow pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

private:
    static void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent,
                                           void *pSessionUserData );
    void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent );

    QCStatus_e SetInitialFrameConfig();
    QCStatus_e UpdateIconfigOF( EvaConfigList_t *pConfigList );

    QCStatus_e RegisterEvaMem( const QCSharedBuffer_t *pBuffer, EvaMem_t **pEvaMem );

    QCStatus_e ValidateConfig( const OpticalFlow_Config_t *pConfig );
    QCStatus_e ValidateImageBuffer( const QCSharedBuffer_t *pImage );

    EvaColorFormat_e GetEvaColorFormat( QCImageFormat_e colorFormat );

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
}   // namespace QC

#endif   // QC_OPTICALFLOW_HPP
