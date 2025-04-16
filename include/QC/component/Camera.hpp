// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CAMERA_HPP
#define QC_CAMERA_HPP

#include "QC/component/ComponentIF.hpp"
#include "qcarcam.h"

#include <mutex>
#include <queue>

using namespace QC::common;

namespace QC
{
namespace component
{

#define MAX_CAMERA_STREAM ( 8 )

/** @brief Camera frame structure */
typedef struct
{
    QCSharedBuffer_t sharedBuffer; /**< Shared buffer associated with the image */
    uint64_t timestamp;            /**< Hardware timestamp (in nanoseconds) */
    uint64_t timestampQGPTP; /**< Generic Precision Time Protocol (GPTP) timestamp in nanoseconds */
    uint32_t frameIndex;     /**< Index of the camera frame */
    uint32_t flags;          /**< Flag to indicate error state of the buffer */
    uint32_t streamId;       /**< Qcarcam buffer list id */
} CameraFrame_t;

/** @brief Camera input structure */
typedef struct
{
    QCarCamInput_t *pCameraInputs; /**< pointer to the list of qcarcam inputs info */
    QCarCamInputModes_t
            *pCamInputModes; /**< pointer to the list of qcarcam input modes for each input */
    uint32_t numInputs;      /**< num of qcarcam inputs */
} CameraInputs_t;

/** @brief callback for camera frame done */
typedef void ( *QCCamFrameCallback_t )( CameraFrame_t *pFrame, void *pPrivData );

/** @brief callback for camera event */
typedef void ( *QCCamEventCallback_t )( const uint32_t eventId, const void *pPayload,
                                        void *pPrivData );

/** @brief Camera stream config */
typedef struct
{
    uint32_t streamId;             /**< Camera stream id */
    uint32_t width;                /**< Frame width */
    uint32_t height;               /**< Frame height */
    uint32_t bufCnt;               /**< Buffer count set to camera */
    uint32_t submitRequestPattern; /**< Buffer submit request pattern */
    QCImageFormat_e format;        /**< Camera frame format */
} CameraStreamConfig_t;

/** @brief camera configuration */
typedef struct Camera_Config
{
    uint32_t numStream; /**< Number of camera stream */
    uint32_t inputId;   /**< Camera input id */
    uint32_t srcId;     /**< Input source identifier. See #QCarCamInputSrc_t */
    uint32_t clientId; /**< client id, used for multi client usecase, set to 0 by default for single
                         client usecase */
    uint32_t inputMode;   /**< The input mode id is the index into #QCarCamInputModes_t pModex*/
    uint32_t ispUserCase; /**< ISP user case defined by qcarcam */
    uint32_t camFrameDropPattern; /**< Frame drop patten defined by qcarcam. Set to 0 when frame
                                 drop is not used */
    uint32_t camFrameDropPeriod;  /**< Frame drop period defined by qcarcam. */
    uint32_t opMode;              /**< Operation mode defined by qcarcam */
    CameraStreamConfig_t streamConfig[MAX_CAMERA_STREAM]; /**< Per stream configuration */
    bool bAllocator;   /**< Flag to indicate if component is buffer allocator*/
    bool bRequestMode; /**< Flag to set request buffer mode */
    bool bPrimary;     /**< Flag to indicate if the session is primary or not when configured the
                          clientId */
    bool bRecovery;    /**< Flag to enable the self-recovery for the session */
} Camera_Config_t;

/** Camera Interface */
class Camera : public ComponentIF
{
public:
    /**
     * @brief Construct a new camera object
     */
    Camera();

    /**
     * @brief Destroy the camera object
     */
    ~Camera();

    /**
     * @brief get camera inputs info
     *
     * @param[out] pCamInputs Input info queried from Camera
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetInputsInfo( CameraInputs_t *pCamInputs );

    /**
     * @brief init the Camera object
     *
     * @param[in] pName   Name of the component
     * @param[in] pConfig Camera configurations
     * @param[in] level   Logging level
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const Camera_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief set a list of shared buffers to camera
     *
     * @param[in] pBuffers Pointer to the buffer list
     * @param[in] numBuffers Number of buffers in the list
     * @param[in] streamId Buffer list id
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e SetBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers,
                           uint32_t streamId );


    /**
     * @brief get a list of shared buffers use by the camera
     *
     * @param[in] pBuffers Pointer to the buffer list
     * @param[in] numBuffers Number of buffers in the list
     * @param[in] streamId Buffer list id
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetBuffers( QCSharedBuffer_t *pBuffers, uint32_t numBuffers, uint32_t streamId );

    /**
     * @brief register callbacks to camera
     *
     * @param[in] frameCallback Frame callback function
     * @param[in] eventCallback Event callback function
     * @param[in] pAppPriv App private data
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterCallback( QCCamFrameCallback_t frameCallback,
                                 QCCamEventCallback_t eventCallback, void *pAppPriv );

    /**
     * @brief Start the Camera object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start() final;

    /**
     * @brief Pause the Camera object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Pause();

    /**
     * @brief Resume the Camera object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Resume();

    /**
     * @brief release a camera frame
     *
     * @param[in] pFrame the camera frame to be released
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e ReleaseFrame( const CameraFrame_t *pFrame );

    /**
     * @brief request a frame from camera
     *
     * @param[in] pFrame the frame to request from camera
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RequestFrame( const CameraFrame_t *pFrame );

    /**
     * @brief Stop the Camera object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop() final;

    /**
     * @brief Deinit the Camera object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit() final;

private:
    QCarCamColorFmt_e GetQcarCamFormat( QCImageFormat_e colorFormat );

    QCStatus_e SubmitAllBuffers();
    QCStatus_e AllocateBuffers();
    QCStatus_e FreeBuffers();

    QCStatus_e ImportBuffers();
#ifdef CAMERA_UNIT_TEST
public:
#endif
    QCStatus_e UnImportBuffers();

private:
    CameraFrame_t *GetFrame( const QCarCamFrameInfo_t *pFrameInfo );

    static QCarCamRet_e QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                        const QCarCamEventPayload_t *pPayload, void *pPrivateData );
    QCarCamRet_e QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                 const QCarCamEventPayload_t *pPayload );

    QCStatus_e QueryInputs();

    QCStatus_e ValidateConfig( const Camera_Config_t *pConfig );

    bool m_bIsAllocator;
    bool m_bRequestMode;
    uint32_t m_nNumStream;
    uint32_t m_nInputId;
    uint32_t m_nRequestId;
    uint32_t m_nClientId;
    bool m_bIsPrimary;
    bool m_bRecovery;
    void *m_pAppPriv = nullptr;
    QCCamEventCallback_t m_EventCallback = nullptr;
    QCCamFrameCallback_t m_FrameCallback = nullptr;
    CameraStreamConfig_t m_streamConfig[MAX_CAMERA_STREAM] = { 0 };
    CameraFrame_t *m_pCameraFrames[MAX_CAMERA_STREAM] = { 0 };
    QCarCamBuffer_t *m_pQcarcamBuffer[MAX_CAMERA_STREAM] = { 0 };
    QCarCamBufferList_t m_qcarcamBuffers[MAX_CAMERA_STREAM] = { 0 };
    QCarCamHndl_t m_QcarCamHndl;
    bool m_bReservedOK = false;

    uint32_t m_maxBufCnt = 0; /* the maximum buffer count of all the configured streams */

    bool m_bRequestPatternMode = false;
    uint32_t m_submitRequestPattern[MAX_CAMERA_STREAM] = { 0 };
    uint32_t m_refStreamId = MAX_CAMERA_STREAM; /* Camera stream id */
    std::queue<uint32_t> m_freeBufIdxQueue[MAX_CAMERA_STREAM];
    std::mutex m_mutex;
};   // class Camera

}   // namespace component
}   // namespace QC

#endif   // QC_CAMERA_HPP
