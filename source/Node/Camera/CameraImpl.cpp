// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include <cstring>
#include <stdint.h>
#include <thread>

#include "CameraImpl.hpp"

namespace QC
{
namespace Node
{

#if defined( __QNXNTO__ )
using MemUtils = QC::Memory::PMEMUtils;
#else
using MemUtils = QC::Memory::DMABUFFUtils;
#endif

static int g_nCamInitRefCount = 0;
static std::mutex g_camInitMutex;

static CameraInputs_t s_cameraInputsInfo = { nullptr, nullptr, 0 };

static void FreeCameraInputsInfo( void )
{
    if ( nullptr != s_cameraInputsInfo.pCameraInputs )
    {
        delete ( s_cameraInputsInfo.pCameraInputs );
        s_cameraInputsInfo.pCameraInputs = nullptr;
    }
    if ( nullptr != s_cameraInputsInfo.pCamInputModes )
    {
        for ( uint32_t i = 0; i < s_cameraInputsInfo.numInputs; i++ )
        {
            if ( nullptr != s_cameraInputsInfo.pCamInputModes[i].pModes )
            {
                delete ( s_cameraInputsInfo.pCamInputModes[i].pModes );
                s_cameraInputsInfo.pCamInputModes[i].pModes = nullptr;
            }
        }
        delete ( s_cameraInputsInfo.pCamInputModes );
        s_cameraInputsInfo.pCamInputModes = nullptr;
    }

    s_cameraInputsInfo.numInputs = 0;
}

CameraImpl::CameraImpl( QCNodeID_t &nodeId, Logger &logger )
    : m_nodeId( nodeId ),
      m_logger( logger ),
      m_state( QC_OBJECT_STATE_INITIAL ),
      m_QcarCamHndl( QCARCAM_HNDL_INVALID )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamInit_t qcarcamInit = { 0 };
    qcarcamInit.apiVersion = QCARCAM_VERSION;

    std::lock_guard<std::mutex> guard( g_camInitMutex );
    if ( 0 == g_nCamInitRefCount )
    {
        status = QCarCamInitialize( (const QCarCamInit_t *) &qcarcamInit );
        if ( QCARCAM_RET_OK != status )
        {
            ret = QC_STATUS_FAIL;
            m_state = QC_OBJECT_STATE_ERROR;
            QC_ERROR( "Failed to initialize QCarCamera", status );
        }
        else
        {
            QC_INFO( "Initialize QCarCamera successfully" );
            g_nCamInitRefCount++;

            ret = QueryInputs();
            if ( QC_STATUS_OK != ret )
            {
                ret = QC_STATUS_OK;
                QC_ERROR( "QueryInputs failed: %d", ret );
            }
        }
    }
    else
    {
        g_nCamInitRefCount++;
    }
}

CameraImpl::~CameraImpl()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    std::lock_guard<std::mutex> guard( g_camInitMutex );
    if ( 0 < g_nCamInitRefCount )
    {
        g_nCamInitRefCount--;

        if ( 0 == g_nCamInitRefCount )
        {
            status = QCarCamUninitialize();
            if ( QCARCAM_RET_OK != status )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to deinit QCarCamera: %d", status );
            }

            FreeCameraInputsInfo();
        }
        else
        {
            QC_INFO( "Skip QCarCamUninitialize" );
        }
    }
    else
    {
        QC_ERROR( "g_nCamInitRefCount not greater than 0, unexpected" );
    }

    for ( uint32_t i = 0; i < QCNODE_CAMERA_MAX_STREAM_NUM; i++ )
    {
        std::queue<uint32_t> empty;
        if ( false == m_freeBufIdxQueue[i].empty() )
        {
            std::swap( m_freeBufIdxQueue[i], empty );
        }
    }

    m_callback = nullptr;
}

QCStatus_e
CameraImpl::Initialize( QCNodeEventCallBack_t callback,
                        std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t param = 0;
    bool isQCarCamera = false;
    CameraInputs_t camInputsInfo;

    QC_INFO( "Camera node version: %u.%u.%u", QCNODE_CAMERA_VERSION_MAJOR,
             QCNODE_CAMERA_VERSION_MINOR, QCNODE_CAMERA_VERSION_PATCH );

    QC_TRACE_INIT( [&]() {
        std::ostringstream oss;
        oss << "{";
        oss << "\"name\": \"" << m_nodeId.name << "\", ";
        oss << "\"processor\": \"" << "camera" << "\"";
        oss << "}";
        return oss.str();
    }() );
    QC_TRACE_BEGIN( "Init", {} );

    if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Camera not in initial state!" );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = ValidateConfig( &m_config );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;

        m_inputId = m_config.inputId;
        m_clientId = m_config.clientId;
        m_numStream = m_config.numStream;
        m_maxBufCnt = 0;
        m_bRequestMode = m_config.bRequestMode;
        m_bIsPrimary = m_config.bPrimary;
        m_bRecovery = m_config.bRecovery;
        m_callback = callback;

        for ( uint32_t i = 0; i < m_numStream; i++ )
        {
            m_streamConfigs[i] = m_config.streamConfigs[i];
            if ( m_streamConfigs[i].bufCnt > m_maxBufCnt )
            {
                m_maxBufCnt = m_streamConfigs[i].bufCnt;
            }
        }

        /* setup submit request pattern for multiple streaming */
        m_bRequestPatternMode = false;
        if ( ( true == m_bRequestMode ) && ( m_numStream > 1 ) )
        {
            m_refStreamId = QCNODE_CAMERA_MAX_STREAM_NUM;
            for ( uint32_t i = 0; i < m_numStream; i++ )
            {
                if ( 0 == m_streamConfigs[i].submitRequestPattern )
                {
                    if ( QCNODE_CAMERA_MAX_STREAM_NUM == m_refStreamId )
                    {
                        /* the first stream with 0 pattern acting as reference stream */
                        m_refStreamId = m_streamConfigs[i].streamId;
                    }
                    m_submitRequestPattern[i] = 0;
                }
                else
                {
                    /* if there is anyone none-zero pattern, a submit request pattern mode for FPS
                     HW drop control */
                    m_submitRequestPattern[i] = m_streamConfigs[i].submitRequestPattern;
                    m_bRequestPatternMode = true;
                }
            }

            if ( true == m_bRequestPatternMode )
            {
                if ( QCNODE_CAMERA_MAX_STREAM_NUM == m_refStreamId )
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "Need at least 1 stream with 0 submitRequestPattern" );
                }
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = GetInputsInfo( &camInputsInfo );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "GetInputsInfo failed: %d", ret );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QCarCamInputModes_t *pCamInputModes = nullptr;
        for ( uint32_t i = 0; i < camInputsInfo.numInputs; i++ )
        {
            if ( camInputsInfo.pCameraInputs[i].inputId == m_inputId )
            {
                pCamInputModes = &camInputsInfo.pCamInputModes[i];
                break;
            }
        }

        if ( nullptr == pCamInputModes )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "camera input id %u not found", m_inputId );
        }
        else if ( 0 == pCamInputModes->numModes )
        {
            ret = QC_STATUS_OUT_OF_BOUND;
            QC_ERROR( "no mode 0 for camera input id %u", m_inputId );
        }
        else
        {
            // do nothing
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QCarCamInputStream_t inputParams = { 0 };
        inputParams.inputId = m_inputId;
        inputParams.srcId = m_config.srcId;
        inputParams.inputMode = m_config.inputMode;

        QCarCamOpen_t openParams = { (QCarCamOpmode_e) 0, 0 };
        openParams.opMode = (QCarCamOpmode_e) m_config.opMode;
        openParams.numInputs = 1;
        openParams.clientId = m_clientId;
        openParams.inputs[0] = inputParams;

        if ( m_bRecovery )
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_RECOVERY;
        }

        if ( m_bRequestMode )
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_REQUEST_MODE;
        }

        if ( 0 != m_clientId )
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_MULTI_CLIENT_SESSION;
        }

        status = QCarCamOpen( &openParams, &m_QcarCamHndl );
        if ( ( QCARCAM_RET_OK != status ) || ( QCARCAM_HNDL_INVALID == m_QcarCamHndl ) )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "QCarCamOpen failed: %d", status );
        }
        else
        {
            QC_INFO( "QCarCamOpen Success handle: %lu", m_QcarCamHndl );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        status = QCarCamRegisterEventCallback( m_QcarCamHndl, &QcarcamEventCb, this );
        if ( QCARCAM_RET_OK != status )
        {
            ret = QC_STATUS_FAIL;
            m_state = QC_OBJECT_STATE_ERROR;
            QC_ERROR( "QCARCAM_PARAM_EVENT_CB failed ret %d", status );
        }
        else
        {
            QC_INFO( "QCARCAM_PARAM_EVENT_CB Success" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        param = QCARCAM_EVENT_FRAME_READY | QCARCAM_EVENT_INPUT_SIGNAL | QCARCAM_EVENT_ERROR |
                QCARCAM_EVENT_MC_NOTIFY;
        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_EVENT_MASK, &param,
                                  sizeof( param ) );
        if ( QCARCAM_RET_OK != status )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "QCARCAM_PARAM_EVENT_MASK failed ret %d", status );
        }
        else
        {
            QC_INFO( "QCARCAM_PARAM_EVENT_MASK Success" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        // setup isp settings
        QCarCamIspUsecaseConfig_t ispConfig = { 0 };

        ispConfig.id = 0;
        ispConfig.cameraId = 0;
        ispConfig.usecaseId = (QCarCamIspUsecase_e) m_config.ispUseCase;

        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE,
                                  &ispConfig, sizeof( ispConfig ) );
        if ( status != QCARCAM_RET_OK )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE failed ret %d", status );
        }
        else
        {
            QC_INFO( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE success" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( 0 == m_config.camFrameDropPattern )
        {
            QC_INFO( "Ignore frame drop config" );
        }
        else
        {
            // setup frame rate params
            QCarCamFrameDropConfig_t frameDropConfig = { 0 };
            frameDropConfig.frameDropPeriod = m_config.camFrameDropPeriod;
            frameDropConfig.frameDropPattern = m_config.camFrameDropPattern;
            status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_FRAME_DROP_CONTROL,
                                      &frameDropConfig, sizeof( frameDropConfig ) );
            if ( QCARCAM_RET_OK != status )
            {
                QC_ERROR( "QCARCAM_PARAM_FRAME_RATE failed ret %d", status );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                QC_INFO( "QCARCAM_PARAM_FRAME_RATE Success" );
            }
        }
        m_state = QC_OBJECT_STATE_READY;
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( 0 != m_clientId ) && ( false == m_bIsPrimary ) )
        {
            /* for multi-client, non primary session, query the primary's buffers */
            ret = ImportBuffers();
            if ( QC_STATUS_OK == ret )
            {
                QC_INFO( "Import buffers successfully" );
            }
            else
            {
                QC_ERROR( "Failed to import buffers" );
            }
        }
        else
        {
            /* set buffers */
            ret = SetBuffers( buffers );
            if ( QC_STATUS_OK == ret )
            {
                QC_INFO( "Set buffers successfully" );
            }
            else
            {
                QC_ERROR( "Failed to set buffers" );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_requestId = 0;

        status = QCarCamReserve( m_QcarCamHndl );
        if ( QCARCAM_RET_OK == status )
        {
            QC_INFO( "QCarCamReserve successfully" );
        }
        else
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "QCarCamReserve failed with ret %d, exit", status );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QC_INFO( "Initialize camera successfully" );
    }
    else
    {
        m_state = QC_OBJECT_STATE_INITIAL;

        /* clean up QCarCamHndl */
        if ( QCARCAM_HNDL_INVALID != m_QcarCamHndl )
        {
            QC_ERROR( "Error happens in Init" );
            (void) QCarCamClose( m_QcarCamHndl );
            m_QcarCamHndl = QCARCAM_HNDL_INVALID;
        }
    }

    QC_TRACE_END( "Init", {} );

    return ret;
}

QCStatus_e CameraImpl::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    bool bStartOK = false;

    QC_TRACE_BEGIN( "Start", {} );

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Camera not in ready state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        m_state = QC_OBJECT_STATE_STARTING;
    }

    if ( QC_STATUS_OK == ret )
    {
        status = QCarCamStart( m_QcarCamHndl );
        if ( QCARCAM_RET_OK != status )
        {
            QC_ERROR( "QCarCamStart failed with ret %d , exit", status );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            bStartOK = true;
            m_state = QC_OBJECT_STATE_RUNNING;
            QC_INFO( "QCarCamStart success" );
        }
    }

    if ( ( QC_STATUS_OK == ret ) && ( true == m_bRequestMode ) )
    {
        ret = SubmitAllBuffers();
    }

    if ( QC_STATUS_OK != ret )
    {
        /* error clean up */
        m_state = QC_OBJECT_STATE_READY;
        if ( bStartOK )
        {
            (void) QCarCamStop( m_QcarCamHndl );
        }
    }

    QC_TRACE_END( "Start", {} );

    return ret;
}

QCStatus_e CameraImpl::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;

    QCBufferDescriptorBase_t &bufDesc = frameDesc.GetBuffer( 0 );
    CameraFrameDescriptor_t *pCamFrameDesc = dynamic_cast<CameraFrameDescriptor_t *>( &bufDesc );

    uint64_t streamId = 0;
    uint64_t frameId = 0;
    if ( nullptr == pCamFrameDesc )
    {
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        streamId = pCamFrameDesc->streamId;
        frameId = pCamFrameDesc->id;
    }

    QC_TRACE_BEGIN( "Execute", { QCNodeTraceArg( "streamId", streamId ),
                                 QCNodeTraceArg( "frameId", frameId ) } );

    if ( QC_STATUS_OK == ret )
    {
        if ( m_bRequestMode )
        {
            ret = RequestFrame( pCamFrameDesc );
        }
        else
        {
            ret = ReleaseFrame( pCamFrameDesc );
        }
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to process camera frame" );
    }

    QC_TRACE_END( "Execute", { QCNodeTraceArg( "streamId", streamId ),
                               QCNodeTraceArg( "frameId", frameId ) } );

    return ret;
}

QCStatus_e CameraImpl::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    QC_TRACE_BEGIN( "Stop", {} );

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_STOPING;

        status = QCarCamStop( m_QcarCamHndl );
        if ( QCARCAM_RET_OK == status )
        {
            m_state = QC_OBJECT_STATE_READY;
            QC_INFO( "Qcarcam Stop call success" );
        }
        else
        {
            QC_ERROR( "Qcarcam could not be stopped: status=%d", status );
            m_state = QC_OBJECT_STATE_RUNNING;
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        QC_ERROR( "Camera not in running state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    QC_TRACE_END( "Stop", {} );

    return ret;
}

QCStatus_e CameraImpl::DeInitialize()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    QC_TRACE_BEGIN( "DeInit", {} );

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Camera not in ready state: %d", m_state );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( 0 == m_QcarCamHndl )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "Qcarcam null handle" );
        }

        status = QCarCamRelease( m_QcarCamHndl );
        if ( QCARCAM_RET_OK != status )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "QCarCamRelease failed %d", status );
        }

        status = QCarCamClose( m_QcarCamHndl );
        if ( QCARCAM_RET_OK != status )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "QCarCamClose failed %d", status );
        }

        if ( ( 0 != m_clientId ) && ( false == m_bIsPrimary ) )
        {
            ret = UnImportBuffers();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Error in unimport buffers" );
            }
        }

        for ( uint32_t i = 0; i < m_numStream; i++ )
        {
            uint32_t streamId = m_streamConfigs[i].streamId;
            if ( nullptr != m_pCameraFrames[streamId] )
            {
                delete[] m_pCameraFrames[streamId];
                m_pCameraFrames[streamId] = nullptr;
            }

            if ( nullptr != m_pQcarcamBuffers[streamId] )
            {
                delete[] m_pQcarcamBuffers[streamId];
                m_pQcarcamBuffers[streamId] = nullptr;
            }
        }
    }

    QC_TRACE_END( "DeInit", {} );

    return ret;
}


QCObjectState_e CameraImpl::GetState()
{
    return m_state;
}

QCStatus_e CameraImpl::ReleaseFrame( const CameraFrameDescriptor_t *pFrame )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "QCarCamera is not in running state: %d", m_state );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( true == m_bRequestMode )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "ReleaseFrame is not allowed for request mode" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pFrame )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "pFrame is nullptr" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        status = QCarCamReleaseFrame( m_QcarCamHndl, pFrame->streamId, pFrame->frameIdx );
        if ( QCARCAM_RET_OK == status )
        {
            QC_INFO( "QCarCamReleaseFrame success for index: %u", pFrame->frameIdx );
        }
        else
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "QCarCamReleaseFrame fail for id: %d index: %u, status: %d", pFrame->streamId,
                      pFrame->frameIdx, status );
        }
    }

    return ret;
}

QCStatus_e CameraImpl::RequestFrame( const CameraFrameDescriptor_t *pFrame )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamRequest_t request = { 0 };

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "QCarCamera is not in running state: %d", m_state );
    }
    else if ( false == m_bRequestMode )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "RequestFrame is not allowed for non request mode" );
    }
    else if ( ( 0 != m_clientId ) && ( false == m_bIsPrimary ) )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "RequestFrame is not allowed for multi-client non primary session" );
    }
    else if ( nullptr == pFrame )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "pFrame is nullptr" );
    }
    else
    {
        uint32_t bufferListId = pFrame->streamId;
        uint32_t bufferIdx = pFrame->frameIdx;
        if ( true == m_bRequestPatternMode )
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            m_freeBufIdxQueue[bufferListId].push( bufferIdx );
            if ( m_refStreamId == bufferListId )
            {
                /* this is the reference frame, do submit requst */
                for ( uint32_t i = 0; i < m_numStream; i++ )
                {
                    uint32_t streamId = m_streamConfigs[i].streamId;
                    if ( m_submitRequestPattern[i] > 0 )
                    {
                        m_submitRequestPattern[i]--;
                    }
                    if ( 0 == m_submitRequestPattern[i] )
                    {
                        if ( false == m_freeBufIdxQueue[streamId].empty() )
                        {
                            QCarCamStreamRequest_t *pStreamRequest =
                                    &request.streamRequests[request.numStreamRequests];
                            pStreamRequest->bufferlistId = streamId;
                            pStreamRequest->bufferIdx = m_freeBufIdxQueue[streamId].front();
                            m_freeBufIdxQueue[streamId].pop();
                            request.numStreamRequests++;
                            m_submitRequestPattern[i] = m_streamConfigs[i].submitRequestPattern;
                            QC_DEBUG( "RequestFrame m_QcarCamHndl: %lu, bufferlistId: %u, "
                                      "bufferIdx: %u",
                                      m_QcarCamHndl, pStreamRequest->bufferlistId,
                                      pStreamRequest->bufferIdx );
                        }
                    }
                }
            }
        }
        else
        {
            QCarCamStreamRequest_t *pStreamRequest = &request.streamRequests[0];
            pStreamRequest->bufferlistId = bufferListId;
            pStreamRequest->bufferIdx = bufferIdx;
            request.numStreamRequests = 1;
        }

        if ( request.numStreamRequests > 0 )
        {
            request.requestId = __atomic_fetch_add( &m_requestId, 1, __ATOMIC_RELAXED );
            QC_DEBUG( "RequestFrame begin m_QcarCamHndl: %lu, bufferlistId: %u, "
                      "bufferIdx: %u, request id: %u",
                      m_QcarCamHndl, bufferListId, bufferIdx, request.requestId );
            status = QCarCamSubmitRequest( m_QcarCamHndl, &request );
            if ( QCARCAM_RET_OK != status )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "RequestFrame fail m_QcarCamHndl: %lu, bufferlistId: %u, bufferIdx: "
                          "%u request id: %u, status=%d",
                          m_QcarCamHndl, bufferListId, bufferIdx, request.requestId, status );
            }
            else
            {
                QC_DEBUG( "RequestFrame success m_QcarCamHndl: %lu, bufferlistId: %u, "
                          "bufferIdx: %u, request id: %u",
                          m_QcarCamHndl, bufferListId, bufferIdx, request.requestId );
            }
        }
    }

    return ret;
}

QCStatus_e
CameraImpl::SetBuffers( std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> &buffers )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    uint32_t offset = 0;
    uint32_t streamId = 0;
    uint32_t numBuffers = 0;
    uint32_t bufferIdx = 0;
    QCImageFormat_e format = QC_IMAGE_FORMAT_MAX;
    CameraFrameDescriptor_t *pCamFrame = nullptr;
    QCarCamBuffer_t *pQcarcamBuf = nullptr;
    std::vector<uint32_t> bufferIds;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Camera not in ready state: %d", m_state );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( 0 != m_clientId ) && ( false == m_bIsPrimary ) )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "set buffer is not allowed for multi-client non primary session" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < m_numStream; i++ )
        {
            streamId = m_streamConfigs[i].streamId;
            format = m_streamConfigs[i].format;
            numBuffers = m_streamConfigs[i].bufCnt;

            m_pCameraFrames[streamId] = new CameraFrameDescriptor_t[numBuffers];
            m_pQcarcamBuffers[streamId] = new QCarCamBuffer_t[numBuffers];
            m_qcarcamBuffers[streamId].id = streamId;
            m_qcarcamBuffers[streamId].nBuffers = numBuffers;
            m_qcarcamBuffers[streamId].pBuffers = m_pQcarcamBuffers[streamId];
            m_qcarcamBuffers[streamId].colorFmt = GetQcarCamFormat( format );
            m_qcarcamBuffers[streamId].flags = QCARCAM_BUFFER_FLAG_OS_HNDL;

            for ( uint32_t j = 0; j < numBuffers; j++ )
            {
                m_pCameraFrames[streamId][j] = buffers[bufferIdx];
                pCamFrame = &m_pCameraFrames[streamId][j];
                pQcarcamBuf = &m_pQcarcamBuffers[streamId][j];

                pCamFrame->streamId = streamId;
                pCamFrame->frameIdx = bufferIdx;
                bufferIdx++;

                if ( format != pCamFrame->format )
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "Buffer property error for stream %u buffer %u: image format does "
                              "not match, config: %u, buffer: %u",
                              streamId, j, format, pCamFrame->format );
                    break;
                }
                else if ( m_streamConfigs[i].width != pCamFrame->width )
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "Buffer property error for stream %u buffer %u: image width does not "
                              "match, config: %u, buffer: %u",
                              streamId, j, m_streamConfigs[i].width, pCamFrame->width );
                    break;
                }
                else if ( m_streamConfigs[i].height != pCamFrame->height )
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "Buffer property error for stream %u buffer %u: image height does "
                              "not match, config: %u, buffer: %u",
                              streamId, j, m_streamConfigs[i].height, pCamFrame->height );
                    break;
                }

                if ( QC_STATUS_OK == ret )
                {
                    offset = 0;
                    pQcarcamBuf->numPlanes = pCamFrame->numPlanes;

                    if ( ( QC_IMAGE_FORMAT_NV12_UBWC == format ) ||
                         ( QC_IMAGE_FORMAT_TP10_UBWC == format ) )
                    {
                        pQcarcamBuf->numPlanes = 2;
                    }
                    if ( pQcarcamBuf->numPlanes > QCARCAM_MAX_NUM_PLANES )
                    {
                        pQcarcamBuf->numPlanes = QCARCAM_MAX_NUM_PLANES;
                    }

                    for ( uint32_t k = 0; k < pQcarcamBuf->numPlanes; k++ )
                    {
                        pQcarcamBuf->planes[k].memHndl = pCamFrame->dmaHandle;
                        pQcarcamBuf->planes[k].width = pCamFrame->width;
                        pQcarcamBuf->planes[k].height = pCamFrame->height;
                        pQcarcamBuf->planes[k].stride = pCamFrame->stride[k];
                        pQcarcamBuf->planes[k].size = pCamFrame->planeBufSize[k];
                        pQcarcamBuf->planes[k].offset = offset;
                        offset += pCamFrame->planeBufSize[k];
                    }

                    if ( ( QC_IMAGE_FORMAT_NV12 == format ) || ( QC_IMAGE_FORMAT_P010 == format ) ||
                         ( QC_IMAGE_FORMAT_NV12_UBWC == format ) ||
                         ( QC_IMAGE_FORMAT_TP10_UBWC == format ) )
                    {
                        pQcarcamBuf->planes[1].height /= 2;
                    }
                    if ( ( QC_IMAGE_FORMAT_NV12_UBWC == format ) ||
                         ( QC_IMAGE_FORMAT_TP10_UBWC == format ) )
                    {
                        pQcarcamBuf->planes[0].size =
                                pCamFrame->planeBufSize[0] + pCamFrame->planeBufSize[1];
                        pQcarcamBuf->planes[0].offset = 0;
                        pQcarcamBuf->planes[1].size =
                                pCamFrame->planeBufSize[2] + pCamFrame->planeBufSize[3];
                        pQcarcamBuf->planes[1].offset = pQcarcamBuf->planes[0].size;
                    }

                    QC_INFO( "register buffer index %u: memHndl: %llu va: %p width: %u, "
                             "height: %u, stride: %u size: %u",
                             i, pQcarcamBuf->planes[0].memHndl, pCamFrame->pBuf,
                             pQcarcamBuf->planes[0].width, pQcarcamBuf->planes[0].height,
                             pQcarcamBuf->planes[0].stride, pQcarcamBuf->planes[0].size );
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                if ( QC_STATUS_OK == ret )
                {
                    // setup buffers
                    status = QCarCamSetBuffers(
                            m_QcarCamHndl,
                            (const QCarCamBufferList_t *) &m_qcarcamBuffers[streamId] );
                    if ( QCARCAM_RET_OK != status )
                    {
                        ret = QC_STATUS_FAIL;
                        if ( nullptr != m_pCameraFrames[streamId] )
                        {
                            delete[] m_pCameraFrames[streamId];
                            m_pCameraFrames[streamId] = nullptr;
                        }
                        if ( nullptr != m_pQcarcamBuffers[streamId] )
                        {
                            delete[] m_pQcarcamBuffers[streamId];
                            m_pQcarcamBuffers[streamId] = nullptr;
                        }
                        QC_ERROR( "QCarCamSetBuffers error ret %d  handle %lu", status,
                                  m_QcarCamHndl );
                    }
                    else
                    {
                        QC_INFO( "Set QCarCam buffers successfully" );
                    }
                }
            }
        }
    }

    return ret;
}

QCStatus_e CameraImpl::SubmitAllBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    if ( ( ( 0 == m_clientId ) || ( true == m_bIsPrimary ) ) )
    {
        for ( uint32_t bufIdx = 0; bufIdx < m_maxBufCnt; bufIdx++ )
        {
            QCarCamRequest_t request = { 0 };
            request.numStreamRequests = 0;
            for ( uint32_t i = 0; i < m_numStream; i++ )
            {
                if ( bufIdx < m_streamConfigs[i].bufCnt )
                {
                    QCarCamStreamRequest_t *pStreamRequest =
                            &request.streamRequests[request.numStreamRequests];
                    pStreamRequest->bufferlistId = m_streamConfigs[i].streamId;
                    pStreamRequest->bufferIdx = bufIdx;
                    request.numStreamRequests++;
                    QC_DEBUG( "RequestFrame m_QcarCamHndl: %lu, bufferlistId: %u, "
                              "bufferIdx: %u",
                              m_QcarCamHndl, pStreamRequest->bufferlistId,
                              pStreamRequest->bufferIdx );
                }
            }
            request.requestId = __atomic_fetch_add( &m_requestId, 1, __ATOMIC_RELAXED );
            status = QCarCamSubmitRequest( m_QcarCamHndl, &request );
            if ( QCARCAM_RET_OK != status )
            {
                QC_ERROR( "RequestFrame fail m_QcarCamHndl: %lu, bufferIdx: %u "
                          "request id: %u, status=%d",
                          m_QcarCamHndl, bufIdx, request.requestId, status );
                ret = QC_STATUS_FAIL;
                break;
            }
        }
    }

    return ret;
}

QCStatus_e CameraImpl::ImportBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    MemUtils memUtils;
    uint32_t streamId = 0;

    CameraFrameDescriptor_t *pCamFrame = nullptr;
    QCarCamBuffer_t *pQcarcamBuf = nullptr;

    for ( uint32_t i = 0; i < m_numStream; i++ )
    {
        uint32_t bufCnt = m_streamConfigs[i].bufCnt;
        if ( bufCnt > 0 )
        {
            streamId = m_streamConfigs[i].streamId;
            m_pCameraFrames[streamId] = new CameraFrameDescriptor_t[bufCnt];
            m_pQcarcamBuffers[streamId] = new QCarCamBuffer_t[bufCnt];
            m_qcarcamBuffers[streamId].id = m_streamConfigs[i].streamId;
            m_qcarcamBuffers[streamId].nBuffers = bufCnt;
            m_qcarcamBuffers[streamId].pBuffers = m_pQcarcamBuffers[streamId];
            m_qcarcamBuffers[streamId].colorFmt = GetQcarCamFormat( m_streamConfigs[i].format );
            m_qcarcamBuffers[streamId].flags = QCARCAM_BUFFER_FLAG_OS_HNDL;
        }
        else
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "Invalid buffer count: %d", bufCnt );
            break;
        }

        if ( QC_STATUS_OK == ret )
        {
            // get buffers
            status = QCarCamGetBuffers( m_QcarCamHndl, &m_qcarcamBuffers[streamId] );
            if ( QCARCAM_RET_OK == status )
            {
                QC_INFO( "QCarCamGetBuffers successful" );
                for ( uint32_t j = 0; j < bufCnt; j++ )
                {
                    CameraFrameDescriptor_t camFrameDesc;
                    pQcarcamBuf = &m_pQcarcamBuffers[streamId][j];
                    pCamFrame = &m_pCameraFrames[streamId][j];

                    camFrameDesc.pid = 0;
                    camFrameDesc.size = 0;
                    camFrameDesc.offset = 0;
                    camFrameDesc.type = QC_BUFFER_TYPE_IMAGE;
                    camFrameDesc.cache = QC_CACHEABLE;
                    camFrameDesc.dmaHandle = pQcarcamBuf->planes[0].memHndl;
                    camFrameDesc.allocatorType = QC_MEMORY_ALLOCATOR_DMA_CAMERA;

                    camFrameDesc.batchSize = 1;
                    camFrameDesc.format = m_streamConfigs[i].format;
                    camFrameDesc.numPlanes = pQcarcamBuf->numPlanes;
                    camFrameDesc.width = pQcarcamBuf->planes[0].width;
                    camFrameDesc.height = pQcarcamBuf->planes[0].height;

                    for ( uint32_t k = 0; k < pQcarcamBuf->numPlanes; k++ )
                    {
                        camFrameDesc.stride[k] = pQcarcamBuf->planes[k].stride;
                        camFrameDesc.planeBufSize[k] = pQcarcamBuf->planes[k].size;
                        camFrameDesc.actualHeight[k] =
                                pQcarcamBuf->planes[k].size / pQcarcamBuf->planes[k].stride;
                        camFrameDesc.size += pQcarcamBuf->planes[k].size;
                    }

                    ret = memUtils.MemoryMap( camFrameDesc, *pCamFrame );
                    if ( QC_STATUS_OK == ret )
                    {
                        void *pBuf = pCamFrame->pBuf;
                        *pCamFrame = camFrameDesc;
                        pCamFrame->pBuf = pBuf;
                        pCamFrame->frameIdx = j;
                        pCamFrame->streamId = m_streamConfigs[i].streamId;
                        QC_INFO( "buffer index %u: memHndl: %llu va: %p width: %u, "
                                 "height: %u, stride: %u size: %u",
                                 i, pQcarcamBuf->planes[0].memHndl, pCamFrame->pBuf,
                                 pQcarcamBuf->planes[0].width, pQcarcamBuf->planes[0].height,
                                 pQcarcamBuf->planes[0].stride, pQcarcamBuf->planes[0].size );
                    }
                    else
                    {
                        QC_ERROR( "Failed to import frame memory for stream %u buffer %u", i, j );
                        break;
                    }
                }
            }
            else
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "QCarCamGetBuffers error ret %d  handle %lu", status, m_QcarCamHndl );
                if ( nullptr != m_pCameraFrames[streamId] )
                {
                    delete[] m_pCameraFrames[streamId];
                    m_pCameraFrames[streamId] = nullptr;
                }
                if ( nullptr != m_pQcarcamBuffers[streamId] )
                {
                    delete[] m_pQcarcamBuffers[streamId];
                    m_pQcarcamBuffers[streamId] = nullptr;
                }
            }
        }
    }

    return ret;
}

QCStatus_e CameraImpl::UnImportBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;

    MemUtils memUtils;
    uint32_t streamId = 0;
    CameraFrameDescriptor_t *pCamFrame = nullptr;

    for ( uint32_t i = 0; i < m_numStream; i++ )
    {
        streamId = m_streamConfigs[i].streamId;
        if ( nullptr != m_pCameraFrames[streamId] )
        {
            for ( uint32_t k = 0; k < m_streamConfigs[i].bufCnt; k++ )
            {
                pCamFrame = &m_pCameraFrames[streamId][k];
                ret = memUtils.MemoryUnMap( *pCamFrame );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to unimport frame memory for stream %u buffer %u", i, k );
                }
            }
        }
        else
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "m_pCameraFrames is nullptr" );
        }
    }

    return ret;
}

QCStatus_e CameraImpl::QueryInputs()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    uint32_t inputCount = 0;
    uint32_t queryCount = 0;
    s_cameraInputsInfo.numInputs = 0;

    do
    {
        status = QCarCamQueryInputs( NULL, 0, &inputCount );
        if ( ( QCARCAM_RET_OK != status ) || ( 0 == inputCount ) )
        {
            queryCount++;
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
        }
        else
        {
            break;
        }
    } while ( ( 0 == inputCount ) && ( queryCount < MAX_QUERY_TIMES ) );

    if ( QC_STATUS_OK == ret )
    {
        if ( 0 == inputCount )
        {
            ret = QC_STATUS_FAIL;
            QC_LOG_ERROR( "Didn't detect any camera connection" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        s_cameraInputsInfo.pCameraInputs = new QCarCamInput_t[inputCount];
        s_cameraInputsInfo.pCamInputModes = new QCarCamInputModes_t[inputCount];
        if ( ( nullptr == s_cameraInputsInfo.pCameraInputs ) ||
             ( nullptr == s_cameraInputsInfo.pCamInputModes ) )
        {
            QC_LOG_ERROR( "Failed to allocate memory" );
            ret = QC_STATUS_NOMEM;
        }
        else
        {
            (void) memset( s_cameraInputsInfo.pCamInputModes, 0,
                           sizeof( QCarCamInputModes_t ) * inputCount );

            status = QCarCamQueryInputs( s_cameraInputsInfo.pCameraInputs, inputCount,
                                         &s_cameraInputsInfo.numInputs );

            if ( ( QCARCAM_RET_OK != status ) || ( s_cameraInputsInfo.numInputs != inputCount ) )
            {
                QC_LOG_ERROR( "Query failed QCarCamQueryInputs %u %u: ret = %d",
                              s_cameraInputsInfo.numInputs, inputCount, status );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                for ( uint32_t i = 0; i < inputCount; i++ )
                {
                    QC_LOG_INFO( "Available camera input id: %u, numModes = %u",
                                 s_cameraInputsInfo.pCameraInputs[i].inputId,
                                 s_cameraInputsInfo.pCameraInputs[i].numModes );

                    s_cameraInputsInfo.pCamInputModes[i].pModes =
                            new QCarCamMode_t[s_cameraInputsInfo.pCameraInputs[i].numModes];
                    s_cameraInputsInfo.pCamInputModes[i].numModes =
                            s_cameraInputsInfo.pCameraInputs[i].numModes;
                    if ( nullptr != s_cameraInputsInfo.pCamInputModes[i].pModes )
                    {
                        status =
                                QCarCamQueryInputModes( s_cameraInputsInfo.pCameraInputs[i].inputId,
                                                        &s_cameraInputsInfo.pCamInputModes[i] );
                        if ( QCARCAM_RET_OK != status )
                        {
                            ret = QC_STATUS_FAIL;
                            QC_LOG_ERROR( "Query Input Modes failed for input %u: ret = %d",
                                          s_cameraInputsInfo.pCameraInputs[i].inputId, status );
                            break;
                        }
                        else
                        {
                            QC_LOG_INFO(
                                    "Found camera with input %du: mode 0 src 0: "
                                    "resolution %ux%u",
                                    s_cameraInputsInfo.pCameraInputs[i].inputId,
                                    s_cameraInputsInfo.pCamInputModes[i].pModes[0].sources[0].width,
                                    s_cameraInputsInfo.pCamInputModes[i]
                                            .pModes[0]
                                            .sources[0]
                                            .height );
                        }
                    }
                    else
                    {
                        ret = QC_STATUS_NOMEM;
                        QC_LOG_ERROR( "Failed to allocate memory for input %u modes",
                                      s_cameraInputsInfo.pCameraInputs[i].inputId );
                    }
                }
            }
        }

        if ( QC_STATUS_OK != ret )
        {
            FreeCameraInputsInfo();
        }
    }

    return ret;
}

QCStatus_e CameraImpl::GetInputsInfo( CameraInputs_t *pCamInputs )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pCamInputs )
    {
        QC_LOG_ERROR( "pCamInputs is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( nullptr == s_cameraInputsInfo.pCameraInputs )
    {
        QC_LOG_ERROR( "Error in camera inputs" );
        pCamInputs->numInputs = 0;
        ret = QC_STATUS_FAIL;
    }
    else
    {
        *pCamInputs = s_cameraInputsInfo;
    }

    return ret;
}

CameraFrameDescriptor_t *CameraImpl::GetFrame( const QCarCamFrameInfo_t *pFrameInfo )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t frameIdx = 0;
    uint64_t timeout = 0;
    QCarCamFrameInfo_t frameInfo = { 0 };
    frameInfo.id = pFrameInfo->id;
    CameraFrameDescriptor_t *pCamFrame = nullptr;

    if ( QC_STATUS_OK == ret )
    {
        if ( !m_bRequestMode )
        {
            status = QCarCamGetFrame( m_QcarCamHndl, &frameInfo, timeout, 0 );
            if ( QCARCAM_RET_OK == status )
            {
                frameIdx = frameInfo.bufferIndex;
                pCamFrame = m_pCameraFrames[frameInfo.id] + frameIdx;
                pCamFrame->timestamp = frameInfo.sofTimestamp.timestamp;
                pCamFrame->timestampQGPTP = frameInfo.sofTimestamp.timestampGPTP;
                pCamFrame->flags = frameInfo.flags;
                QC_DEBUG( "Get Camera Frame Info: "
                          "bufferListId: %u "
                          "buffer index: %u "
                          "buffer pointer addr: %p "
                          "buffer data addr: %p "
                          "buffer size: %u "
                          "timestamp: %llu "
                          "timestampGPTP: %llu ",
                          "flags: %x ", frameInfo.id, frameIdx, pCamFrame, pCamFrame->pBuf,
                          pCamFrame->size, pCamFrame->timestamp, pCamFrame->timestampQGPTP,
                          pCamFrame->flags );
            }
            else
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "QCarCamGetFrame failed, m_QcarCamHndl: %lu, status=%d", m_QcarCamHndl,
                          status );
            }
        }
        else
        {
            frameIdx = pFrameInfo->bufferIndex;
            pCamFrame = m_pCameraFrames[pFrameInfo->id] + frameIdx;
            pCamFrame->timestamp = pFrameInfo->sofTimestamp.timestamp;
            pCamFrame->timestampQGPTP = pFrameInfo->sofTimestamp.timestampGPTP;
            pCamFrame->flags = pFrameInfo->flags;
            QC_DEBUG( "Get Camera Frame Info: "
                      "bufferListId: %u "
                      "buffer index: %u "
                      "buffer pointer addr: %p "
                      "buffer data addr: %p "
                      "buffer size: %u "
                      "timestamp: %llu "
                      "timestampGPTP: %llu ",
                      "flags: %x ", frameInfo.id, frameIdx, pCamFrame, pCamFrame->pBuf,
                      pCamFrame->size, pCamFrame->timestamp, pCamFrame->timestampQGPTP,
                      pCamFrame->flags );
        }
    }

    return pCamFrame;
}

QCStatus_e CameraImpl::ValidateConfig( const CameraImplConfig_t *pConfig )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pConfig )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "pConfig is nullptr" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( pConfig->numStream > QCNODE_CAMERA_MAX_STREAM_NUM ) || ( 0 == pConfig->numStream ) )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Invalid numStream: %u", pConfig->numStream );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( true == pConfig->bPrimary ) && ( 0 == pConfig->clientId ) )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Invalid client id for primary session" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( uint32_t i = 0; i < pConfig->numStream; i++ )
        {
            if ( pConfig->streamConfigs[i].streamId >= QCNODE_CAMERA_MAX_STREAM_NUM )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Invalid streamId: %u for stream %u", pConfig->streamConfigs[i].streamId,
                          i );
                break;
            }
        }
    }

    return ret;
}

void CameraImpl::FrameCallback( CameraFrameDescriptor_t *pFrame, void *pPrivData )
{
    CameraImpl *self = reinterpret_cast<CameraImpl *>( pPrivData );
    if ( nullptr != self )
    {
        self->FrameCallback( pFrame );
    }
    else
    {
        QC_LOG_ERROR( "Camera: pAppPriv is invalid" );
    }
}

void CameraImpl::FrameCallback( CameraFrameDescriptor_t *pFrame )
{
    QCStatus_e ret = QC_STATUS_OK;
    NodeFrameDescriptor frameDesc( 1 );

    if ( nullptr == m_callback )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "callback is invalid" );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = frameDesc.SetBuffer( 0, *pFrame );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_OBJECT_STATE_RUNNING == m_state )
        {
            QC_TRACE_EVENT( "FrameReady",
                            { QCNodeTraceArg( "streamId", pFrame->streamId ),
                              QCNodeTraceArg( "frameId", m_frameId[pFrame->streamId] ) } );
            QCNodeEventInfo_t info( frameDesc, m_nodeId, QC_STATUS_OK,
                                    static_cast<QCObjectState_e>( m_state ) );
            pFrame->id = m_frameId[pFrame->streamId]++;
            m_callback( info );
        }
    }
    else
    {
        QC_ERROR( "Failed to set frame buffer descriptor" );
    }
}

void CameraImpl::EventCallback( const uint32_t eventId, const void *pPayload, void *pPrivData )
{
    CameraImpl *self = reinterpret_cast<CameraImpl *>( pPrivData );

    self->EventCallback( eventId, pPayload );
}

void CameraImpl::EventCallback( const uint32_t eventId, const void *pPayload )
{
    QCStatus_e ret = QC_STATUS_OK;
    NodeFrameDescriptor frameDesc( 1 );
    QCBufferDescriptorBase_t eventDesc;
    CameraImpEvent_t event;

    if ( nullptr == m_callback )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "callback is invalid" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_OBJECT_STATE_RUNNING == m_state )
        {
            QCNodeEventInfo_t info( frameDesc, m_nodeId, QC_STATUS_FAIL,
                                    static_cast<QCObjectState_e>( m_state ) );
            QC_INFO( "Received event: %d, pPayload:%p", eventId, pPayload );
        }
    }
    else
    {
        QC_ERROR( "Failed to set event buffer descriptor" );
    }
}

QCarCamRet_e CameraImpl::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                         const QCarCamEventPayload_t *pPayload, void *pPrivateData )
{
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( nullptr == pPrivateData )
    {
        QC_LOG_ERROR( "invalid pPrivateData" );
    }
    else
    {
        CameraImpl *self = (CameraImpl *) pPrivateData;
        status = self->QcarcamEventCb( hndl, eventId, pPayload );
    }

    return status;
}

QCarCamRet_e CameraImpl::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                         const QCarCamEventPayload_t *pPayload )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    CameraFrameDescriptor_t *pCameraFrame = nullptr;

    QC_DEBUG( "QcarcamEventCb eventId: %u", eventId );

    switch ( eventId )
    {
        case QCARCAM_EVENT_FRAME_READY:
        {
            pCameraFrame = GetFrame( &pPayload->frameInfo );
            if ( nullptr != pCameraFrame )
            {
                FrameCallback( pCameraFrame, (void *) this );
            }
            else
            {
                QC_ERROR( "GetFrame failed, returning nullptr" );
            }

            break;
        }
        case QCARCAM_EVENT_INPUT_SIGNAL:
        {
            QC_ERROR( "QCARCAM received new input signal" );
            break;
        }
        case QCARCAM_EVENT_MC_NOTIFY:
        {
            QC_DEBUG( "received QCARCAM_EVENT_MC_NOTIFY" );
            switch ( pPayload->mcEventInfo.event )
            {
                case QCARCAM_MC_STREAM_CREATE:
                    QC_DEBUG( "MC_STREAM_CREATE, numStreams=%u not implemented",
                              pPayload->mcEventInfo.numStreams );
                    for ( uint32_t j = 0U; j < pPayload->mcEventInfo.numStreams; j++ )
                    {
                        QC_DEBUG( "buffer=%#x", pPayload->mcEventInfo.bufferListId[j] );
                    }
                    break;
                case QCARCAM_MC_STREAM_DESTROY:
                    QC_DEBUG( "MC_STREAM_DESTROY, numStreams=%u not implemented",
                              pPayload->mcEventInfo.numStreams );
                    for ( uint32_t j = 0U; j < pPayload->mcEventInfo.numStreams; j++ )
                    {
                        QC_DEBUG( "buffer=%#x", pPayload->mcEventInfo.bufferListId[j] );
                    }
                    break;
                case QCARCAM_MC_STREAM_START:
                    QC_DEBUG( "MC_STREAM_START, numStreams=%u not implemented",
                              pPayload->mcEventInfo.numStreams );
                    for ( uint32_t j = 0U; j < pPayload->mcEventInfo.numStreams; j++ )
                    {
                        QC_DEBUG( "buffer=%#x", pPayload->mcEventInfo.bufferListId[j] );
                    }
                    break;
                case QCARCAM_MC_STREAM_STOP:
                    QC_DEBUG( "MC_STREAM_STOP, numStreams=%u not implemented",
                              pPayload->mcEventInfo.numStreams );
                    for ( uint32_t j = 0U; j < pPayload->mcEventInfo.numStreams; j++ )
                    {
                        QC_DEBUG( "buffer=%#x", pPayload->mcEventInfo.bufferListId[j] );
                    }
                    break;
                default:
                    QC_ERROR( "event_cb Received unsupported mc event %d",
                              pPayload->mcEventInfo.event );
                    break;
            }
            /* Let the user applicaiton to handle the multi-client event */
            EventCallback( eventId, (void *) pPayload, (void *) this );
            break;
        }
        case QCARCAM_EVENT_ERROR:
        {
            QC_ERROR( "QCARCAM_EVENT_ERROR: error Id=%d, code=%u, source=%u",
                      pPayload->errInfo.errorId, pPayload->errInfo.errorCode,
                      pPayload->errInfo.errorSource );
            EventCallback( eventId, (void *) pPayload, (void *) this );
            break;
        }
        default:
        {
            QC_ERROR( "event_cb Received unsupported event %d", eventId );
            break;
        }
    }

    return status;
}

QCarCamColorFmt_e CameraImpl::GetQcarCamFormat( QCImageFormat_e colorFormat )
{
    QCarCamColorFmt_e qcarcamFormat = QCARCAM_FMT_MAX;

    switch ( colorFormat )
    {
        case QC_IMAGE_FORMAT_RGB888:
        {
            qcarcamFormat = QCARCAM_FMT_RGB_888;
            break;
        }
        case QC_IMAGE_FORMAT_BGR888:
        {
            qcarcamFormat = QCARCAM_FMT_BGR_888;
            break;
        }
        case QC_IMAGE_FORMAT_UYVY:
        {
            qcarcamFormat = QCARCAM_FMT_UYVY_8;
            break;
        }
        case QC_IMAGE_FORMAT_NV12:
        {
            qcarcamFormat = QCARCAM_FMT_NV12;
            break;
        }
        case QC_IMAGE_FORMAT_NV12_UBWC:
        {
            qcarcamFormat = QCARCAM_FMT_UBWC_NV12;
            break;
        }
        case QC_IMAGE_FORMAT_P010:
        {
            qcarcamFormat = QCARCAM_FMT_P010;
            break;
        }
        case QC_IMAGE_FORMAT_TP10_UBWC:
        {
            qcarcamFormat = QCARCAM_FMT_UBWC_TP10;
            break;
        }
        default:
        {
            QC_ERROR( "Unsupport corlor sormat: %d", colorFormat );
            break;
        }
    }

    return qcarcamFormat;
}

}   // namespace Node
}   // namespace QC
