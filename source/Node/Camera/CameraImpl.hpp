// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_NODE_CAMERA_IMPL_HPP
#define QC_NODE_CAMERA_IMPL_HPP

#include <mutex>
#include <queue>

#include "QC/Infras/NodeTrace/NodeTrace.hpp"
#include "QC/Node/Camera.hpp"

#if defined( __QNXNTO__ )
#include "QC/Infras/Memory/PMEMUtils.hpp"
#else
#include "QC/Infras/Memory/DMABUFFUtils.hpp"
#endif

namespace QC
{
namespace Node
{

using namespace QC::Memory;

#define MAX_QUERY_TIMES 20

/**
 * @brief Camera Inputs structure
 *
 * @param pCameraInputs     Pointer to the list of qcarcam inputs info
 * @param pCamInputModes    Pointer to the list of qcarcam input modes for each input  
 * @param numInputs         Number of qcarcam inputs
 *
 */
typedef struct
{
    QCarCamInput_t *pCameraInputs;
    QCarCamInputModes_t *pCamInputModes;
    uint32_t numInputs;
} CameraInputs_t;

/**
 * @brief Camera Stream config
 *
 * @param streamId                Camera stream id
 * 
 * @param width                   Camera Frame width
 * 
 * @param height                  Camera Frame height
 *
 * @param bufCnt                  Buffer count set to camera
 *
 * @param submitRequestPattern    Buffer submit request pattern
 *
 * @param format                  Camera frame format
 *
 * @note This field is optional. If empty, buffers will be
 * registered automatically when the ProcessFrameDescriptor
 * API is called.
 */
typedef struct
{
    uint32_t streamId;
    uint32_t width;
    uint32_t height;
    uint32_t bufCnt;
    uint32_t submitRequestPattern;
    QCImageFormat_e format;
} CameraStreamConfig_t;

/**
 * @brief Configuration structure for Camera Node
 *
 * @param numStream             Number of camera stream
 * @note                        If numStream value is larger than 1, the
 *                              CameraNode will be set to multi-stream mode, and
 *                              streamConfigs will take effect.
 * 
 * @param inputId               Camera input id
 * 
 * @param srcId                 Input source identifier, see QCarCamInputSrc_t
 *
 * @param clientId              Client id for multi client usecase, set to 0 by
 *                              default for single client usecase
 *
 * @param inputMode             The input mode id is the index into
 *                              QCarCamInputModes_t pModex
 *
 * @param ispUseCase            ISP use case defined by qcarcam
 *
 * @param camFrameDropPattern   Frame drop patten defined by qcarcam, Set to 0
 *                              when frame drop is not used
 *
 * @param opMode                Operation mode defined by qcarcam
 *
 * @param bAllocator            Flag to indicate if node is buffer allocator
 *
 * @param bRequestMode          Flag to set request buffer mode
 *
 * @param bPrimary              Flag to indicate if the session is primary or
 *                              not, used for multi client usecase
 *
 * @param bRecovery             Flag to enable the self-recovery for the session
 *
 * @param streamConfigs         Configuration array for each stream. Only works
 *                              in multi-stream mode
 */
typedef struct Camera_Config : public QCNodeConfigBase_t
{
    uint32_t numStream;
    uint32_t inputId;
    uint32_t srcId;
    uint32_t clientId;
    uint32_t inputMode;
    uint32_t ispUseCase;
    uint32_t camFrameDropPattern;
    uint32_t camFrameDropPeriod;
    uint32_t opMode;
    bool bRequestMode;
    bool bPrimary;
    bool bRecovery;
    CameraStreamConfig_t streamConfigs[QCNODE_CAMERA_MAX_STREAM_NUM];
} CameraImplConfig_t;

// TODO
typedef struct CameraImplMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bEnablePerf;
} CameraImplMonitorConfig_t;

/**
 * @brief Camera Node Event Data Structure
 * @param eventId The QCarcamera callback event type.
 * @param pPayload The pointer of QCarcamera event payload.
 */
typedef struct
{
    uint32_t eventId;
    const void *pPayload;
} CameraImpEvent_t;


class CameraImpl
{

public:
    /**
     * @brief Construct a new CameraImpl object
     */
    CameraImpl( QCNodeID_t &nodeId, Logger &logger );

    /**
     * @brief Destroy the CameraImpl object
     */
    ~CameraImpl();

    /**
     * @brief Initialize CameraImpl object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Initialize( QCNodeEventCallBack_t callback,
                           std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );

    /**
     * @brief Start the CameraImpl object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief Process a camera frame
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the CameraImpl object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief Deinit the CameraImpl object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeInitialize();

    /**
     * @brief Get the CameraImpl configuration structure object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    CameraImplConfig_t &GetConifg() { return m_config; }

    /**
     * @brief Get the monitor configuration structure object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    CameraImplMonitorConfig_t &GetMonitorConifg() { return m_monitorConfig; }

    /**
     * @brief Retrieves the current state of the Camera Node.
     *
     * This function returns the current operational state of the Camera Node,
     * which may indicate whether the node is initialized, running, stopped, or in
     * an error state.
     *
     * @return The current state of the Camera Node.
     */
    QCObjectState_e GetState();

private:
    QCStatus_e ReleaseFrame( const CameraFrameDescriptor_t *pFrame );
    QCStatus_e RequestFrame( const CameraFrameDescriptor_t *pFrame );

    QCStatus_e SetBuffers( std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers );
    QCStatus_e SubmitAllBuffers();
    QCStatus_e ImportBuffers();
    QCStatus_e UnImportBuffers();

    QCStatus_e QueryInputs();
    QCStatus_e GetInputsInfo( CameraInputs_t *pCamInputs );
    CameraFrameDescriptor_t *GetFrame( const QCarCamFrameInfo_t *pFrameInfo );
    QCStatus_e ValidateConfig( const CameraImplConfig_t *pConfig );

    static void FrameCallback( CameraFrameDescriptor_t *pFrame, void *pPrivData );
    static void EventCallback( const uint32_t eventId, const void *pPayload, void *pPrivData );
    void FrameCallback( CameraFrameDescriptor_t *pFrame );
    void EventCallback( const uint32_t eventId, const void *pPayload );

    static QCarCamRet_e QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                        const QCarCamEventPayload_t *pPayload, void *pPrivateData );
    QCarCamRet_e QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                 const QCarCamEventPayload_t *pPayload );

    QCarCamColorFmt_e GetQcarCamFormat( QCImageFormat_e colorFormat );

private:
    QCNodeID_t &m_nodeId;
    Logger &m_logger;
    CameraImplConfig_t m_config;
    CameraImplMonitorConfig_t m_monitorConfig;
    QCObjectState_e m_state;

    bool m_bRequestMode;
    bool m_bIsPrimary;
    bool m_bRecovery;
    bool m_bReservedOK = false;
    bool m_bQCarCamInitialized = false;
    bool m_bRequestPatternMode = false;

    uint32_t m_numStream;
    uint32_t m_inputId;
    uint32_t m_requestId;
    uint32_t m_clientId;
    uint32_t m_maxBufCnt;
    uint32_t m_refStreamId = QCNODE_CAMERA_MAX_STREAM_NUM;
    uint32_t m_submitRequestPattern[QCNODE_CAMERA_MAX_STREAM_NUM];
    uint64_t m_frameId[QCNODE_CAMERA_MAX_STREAM_NUM] = { 0 };

    std::mutex m_mutex;
    std::queue<uint32_t> m_freeBufIdxQueue[QCNODE_CAMERA_MAX_STREAM_NUM];

    CameraStreamConfig_t m_streamConfigs[QCNODE_CAMERA_MAX_STREAM_NUM];
    CameraFrameDescriptor_t *m_pCameraFrames[QCNODE_CAMERA_MAX_STREAM_NUM];

    QCarCamBuffer_t *m_pQcarcamBuffers[QCNODE_CAMERA_MAX_STREAM_NUM];
    QCarCamBufferList_t m_qcarcamBuffers[QCNODE_CAMERA_MAX_STREAM_NUM];

    QCarCamHndl_t m_QcarCamHndl;
    QCNodeEventCallBack_t m_callback = nullptr;

    QC_DECLARE_NODETRACE();
};

}   // namespace Node
}   // namespace QC
#endif   // QC_NODE_CAMERA_IMPL_HPP
