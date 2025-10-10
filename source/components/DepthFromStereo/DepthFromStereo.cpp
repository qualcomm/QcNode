// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/component/DepthFromStereo.hpp"

namespace QC
{
namespace component
{

#define QC_EVA_DFS_NUM_ICONFIG 6
#define QC_EVA_DFS_NUM_FCONFIG 15

#define ALIGN_S( size, align ) ( ( ( ( size ) + (align) -1 ) / ( align ) ) * ( align ) )

static const char *evaDFSIConfigStrings[QC_EVA_DFS_NUM_ICONFIG] = {
        EVA_DFS_ICONFIG_ACTUAL_FPS,         EVA_DFS_ICONFIG_OPERATIONAL_FPS,
        EVA_DFS_ICONFIG_PRIMARY_IMAGE_INFO, EVA_DFS_ICONFIG_AUXILIARY_IMAGE_INFO,
        EVA_DFS_ICONFIG_CONF_OUTPUT_ENABLE, EVA_DFS_ICONFIG_SLIC_OUTPUT_ENABLE,
};

static const char *evaDFSFrameConfigStrings[QC_EVA_DFS_NUM_FCONFIG] = {
        EVA_DFS_FCONFIG_HOLE_FILL_ENABLE,
        EVA_DFS_FCONFIG_HOLE_FILL_CONFIG,
        EVA_DFS_FCONFIG_AM_FILTER_ENABLE,
        EVA_DFS_FCONFIG_AM_FILTER_CONF_THRESH,
        EVA_DFS_FCONFIG_P1_CONFIG,
        EVA_DFS_FCONFIG_P2_CONFIG,
        EVA_DFS_FCONFIG_CENSUS_CONFIG,
        EVA_DFS_FCONFIG_SEARCH_DIR,
        EVA_DFS_FCONFIG_OCCLUSION_CONF,
        EVA_DFS_FCONFIG_MV_VAR_CONF,
        EVA_DFS_FCONFIG_MV_VAR_THRESH,
        EVA_DFS_FCONFIG_FLATNESS_CONF,
        EVA_DFS_FCONFIG_FLATNESS_THRESH,
        EVA_DFS_FCONFIG_FLATNESS_OVERRIDE,
        EVA_DFS_FCONFIG_RECTIFICATION_ERROR_TOLERANCE,
};

DepthFromStereo_Config::DepthFromStereo_Config()
{
    this->format = QC_IMAGE_FORMAT_NV12;
    this->width = 0;
    this->height = 0;
    this->frameRate = 30;
    this->bHolefillEnable = true;
    this->holeFillConfig.nHighConfThresh = 210;
    this->holeFillConfig.nFillConfThresh = 150;
    this->holeFillConfig.nMinValidPixels = 1;
    this->holeFillConfig.nZeroOverride = 0;

    this->bAmFilterEnable = true;
    this->amFilterConfThreshold = 200;

    this->p1Config.nAdapEnable = 1;
    this->p1Config.nDefaultValue = 11;
    this->p1Config.nEdgeValue = 10;
    this->p1Config.nSmoothValue = 22;
    this->p1Config.nEdgeThresh = 16;
    this->p1Config.nSmoothThresh = 4;

    this->p2Config.nAdapEnable = 1;
    this->p2Config.nDefaultValue = 33;
    this->p2Config.nEdgeValue = 22;
    this->p2Config.nSmoothValue = 63;

    this->censusConfig.nAuxNoiseThreshOffset = 0;
    this->censusConfig.nAuxNoiseThreshScale = 0;
    this->censusConfig.nPriNoiseThreshOffset = 0;
    this->censusConfig.nPriNoiseThreshScale = 0;

    this->dfsSearchDir = EVA_DFS_SEARCH_L2R;

    this->occlusionConf = 16;
    this->mvVarConf = 10;
    this->mvVarThresh = 1;
    this->flatnessConf = 6;
    this->flatnessThresh = 3;
    this->bFlatnessOverride = false;
    this->rectificationErrorTolerance = 0.29;
}

DepthFromStereo_Config::DepthFromStereo_Config( const DepthFromStereo_Config &rhs )
{
    (void) memcpy( this, &rhs, sizeof( rhs ) );
}

DepthFromStereo_Config &DepthFromStereo_Config::operator=( const DepthFromStereo_Config &rhs )
{
    (void) memcpy( this, &rhs, sizeof( rhs ) );
    return *this;
}

DepthFromStereo::DepthFromStereo() {}

DepthFromStereo::~DepthFromStereo() {}


void DepthFromStereo::EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent,
                                                 void *pSessionUserData )
{
    DepthFromStereo *self = (DepthFromStereo *) pSessionUserData;
    if ( nullptr != self )
    {
        self->EvaSessionCallbackHandler( hSession, eEvent );
    }
}

void DepthFromStereo::EvaSessionCallbackHandler( const EvaSession_t hSession, EvaEvent_e eEvent )
{
    QC_DEBUG( "Received Session callback for session: %p, eEvent: %d", hSession, eEvent );
    if ( EVA_EVFATAL == eEvent )
    {
        QC_ERROR( "set state to ERROR" );
        m_state = QC_OBJECT_STATE_ERROR;
    }
}

QCStatus_e DepthFromStereo::UpdateIconfig( EvaConfigList_t *pConfigList )
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc = EvaDFSQueryConfigIndices( &evaDFSIConfigStrings[0], pConfigList );
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

        // CONFIG_PRIMARY_IMAGE_INFO
        pConfigList->pConfigs[ind].uValue.ptr = &m_imageInfo;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaImageInfo_t );
        ind++;

        // CONFIG_AUXILIARY_IMAGE_INFO
        pConfigList->pConfigs[ind].uValue.ptr = &m_imageInfo;
        pConfigList->pConfigs[ind].nStructSize = sizeof( EvaImageInfo_t );
        ind++;

        // ICONFIG_CONF_OUTPUT_ENABLE
        pConfigList->pConfigs[ind].uValue.b = true;
        ind++;

        // ICONFIG_SLIC_OUTPUT_ENABLE
        pConfigList->pConfigs[ind].uValue.b = false;
        ind++;
    }

    return ret;
}

QCStatus_e DepthFromStereo::ValidateConfig( const DepthFromStereo_Config_t *pConfig )
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( nullptr == pConfig )
    {
        QC_ERROR( "pConfig is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( pConfig->format != QC_IMAGE_FORMAT_NV12 ) &&
              ( pConfig->format != QC_IMAGE_FORMAT_P010 ) &&
              ( pConfig->format != QC_IMAGE_FORMAT_NV12_UBWC ) &&
              ( pConfig->format != QC_IMAGE_FORMAT_TP10_UBWC ) )
    {
        QC_ERROR( "invalid format!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( EVA_DFS_SEARCH_L2R != pConfig->dfsSearchDir ) &&
              ( EVA_DFS_SEARCH_R2L != pConfig->dfsSearchDir ) )
    {
        QC_ERROR( "invalid serach direction!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }
    return ret;
}

EvaColorFormat_e DepthFromStereo::GetEvaColorFormat( QCImageFormat_e colorFormat )
{
    EvaColorFormat_e evaFormat = EVA_COLORFORMAT_MAX;

    switch ( colorFormat )
    {
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
        case QC_IMAGE_FORMAT_NV12_UBWC:
        {
            evaFormat = EVA_COLORFORMAT_NV12_UBWC;
            break;
        }
        case QC_IMAGE_FORMAT_TP10_UBWC:
        {
            evaFormat = EVA_COLORFORMAT_TP10_UBWC;
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

QCStatus_e DepthFromStereo::Init( const char *pName, const DepthFromStereo_Config_t *pConfig,
                                  Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;
    bool bIFInitOK = false;
    EvaStatus_e rc;

    EvaConfigList_t configList;
    EvaConfig_t configs[QC_EVA_DFS_NUM_ICONFIG];

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK == ret )
    {
        bIFInitOK = true;
        ret = ValidateConfig( pConfig );
        if ( QC_STATUS_OK == ret )
        { /* parameters are OK */
            m_config = *pConfig;
            m_outputWidth = m_config.width;
            m_outputHeight = m_config.height;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        m_hDFSSession = EvaCreateSession( EvaSessionCallbackHandler, this, NULL );
        if ( nullptr == m_hDFSSession )
        {
            QC_ERROR( "EVADepthFromStereo : Failed to Create Session" );
            ret = QC_STATUS_FAIL;
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        EvaColorFormat_e evaFormat = GetEvaColorFormat( m_config.format );
        rc = EvaQueryImageInfo( evaFormat, m_config.width, m_config.height, &m_imageInfo );
        if ( rc != EVA_SUCCESS )
        {
            QC_ERROR( "EVADepthFromStereo: Failed to get image info: %d", rc );
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
        configList.nConfigs = QC_EVA_DFS_NUM_ICONFIG;
        configList.pConfigs = configs;
        ret = UpdateIconfig( &configList );
    }

    if ( QC_STATUS_OK == ret )
    {
        m_hEvaDFS = EvaInitDFS( m_hDFSSession, &configList, &m_outBufInfo, NULL, this );
        if ( nullptr == m_hEvaDFS )
        {
            QC_ERROR( "Failed to Init DFS" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "output disparity buffer size %u, conf map buf size %u",
                      m_outBufInfo.nDispMapBytes, m_outBufInfo.nConfMapBytes );
        }
    }

    if ( ret != QC_STATUS_OK )
    { /* do error clean up */
        QC_ERROR( "DepthFromStereo Init failed: %d!", ret );

        if ( nullptr != m_hDFSSession )
        {
            (void) EvaDeleteSession( m_hDFSSession );
            m_hDFSSession = nullptr;
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

QCStatus_e DepthFromStereo::RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
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

QCStatus_e DepthFromStereo::SetInitialFrameConfig()
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;

    EvaConfigList_t configList;
    EvaConfig_t configs[QC_EVA_DFS_NUM_FCONFIG];
    (void) memset( configs, 0, sizeof( EvaConfig_t ) * QC_EVA_DFS_NUM_FCONFIG );

    configList.nConfigs = QC_EVA_DFS_NUM_FCONFIG;
    configList.pConfigs = configs;

    rc = EvaDFSQueryConfigIndices( &evaDFSFrameConfigStrings[0], &configList );
    if ( EVA_SUCCESS == rc )
    {
        uint32_t ind = 0;

        // FCONFIG_HOLE_FILL_ENABLE
        configList.pConfigs[ind].uValue.u8 = (uint8_t) m_config.bHolefillEnable;
        ind++;

        // FCONFIG_HOLE_FILL_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.holeFillConfig;
        configList.pConfigs[ind].nStructSize = sizeof( EvaDFSHoleFillConfig_t );
        ind++;

        // FCONFIG_AM_FILTER_ENABLE
        configList.pConfigs[ind].uValue.b = m_config.bAmFilterEnable;
        ind++;

        // FCONFIG_AM_FILTER_CONF_THRESH
        configList.pConfigs[ind].uValue.u8 = m_config.amFilterConfThreshold;
        ind++;

        // FCONFIG_P1_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.p1Config;
        configList.pConfigs[ind].nStructSize = sizeof( EvaDFSP1Config_t );
        ind++;

        // FCONFIG_P2_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.p2Config;
        configList.pConfigs[ind].nStructSize = sizeof( EvaDFSP2Config_t );
        ind++;

        // FCONFIG_CENSUS_CONFIG
        configList.pConfigs[ind].uValue.ptr = &m_config.censusConfig;
        configList.pConfigs[ind].nStructSize = sizeof( EvaDFSCensusConfig_t );
        ind++;

        // FCONFIG_SEARCH_DIR
        configList.pConfigs[ind].uValue.ptr = &m_config.dfsSearchDir;
        configList.pConfigs[ind].nStructSize = sizeof( EvaDFSSearchDir_e );
        ind++;

        // FCONFIG_OCCLUSION_CONF
        configList.pConfigs[ind].uValue.u8 = m_config.occlusionConf;
        ind++;

        // FCONFIG_MV_VAR_CONF
        configList.pConfigs[ind].uValue.u8 = m_config.mvVarConf;
        ind++;

        // FCONFIG_MV_VAR_THRESH
        configList.pConfigs[ind].uValue.u8 = m_config.mvVarThresh;
        ind++;

        // FCONFIG_FLATNESS_CONF
        configList.pConfigs[ind].uValue.u8 = m_config.flatnessConf;
        ind++;

        // FCONFIG_FLATNESS_THRESH
        configList.pConfigs[ind].uValue.u8 = m_config.flatnessThresh;
        ind++;

        // FCONFIG_FLATNESS_OVERRIDE
        configList.pConfigs[ind].uValue.b = m_config.bFlatnessOverride;
        ind++;

        // FCONFIG_RECTIFICATION_ERROR_TOLERANCE
        configList.pConfigs[ind].uValue.f32 = m_config.rectificationErrorTolerance;
        ind++;

        rc = EvaDFSSetFrameConfig( m_hEvaDFS, &configList );
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


QCStatus_e DepthFromStereo::Start()
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
        rc = EvaStartSession( m_hDFSSession );
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

QCStatus_e DepthFromStereo::RegisterEvaMem( const QCSharedBuffer_t *pBuffer, EvaMem_t **pEvaMem )
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
        rc = EvaMemRegister( m_hDFSSession, &mem );
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

QCStatus_e DepthFromStereo::ValidateImageBuffer( const QCSharedBuffer_t *pImage )
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


QCStatus_e DepthFromStereo::Execute( const QCSharedBuffer_t *pPriImage,
                                     const QCSharedBuffer_t *pAuxImage,
                                     const QCSharedBuffer_t *pDispMap,
                                     const QCSharedBuffer_t *pConfMap )
{
    QCStatus_e ret = QC_STATUS_OK;
    EvaStatus_e rc;
    EvaMem_t *pPriMem = nullptr;
    EvaMem_t *pAuxMem = nullptr;
    EvaMem_t *pDispMem = nullptr;
    EvaMem_t *pConfMem = nullptr;
    EvaImage_t priImage;
    EvaImage_t auxImage;
    EvaDFSOutput_t dfsOutput;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        QC_ERROR( "Execute is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pDispMap )
    {
        QC_ERROR( "pDispMap is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pDispMap->type ) ||
              ( nullptr == pDispMap->buffer.pData ) || ( 4 != pDispMap->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_UINT_16 != pDispMap->tensorProps.type ) ||
              ( 1 != pDispMap->tensorProps.dims[0] ) ||
              ( ALIGN_S( m_outputHeight, 2 ) != pDispMap->tensorProps.dims[1] ) ||
              ( ALIGN_S( m_outputWidth, 128 ) != pDispMap->tensorProps.dims[2] ) ||
              ( 1 != pDispMap->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pDispMap is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else if ( nullptr == pConfMap )
    {
        QC_ERROR( "pConfMap is nullptr!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( ( QC_BUFFER_TYPE_TENSOR != pConfMap->type ) ||
              ( nullptr == pConfMap->buffer.pData ) || ( 4 != pConfMap->tensorProps.numDims ) ||
              ( QC_TENSOR_TYPE_UINT_8 != pConfMap->tensorProps.type ) ||
              ( 1 != pConfMap->tensorProps.dims[0] ) ||
              ( ALIGN_S( m_outputHeight, 1 ) != pConfMap->tensorProps.dims[1] ) ||
              ( ALIGN_S( m_outputWidth, 128 ) != pConfMap->tensorProps.dims[2] ) ||
              ( 1 != pConfMap->tensorProps.dims[3] ) )
    {
        QC_ERROR( "pConfMap is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        /* OK */
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = ValidateImageBuffer( pPriImage );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = ValidateImageBuffer( pAuxImage );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pPriImage, &pPriMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pAuxImage, &pAuxMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pDispMap, &pDispMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        ret = RegisterEvaMem( pConfMap, &pConfMem );
    }

    if ( QC_STATUS_OK == ret )
    {
        priImage.pBuffer = pPriMem;
        priImage.sImageInfo = m_imageInfo;
        auxImage.pBuffer = pAuxMem;
        auxImage.sImageInfo = m_imageInfo;

        dfsOutput.pDispMap = pDispMem;
        dfsOutput.nDispMapSize = pDispMem->nSize;
        dfsOutput.pConfMap = pConfMem;
        dfsOutput.nConfMapSize = pConfMem->nSize;

        rc = EvaDFS_Sync( m_hEvaDFS, &priImage, &auxImage, &dfsOutput, nullptr );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "EvaDFS_Sync failed: %d", rc );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e DepthFromStereo::Stop()
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
        rc = EvaStopSession( m_hDFSSession );
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

QCStatus_e DepthFromStereo::DeRegisterBuffers( const QCSharedBuffer_t *pBuffers,
                                               uint32_t numBuffers )
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
                rc = EvaMemDeregister( m_hDFSSession, &mem );
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

QCStatus_e DepthFromStereo::Deinit()
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
            rc = EvaMemDeregister( m_hDFSSession, &mem );
            if ( EVA_SUCCESS != rc )
            {
                QC_ERROR( "EvaMemDeregister(%p) failed: %d", mem.pAddress, rc );
                ret = QC_STATUS_FAIL;
            }
        }
        m_evaMemMap.clear();

        rc = EvaDeInitDFS( m_hEvaDFS );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "Failed to deinit DFS: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        m_hEvaDFS = nullptr;

        rc = EvaDeleteSession( m_hDFSSession );
        if ( EVA_SUCCESS != rc )
        {
            QC_ERROR( "Failed to delete Session: %d", rc );
            ret = QC_STATUS_FAIL;
        }
        m_hDFSSession = nullptr;

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
