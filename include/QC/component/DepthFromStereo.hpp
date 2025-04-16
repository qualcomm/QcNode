// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_DEPTH_FROM_STEREO_HPP
#define QC_DEPTH_FROM_STEREO_HPP

#include <cinttypes>
#include <inttypes.h>
#include <map>
#include <memory>
#include <unistd.h>

#include "QC/component/ComponentIF.hpp"

#include <eva_dfs.h>
#include <eva_mem.h>
#include <eva_session.h>
#include <eva_utils.h>

namespace QC
{
namespace component
{

/** @brief DepthFromStereo component configuration */
typedef struct DepthFromStereo_Config
{
    QCImageFormat_e format; /**< the input image format */
    uint32_t width;         /**< the input image width */
    uint32_t height;        /**< the input image height */
    uint32_t frameRate;     /**< the input stream frame rate */
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
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const DepthFromStereo_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for DepthFromStereo pipeline
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note Register buffers for input and output data. This step could be done by user or skipped.
     * If skipped, all the buffers will be registered at execute step.
     */
    QCStatus_e RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Start the DepthFromStereo pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief execute the DepthFromStereo pipeline
     * @param[in] pPriImage the primary image
     * @param[in] pAuxImage the auxiliary image
     * @param[out] pDispMap the disparity map buffer
     * @param[out] pConfMap the disparity confidence map buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( const QCSharedBuffer_t *pPriImage, const QCSharedBuffer_t *pAuxImage,
                        const QCSharedBuffer_t *pDispMap, const QCSharedBuffer_t *pConfMap );

    /**
     * @brief Stop the DepthFromStereo pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief Deregister buffers for DepthFromStereo pipeline
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note Deregister buffers for input and output data. This step could be done by user or
     * skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    QCStatus_e DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the DepthFromStereo pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

private:
    static void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent,
                                           void *pSessionUserData );
    void EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent );

    QCStatus_e SetInitialFrameConfig();
    QCStatus_e UpdateIconfig( EvaConfigList_t *pConfigList );

    QCStatus_e RegisterEvaMem( const QCSharedBuffer_t *pBuffer, EvaMem_t **pEvaMem );

    QCStatus_e ValidateConfig( const DepthFromStereo_Config_t *pConfig );
    QCStatus_e ValidateImageBuffer( const QCSharedBuffer_t *pImage );

    EvaColorFormat_e GetEvaColorFormat( QCImageFormat_e colorFormat );

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
}   // namespace QC

#endif   // QC_DEPTH_FROM_STEREO_HPP
