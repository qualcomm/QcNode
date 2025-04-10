// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "ridehal/component/DepthFromStereo.hpp"

namespace ridehal
{
namespace component
{

#define RIDEHAL_EVA_DFS_NUM_ICONFIG 6
#define RIDEHAL_EVA_DFS_NUM_FCONFIG 15

#define ALIGN_S( size, align ) ( ( ( ( size ) + ( align ) - 1 ) / ( align ) ) * ( align ) )

static const char *evaDFSIConfigStrings[RIDEHAL_EVA_DFS_NUM_ICONFIG] = {
        EVA_DFS_ICONFIG_ACTUAL_FPS,         EVA_DFS_ICONFIG_OPERATIONAL_FPS,
        EVA_DFS_ICONFIG_PRIMARY_IMAGE_INFO, EVA_DFS_ICONFIG_AUXILIARY_IMAGE_INFO,
        EVA_DFS_ICONFIG_CONF_OUTPUT_ENABLE, EVA_DFS_ICONFIG_SLIC_OUTPUT_ENABLE,
};

static const char *evaDFSFrameConfigStrings[RIDEHAL_EVA_DFS_NUM_FCONFIG] = {
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
    this->format = RIDEHAL_IMAGE_FORMAT_NV12;
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
    RIDEHAL_DEBUG( "Received Session callback for session: %p, eEvent: %d", hSession, eEvent );
    if ( EVA_EVFATAL == eEvent )
    {
        RIDEHAL_ERROR( "set state to ERROR" );
        m_state = RIDEHAL_COMPONENT_STATE_ERROR;
    }
}

RideHalError_e DepthFromStereo::UpdateIconfig( EvaConfigList_t *pConfigList )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc = EvaDFSQueryConfigIndices( &evaDFSIConfigStrings[0], pConfigList );
    if ( rc != EVA_SUCCESS )
    {
        RIDEHAL_ERROR( "Failed to Query I config Indices: %d", rc );
        ret = RIDEHAL_ERROR_FAIL;
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

RideHalError_e DepthFromStereo::ValidateConfig( const DepthFromStereo_Config_t *pConfig )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( nullptr == pConfig )
    {
        RIDEHAL_ERROR( "pConfig is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( pConfig->format != RIDEHAL_IMAGE_FORMAT_NV12 ) &&
              ( pConfig->format != RIDEHAL_IMAGE_FORMAT_P010 ) &&
              ( pConfig->format != RIDEHAL_IMAGE_FORMAT_NV12_UBWC ) &&
              ( pConfig->format != RIDEHAL_IMAGE_FORMAT_TP10_UBWC ) )
    {
        RIDEHAL_ERROR( "invalid format!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( EVA_DFS_SEARCH_L2R != pConfig->dfsSearchDir ) &&
              ( EVA_DFS_SEARCH_R2L != pConfig->dfsSearchDir ) )
    {
        RIDEHAL_ERROR( "invalid serach direction!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        /* OK */
    }
    return ret;
}

EvaColorFormat_e DepthFromStereo::GetEvaColorFormat( RideHal_ImageFormat_e colorFormat )
{
    EvaColorFormat_e evaFormat = EVA_COLORFORMAT_MAX;

    switch ( colorFormat )
    {
        case RIDEHAL_IMAGE_FORMAT_NV12:
        {
            evaFormat = EVA_COLORFORMAT_NV12;
            break;
        }
        case RIDEHAL_IMAGE_FORMAT_P010:
        {
            evaFormat = EVA_COLORFORMAT_P010_MSB;
            break;
        }
        case RIDEHAL_IMAGE_FORMAT_NV12_UBWC:
        {
            evaFormat = EVA_COLORFORMAT_NV12_UBWC;
            break;
        }
        case RIDEHAL_IMAGE_FORMAT_TP10_UBWC:
        {
            evaFormat = EVA_COLORFORMAT_TP10_UBWC;
            break;
        }
        default:
        {
            RIDEHAL_ERROR( "Unsupport corlor sormat: %d", colorFormat );
            break;
        }
    }

    return evaFormat;
}

RideHalError_e DepthFromStereo::Init( const char *pName, const DepthFromStereo_Config_t *pConfig,
                                      Logger_Level_e level )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    bool bIFInitOK = false;
    EvaStatus_e rc;

    EvaConfigList_t configList;
    EvaConfig_t configs[RIDEHAL_EVA_DFS_NUM_ICONFIG];

    ret = ComponentIF::Init( pName, level );
    if ( RIDEHAL_ERROR_NONE == ret )
    {
        bIFInitOK = true;
        ret = ValidateConfig( pConfig );
        if ( RIDEHAL_ERROR_NONE == ret )
        { /* parameters are OK */
            m_config = *pConfig;
            m_outputWidth = m_config.width;
            m_outputHeight = m_config.height;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_hDFSSession = EvaCreateSession( EvaSessionCallbackHandler, this, NULL );
        if ( nullptr == m_hDFSSession )
        {
            RIDEHAL_ERROR( "EVADepthFromStereo : Failed to Create Session" );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        EvaColorFormat_e evaFormat = GetEvaColorFormat( m_config.format );
        rc = EvaQueryImageInfo( evaFormat, m_config.width, m_config.height, &m_imageInfo );
        if ( rc != EVA_SUCCESS )
        {
            RIDEHAL_ERROR( "EVADepthFromStereo: Failed to get image info: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG(
                    "ImageInfo: WxH=%ux%u format=%d planes=%u total size=%u, width stride=[%u "
                    "%u %u %u], aligned size=[%u %u %u %u]",
                    m_imageInfo.nWidth, m_imageInfo.nHeight, m_imageInfo.eFormat,
                    m_imageInfo.nPlanes, m_imageInfo.nTotalSize, m_imageInfo.nWidthStride[0],
                    m_imageInfo.nWidthStride[1], m_imageInfo.nWidthStride[2],
                    m_imageInfo.nWidthStride[3], m_imageInfo.nAlignedSize[0],
                    m_imageInfo.nAlignedSize[1], m_imageInfo.nAlignedSize[2],
                    m_imageInfo.nAlignedSize[3] );
        }
    }


    if ( RIDEHAL_ERROR_NONE == ret )
    {
        configList.nConfigs = RIDEHAL_EVA_DFS_NUM_ICONFIG;
        configList.pConfigs = configs;
        ret = UpdateIconfig( &configList );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        m_hEvaDFS = EvaInitDFS( m_hDFSSession, &configList, &m_outBufInfo, NULL, this );
        if ( nullptr == m_hEvaDFS )
        {
            RIDEHAL_ERROR( "Failed to Init DFS" );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            RIDEHAL_DEBUG( "output disparity buffer size %u, conf map buf size %u",
                           m_outBufInfo.nDispMapBytes, m_outBufInfo.nConfMapBytes );
        }
    }

    if ( ret != RIDEHAL_ERROR_NONE )
    { /* do error clean up */
        RIDEHAL_ERROR( "DepthFromStereo Init failed: %d!", ret );

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
        m_state = RIDEHAL_COMPONENT_STATE_READY;
    }

    return ret;
}

RideHalError_e DepthFromStereo::RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                                 uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "Register Buffers is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "pBuffers is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 0 == numBuffers )
    {
        RIDEHAL_ERROR( "numBuffers is 0!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t i = 0; i < numBuffers; i++ )
        {
            EvaMem_t *pEvaMem;
            ret = RegisterEvaMem( &pBuffers[i], &pEvaMem );
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                break;
            }
        }
    }
    return ret;
}

RideHalError_e DepthFromStereo::SetInitialFrameConfig()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc;

    EvaConfigList_t configList;
    EvaConfig_t configs[RIDEHAL_EVA_DFS_NUM_FCONFIG];
    (void) memset( configs, 0, sizeof( EvaConfig_t ) * RIDEHAL_EVA_DFS_NUM_FCONFIG );

    configList.nConfigs = RIDEHAL_EVA_DFS_NUM_FCONFIG;
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
            RIDEHAL_ERROR( "Failed to Set F config: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    else
    {
        RIDEHAL_ERROR( "Failed to Query F config Indices: %d", rc );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}


RideHalError_e DepthFromStereo::Start()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "Start is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        rc = EvaStartSession( m_hDFSSession );
        if ( EVA_SUCCESS != rc )
        {
            RIDEHAL_ERROR( "Failed to Start Session: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            ret = SetInitialFrameConfig();
            if ( RIDEHAL_ERROR_NONE != ret )
            {
                RIDEHAL_ERROR( "Failed to set Session config" );
            }
            else
            {
                m_state = RIDEHAL_COMPONENT_STATE_RUNNING;
            }
        }
    }

    return ret;
}

RideHalError_e DepthFromStereo::RegisterEvaMem( const RideHal_SharedBuffer_t *pBuffer,
                                                EvaMem_t **pEvaMem )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc;
    EvaMem_t mem;

    auto it = m_evaMemMap.find( pBuffer->data() );
    if ( it == m_evaMemMap.end() )
    {
        mem.nSize = pBuffer->size;
        mem.nOffset = pBuffer->offset;
        mem.pAddress = pBuffer->data();
        mem.hMemHandle = (void *) pBuffer->buffer.dmaHandle;
        if ( 0 != ( pBuffer->buffer.flags & RIDEHAL_BUFFER_FLAGS_CACHE_WB_WA ) )
        {
            mem.bCached = true;
        }
        rc = EvaMemRegister( m_hDFSSession, &mem );
        if ( EVA_SUCCESS != rc )
        {
            RIDEHAL_ERROR( "EvaMemRegister(%p) failed: %d", pBuffer->data(), rc );
            ret = RIDEHAL_ERROR_FAIL;
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

RideHalError_e DepthFromStereo::ValidateImageBuffer( const RideHal_SharedBuffer_t *pImage )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;

    if ( nullptr == pImage )
    {
        RIDEHAL_ERROR( "pImage is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_IMAGE != pImage->type ) ||
              ( nullptr == pImage->buffer.pData ) ||
              ( m_config.format != pImage->imgProps.format ) ||
              ( m_config.width != pImage->imgProps.width ) ||
              ( m_config.height != pImage->imgProps.height ) ||
              ( m_imageInfo.nPlanes != pImage->imgProps.numPlanes ) )
    {
        RIDEHAL_ERROR( "pImage is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else
    {
        for ( uint32_t i = 0; i < m_imageInfo.nPlanes; i++ )
        {
            if ( m_imageInfo.nWidthStride[i] != pImage->imgProps.stride[i] )
            {
                RIDEHAL_ERROR( "pImage with invalid stride in plane %u: %u != %u", i,
                               m_imageInfo.nWidthStride[i], pImage->imgProps.stride[i] );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
            else if ( m_imageInfo.nAlignedSize[i] != pImage->imgProps.planeBufSize[i] )
            {
                RIDEHAL_ERROR( "pImage with invalid plane buf size in plane %u: %u != %u", i,
                               m_imageInfo.nAlignedSize[i], pImage->imgProps.planeBufSize[i] );
                ret = RIDEHAL_ERROR_INVALID_BUF;
            }
            else
            {
                /* OK */
            }
        }
    }
    return ret;
}


RideHalError_e DepthFromStereo::Execute( const RideHal_SharedBuffer_t *pPriImage,
                                         const RideHal_SharedBuffer_t *pAuxImage,
                                         const RideHal_SharedBuffer_t *pDispMap,
                                         const RideHal_SharedBuffer_t *pConfMap )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc;
    EvaMem_t *pPriMem = nullptr;
    EvaMem_t *pAuxMem = nullptr;
    EvaMem_t *pDispMem = nullptr;
    EvaMem_t *pConfMem = nullptr;
    EvaImage_t priImage;
    EvaImage_t auxImage;
    EvaDFSOutput_t dfsOutput;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "Execute is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pDispMap )
    {
        RIDEHAL_ERROR( "pDispMap is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pDispMap->type ) ||
              ( nullptr == pDispMap->buffer.pData ) || ( 4 != pDispMap->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_UINT_16 != pDispMap->tensorProps.type ) ||
              ( 1 != pDispMap->tensorProps.dims[0] ) ||
              ( ALIGN_S( m_outputHeight, 2 ) != pDispMap->tensorProps.dims[1] ) ||
              ( ALIGN_S( m_outputWidth, 128 ) != pDispMap->tensorProps.dims[2] ) ||
              ( 1 != pDispMap->tensorProps.dims[3] ) )
    {
        RIDEHAL_ERROR( "pDispMap is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else if ( nullptr == pConfMap )
    {
        RIDEHAL_ERROR( "pConfMap is nullptr!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( ( RIDEHAL_BUFFER_TYPE_TENSOR != pConfMap->type ) ||
              ( nullptr == pConfMap->buffer.pData ) || ( 4 != pConfMap->tensorProps.numDims ) ||
              ( RIDEHAL_TENSOR_TYPE_UINT_8 != pConfMap->tensorProps.type ) ||
              ( 1 != pConfMap->tensorProps.dims[0] ) ||
              ( ALIGN_S( m_outputHeight, 1 ) != pConfMap->tensorProps.dims[1] ) ||
              ( ALIGN_S( m_outputWidth, 128 ) != pConfMap->tensorProps.dims[2] ) ||
              ( 1 != pConfMap->tensorProps.dims[3] ) )
    {
        RIDEHAL_ERROR( "pConfMap is invalid!" );
        ret = RIDEHAL_ERROR_INVALID_BUF;
    }
    else
    {
        /* OK */
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ValidateImageBuffer( pPriImage );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = ValidateImageBuffer( pAuxImage );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = RegisterEvaMem( pPriImage, &pPriMem );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = RegisterEvaMem( pAuxImage, &pAuxMem );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = RegisterEvaMem( pDispMap, &pDispMem );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        ret = RegisterEvaMem( pConfMap, &pConfMem );
    }

    if ( RIDEHAL_ERROR_NONE == ret )
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
            RIDEHAL_ERROR( "EvaDFS_Sync failed: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e DepthFromStereo::Stop()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc;

    if ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state )
    {
        RIDEHAL_ERROR( "Stop is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        rc = EvaStopSession( m_hDFSSession );
        if ( EVA_SUCCESS != rc )
        {
            RIDEHAL_ERROR( "Failed to stop Session: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            m_state = RIDEHAL_COMPONENT_STATE_READY;
        }
    }

    return ret;
}

RideHalError_e DepthFromStereo::DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers,
                                                   uint32_t numBuffers )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc;

    if ( ( RIDEHAL_COMPONENT_STATE_READY != m_state ) &&
         ( RIDEHAL_COMPONENT_STATE_RUNNING != m_state ) )
    {
        RIDEHAL_ERROR( "DeRegisterBuffers is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        RIDEHAL_ERROR( "Empty buffers pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else if ( 0 == numBuffers )
    {
        RIDEHAL_ERROR( "numBuffers is 0!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
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
                    RIDEHAL_ERROR( "EvaMemDeregister(%p) failed: %d", mem.pAddress, rc );
                    ret = RIDEHAL_ERROR_FAIL;
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

RideHalError_e DepthFromStereo::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    EvaStatus_e rc;

    if ( RIDEHAL_COMPONENT_STATE_READY != m_state )
    {
        RIDEHAL_ERROR( "Deinit is not allowed when in state %d!", m_state );
        ret = RIDEHAL_ERROR_BAD_STATE;
    }
    else
    {
        for ( auto &it : m_evaMemMap )
        {
            auto &mem = it.second;
            rc = EvaMemDeregister( m_hDFSSession, &mem );
            if ( EVA_SUCCESS != rc )
            {
                RIDEHAL_ERROR( "EvaMemDeregister(%p) failed: %d", mem.pAddress, rc );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
        m_evaMemMap.clear();

        rc = EvaDeInitDFS( m_hEvaDFS );
        if ( EVA_SUCCESS != rc )
        {
            RIDEHAL_ERROR( "Failed to deinit DFS: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        m_hEvaDFS = nullptr;

        rc = EvaDeleteSession( m_hDFSSession );
        if ( EVA_SUCCESS != rc )
        {
            RIDEHAL_ERROR( "Failed to delete Session: %d", rc );
            ret = RIDEHAL_ERROR_FAIL;
        }
        m_hDFSSession = nullptr;

        RideHalError_e ret2 = ComponentIF::Deinit();
        if ( RIDEHAL_ERROR_NONE != ret2 )
        {
            RIDEHAL_ERROR( "Deinit ComponentIF failed!" );
            ret = ret2;
        }
    }

    return ret;
}

}   // namespace component
}   // namespace ridehal
