// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/component/OpticalFlow.hpp"

namespace QC
{
namespace component
{

#define QC_EVA_OF_NUM_ICONFIG 11
#define QC_EVA_OF_NUM_FCONFIG 14

#define ALIGN_S( size, align ) ( ( ( ( size ) + (align) -1 ) / ( align ) ) * ( align ) )

static const char *evaOFIConfigStrings[QC_EVA_OF_NUM_ICONFIG] = {
        EVA_OF_ICONFIG_ACTUAL_FPS,
        EVA_OF_ICONFIG_OPERATIONAL_FPS,
        EVA_OF_ICONFIG_REF_IMAGE_INFO,
        EVA_OF_ICONFIG_CUR_IMAGE_INFO,
        EVA_OF_ICONFIG_AM_FILTER_CONFIG,
        EVA_OF_ICONFIG_CONF_OUTPUT_ENABLE,
        EVA_OF_ICONFIG_SLIC_OUTPUT_ENABLE,
        EVA_OF_ICONFIG_MV_PACK_FORMAT,
        EVA_OF_ICONFIG_OF_DIRECTION,
        EVA_OF_ICONFIG_QUALITY_LEVEL,
        EVA_OF_ICONFIG_PROCESS_FILTER_OPERATION_MODE,
};

static const char *evaOFFrameConfigStrings[QC_EVA_OF_NUM_FCONFIG] = {
        EVA_OF_FCONFIG_AM_FILTER_ENABLE,  EVA_OF_FCONFIG_HOLE_FILL_ENABLE,
        EVA_OF_FCONFIG_HOLE_FILL_CONFIG,  EVA_OF_FCONFIG_P1_CONFIG,
        EVA_OF_FCONFIG_P2_CONFIG,         EVA_OF_FCONFIG_CENSUS_CONFIG,
        EVA_OF_FCONFIG_CONF_WEIGHT,       EVA_OF_FCONFIG_DS_THRESH,
        EVA_OF_FCONFIG_FLATNESS_THRESH,   EVA_OF_FCONFIG_FLATNESS_OVERRIDE,
        EVA_OF_FCONFIG_MV_VAR_THRESH,     EVA_OF_FCONFIG_MV_CAND_PRUN,
        EVA_OF_FCONFIG_DS4_MAX_OVERWRITE, EVA_OF_FCONFIG_TIME_OF_DAY,
};

OpticalFlow_Config::OpticalFlow_Config()
{
    this->format = QC_IMAGE_FORMAT_NV12;
    this->width = 0;
    this->height = 0;
    this->frameRate = 30;
    this->amFilter.nConfThresh = 150;
    this->amFilter.nStepSize = 1;
    this->amFilter.nUpScale = 0;
    this->amFilter.nOutputIntOnly = 0;
    this->amFilter.nOutputFormat = 1;
    this->mvPacketFormat = EVA_OF_MV_PACK_LSB;
    this->direction = EVA_OF_FORWARD_DIRECTION;
    this->qualityLevel = EVA_OF_QUALITY_NORMAL;
    this->filterOperationMode = EVA_OF_MODE_CPU;
    this->bAmFilterEnable = true;
    this->bHolefillEnable = true;
    this->holeFillConfig.nHighConfThresh = 210;
    this->holeFillConfig.nFillConfThresh = 150;
    this->holeFillConfig.nMinValidPixels = 1;
    this->holeFillConfig.nZeroOverride = 0;
    this->p1Config.nAdapEnable = 1;
    this->p1Config.nDefaultValue = 11;
    this->p1Config.nEdgeValue = 10;
    this->p1Config.nSmoothValue = 22;
    this->p1Config.nEdgeThresh = 8;
    this->p1Config.nSmoothThresh = 2;
    this->p2Config.nAdapEnable = 1;
    this->p2Config.nDefaultValue = 33;
    this->p2Config.nEdgeValue = 22;
    this->p2Config.nSmoothValue = 63;
    this->censusConfig.nCurNoiseThreshOffset = 0;
    this->censusConfig.nCurNoiseThreshScale = 0;
    this->censusConfig.nRefNoiseThreshOffset = 0;
    this->censusConfig.nRefNoiseThreshScale = 0;
    this->confWeight.nOcclusion = 14;
    this->confWeight.nMvVariance = 10;
    this->confWeight.nDsMvVariance = 2;
    this->confWeight.nFlatness = 6;
    this->nDsThresh = 1;
    this->nFlatnessThresh = 1;
    this->bFlatnessOverride = false;
    this->nMvVarThresh = 1;
    this->mvCandPrun = EVA_OF_SP_DS4_DS8_00;
    this->bDs4MaxOverwrite = false;
    this->timeOption = EVA_OF_TIME_DAY;
}

OpticalFlow_Config::OpticalFlow_Config( const OpticalFlow_Config &rhs )
{
    (void) memcpy( this, &rhs, sizeof( rhs ) );
}

OpticalFlow_Config &OpticalFlow_Config::operator=( const OpticalFlow_Config &rhs )
{
    (void) memcpy( this, &rhs, sizeof( rhs ) );
    return *this;
}

OpticalFlow::OpticalFlow() {}

OpticalFlow::~OpticalFlow() {}


void OpticalFlow::EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent,
                                             void *pSessionUserData )
{
    OpticalFlow *self = (OpticalFlow *) pSessionUserData;
    if ( nullptr != self )
    {
        self->EvaSessionCallbackHandler( hSession, eEvent );
    }
}

void OpticalFlow::EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent )
{
    QC_DEBUG( "Received Session callback for session: %p, eEvent: %d", hSession, eEvent );
    if ( EVA_EVFATAL == eEvent )
    {
        QC_ERROR( "set state to ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }
}

QCStatus_e OpticalFlow::UpdateIconfigOF( EvaConfigList_t *pConfigList )
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc = EvaOFQueryConfigIndices( &evaOFIConfigStrings[0], pConfigList );
    if ( rc != EVA_SUCCESS )
    {
        QC_ERROR( "Failed to Query I config Indices: %d", rc );
        ret = QC_STATUS_FAIL;
    }
    else
    {
        uint32_t ind = 0;

        // ICONFIG_ACTUAL_FPS
        pConfigList->pConfigs[ind].uValue.u32 = m_config.frameRate;
        ind++;

        // ICONFIG_OPERATIONAL_FPS
        pConfigList->pConfigs[ind].uValue.u32 = m_config.frameRate;
        ind++;

        // ICONFIG_REF_IMAGE_INFO
        pConfigList->pConfigs[ind].uValue.ptr = &m_imageInfo;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaImageInfo_t );
        ind++;

        // ICONFIG_CUR_IMAGE_INFO
        pConfigList->pConfigs[ind].uValue.ptr = &m_imageInfo;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaImageInfo_t );
        ind++;

        // ICONFIG_AM_FILTER_CONFIG
        pConfigList->pConfigs[ind].uValue.ptr = &m_config.amFilter;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaOFAmFilterConfig_t );
        ind++;

        // ICONFIG_CONF_OUTPUT_ENABLE
        pConfigList->pConfigs[ind].uValue.b = true;
        ind++;

        // ICONFIG_SLIC_OUTPUT_ENABLE
        pConfigList->pConfigs[ind].uValue.b = false;
        ind++;

        // ICONFIG_MV_PACK_FORMAT
        pConfigList->pConfigs[ind].uValue.ptr = &m_config.mvPacketFormat;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaOFMvPackFormat_e );
        ind++;

        // ICONFIG_OF_DIRECTION
        pConfigList->pConfigs[ind].uValue.ptr = &m_config.direction;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaOFDirection_e );
        ind++;

        // ICONFIG_QUALITY_LEVEL
        pConfigList->pConfigs[ind].uValue.u32 = (uint32_t) m_config.qualityLevel;
        ind++;

        // ICONFIG_FILTER_OPERATION_MODE
        pConfigList->pConfigs[ind].uValue.ptr = &m_config.filterOperationMode;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaOFFilterOperationMode_e );
        ind++;
    }

    return ret;
}

QCStatus_e OpticalFlow::ValidateConfig( const OpticalFlow_Config_t *pConfig )
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( nullptr == pConfig )
    {
        QC_ERROR( "pConfig is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( pConfig->format != QC_IMAGE_FORMAT_UYVY ) &&
              ( pConfig->format != QC_IMAGE_FORMAT_NV12 ) &&
              ( pConfig->format != QC_IMAGE_FORMAT_P010 ) )
    {
        QC_ERROR( "invalid format!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( EVA_OF_FORWARD_DIRECTION != pConfig->direction ) &&
              ( EVA_OF_BACKWARD_DIRECTION != pConfig->direction ) )
    {
        QC_ERROR( "invalid direction!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }
    return ret;
}

EvaColorFormat_e OpticalFlow::GetEvaColorFormat( QCImageFormat_e colorFormat )
{
    EvaColorFormat_e evaFormat = EVA_COLORFORMAT_MAX;

    switch ( colorFormat )
    {
        case QC_IMAGE_FORMAT_UYVY:
        {
            evaFormat = EVA_COLORFORMAT_UYVY;
            break;
        }
        case QC_IMAGE_FORMAT_NV12:
        {
            evaFormat = EVA_COLORFORMAT_NV12;
            break;
        }
        case QC_IMAGE_FORMAT_P010:
        {
            evaFormat = EVA_COLORFORMAT_P010_MSB;
            break;
        }
        default:
        {
            QC_ERROR( "Unsupport corlor sormat: %d", colorFormat );
            break;
        }
    }

    return evaFormat;
}

QCStatus_e OpticalFlow::Init( const char *pName, const OpticalFlow_Config_t *pConfig,
                              Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;
    bool bIFInitOK = false;
    EvaStatus_e rc;

    EvaConfigList_t configList;
    EvaConfig_t configs[QC_EVA_OF_NUM_ICONFIG];

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK == ret )
    {
        bIFInitOK = true;
        ret = ValidateConfig( pConfig );
        if ( QC_STATUS_OK == ret )
        { /* parameters are OK */
            m_config = *pConfig;
            m_outputWidth = ( m_config.width >> m_config.amFilter.nStepSize )
                            << m_config.amFilter.nUpScale;
            m_outputHeight = ( m_config.height >> m_config.amFilter.nStepSize )
                             << m_config.amFilter.nUpScale;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_hOFSession = EvaCreateSession( EvaSessionCallbackHandler, this, NULL );
        if ( nullptr == m_hOFSession )
        {
            QC_ERROR( "EVAOpticalFlow : Failed to Create Session" );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        EvaColorFormat_e evaFormat = GetEvaColorFormat( m_config.format );
        rc = EvaQueryImageInfo( evaFormat, m_config.width, m_config.height, &m_imageInfo );
        if ( rc != EVA_SUCCESS )
        {
            QC_ERROR( "EVAOpticalFlow: Failed to get image info: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "ImageInfo: WxH=%ux%u format=%d planes=%u total size=%u, width stride=[%u "
                      "%u %u %u], aligned size=[%u %u %u %u]",
                      m_imageInfo.nWidth, m_imageInfo.nHeight, m_imageInfo.eFormat,
                      m_imageInfo.nPlanes, m_imageInfo.nTotalSize, m_imageInfo.nWidthStride[0],
                      m_imageInfo.nWidthStride[1], m_imageInfo.nWidthStride[2],
                      m_imageInfo.nWidthStride[3], m_imageInfo.nAlignedSize[0],
                      m_imageInfo.nAlignedSize[1], m_imageInfo.nAlignedSize[2],
                      m_imageInfo.nAlignedSize[3] );
        }
    }


    if ( QC_STATUS_OK == ret )
    {
        configList.nConfigs = QC_EVA_OF_NUM_ICONFIG;
        configList.pConfigs = configs;
        ret = UpdateIconfigOF( &configList );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_hEvaOF = EvaInitOF( m_hOFSession, &configList, &m_outBufInfo, NULL, this );
        if ( nullptr == m_hEvaOF )
        {
            QC_ERROR( "Failed to Init OF" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "output mv buffer size %u, conf map buf size %u", m_outBufInfo.nMvFwdMapBytes,
                      m_outBufInfo.nConfMapBytes );
        }
    }

    if ( ret != QC_STATUS_OK )
    { /* do error clean up */
        QC_ERROR( "OpticalFlow Init failed: %d!", ret );

        if ( nullptr != m_hOFSession )
        {
            (void) EvaDeleteSession( m_hOFSession );
            m_hOFSession = nullptr;
        }

        if ( bIFInitOK )
        {
            (void) ComponentIF::Deinit();
        }
    }
    else
    {
        m_state = QC_OBJECT_STATE_READY;
    }

    return ret;
}

QCStatus_e OpticalFlow::RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "Register Buffers is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "pBuffers is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == numBuffers )
    {
        QC_ERROR( "numBuffers is 0!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            EvaMem_t *pEvaMem;
            ret = RegisterEvaMem( &pBuffers[i], &pEvaMem );
            if ( QC_STATUS_OK != ret )
            {
                break;
            }
        }
    }
    return ret;
}

QCStatus_e OpticalFlow::SetInitialFrameConfig()
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;


    EvaConfigList_t configList;
    EvaConfig_t configs[QC_EVA_OF_NUM_FCONFIG];
    (void) memset( configs, 0, sizeof( EvaConfig_t ) * QC_EVA_OF_NUM_FCONFIG );

    configList.nConfigs = QC_EVA_OF_NUM_FCONFIG;
    configList.pConfigs = configs;

    rc = EvaOFQueryConfigIndices( &evaOFFrameConfigStrings[0], &configList );
    if ( EVA_SUCCESS == rc )
    {
        uint32_t ind = 0;
        // FCONFIG_AM_FILTER_ENABLE
        configList.pConfigs[ind].uValue.b = m_config.bAmFilterEnable;
        ind++;

        // FCONFIG_HOLE_FILL_ENABLE
        configList.pConfigs[ind].uValue.u8 = (uint8_t) m_config.bHolefillEnable;
        ind++;

        // FCONFIG_HOLE_FILL_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.holeFillConfig;
        configList.pConfigs[ind].nStructSize = sizeof( EvaOFHoleFillConfig_t );
        ind++;

        // FCONFIG_P1_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.p1Config;
        configList.pConfigs[ind].nStructSize = sizeof( EvaOFP1Config_t );
        ind++;

        // FCONFIG_P2_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.p2Config;
        configList.pConfigs[ind].nStructSize = sizeof( EvaOFP2Config_t );
        ind++;

        // FCONFIG_CENSUS_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.censusConfig;
        configList.pConfigs[ind].nStructSize = sizeof( EvaOFCensusConfig_t );
        ind++;

        // FCONFIG_CONF_WEIGHT
        configList.pConfigs[ind].uValue.ptr = &m_config.confWeight;
        configList.pConfigs[ind].nStructSize = sizeof( EvaOFConfWeight_t );
        ind++;

        // FCONFIG_DS_THRESH
        configList.pConfigs[ind].uValue.u8 = m_config.nDsThresh;
        ind++;

        // FCONFIG_FLATNESS_THRESH
        configList.pConfigs[ind].uValue.u8 = m_config.nFlatnessThresh;
        ind++;

        // FCONFIG_FLATNESS_OVERRIDE
        configList.pConfigs[ind].uValue.b = m_config.bFlatnessOverride;
        ind++;

        // FCONFIG_MV_VAR_THRESH
        configList.pConfigs[ind].uValue.u8 = m_config.nMvVarThresh;
        ind++;

        // FCONFIG_MV_CAND_PRUN
        configList.pConfigs[ind].uValue.ptr = &m_config.mvCandPrun;
        configList.pConfigs[ind].nStructSize = sizeof( EvaOFMvCandPrun_e );
        ind++;

        // FCONFIG_DS4_MAX_OVERWRITE
        configList.pConfigs[ind].uValue.b = m_config.bDs4MaxOverwrite;
        ind++;

        // FCONFIG_TIME_OF_DAY
        configList.pConfigs[ind].uValue.ptr = &m_config.timeOption;
        configList.pConfigs[ind].nStructSize = sizeof( EvaOFTimeOption_e );
        ind++;

        rc = EvaOFSetFrameConfig( m_hEvaOF, &configList );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "Failed to Set F config: %d", rc );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        QC_ERROR( "Failed to Query F config Indices: %d", rc );
        ret = QC_STATUS_FAIL;
    }


    return ret;
}


QCStatus_e OpticalFlow::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Start is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        rc = EvaStartSession( m_hOFSession );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "Failed to Start Session: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            ret = SetInitialFrameConfig();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to set Session config" );
            }
            else
            {
                m_state = QC_OBJECT_STATE_RUNNING;
            }
        }
    }

    return ret;
}

QCStatus_e OpticalFlow::RegisterEvaMem( const QCSharedBuffer_t *pBuffer, EvaMem_t **pEvaMem )
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;
    EvaMem_t mem;

    auto it = m_evaMemMap.find( pBuffer->data() );
    if ( it == m_evaMemMap.end() )
    {
        mem.nSize = pBuffer->size;
        mem.nOffset = pBuffer->offset;
        mem.pAddress = pBuffer->data();
        mem.hMemHandle = (void *) pBuffer->buffer.dmaHandle;
        if ( 0 != ( pBuffer->buffer.flags & QC_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            mem.bCached = true;
        }
        rc = EvaMemRegister( m_hOFSession, &mem );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "EvaMemRegister(%p) failed: %d", pBuffer->data(), rc );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            m_evaMemMap[pBuffer->data()] = mem;
            *pEvaMem = &m_evaMemMap[pBuffer->data()];
        }
    }
    else
    {
        *pEvaMem = &it->second;
    }
    return ret;
}

QCStatus_e OpticalFlow::ValidateImageBuffer( const QCSharedBuffer_t *pImage )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pImage )
    {
        QC_ERROR( "pImage is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_IMAGE != pImage->type ) || ( nullptr == pImage->buffer.pData ) ||
              ( m_config.format != pImage->imgProps.format ) ||
              ( m_config.width != pImage->imgProps.width ) ||
              ( m_config.height != pImage->imgProps.height ) ||
              ( m_imageInfo.nPlanes != pImage->imgProps.numPlanes ) )
    {
        QC_ERROR( "pImage is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        for ( uint32_t i = 0; i < m_imageInfo.nPlanes; i++ )
        {
            if ( m_imageInfo.nWidthStride[i] != pImage->imgProps.stride[i] )
            {
                QC_ERROR( "pImage with invalid stride in plane %u: %u != %u", i,
                          m_imageInfo.nWidthStride[i], pImage->imgProps.stride[i] );
                ret = QC_STATUS_INVALID_BUF;
            }
            else if ( m_imageInfo.nAlignedSize[i] != pImage->imgProps.planeBufSize[i] )
            {
                QC_ERROR( "pImage with invalid plane buf size in plane %u: %u != %u", i,
                          m_imageInfo.nAlignedSize[i], pImage->imgProps.planeBufSize[i] );
                ret = QC_STATUS_INVALID_BUF;
            }
            else
            {
                /* OK */
            }
        }
    }
    return ret;
}


QCStatus_e OpticalFlow::Execute( const QCSharedBuffer_t *pRefImage,
                                 const QCSharedBuffer_t *pCurImage,
                                 const QCSharedBuffer_t *pOutMvBuf,
                                 const QCSharedBuffer_t *pConfBuf )
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;
    EvaMem_t *pRefMem = nullptr;
    EvaMem_t *pCurMem = nullptr;
    EvaMem_t *pOutMvMem = nullptr;
    EvaMem_t *pConfMem = nullptr;
    EvaImage_t refImage;
    EvaImage_t curImage;
    EvaOFOutput_t ofOutput;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Execute is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pOutMvBuf )
    {
        QC_ERROR( "pOutMvBuf is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pOutMvBuf->type ) ||
              ( nullptr == pOutMvBuf->buffer.pData ) || ( 4 != pOutMvBuf->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_UINT_16 != pOutMvBuf->tensorProps.type ) ||
              ( 1 != pOutMvBuf->tensorProps.dims[0] ) ||
              ( ALIGN_S( m_outputHeight, 8 ) != pOutMvBuf->tensorProps.dims[1] ) ||
              ( ALIGN_S( m_outputWidth * 2, 128 ) != pOutMvBuf->tensorProps.dims[2] ) ||
              ( 1 != pOutMvBuf->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pOutMvBuf is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pConfBuf )
    {
        QC_ERROR( "pConfBuf is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pConfBuf->type ) ||
              ( nullptr == pConfBuf->buffer.pData ) || ( 4 != pConfBuf->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_UINT_8 != pConfBuf->tensorProps.type ) ||
              ( 1 != pConfBuf->tensorProps.dims[0] ) ||
              ( ALIGN_S( m_outputHeight, 8 ) != pConfBuf->tensorProps.dims[1] ) ||
              ( ALIGN_S( m_outputWidth, 128 ) != pConfBuf->tensorProps.dims[2] ) ||
              ( 1 != pConfBuf->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pConfBuf is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = ValidateImageBuffer( pRefImage );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = ValidateImageBuffer( pCurImage );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pRefImage, &pRefMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pCurImage, &pCurMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pOutMvBuf, &pOutMvMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pConfBuf, &pConfMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        refImage.pBuffer = pRefMem;
        refImage.sImageInfo = m_imageInfo;
        curImage.pBuffer = pCurMem;
        curImage.sImageInfo = m_imageInfo;

        if ( EVA_OF_FORWARD_DIRECTION == m_config.direction )
        {
            ofOutput.pOutMvFwdBuf = pOutMvMem;
            ofOutput.nOutMvFwdSize = pOutMvMem->nSize;
        }
        else
        {
            ofOutput.pOutMvBwdBuf = pOutMvMem;
            ofOutput.nOutMvBwdSize = pOutMvMem->nSize;
        }
        ofOutput.pConfBuf = pConfMem;
        ofOutput.nConfSize = pConfMem->nSize;
        rc = EvaOF_Sync( m_hEvaOF, &refImage, &curImage, EVA_OF_CONTINUOUS, &ofOutput, nullptr );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "EvaOF_Sync failed: %d", rc );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e OpticalFlow::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Stop is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        rc = EvaStopSession( m_hOFSession );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "Failed to stop Session: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            m_state = QC_OBJECT_STATE_READY;
        }
    }

    return ret;
}

QCStatus_e OpticalFlow::DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "DeRegisterBuffers is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( 0 == numBuffers )
    {
        QC_ERROR( "numBuffers is 0!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            auto it = m_evaMemMap.find( pBuffers[i].data() );
            if ( it != m_evaMemMap.end() )
            {
                auto &mem = it->second;
                rc = EvaMemDeregister( m_hOFSession, &mem );
                if ( EVA_SUCCESS != rc )
                {
                    QC_ERROR( "EvaMemDeregister(%p) failed: %d", mem.pAddress, rc );
                    ret = QC_STATUS_FAIL;
                    break;
                }
                else
                {
                    (void) m_evaMemMap.erase( it );
                }
            }
        }
    }

    return ret;
}

QCStatus_e OpticalFlow::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Deinit is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        for ( auto &it : m_evaMemMap )
        {
            auto &mem = it.second;
            rc = EvaMemDeregister( m_hOFSession, &mem );
            if ( EVA_SUCCESS != rc )
            {
                QC_ERROR( "EvaMemDeregister(%p) failed: %d", mem.pAddress, rc );
                ret = QC_STATUS_FAIL;
            }
        }
        m_evaMemMap.clear();

        rc = EvaDeInitOF( m_hEvaOF );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "Failed to deinit OF: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        m_hEvaOF = nullptr;

        rc = EvaDeleteSession( m_hOFSession );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "Failed to delete Session: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        m_hOFSession = nullptr;

        QCStatus_e ret2 = ComponentIF::Deinit();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "Deinit ComponentIF failed!" );
            ret = ret2;
        }
    }

    return ret;
}

}   // namespace component
}   // namespace QC
