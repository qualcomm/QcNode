// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/component/Camera.hpp"
#include <cstring>
#include <thread>

#define MAX_QUERY_TIMES ( 20 )

namespace QC
{
namespace component
{

#define ALIGN_S( size, align ) ( ( ( ( size ) + (align) -1 ) / ( align ) ) * ( align ) )

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

QCarCamColorFmt_e Camera::GetQcarCamFormat( QCImageFormat_e colorFormat )
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

QCarCamRet_e Camera::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                     const QCarCamEventPayload_t *pPayload, void *pPrivateData )
{
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( nullptr == pPrivateData )
    {
        QC_LOG_ERROR( "invalid pPrivateData" );
    }
    else
    {
        Camera *self = (Camera *) pPrivateData;
        status = self->QcarcamEventCb( hndl, eventId, pPayload );
    }

    return status;
}

QCarCamRet_e Camera::QcarcamEventCb( const QCarCamHndl_t hndl, const uint32_t eventId,
                                     const QCarCamEventPayload_t *pPayload )
{
    CameraFrame_t *pCameraFrame = nullptr;

    QC_DEBUG( "QcarcamEventCb eventId: %u", eventId );
    switch ( eventId )
    {
        case QCARCAM_EVENT_FRAME_READY:
        {
            pCameraFrame = GetFrame( &pPayload->frameInfo );
            if ( nullptr != pCameraFrame )
            {
                if ( nullptr != m_FrameCallback )
                {
                    m_FrameCallback( pCameraFrame, m_pAppPriv );
                }
                else
                {
                    QC_ERROR( "frame callback is nullptr!" );
                }
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
            if ( nullptr != m_EventCallback )
            {
                m_EventCallback( eventId, (void *) pPayload, m_pAppPriv );
            }
            else
            {
                QC_ERROR( "event callback is nullptr!" );
            }
            break;
        }
        case QCARCAM_EVENT_ERROR:
        {
            QC_ERROR( "QCARCAM_EVENT_ERROR: error Id=%d, code=%u, source=%u",
                      pPayload->errInfo.errorId, pPayload->errInfo.errorCode,
                      pPayload->errInfo.errorSource );
            if ( nullptr != m_EventCallback )
            {
                m_EventCallback( eventId, (void *) pPayload, m_pAppPriv );
            }
            else
            {
                QC_ERROR( "event callback is nullptr!" );
            }
            break;
        }
        default:
        {
            QC_ERROR( "event_cb Received unsupported event %d", eventId );
            break;
        }
    }

    return QCARCAM_RET_OK;
}

Camera::Camera()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamInit_t qcarcamInit = { 0 };
    qcarcamInit.apiVersion = QCARCAM_VERSION;
    QCarCamRet_e status = QCARCAM_RET_OK;
    m_QcarCamHndl = QCARCAM_HNDL_INVALID;

    std::lock_guard<std::mutex> guard( g_camInitMutex );
    if ( 0 == g_nCamInitRefCount )
    {
        status = QCarCamInitialize( (const QCarCamInit_t *) &qcarcamInit );
        if ( QCARCAM_RET_OK != status )
        {
            QC_LOG_ERROR( "QCarCamInitialize failed", status );
            m_state = QC_OBJECT_STATE_ERROR;
        }
        else
        {
            QC_LOG_INFO( "QCarCamInitialize success" );
            m_state = QC_OBJECT_STATE_INITIAL;
            g_nCamInitRefCount++;

            ret = QueryInputs();

            if ( QC_STATUS_OK != ret )
            {
                QC_LOG_ERROR( "QueryInputs failed: %d", ret );
            }
        }
    }
    else
    {
        g_nCamInitRefCount++;
    }
}

Camera::~Camera()
{
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
                QC_LOG_ERROR( "QCarCamUninitialize failed: %d", status );
            }

            FreeCameraInputsInfo();
        }
        else
        {
            QC_LOG_INFO( "Skip QCarCamUninitialize" );
        }
    }
    else
    {
        QC_LOG_ERROR( "g_nCamInitRefCount not greater than 0, unexpected" );
    }
}

QCStatus_e Camera::ValidateConfig( const Camera_Config_t *pConfig )
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( nullptr == pConfig )
    {
        QC_ERROR( "pConfig is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( pConfig->numStream > MAX_CAMERA_STREAM ) || ( 0 == pConfig->numStream ) )
    {
        QC_ERROR( "Invalid numStream: %u", pConfig->numStream );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( true == pConfig->bPrimary ) && ( 0 == pConfig->clientId ) )
    {
        QC_ERROR( "Invalid client id for primary session" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < pConfig->numStream; i++ )
        {
            if ( pConfig->streamConfig[i].streamId >= MAX_CAMERA_STREAM )
            {
                QC_ERROR( "Invalid streamId: %u for stream %u", pConfig->streamConfig[i].streamId,
                          i );
                ret = QC_STATUS_BAD_ARGUMENTS;
                break;
            }
        }
    }

    return ret;
}

QCStatus_e Camera::Init( const char *pName, const Camera_Config_t *pConfig, Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t param = 0;
    bool bIFInitOK = false;

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK == ret )
    {
        bIFInitOK = true;
        ret = ValidateConfig( pConfig );
    }

    if ( QC_STATUS_OK == ret )
    { /* save configuration parameters */
        m_state = QC_OBJECT_STATE_INITIALIZING;
        m_maxBufCnt = 0;
        for ( uint32_t i = 0; i < pConfig->numStream; i++ )
        {
            m_streamConfig[i] = pConfig->streamConfig[i];
            if ( m_streamConfig[i].bufCnt > m_maxBufCnt )
            {
                m_maxBufCnt = m_streamConfig[i].bufCnt;
            }
        }
        m_bIsAllocator = pConfig->bAllocator;
        m_nInputId = pConfig->inputId;
        m_bRequestMode = pConfig->bRequestMode;
        m_nNumStream = pConfig->numStream;
        m_nClientId = pConfig->clientId;
        m_bIsPrimary = pConfig->bPrimary;
        m_bRecovery = pConfig->bRecovery;
    }

    if ( QC_STATUS_OK == ret )
    { /* setup submit request pattern for multiple streaming */
        m_bRequestPatternMode = false;
        if ( ( true == m_bRequestMode ) && ( pConfig->numStream > 1 ) )
        {
            m_refStreamId = MAX_CAMERA_STREAM;
            for ( uint32_t i = 0; i < pConfig->numStream; i++ )
            {
                if ( 0 == m_streamConfig[i].submitRequestPattern )
                {
                    if ( MAX_CAMERA_STREAM == m_refStreamId )
                    { /* the first stream with 0 pattern acting as reference stream */
                        m_refStreamId = m_streamConfig[i].streamId;
                    }
                    m_submitRequestPattern[i] = 0;
                }
                else
                { /* if there is anyone none-zero pattern, a submit request pattern mode for FPS HW
                     drop control */
                    m_submitRequestPattern[i] = m_streamConfig[i].submitRequestPattern;
                    m_bRequestPatternMode = true;
                }
            }

            if ( true == m_bRequestPatternMode )
            {
                if ( MAX_CAMERA_STREAM == m_refStreamId )
                {
                    QC_ERROR( "Need at least 1 stream with 0 submitRequestPattern" );
                    ret = QC_STATUS_BAD_ARGUMENTS;
                }
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        CameraInputs_t camInputsInfo;
        ret = GetInputsInfo( &camInputsInfo );
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "GetInputsInfo failed: %d", ret );
        }
        else
        {
            QCarCamInputModes_t *pCamInputModes = nullptr;
            for ( uint32_t i = 0; i < camInputsInfo.numInputs; i++ )
            {
                if ( camInputsInfo.pCameraInputs[i].inputId == m_nInputId )
                {
                    pCamInputModes = &camInputsInfo.pCamInputModes[i];
                    break;
                }
            }

            if ( nullptr == pCamInputModes )
            {
                QC_ERROR( "camera input id %u not found", m_nInputId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else if ( 0 == pCamInputModes->numModes )
            {
                QC_ERROR( "no mode 0 for camera input id %u", m_nInputId );
                ret = QC_STATUS_OUT_OF_BOUND;
            }
            else
            {
                // do nothing
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        QCarCamInputStream_t inputParams = { 0 };
        inputParams.inputId = pConfig->inputId;
        inputParams.srcId = pConfig->srcId;
        inputParams.inputMode = pConfig->inputMode;

        QCarCamOpen_t openParams = { (QCarCamOpmode_e) 0, 0 };
        openParams.opMode = (QCarCamOpmode_e) pConfig->opMode;
        openParams.numInputs = 1;
        openParams.clientId = m_nClientId;
        openParams.inputs[0] = inputParams;

        if ( m_bRecovery )
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_RECOVERY;
        }

        if ( m_bRequestMode )
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_REQUEST_MODE;
        }

        if ( 0 != m_nClientId )
        {
            openParams.flags |= QCARCAM_OPEN_FLAGS_MULTI_CLIENT_SESSION;
        }

        status = QCarCamOpen( &openParams, &m_QcarCamHndl );
        if ( ( QCARCAM_RET_OK != status ) || ( QCARCAM_HNDL_INVALID == m_QcarCamHndl ) )
        {
            QC_ERROR( "QCarCamOpen failed: %d", status );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_INFO( "QCarCamOpen Success handle: %lu", m_QcarCamHndl );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        // setup params
        status = QCarCamRegisterEventCallback( m_QcarCamHndl, &QcarcamEventCb, this );
        if ( QCARCAM_RET_OK != status )
        {
            QC_ERROR( "QCARCAM_PARAM_EVENT_CB failed ret %d", status );
            ret = QC_STATUS_FAIL;
            m_state = QC_OBJECT_STATE_ERROR;
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
            QC_ERROR( "QCARCAM_PARAM_EVENT_MASK failed ret %d", status );
            ret = QC_STATUS_FAIL;
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
        ispConfig.usecaseId = (QCarCamIspUsecase_e) pConfig->ispUserCase;

        status = QCarCamSetParam( m_QcarCamHndl, QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE,
                                  &ispConfig, sizeof( ispConfig ) );

        if ( status != QCARCAM_RET_OK )
        {
            QC_ERROR( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE failed ret %d", status );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_INFO( "QCARCAM_STREAM_CONFIG_PARAM_ISP_USECASE success" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( 0 == pConfig->camFrameDropPattern )
        {
            QC_INFO( "Ignore frame drop config" );
        }
        else
        {
            // setup frame rate params
            QCarCamFrameDropConfig_t frameDropConfig = { 0 };
            frameDropConfig.frameDropPeriod = pConfig->camFrameDropPeriod;
            frameDropConfig.frameDropPattern = pConfig->camFrameDropPattern;
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
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( ( 0 != m_nClientId ) && ( false == m_bIsPrimary ) )
        { /* for multi-client, non primary session, query the primary's buffers */
            ret = ImportBuffers();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "ImportBuffers failed" );
            }
        }
        else if ( true == m_bIsAllocator )
        { /* for multi-client, primary session, or single-client case, buffer allocated by this
             Camera Component */
            ret = AllocateBuffers();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "AllocateBuffers failed" );
            }
        }
        else
        {
            /* do nothing as APP allocate the buffer and set through SetBuffers API call later */
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_nRequestId = 0;
        m_bReservedOK = false;
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    { /* do error clean up */
        if ( QCARCAM_HNDL_INVALID != m_QcarCamHndl )
        {
            QC_ERROR( "Error happens in Init" );
            (void) QCarCamClose( m_QcarCamHndl );
            m_state = QC_OBJECT_STATE_INITIAL;
            m_QcarCamHndl = QCARCAM_HNDL_INVALID;
        }

        if ( bIFInitOK )
        {
            (void) ComponentIF::Deinit();
        }
    }

    return ret;
}

QCStatus_e Camera::SubmitAllBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    if ( ( ( 0 == m_nClientId ) || ( true == m_bIsPrimary ) ) )
    {
        for ( uint32_t bufIdx = 0; bufIdx < m_maxBufCnt; bufIdx++ )
        {
            QCarCamRequest_t request = { 0 };
            request.numStreamRequests = 0;
            for ( uint32_t i = 0; i < m_nNumStream; i++ )
            {
                if ( bufIdx < m_streamConfig[i].bufCnt )
                {
                    QCarCamStreamRequest_t *pStreamRequest =
                            &request.streamRequests[request.numStreamRequests];
                    pStreamRequest->bufferlistId = m_streamConfig[i].streamId;
                    pStreamRequest->bufferIdx = bufIdx;
                    request.numStreamRequests++;
                    QC_DEBUG( "RequestFrame m_QcarCamHndl: %lu, bufferlistId: %u, "
                              "bufferIdx: %u",
                              m_QcarCamHndl, pStreamRequest->bufferlistId,
                              pStreamRequest->bufferIdx );
                }
            }
            request.requestId = __atomic_fetch_add( &m_nRequestId, 1, __ATOMIC_RELAXED );
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

QCStatus_e Camera::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    bool bReservedOK = false;
    bool bStartOK = false;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Camera not in ready state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == m_FrameCallback )
    {
        QC_ERROR( "Camera callbacks is not registerred!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        m_state = QC_OBJECT_STATE_STARTING;
        if ( false == m_bReservedOK )
        { /* only do reserve once */
            status = QCarCamReserve( m_QcarCamHndl );
            if ( QCARCAM_RET_OK != status )
            {
                QC_ERROR( "QCarCamReserve failed with ret %d, exit", status );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                bReservedOK = true;
                m_bReservedOK = true;
            }
        }
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
    { /* error clean up */
        m_state = QC_OBJECT_STATE_READY;
        if ( bStartOK )
        {
            (void) QCarCamStop( m_QcarCamHndl );
        }
        if ( bReservedOK )
        {
            (void) QCarCamRelease( m_QcarCamHndl );
        }
    }

    return ret;
}


QCStatus_e Camera::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

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

    return ret;
}

QCStatus_e Camera::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2 = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        if ( 0 == m_QcarCamHndl )
        {
            QC_ERROR( "Qcarcam null handle" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            if ( true == m_bReservedOK )
            {
                status = QCarCamRelease( m_QcarCamHndl );
                if ( QCARCAM_RET_OK != status )
                {
                    QC_ERROR( "QCarCamRelease failed %d", status );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    m_bReservedOK = false;
                }
            }

            status = QCarCamClose( m_QcarCamHndl );
            if ( QCARCAM_RET_OK != status )
            {
                QC_ERROR( "QCarCamClose failed %d", status );
                ret = QC_STATUS_FAIL;
            }

            if ( ( 0 != m_nClientId ) && ( false == m_bIsPrimary ) )
            {
                ret2 = UnImportBuffers();
                if ( QC_STATUS_OK != ret2 )
                {
                    QC_ERROR( "Error in unimport buffers" );
                    ret = ret2;
                }
            }
            else if ( m_bIsAllocator )
            {
                ret2 = FreeBuffers();
                if ( QC_STATUS_OK != ret2 )
                {
                    QC_ERROR( "Error in free buffers" );
                    ret = ret2;
                }
            }
            else
            {
                /* do nothing */
            }
        }

        for ( uint32_t i = 0; i < m_nNumStream; i++ )
        {
            uint32_t streamId = m_streamConfig[i].streamId;
            if ( nullptr != m_pCameraFrames[streamId] )
            {
                delete[] m_pCameraFrames[streamId];
                m_pCameraFrames[streamId] = nullptr;
            }

            if ( nullptr != m_pQcarcamBuffer[streamId] )
            {
                delete[] m_pQcarcamBuffer[streamId];
                m_pQcarcamBuffer[streamId] = nullptr;
            }
        }

        ret2 = ComponentIF::Deinit();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "Fail to deinit ComponentIF" );
            ret = ret2;
        }
    }
    else
    {
        QC_ERROR( "Camera not in ready state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Camera::Pause()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        status = QCarCamPause( m_QcarCamHndl );
        if ( QCARCAM_RET_OK == status )
        {
            QC_INFO( "QCarCamPause success" );
            m_state = QC_OBJECT_STATE_PAUSE;
        }
        else
        {
            QC_ERROR( "QCarCamPause fail: status = %d", status );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        QC_ERROR( "Camera not in running state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Camera::Resume()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( QC_OBJECT_STATE_PAUSE == m_state )
    {
        status = QCarCamResume( m_QcarCamHndl );
        if ( QCARCAM_RET_OK == status )
        {
            QC_INFO( "QCarCamResume success" );
            m_state = QC_OBJECT_STATE_RUNNING;
            if ( true == m_bRequestMode )
            {
                ret = SubmitAllBuffers();
            }
        }
        else
        {
            QC_ERROR( "QCarCamResume fail: status = %d", status );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        QC_ERROR( "Camera not in pause state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Camera::ReleaseFrame( const CameraFrame_t *pFrame )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Camera not in running state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pFrame )
    {
        QC_ERROR( "pFrame is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( true == m_bRequestMode )
    {
        QC_ERROR( "ReleaseFrame is not allowed for request mode" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }
    else
    {
        status = QCarCamReleaseFrame( m_QcarCamHndl, pFrame->streamId, pFrame->frameIndex );
        if ( QCARCAM_RET_OK == status )
        {
            QC_INFO( "QCarCamReleaseFrame success for index: %u", pFrame->frameIndex );
        }
        else
        {
            QC_ERROR( "QCarCamReleaseFrame fail for id: %d index: %u, status: %d", pFrame->streamId,
                      pFrame->frameIndex, status );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

CameraFrame_t *Camera::GetFrame( const QCarCamFrameInfo_t *pFrameInfo )
{
    uint32_t frameIndex = 0;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamFrameInfo_t frameInformation = { 0 };
    CameraFrame_t *pCameraFrame = nullptr;
    uint64_t timeout = 0;
    frameInformation.id = pFrameInfo->id;

    if ( !m_bRequestMode )
    {
        status = QCarCamGetFrame( m_QcarCamHndl, &frameInformation, timeout, 0 );

        if ( QCARCAM_RET_OK == status )
        {
            frameIndex = frameInformation.bufferIndex;
            pCameraFrame = &m_pCameraFrames[frameInformation.id][frameIndex];
            pCameraFrame->timestamp = frameInformation.sofTimestamp.timestamp;
            pCameraFrame->timestampQGPTP = frameInformation.sofTimestamp.timestampGPTP;
            pCameraFrame->flags = frameInformation.flags;
            QC_DEBUG( "GetFrame bufferlistId: %u, bufferIdx: %u ptr: %p buffer: %p, size: %u, "
                      "timestamp: %llu, "
                      "timestampGPTP: %llu, flags: %x",
                      frameInformation.id, frameIndex, pCameraFrame,
                      pCameraFrame->sharedBuffer.data(), pCameraFrame->sharedBuffer.size,
                      pCameraFrame->timestamp, pCameraFrame->timestampQGPTP, pCameraFrame->flags );
        }
        else
        {
            QC_ERROR( "QCarCamGetFrame failed, m_QcarCamHndl: %lu, status=%d", m_QcarCamHndl,
                      status );
        }
    }
    else
    {
        frameIndex = pFrameInfo->bufferIndex;
        pCameraFrame = &m_pCameraFrames[pFrameInfo->id][frameIndex];
        pCameraFrame->timestamp = pFrameInfo->sofTimestamp.timestamp;
        pCameraFrame->timestampQGPTP = pFrameInfo->sofTimestamp.timestampGPTP;
        pCameraFrame->flags = pFrameInfo->flags;
        QC_DEBUG( "GetFrame bufferlistId: %u, bufferIdx: %u ptr: %p buffer: %p, size: %d, "
                  "timestamp: %llu, "
                  "timestampGPTP: %llu, flags: %x",
                  frameInformation.id, frameIndex, pCameraFrame, pCameraFrame->sharedBuffer.data(),
                  pCameraFrame->sharedBuffer.size, pCameraFrame->timestamp,
                  pCameraFrame->timestampQGPTP, pCameraFrame->flags );
    }

    return pCameraFrame;
}

QCStatus_e Camera::RequestFrame( const CameraFrame_t *pFrame )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    QCarCamRequest_t request = { 0 };

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Camera not in running state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( false == m_bRequestMode )
    {
        QC_ERROR( "RequestFrame is not allowed for non request mode" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }
    else if ( ( 0 != m_nClientId ) && ( false == m_bIsPrimary ) )
    {
        QC_ERROR( "RequestFrame is not allowed for multi-client non primary session" );
        ret = QC_STATUS_OUT_OF_BOUND;
    }
    else if ( nullptr == pFrame )
    {
        QC_ERROR( "pFrame is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        uint32_t bufferListId = pFrame->streamId;
        uint32_t bufferIdx = pFrame->frameIndex;
        if ( true == m_bRequestPatternMode )
        {
            std::unique_lock<std::mutex> lock( m_mutex );
            m_freeBufIdxQueue[bufferListId].push( bufferIdx );
            if ( m_refStreamId == bufferListId )
            { /* this is the reference frame, do submit requst */
                for ( uint32_t i = 0; i < m_nNumStream; i++ )
                {
                    uint32_t streamId = m_streamConfig[i].streamId;
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
                            m_submitRequestPattern[i] = m_streamConfig[i].submitRequestPattern;
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
            request.requestId = __atomic_fetch_add( &m_nRequestId, 1, __ATOMIC_RELAXED );
            QC_DEBUG( "RequestFrame begin m_QcarCamHndl: %lu, bufferlistId: %u, "
                      "bufferIdx: %u, request id: %u",
                      m_QcarCamHndl, bufferListId, bufferIdx, request.requestId );
            status = QCarCamSubmitRequest( m_QcarCamHndl, &request );
            if ( QCARCAM_RET_OK != status )
            {
                QC_ERROR( "RequestFrame fail m_QcarCamHndl: %lu, bufferlistId: %u, bufferIdx: "
                          "%u request id: %u, status=%d",
                          m_QcarCamHndl, bufferListId, bufferIdx, request.requestId, status );
                ret = QC_STATUS_FAIL;
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

QCStatus_e Camera::RegisterCallback( QCCamFrameCallback_t frameCallback,
                                     QCCamEventCallback_t eventCallback, void *pAppPriv )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_INITIAL != m_state ) && ( QC_OBJECT_STATE_READY != m_state ) )
    {
        QC_ERROR( "Register Callback failed due to wrong state!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( ( nullptr == frameCallback ) || ( nullptr == eventCallback ) ||
              ( nullptr == pAppPriv ) )
    {
        QC_ERROR( "Register Callback with nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        m_FrameCallback = frameCallback;
        m_EventCallback = eventCallback;
        m_pAppPriv = pAppPriv;
    }

    return ret;
}

QCStatus_e Camera::AllocateBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2 = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t streamId = 0;

    for ( int i = 0; i < m_nNumStream; i++ )
    {
        if ( 0 < m_streamConfig[i].bufCnt )
        {
            streamId = m_streamConfig[i].streamId;

            m_pCameraFrames[streamId] = new CameraFrame_t[m_streamConfig[i].bufCnt];
            m_pQcarcamBuffer[streamId] = new QCarCamBuffer_t[m_streamConfig[i].bufCnt];
            m_qcarcamBuffers[streamId].id = m_streamConfig[i].streamId;
            m_qcarcamBuffers[streamId].nBuffers = m_streamConfig[i].bufCnt;
            m_qcarcamBuffers[streamId].pBuffers = m_pQcarcamBuffer[streamId];
            m_qcarcamBuffers[streamId].colorFmt = GetQcarCamFormat( m_streamConfig[i].format );
            m_qcarcamBuffers[streamId].flags = QCARCAM_BUFFER_FLAG_OS_HNDL;
        }
        else
        {
            QC_ERROR( "Invalid buffer count: %d", m_streamConfig[i].bufCnt );
            ret = QC_STATUS_FAIL;
            break;
        }

        for ( uint32_t j = 0; j < m_streamConfig[i].bufCnt; j++ )
        {
            QCarCamBuffer_t *pQcarcamBuf = &m_pQcarcamBuffer[streamId][j];
            CameraFrame_t *pCamFrame = &m_pCameraFrames[streamId][j];
            if ( ( QC_IMAGE_FORMAT_RGB888 == m_streamConfig[i].format ) ||
                 ( QC_IMAGE_FORMAT_BGR888 == m_streamConfig[i].format ) )
            {
                QCImageProps_t imgProp;
                imgProp.format = m_streamConfig[i].format;
                imgProp.batchSize = 1;
                imgProp.width = m_streamConfig[i].width;
                imgProp.height = m_streamConfig[i].height;
                imgProp.stride[0] = ALIGN_S( m_streamConfig[i].width * 3, 16 );
                imgProp.actualHeight[0] = m_streamConfig[i].height;
                imgProp.numPlanes = 1;
                imgProp.planeBufSize[0] = 0;
                ret2 = pCamFrame->sharedBuffer.Allocate( &imgProp );
            }
            else
            {
                ret2 = pCamFrame->sharedBuffer.Allocate( m_streamConfig[i].width,
                                                         m_streamConfig[i].height,
                                                         m_streamConfig[i].format );
            }
            if ( QC_STATUS_OK != ret2 )
            {
                QC_ERROR( "Buffer allocation failed" );
                ret = ret2;
                break;
            }
            else
            {
                uint32_t offset = 0;
                QCImageFormat_e format = pCamFrame->sharedBuffer.imgProps.format;
                pQcarcamBuf->numPlanes = pCamFrame->sharedBuffer.imgProps.numPlanes;
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
                    pQcarcamBuf->planes[k].memHndl = pCamFrame->sharedBuffer.buffer.dmaHandle;
                    pQcarcamBuf->planes[k].width = pCamFrame->sharedBuffer.imgProps.width;
                    pQcarcamBuf->planes[k].height = pCamFrame->sharedBuffer.imgProps.height;
                    pQcarcamBuf->planes[k].stride = pCamFrame->sharedBuffer.imgProps.stride[k];
                    pQcarcamBuf->planes[k].size = pCamFrame->sharedBuffer.imgProps.planeBufSize[k];
                    pQcarcamBuf->planes[k].offset = offset;
                    offset += pCamFrame->sharedBuffer.imgProps.planeBufSize[k];
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
                    pQcarcamBuf->planes[0].size = pCamFrame->sharedBuffer.imgProps.planeBufSize[0] +
                                                  pCamFrame->sharedBuffer.imgProps.planeBufSize[1];
                    pQcarcamBuf->planes[0].offset = 0;
                    pQcarcamBuf->planes[1].size = pCamFrame->sharedBuffer.imgProps.planeBufSize[2] +
                                                  pCamFrame->sharedBuffer.imgProps.planeBufSize[3];
                    pQcarcamBuf->planes[1].offset = pQcarcamBuf->planes[0].size;
                }
                pCamFrame->frameIndex = j;
                pCamFrame->streamId = m_streamConfig[i].streamId;
                QC_INFO( "register buffer index %u: memHndl: %llu va: %p width: %u, "
                         "height: %u, stride: %u size: %u",
                         i, pQcarcamBuf->planes[0].memHndl, pCamFrame->sharedBuffer.data(),
                         pQcarcamBuf->planes[0].width, pQcarcamBuf->planes[0].height,
                         pQcarcamBuf->planes[0].stride, pQcarcamBuf->planes[0].size );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            // setup buffers
            status = QCarCamSetBuffers( m_QcarCamHndl,
                                        (const QCarCamBufferList_t *) &m_qcarcamBuffers[streamId] );
            if ( QCARCAM_RET_OK != status )
            {
                QC_ERROR( "QCarCamSetBuffers error ret %d  handle %lu", status, m_QcarCamHndl );
                if ( nullptr != m_pCameraFrames[streamId] )
                {
                    delete[] m_pCameraFrames[streamId];
                    m_pCameraFrames[streamId] = nullptr;
                }
                if ( nullptr != m_pQcarcamBuffer[streamId] )
                {
                    delete[] m_pQcarcamBuffer[streamId];
                    m_pQcarcamBuffer[streamId] = nullptr;
                }
                ret = QC_STATUS_FAIL;
            }
            else
            {
                QC_INFO( "QCarCamSetBuffers successful" );
            }
        }
    }

    return ret;
}

QCStatus_e Camera::FreeBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_INFO( "Camera::FreeBuffers" );

    for ( int i = 0; i < m_nNumStream; i++ )
    {
        uint32_t streamId = m_streamConfig[i].streamId;
        if ( nullptr != m_pCameraFrames[streamId] )
        {
            for ( uint32_t j = 0; j < m_streamConfig[i].bufCnt; j++ )
            {
                ret = m_pCameraFrames[streamId][j].sharedBuffer.Free();
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Free buffer failed index: %u %u, ret: %d", i, j, ret );
                }
            }
        }
        else
        {
            QC_ERROR( "m_pCameraFrames is nullptr" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    return ret;
}

QCStatus_e Camera::ImportBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2 = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;
    uint32_t streamId = 0;

    for ( int i = 0; i < m_nNumStream; i++ )
    {
        if ( 0 < m_streamConfig[i].bufCnt )
        {
            streamId = m_streamConfig[i].streamId;
            m_pCameraFrames[streamId] = new CameraFrame_t[m_streamConfig[i].bufCnt];
            m_pQcarcamBuffer[streamId] = new QCarCamBuffer_t[m_streamConfig[i].bufCnt];
            m_qcarcamBuffers[streamId].id = m_streamConfig[i].streamId;
            m_qcarcamBuffers[streamId].nBuffers = m_streamConfig[i].bufCnt;
            m_qcarcamBuffers[streamId].pBuffers = m_pQcarcamBuffer[streamId];
            m_qcarcamBuffers[streamId].colorFmt = GetQcarCamFormat( m_streamConfig[i].format );
            m_qcarcamBuffers[streamId].flags = QCARCAM_BUFFER_FLAG_OS_HNDL;
        }
        else
        {
            QC_ERROR( "Invalid buffer count: %d", m_streamConfig[i].bufCnt );
            ret = QC_STATUS_FAIL;
            break;
        }

        if ( QC_STATUS_OK == ret )
        {
            // get buffers
            status = QCarCamGetBuffers( m_QcarCamHndl, &m_qcarcamBuffers[streamId] );
            if ( QCARCAM_RET_OK != status )
            {
                QC_ERROR( "QCarCamGetBuffers error ret %d  handle %lu", status, m_QcarCamHndl );
                if ( nullptr != m_pCameraFrames[streamId] )
                {
                    delete[] m_pCameraFrames[streamId];
                    m_pCameraFrames[streamId] = nullptr;
                }
                if ( nullptr != m_pQcarcamBuffer[streamId] )
                {
                    delete[] m_pQcarcamBuffer[streamId];
                    m_pQcarcamBuffer[streamId] = nullptr;
                }
                ret = QC_STATUS_FAIL;
            }
            else
            {
                QC_INFO( "QCarCamGetBuffers successful" );
                for ( uint32_t j = 0; j < m_streamConfig[i].bufCnt; j++ )
                {
                    QCarCamBuffer_t *pQcarcamBuf = &m_pQcarcamBuffer[streamId][j];
                    CameraFrame_t *pCamFrame = &m_pCameraFrames[streamId][j];
                    QCSharedBuffer_t sharedBuffer;
                    sharedBuffer.buffer.dmaHandle = pQcarcamBuf->planes[0].memHndl;
                    sharedBuffer.buffer.size = 0;
                    sharedBuffer.buffer.usage = QC_BUFFER_USAGE_CAMERA;
                    sharedBuffer.buffer.flags = QC_BUFFER_FLAGS_CACHE_WB_WA;
                    /* for HGY, the fd is already imported one so no need to know the allocator's
                     * pid, set this to 0, the Import API will skip the dma buf import */
                    sharedBuffer.buffer.pid = 0;
                    sharedBuffer.type = QC_BUFFER_TYPE_IMAGE;
                    sharedBuffer.imgProps.format = m_streamConfig[i].format;
                    sharedBuffer.imgProps.batchSize = 1;
                    sharedBuffer.imgProps.width = pQcarcamBuf->planes[0].width;
                    sharedBuffer.imgProps.height = pQcarcamBuf->planes[0].height;
                    sharedBuffer.imgProps.numPlanes = pQcarcamBuf->numPlanes;
                    for ( uint32_t k = 0; k < pQcarcamBuf->numPlanes; k++ )
                    {
                        sharedBuffer.imgProps.stride[k] = pQcarcamBuf->planes[k].stride;
                        sharedBuffer.imgProps.planeBufSize[k] = pQcarcamBuf->planes[k].size;
                        sharedBuffer.imgProps.actualHeight[k] =
                                pQcarcamBuf->planes[k].size / pQcarcamBuf->planes[k].stride;
                        sharedBuffer.buffer.size += pQcarcamBuf->planes[k].size;
                    }
                    sharedBuffer.size = sharedBuffer.buffer.size;
                    sharedBuffer.offset = 0;

                    ret2 = pCamFrame->sharedBuffer.Import( &sharedBuffer );
                    if ( QC_STATUS_OK != ret2 )
                    {
                        QC_ERROR( "Buffer import failed" );
                        ret = ret2;
                        break;
                    }
                    else
                    {
                        pCamFrame->frameIndex = j;
                        pCamFrame->streamId = m_streamConfig[i].streamId;
                        QC_INFO( "buffer index %u: memHndl: %llu va: %p width: %u, "
                                 "height: %u, stride: %u size: %u",
                                 i, pQcarcamBuf->planes[0].memHndl, pCamFrame->sharedBuffer.data(),
                                 pQcarcamBuf->planes[0].width, pQcarcamBuf->planes[0].height,
                                 pQcarcamBuf->planes[0].stride, pQcarcamBuf->planes[0].size );
                    }
                }
            }
        }
    }

    return ret;
}

QCStatus_e Camera::UnImportBuffers()
{
    QCStatus_e ret = QC_STATUS_OK;

    QC_INFO( "Camera::UnImportBuffers" );

    for ( int i = 0; i < m_nNumStream; i++ )
    {
        uint32_t streamId = m_streamConfig[i].streamId;
        if ( nullptr != m_pCameraFrames[streamId] )
        {
            for ( uint32_t j = 0; j < m_streamConfig[i].bufCnt; j++ )
            {
                ret = m_pCameraFrames[streamId][j].sharedBuffer.UnImport();
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "UnImport buffer failed index: %u %u, ret: %d", i, j, ret );
                }
            }
        }
        else
        {
            QC_ERROR( "m_pCameraFrames is nullptr" );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
    }

    return ret;
}

QCStatus_e Camera::SetBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers,
                               uint32_t streamId )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCarCamRet_e status = QCARCAM_RET_OK;

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        if ( ( 0 != m_nClientId ) && ( false == m_bIsPrimary ) )
        {
            QC_ERROR( "set buffer is not allowed for multi-client non primary session" );
            ret = QC_STATUS_OUT_OF_BOUND;
        }
        else if ( ( nullptr == pBuffers ) || ( 0 >= numBuffers ) )
        {
            QC_ERROR( "invalid parameter pBuffers: %p, numBuffers: %u", pBuffers, numBuffers );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            bool bFound = false;
            for ( uint32_t index = 0; index < m_nNumStream; index++ )
            {
                if ( streamId == m_streamConfig[index].streamId )
                {
                    bFound = true;
                    break;
                }
            }

            if ( bFound == false )
            {
                QC_ERROR( "Did not find stream with id %u", streamId );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                if ( nullptr != m_pCameraFrames[streamId] )
                {
                    QC_ERROR( "buffers already set for stream: %u", streamId );
                    ret = QC_STATUS_ALREADY;
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                m_pCameraFrames[streamId] = new CameraFrame_t[numBuffers];
                m_pQcarcamBuffer[streamId] = new QCarCamBuffer_t[numBuffers];
                m_qcarcamBuffers[streamId].id = streamId;
                m_qcarcamBuffers[streamId].nBuffers = numBuffers;
                m_qcarcamBuffers[streamId].pBuffers = m_pQcarcamBuffer[streamId];
                m_qcarcamBuffers[streamId].colorFmt =
                        GetQcarCamFormat( pBuffers[0].imgProps.format );

                for ( uint32_t i = 0; i < numBuffers; i++ )
                {
                    m_pCameraFrames[streamId][i].sharedBuffer = pBuffers[i];

                    m_pQcarcamBuffer[streamId][i].numPlanes = 1;
                    m_pQcarcamBuffer[streamId][i].planes[0].memHndl =
                            m_pCameraFrames[streamId][i].sharedBuffer.buffer.dmaHandle;
                    m_pQcarcamBuffer[streamId][i].planes[0].width =
                            m_pCameraFrames[streamId][i].sharedBuffer.imgProps.width;
                    m_pQcarcamBuffer[streamId][i].planes[0].height =
                            m_pCameraFrames[streamId][i].sharedBuffer.imgProps.height;
                    m_pQcarcamBuffer[streamId][i].planes[0].stride =
                            m_pCameraFrames[streamId][i].sharedBuffer.imgProps.stride[0];
                    m_pQcarcamBuffer[streamId][i].planes[0].size =
                            m_pCameraFrames[streamId][i].sharedBuffer.size;

                    m_pCameraFrames[streamId][i].streamId = streamId;
                    m_pCameraFrames[streamId][i].frameIndex = i;
                    QC_INFO( "register buffer index %u ptr: %p memHndl: %llu va: %p width: %u, "
                             "height: %u, stride: %u size: %u",
                             i, &m_pCameraFrames[streamId][i],
                             m_pQcarcamBuffer[streamId][i].planes[0].memHndl,
                             m_pCameraFrames[streamId][i].sharedBuffer.data(),
                             m_pQcarcamBuffer[streamId][i].planes[0].width,
                             m_pQcarcamBuffer[streamId][i].planes[0].height,
                             m_pQcarcamBuffer[streamId][i].planes[0].stride,
                             m_pQcarcamBuffer[streamId][i].planes[0].size );
                }

                status = QCarCamSetBuffers(
                        m_QcarCamHndl, (const QCarCamBufferList_t *) &m_qcarcamBuffers[streamId] );
                if ( QCARCAM_RET_OK != status )
                {
                    QC_ERROR( "QCarCamSetBuffers error ret %d  handle %lu", status, m_QcarCamHndl );
                    ret = QC_STATUS_FAIL;
                }
                else
                {
                    QC_INFO( "QCarCamSetBuffers successful" );
                }
            }
        }
    }
    else
    {
        QC_ERROR( "Camera not in ready state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Camera::GetBuffers( QCSharedBuffer_t *pBuffers, uint32_t numBuffers, uint32_t streamId )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "Camera not in ready or running state: %d", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "pBuffers is nullptr" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        bool bFound = false;
        for ( uint32_t index = 0; index < m_nNumStream; index++ )
        {
            if ( streamId == m_streamConfig[index].streamId )
            {
                bFound = true;
                break;
            }
        }

        if ( true == bFound )
        {
            if ( nullptr == m_pCameraFrames[streamId] )
            {
                QC_ERROR( "no buffers for stream: %u", streamId );
                ret = QC_STATUS_BAD_STATE;
            }
        }
        else
        {
            QC_ERROR( "can't find buffers for stream: %u", streamId );
            ret = QC_STATUS_BAD_ARGUMENTS;
        }

        if ( QC_STATUS_OK == ret )
        {
            if ( numBuffers != m_qcarcamBuffers[streamId].nBuffers )
            {
                QC_ERROR( "numBuffers is not equal to %" PRIu32,
                          m_qcarcamBuffers[streamId].nBuffers );
                ret = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                for ( uint32_t idx = 0; idx < numBuffers; idx++ )
                {
                    pBuffers[idx] = m_pCameraFrames[streamId][idx].sharedBuffer;
                }
            }
        }
    }

    return ret;
}

QCStatus_e Camera::QueryInputs()
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

    if ( QCARCAM_RET_OK != status )
    {
        QC_LOG_ERROR( "Failed QCarCamQueryInputs number of inputs %d", status );
        ret = QC_STATUS_FAIL;
    }
    else if ( 0 == inputCount )
    {
        QC_LOG_ERROR( "Didn't detect any camera connection" );
    }
    else
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
                            QC_LOG_ERROR( "Query Input Modes failed for input %u: ret = %d",
                                          s_cameraInputsInfo.pCameraInputs[i].inputId, status );
                            ret = QC_STATUS_FAIL;
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
                        QC_LOG_ERROR( "Failed to allocate memory for input %u modes",
                                      s_cameraInputsInfo.pCameraInputs[i].inputId );
                        ret = QC_STATUS_NOMEM;
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

QCStatus_e Camera::GetInputsInfo( CameraInputs_t *pCamInputs )
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

}   // namespace component
}   // namespace QC
