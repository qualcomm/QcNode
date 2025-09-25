// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "svBuffer.h"
#include <unistd.h>
#include "QC/Node/DepthFromStereo.hpp"

namespace QC
{
namespace Node
{

DepthFromStereo_Config::DepthFromStereo_Config()
{
    this->imageFormat = QC_IMAGE_FORMAT_NV12;
    this->disparityFormat = DISP_FORMAT_P012_LA_Y_ONLY;
    this->width = 0;
    this->height = 0;
    this->frameRate = 30;
    this->confidenceOutputEn = false;
    this->processingMode = PROCESSING_MODE_AUTO;
    this->isFirstRequest = true;
    this->noiseOffsetPri = 0.0f;
    this->noiseOffsetAux = 0.0f;
    this->modelType = 1;
    this->modelSwitchFrameCount = 10;
    this->prevDisparityFactor = 1.0f;
    this->disparityMapPrecision = DISP_MAP_PRECISION_FRAC_6BIT;
    this->refinementLevel = REFINEMENT_LEVEL_REFINED_L2;
    this->occlusionOutputEn = false;
    this->disparityStatsEn = false;
    this->rectificationErrorStatsEn = false;
    this->chromaProcEN = false;
    this->maskLowTextureEn = false;
    this->confidenceThreshold = 210;
    this->disparityThreshold = 64;
    this->noiseScaleAux = 0.0f;
    this->noiseScalePri = 0.0f;
    this->rectificationErrTolerance = 0.29f;
    this->segmentationThreshold = 0.5f;
    this->textureThreshold = 0.167f;
    this->searchDirection = SEARCH_DIRECTION_L2R;
    this->edgePenalty = 0.08f;
    this->initialPenalty = 0.19f;
    this->neighborPenalty = 0.22f;
    this->smoothnessPenalty = 0.41f;
    this->imageSharpnessThreshold = 0.0f;
    this->farAwayDisparityLimit = 0.0f;
    this->disparityEdgeThreshold = 0.43f;
    this->matchingCostMetric = 100;
    this->textureMetric = 100;
    this->edgeAlignMetric = 100;
    this->disparityVarianceMetric = 100;
    this->occlusionMetric = 100;
    this->disparityVarianceTolerance = 0.36f;
    this->occlusionTolerance = 0.5f;
    this->refinementThreshold = 0.75f;
    this->maxDisparityRange = 64;
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

QCStatus_e DepthFromStereoConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( ( dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 ) != QC_IMAGE_FORMAT_NV12 ) and
         ( dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 ) != QC_IMAGE_FORMAT_NV12_UBWC ) )
    {
        errors += "invalid input image format, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( static_cast<DisparityFormat_e>( dt.Get<uint8_t>(
                 "disparityFormat", DISP_FORMAT_P012_LA_Y_ONLY ) ) >= DISP_FORMAT_MAX )
    {
        errors += "invalid disparity map format, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "width", 0 ) == 0 )
    {
        errors += "the width is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "height", 0 ) == 0 )
    {
        errors += "the height is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( dt.Get<uint32_t>( "fps", 0 ) == 0 )
    {
        errors += "the frame rate is zero, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    ProcessingMode_e processingMode = static_cast<ProcessingMode_e>(
            dt.Get<uint8_t>( "processingMode", PROCESSING_MODE_AUTO ) );
    if ( processingMode >= PROCESSING_MODE_MAX )
    {
        errors += "processing mode out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t offsetPrimary = dt.Get<float32_t>( "offsetPrimary", 0.0f );
    if ( offsetPrimary < -1.0f or offsetPrimary > 1.0f )
    {
        errors += "Primary offset out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t offsetAux = dt.Get<float32_t>( "offsetAux", 0.0f );
    if ( offsetAux < -1.0f or offsetAux > 1.0f )
    {
        errors += "Aux offset out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint8_t modelType = dt.Get<uint8_t>( "modelType", 1 );
    if ( modelType < 0 or modelType > 4 )
    {
        errors += "model type out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t prevDisparityFactor = dt.Get<float32_t>( "prevDisparityFactor", 1.0f );
    if ( prevDisparityFactor < 0.5f or prevDisparityFactor > 2 )
    {
        errors += "previous disparity factor out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    DispMapPrecision_e disparityMapPrecision = static_cast<DispMapPrecision_e>(
            dt.Get<uint8_t>( "disparityMapPrecision", DISP_MAP_PRECISION_FRAC_6BIT ) );
    if ( disparityMapPrecision >= DISP_MAP_PRECISION_MAX )
    {
        errors += "disparity map precision out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    RefinementLevel_e refinementLevel = static_cast<RefinementLevel_e>(
            dt.Get<uint8_t>( "refinementLevel", REFINEMENT_LEVEL_REFINED_L2 ) );
    if ( refinementLevel >= REFINEMENT_LEVEL_MAX )
    {
        errors += "refinement level out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t confidenceThreshold = dt.Get<uint32_t>( "confidenceThreshold", 210 );
    if ( confidenceThreshold < 0 or confidenceThreshold > 255 )
    {
        errors += "confidence threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t disparityThreshold = dt.Get<uint32_t>( "disparityThreshold", 210 );
    if ( disparityThreshold < 0 or disparityThreshold > 255 )
    {
        errors += "disparity threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t scalePrimary = dt.Get<float32_t>( "scalePrimary", 0.0f );
    if ( scalePrimary < -1.0f or scalePrimary > 1.0f )
    {
        errors += "Primary scale out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t scaleAux = dt.Get<float32_t>( "scaleAux", 0.0f );
    if ( scaleAux < -1.0f or scaleAux > 1.0f )
    {
        errors += "Aux scale out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t rectificationErrTolerance = dt.Get<float32_t>( "rectificationErrTolerance", 0.29f );
    if ( rectificationErrTolerance < 0 or rectificationErrTolerance > 1 )
    {
        errors += "rectification error tolerance out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t segmentationThreshold = dt.Get<float32_t>( "segmentationThreshold", 0.5f );
    if ( segmentationThreshold < 0 or segmentationThreshold > 1 )
    {
        errors += "segmentation threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t textureThreshold = dt.Get<float32_t>( "textureThreshold", 0.167f );
    if ( textureThreshold < 0 or textureThreshold > 1 )
    {
        errors += "texture threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    SearchDirection_e searchDirection = static_cast<SearchDirection_e>(
            dt.Get<uint8_t>( "searchDirection", SEARCH_DIRECTION_L2R ) );
    if ( searchDirection >= SEARCH_DIRECTION_MAX )
    {
        errors += "search direction out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t initialPenalty = dt.Get<float32_t>( "initialPenalty", 0.19f );
    if ( initialPenalty < 0.0f or initialPenalty > 1.0f )
    {
        errors += "initial penalty out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t edgePenalty = dt.Get<float32_t>( "edgePenalty", 0.08f );
    if ( edgePenalty < 0.0f or edgePenalty > 1.0f )
    {
        errors += "edge penalty out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t smoothnessPenalty = dt.Get<float32_t>( "smoothnessPenalty", 0.41f );
    if ( smoothnessPenalty < 0.0f or smoothnessPenalty > 1.0f )
    {
        errors += "smoothness penalty out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t neighbourPenalty = dt.Get<float32_t>( "neighbourPenalty", 0.22f );
    if ( neighbourPenalty < 0.0f or neighbourPenalty > 1.0f )
    {
        errors += "neighbour penalty out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t imageSharpnessThreshold = dt.Get<float32_t>( "imageSharpnessThreshold", 0.0f );
    if ( imageSharpnessThreshold < 0 or imageSharpnessThreshold > 1 )
    {
        errors += "image sharpness threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t farAwayDisparityLimit = dt.Get<float32_t>( "farAwayDisparityLimit", 0.43f );
    if ( farAwayDisparityLimit < 0 or farAwayDisparityLimit > 1 )
    {
        errors += "far away disparity limit out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t disparityEdgeThreshold = dt.Get<float32_t>( "disparityEdgeThreshold", 0.43f );
    if ( disparityEdgeThreshold < 0 or disparityEdgeThreshold > 1 )
    {
        errors += "disparity edge threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t matchingCostMetric = dt.Get<uint32_t>( "matchingCostMetric", 100 );
    if ( matchingCostMetric < 0 or matchingCostMetric > 100 )
    {
        errors += "matching cost metric out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t textureMetric = dt.Get<uint32_t>( "textureMetric", 100 );
    if ( textureMetric < 0 or textureMetric > 100 )
    {
        errors += "texture metric out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t edgeAlignMetric = dt.Get<uint32_t>( "edgeAlignMetric", 100 );
    if ( edgeAlignMetric < 0 or edgeAlignMetric > 100 )
    {
        errors += "edge align metric out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t disparityVarianceMetric = dt.Get<uint32_t>( "disparityVarianceMetric", 100 );
    if ( disparityVarianceMetric < 0 or disparityVarianceMetric > 100 )
    {
        errors += "disparity variance metric out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t occlusionMetric = dt.Get<uint32_t>( "occlusionMetric", 100 );
    if ( occlusionMetric < 0 or occlusionMetric > 100 )
    {
        errors += "occlusion metric out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t disparityVarianceTolerance = dt.Get<float32_t>( "disparityVarianceTolerance", 0.36f );
    if ( disparityVarianceTolerance < 0 or disparityVarianceTolerance > 1 )
    {
        errors += "disparity variance tolerance out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t occlusionTolerance = dt.Get<float32_t>( "occlusionTolerance", 0.5f );
    if ( occlusionTolerance < 0 or occlusionTolerance > 1 )
    {
        errors += "occlusion tolerance out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t refinementThreshold = dt.Get<float32_t>( "refinementThreshold", 0.75f );
    if ( refinementThreshold < 0 or refinementThreshold > 1 )
    {
        errors += "refinement threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t maxDisparityRange = dt.Get<uint32_t>( "maxDisparityRange", 64 );
    if ( maxDisparityRange < 16 or maxDisparityRange > 92 )
    {
        errors += "max disparity out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}

QCStatus_e DepthFromStereoConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = VerifyStaticConfig( dt, errors );

    if ( QC_STATUS_OK == status )
    {

        m_config.imageFormat = dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 );
        m_config.disparityFormat = static_cast<DisparityFormat_e>(
                dt.Get<uint8_t>( "disparityFormat", DISP_FORMAT_P012_LA_Y_ONLY ) );
        m_config.width = dt.Get<uint32_t>( "width", 0 );
        m_config.height = dt.Get<uint32_t>( "height", 0 );
        m_config.frameRate = dt.Get<uint32_t>( "fps", 30 );
        m_config.confidenceOutputEn = dt.Get<bool>( "confidenceOutputEn", false );
        m_config.processingMode = static_cast<ProcessingMode_e>(
                dt.Get<uint8_t>( "processingMode", PROCESSING_MODE_AUTO ) );
        m_config.isFirstRequest = dt.Get<bool>( "isFirstRequest", true );
        m_config.noiseOffsetPri = dt.Get<float32_t>( "noiseOffsetPrimary", 0.0f );
        m_config.noiseOffsetAux = dt.Get<float32_t>( "noiseOffsetAux", 0.0f );
        m_config.modelType = dt.Get<uint8_t>( "modelType", 1 );
        m_config.modelSwitchFrameCount = dt.Get<uint8_t>( "modelSwitchFrameCount", 10 );
        m_config.prevDisparityFactor = dt.Get<float32_t>( "prevDisparityFactor", 1.0f );
        m_config.disparityMapPrecision = static_cast<DispMapPrecision_e>(
                dt.Get<uint8_t>( "disparityMapPrecision", DISP_MAP_PRECISION_FRAC_6BIT ) );
        m_config.refinementLevel = static_cast<RefinementLevel_e>(
                dt.Get<uint8_t>( "refinementLevel", REFINEMENT_LEVEL_REFINED_L2 ) );
        m_config.occlusionOutputEn = dt.Get<bool>( "occlusionOutputEn", false );
        m_config.disparityStatsEn = dt.Get<bool>( "disparityStatsEn", false );
        m_config.rectificationErrorStatsEn = dt.Get<bool>( "rectificationErrorStatsEn", false );
        m_config.chromaProcEN = dt.Get<bool>( "chromaProcEN", false );
        m_config.maskLowTextureEn = dt.Get<bool>( "maskLowTextureEn", false );
        m_config.confidenceThreshold = dt.Get<uint32_t>( "confidenceThreshold", 210 );
        m_config.disparityThreshold = dt.Get<uint32_t>( "disparityThreshold", 64 );
        m_config.noiseScalePri = dt.Get<float32_t>( "noiseScalePrimary", 0.0f );
        m_config.noiseScaleAux = dt.Get<float32_t>( "noiseScaleAux", 0.0f );
        m_config.rectificationErrTolerance =
                dt.Get<float32_t>( "rectificationErrTolerance", 0.29f );
        m_config.segmentationThreshold = dt.Get<float32_t>( "segmentationThreshold", 0.5f );
        m_config.textureThreshold = dt.Get<float32_t>( "textureThreshold", 0.167f );
        m_config.searchDirection = static_cast<SearchDirection_e>(
                dt.Get<uint8_t>( "searchDirection", SEARCH_DIRECTION_L2R ) );
        m_config.initialPenalty = dt.Get<float32_t>( "initialPenalty", 0.19f );
        m_config.edgePenalty = dt.Get<float32_t>( "edgePenalty", 0.08f );
        m_config.smoothnessPenalty = dt.Get<float32_t>( "smoothnessPenalty", 0.41f );
        m_config.neighborPenalty = dt.Get<float32_t>( "neighbourPenalty", 0.22f );
        m_config.imageSharpnessThreshold = dt.Get<float32_t>( "imageSharpnessThreshold", 0.29f );
        m_config.farAwayDisparityLimit = dt.Get<float32_t>( "farAwayDisparityLimit", 0.5f );
        m_config.disparityEdgeThreshold = dt.Get<float32_t>( "disparityEdgeThreshold", 0.167f );
        m_config.matchingCostMetric = dt.Get<uint32_t>( "matchingCostMetric", 100 );
        m_config.textureMetric = dt.Get<uint32_t>( "textureMetric", 100 );
        m_config.edgeAlignMetric = dt.Get<uint32_t>( "edgeAlignMetric", 100 );
        m_config.disparityVarianceMetric = dt.Get<uint32_t>( "disparityVarianceMetric", 100 );
        m_config.occlusionMetric = dt.Get<uint32_t>( "occlusionMetric", 100 );
        m_config.disparityVarianceTolerance =
                dt.Get<float32_t>( "disparityVarianceTolerance", 0.36f );
        m_config.occlusionTolerance = dt.Get<float32_t>( "occlusionTolerance", 0.5f );
        m_config.refinementThreshold = dt.Get<float32_t>( "refinementThreshold", 0.75f );
        m_config.maxDisparityRange = dt.Get<uint32_t>( "maxDisparityRange", 64 );
    }

    return status;
}

QCStatus_e DepthFromStereoConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e DepthFromStereoConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = NodeConfigIfs::VerifyAndSet( config, errors );

    if ( QC_STATUS_OK == status )
    {
        DataTree dt;
        status = m_dataTree.Get( "static", dt );
        if ( QC_STATUS_OK == status )
        {
            status = ParseStaticConfig( dt, errors );
        }
        else
        {
            status = m_dataTree.Get( "dynamic", dt );
            if ( QC_STATUS_OK == status )
            {
                status = ApplyDynamicConfig( dt, errors );
            }
        }
    }

    return status;
}

const std::string &DepthFromStereoConfigIfs::GetOptions()
{
    return NodeConfigIfs::s_QC_STATUS_UNSUPPORTED;
}


PixelFormat DepthFromStereo::GetInputImageFormat( QCImageFormat_e imageFormat )
{
    PixelFormat format = PixelFormat::NV12;

    switch ( imageFormat )
    {
        case QC_IMAGE_FORMAT_NV12:
        {
            format = PixelFormat::NV12;
            break;
        }
        case QC_IMAGE_FORMAT_NV12_UBWC:
        {
            format = PixelFormat::NV12_UBWC;
            break;
        }
        default:
        {
            QC_ERROR( "DepthFromStereo: Unsupport color format: %d. Setting defalut color format",
                      imageFormat );
            break;
        }
    }

    return format;
}

PixelFormat DepthFromStereo::GetDisparityMapFormat( DisparityFormat_e disparityFormat )
{
    PixelFormat format = PixelFormat::P012_LA_Y_ONLY;

    switch ( disparityFormat )
    {
        case DISP_FORMAT_P010_LA_Y_ONLY:
        {
            format = PixelFormat::P010_LA_Y_ONLY;
            break;
        }
        case DISP_FORMAT_P010_MA_Y_ONLY:
        {
            format = PixelFormat::P010_MA_Y_ONLY;
            break;
        }
        case DISP_FORMAT_P012_LA_Y_ONLY:
        {
            format = PixelFormat::P012_LA_Y_ONLY;
            break;
        }
        case DISP_FORMAT_P012_LA_Y_ONLY_UBWC:
        {
            format = PixelFormat::P012_LA_Y_ONLY_UBWC;
            break;
        }
        case DISP_FORMAT_P012_MA_Y_ONLY:
        {
            format = PixelFormat::P012_MA_Y_ONLY;
            break;
        }
        case DISP_FORMAT_P012_MA_Y_ONLY_UBWC:
        {
            format = PixelFormat::P012_MA_Y_ONLY_UBWC;
            break;
        }
        default:
        {
            QC_ERROR( "DepthFromStereo: Unsupport color format: %d. Setting default color format",
                      disparityFormat );
            break;
        }
    }

    return format;
}

void DepthFromStereo::UpdateIconfig( StereoDisparity::ConfigMap &configMap,
                                     DepthFromStereo_Config_t configuration )
{

    configMap.Set( StereoDisparity::ConfigId::AVERAGE_FPS,
                   static_cast<uint32_t>( configuration.frameRate ) );
    configMap.Set( StereoDisparity::ConfigId::PRIMARY_IMAGE_INFO, &m_imageInfo );
    configMap.Set( StereoDisparity::ConfigId::AUXILIARY_IMAGE_INFO, &m_imageInfo );
    configMap.Set( StereoDisparity::ConfigId::CONFIDENCE_OUTPUT_EN,
                   configuration.confidenceOutputEn );
    configMap.Set( StereoDisparity::ConfigId::OCCLUSION_OUTPUT_EN,
                   configuration.occlusionOutputEn );
    configMap.Set( StereoDisparity::ConfigId::DISPARITY_STATS_EN, configuration.disparityStatsEn );
    configMap.Set( StereoDisparity::ConfigId::RECTIFICATION_ERROR_STATS_EN,
                   configuration.rectificationErrorStatsEn );
    configMap.Set( StereoDisparity::ConfigId::DISPARITY_MAP_FORMAT,
                   GetDisparityMapFormat( configuration.disparityFormat ) );
    if ( configuration.processingMode == PROCESSING_MODE_AUTO )
    {
        configMap.Set( StereoDisparity::ConfigId::MODE, StereoDisparity::Mode::AUTO );
    }
    else if ( configuration.processingMode == PROCESSING_MODE_DL )
    {
        configMap.Set( StereoDisparity::ConfigId::MODE, StereoDisparity::Mode::DL );
    }
    else
    {
        configMap.Set( StereoDisparity::ConfigId::MODE, StereoDisparity::Mode::SGM );
    }
}


QCStatus_e DepthFromStereo::ValidateImageDesc( const ImageDescriptor_t &imgDesc,
                                               const DepthFromStereo_Config_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( ( imgDesc.type != QC_BUFFER_TYPE_IMAGE ) or ( imgDesc.GetDataPtr() == nullptr ) or
         ( config.imageFormat != imgDesc.format ) or ( config.width != imgDesc.width ) or
         ( config.height != imgDesc.height ) or m_imageInfo.nPlanes != imgDesc.numPlanes )
    {
        QC_ERROR( "imgDesc descriptor is invalid!" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        for ( uint32_t i = 0; i < m_imageInfo.nPlanes; i++ )
        {
            if ( m_imageInfo.nWidthStride[i] != imgDesc.stride[i] )
            {
                QC_ERROR( "imgDesc with invalid stride in plane %u: %u != %u", i,
                          m_imageInfo.nWidthStride[i], imgDesc.stride[i] );
                ret = QC_STATUS_INVALID_BUF;
            }
            else if ( m_imageInfo.nAlignedSize[i] != imgDesc.planeBufSize[i] )
            {
                QC_ERROR( "imgDesc with invalid plane buf size in plane %u: %u != %u", i,
                          m_imageInfo.nAlignedSize[i], imgDesc.planeBufSize[i] );
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


void DepthFromStereo::SetInitialFrameConfig( StereoDisparity::ConfigMap &configMapFrame,
                                             DepthFromStereo_Config_t configuration )
{
    noiseToleranceOffset.nOffsetPri = configuration.noiseOffsetPri;
    noiseToleranceOffset.nOffsetAux = configuration.noiseOffsetAux;
    noiseToleranceScale.nScalePri = configuration.noiseScalePri;
    noiseToleranceScale.nScaleAux = configuration.noiseScaleAux;
    penalties.nInitialPenalty = configuration.initialPenalty;
    penalties.nEdgePenalty = configuration.edgePenalty;
    penalties.nSmoothnessPenalty = configuration.smoothnessPenalty;
    penalties.nNeighborPenalty = configuration.neighborPenalty;

    configMapFrame.Set( StereoDisparity::ConfigId::MAX_DISPARITY_RANGE,
                        configuration.maxDisparityRange );
    if ( configuration.disparityMapPrecision == DISP_MAP_PRECISION_FRAC_6BIT )
    {
        configMapFrame.Set( StereoDisparity::ConfigId::DISPARITY_MAP_PRECISION,
                            StereoDisparity::Precision::FRAC_6BIT );
    }
    else if ( configuration.disparityMapPrecision == DISP_MAP_PRECISION_FRAC_4BIT )
    {
        configMapFrame.Set( StereoDisparity::ConfigId::DISPARITY_MAP_PRECISION,
                            StereoDisparity::Precision::FRAC_4BIT );
    }
    else
    {
        configMapFrame.Set( StereoDisparity::ConfigId::DISPARITY_MAP_PRECISION,
                            StereoDisparity::Precision::INT );
    }

    if ( configuration.refinementLevel == REFINEMENT_LEVEL_NONE )
    {
        configMapFrame.Set( StereoDisparity::ConfigId::REFINEMENT_LEVEL,
                            StereoDisparity::RefinementLevel::NONE );
    }
    else if ( configuration.refinementLevel == REFINEMENT_LEVEL_REFINED_L1 )
    {
        configMapFrame.Set( StereoDisparity::ConfigId::REFINEMENT_LEVEL,
                            StereoDisparity::RefinementLevel::REFINED_L1 );
    }
    else
    {
        configMapFrame.Set( StereoDisparity::ConfigId::REFINEMENT_LEVEL,
                            StereoDisparity::RefinementLevel::REFINED_L2 );
    }

    configMapFrame.Set( StereoDisparity::ConfigId::CHROMA_PROC_EN, configuration.chromaProcEN );
    configMapFrame.Set( StereoDisparity::ConfigId::NOISE_TOLERANCE_SCALE, &noiseToleranceScale );
    configMapFrame.Set( StereoDisparity::ConfigId::NOISE_TOLERANCE_OFFSET, &noiseToleranceOffset );
    configMapFrame.Set( StereoDisparity::ConfigId::DISPARITY_VARIANCE_TOLERANCE,
                        configuration.disparityVarianceTolerance );
    configMapFrame.Set( StereoDisparity::ConfigId::OCCLUSION_TOLERANCE,
                        configuration.occlusionTolerance );
    configMapFrame.Set( StereoDisparity::ConfigId::PENALTIES, &penalties );
    configMapFrame.Set( StereoDisparity::ConfigId::MATCHING_COST_METRIC,
                        configuration.matchingCostMetric );
    configMapFrame.Set( StereoDisparity::ConfigId::TEXTURE_METRIC, configuration.textureMetric );
    configMapFrame.Set( StereoDisparity::ConfigId::EDGE_ALIGN_METRIC,
                        configuration.edgeAlignMetric );
    configMapFrame.Set( StereoDisparity::ConfigId::DISPARITY_VARIANCE_METRIC,
                        configuration.disparityVarianceMetric );
    configMapFrame.Set( StereoDisparity::ConfigId::OCCLUSION_METRIC,
                        configuration.occlusionMetric );
    configMapFrame.Set( StereoDisparity::ConfigId::SEGMENTATION_THRESHOLD,
                        configuration.segmentationThreshold );
    configMapFrame.Set( StereoDisparity::ConfigId::IMAGE_SHARPNESS_THRESHOLD,
                        configuration.imageSharpnessThreshold );
    configMapFrame.Set( StereoDisparity::ConfigId::DISPARITY_EDGE_THRESHOLD,
                        configuration.disparityEdgeThreshold );
    configMapFrame.Set( StereoDisparity::ConfigId::REFINEMENT_THRESHOLD,
                        configuration.refinementThreshold );
    configMapFrame.Set( StereoDisparity::ConfigId::TEXTURE_THRESHOLD,
                        configuration.textureThreshold );
    configMapFrame.Set( StereoDisparity::ConfigId::MASK_LOW_TEXTURE_EN,
                        configuration.maskLowTextureEn );
    configMapFrame.Set( StereoDisparity::ConfigId::RECTIFICATION_ERR_TOLERANCE,
                        configuration.rectificationErrTolerance );
    if ( configuration.searchDirection == SEARCH_DIRECTION_L2R )
    {
        configMapFrame.Set( StereoDisparity::ConfigId::SEARCH_DIRECTION,
                            StereoDisparity::SearchDir::L2R );
    }
    else
    {
        configMapFrame.Set( StereoDisparity::ConfigId::SEARCH_DIRECTION,
                            StereoDisparity::SearchDir::R2L );
    }
    configMapFrame.Set( StereoDisparity::ConfigId::FAR_AWAY_DISPARITY_LIMIT,
                        configuration.farAwayDisparityLimit );
    configMapFrame.Set( StereoDisparity::ConfigId::IS_FIRST_REQUEST, configuration.isFirstRequest );
}

QCStatus_e DepthFromStereo::RegisterMemory( const BufferDescriptor_t &bufferDesc, Buffer &pBuff )
{
    QCStatus_e ret = QC_STATUS_OK;
    Status eStatus = Status::EFAIL;
    Buffer buff;

    if ( bufferDesc.pBuf == nullptr )
    {
        QC_ERROR( "DepthFromStereo: Buffer data ptr is nullptr" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        auto it = m_memMap.find( bufferDesc.GetDataPtr() );
        if ( it == m_memMap.end() )
        {
            QC_DEBUG( "DepthFromStereo: Buffer not found in map" );
            QC_DEBUG( "DepthFromStereo: Buffer size %d", bufferDesc.GetDataSize() );
            QC_DEBUG( "DepthFromStereo: Buffer offset %d", bufferDesc.offset );
            QC_DEBUG( "DepthFromStereo: Buffer dmahandle %lu", bufferDesc.dmaHandle );

            buff.nSize = bufferDesc.GetDataSize();
            buff.nOffset = bufferDesc.offset;
            buff.pAddress = bufferDesc.pBuf;
            buff.hHandle = (BufferHandle) bufferDesc.dmaHandle;

            eStatus = BufferRegister( m_session, buff );
            if ( eStatus != Status::SUCCESS )
            {
                QC_ERROR( "DepthFromStereo: DFS memory register(%p) failed: %d",
                          bufferDesc.GetDataPtr(), eStatus );
                ret = QC_STATUS_FAIL;
            }
            else
            {
                m_memMap[bufferDesc.GetDataPtr()] = buff;
                pBuff = m_memMap[bufferDesc.GetDataPtr()];
            }
        }
        else
        {
            pBuff = it->second;
        }
    }
    return ret;
}


QCStatus_e DepthFromStereo::Initialize( QCNodeInit_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    std::string errors;
    Status eStatus = Status::SUCCESS;
    PixelFormat format;
    const DepthFromStereo_Config_t &configuration =
            dynamic_cast<const DepthFromStereo_Config_t &>( GetConfigurationIfs().Get() );

    ret = GetConfigurationIfs().VerifyAndSet( config.config, errors );

    if ( ret == QC_STATUS_OK )
    {
        ret = NodeBase::Init( GetConfigurationIfs().Get().nodeId );
    }
    else
    {
        QC_ERROR( "DepthFromStereo: config error: %s", errors.c_str() );
    }

    if ( ret == QC_STATUS_OK )
    {
        std::string sessionName{ "DEPTH_FROM_STEREO" };
        sessionConfigMap.Set( Session::ConfigId::NAME, &sessionName );
        sessionConfigMap.Set( Session::ConfigId::TYPE, Session::Type::CV );
        sessionConfigMap.Set( Session::ConfigId::PRIORITY, Session::Priority::REALTIME_MED );

        format = GetInputImageFormat( configuration.imageFormat );

        eStatus = ImageInfoQuery( format, configuration.width, configuration.height, m_imageInfo );
        if ( eStatus != Status::SUCCESS )
        {
            QC_ERROR( "DepthFromStereo: Failed to get image info: %d", eStatus );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "DepthFromStereo: Successfully retreived image info" );
        }
    }
    else
    {
        QC_ERROR( "DepthFromStereo: Failure to initialize node" );
    }

    if ( ret == QC_STATUS_OK )
    {
        m_session = Session::Create( sessionConfigMap, nullptr, nullptr );
        if ( m_session == nullptr )
        {
            QC_ERROR( "DepthFromStereo: Failed to create session" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "DepthFromStereo: Successfully created session %d", ret );
        }
    }

    if ( ret == QC_STATUS_OK )
    {
        UpdateIconfig( configMap, configuration );
        m_state = QC_OBJECT_STATE_READY;
    }


    if ( ret == QC_STATUS_OK )
    {
        m_stereoDisparity = StereoDisparity::Create( m_session, configMap );
        if ( m_stereoDisparity == nullptr )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "DepthFromStereo: Stereo disparity creation failed %d", ret );
            Status status = m_session->Destroy();
            if ( status != Status::SUCCESS )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "DepthFromStereo: Session destroy failed %d", ret );
            }
            m_session = nullptr;
        }
    }

    return ret;
}


QCStatus_e DepthFromStereo::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::SUCCESS;
    const DepthFromStereo_Config_t &configuration =
            dynamic_cast<const DepthFromStereo_Config_t &>( GetConfigurationIfs().Get() );

    if ( m_state != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "DepthFromStereo: Start is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_session->Start();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "DepthFromStereo: Session start failed %d", ret );
        }
        if ( ret == QC_STATUS_OK )
        {
            SetInitialFrameConfig( configMap, configuration );
            m_state = QC_OBJECT_STATE_RUNNING;
        }
    }

    return ret;
}


QCStatus_e DepthFromStereo::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::EFAIL;
    Buffer priMem;
    Buffer auxMem;
    Buffer dispMem;
    Buffer confMem;
    StereoDisparity::Output sOutput;
    StereoDisparity::Input sInput;

    if ( m_state != QC_OBJECT_STATE_RUNNING )
    {
        QC_ERROR( "DepthFromStereo: Execute is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        const DepthFromStereo_Config_t &configuration =
                dynamic_cast<const DepthFromStereo_Config_t &>( GetConfigurationIfs().Get() );
        const DepthFromStereoBufferMapping_t &bufferMap = configuration.bufferMap;
        const ImageDescriptor_t *pPriImgDesc = dynamic_cast<const ImageDescriptor_t *>(
                &frameDesc.GetBuffer( bufferMap.primaryImageBufferId ) );
        const ImageDescriptor_t *pAuxImgDesc = dynamic_cast<const ImageDescriptor_t *>(
                &frameDesc.GetBuffer( bufferMap.auxilaryImageBufferId ) );
        const TensorDescriptor_t *pDispMapDesc = dynamic_cast<const TensorDescriptor_t *>(
                &frameDesc.GetBuffer( bufferMap.disparityMapBufferId ) );

        if ( pPriImgDesc == nullptr )
        {
            QC_ERROR( "invalid primary image buffer (id %u)", bufferMap.primaryImageBufferId );
            ret = QC_STATUS_FAIL;
        }

        if ( pAuxImgDesc == nullptr )
        {
            QC_ERROR( "invalid aux image buffer (id %u)", bufferMap.auxilaryImageBufferId );
            ret = QC_STATUS_FAIL;
        }

        if ( pDispMapDesc == nullptr )
        {
            QC_ERROR( "invalid disparity tensor buffer (id %u)", bufferMap.disparityMapBufferId );
            ret = QC_STATUS_FAIL;
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = ValidateImageDesc( *pPriImgDesc, configuration );
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = ValidateImageDesc( *pAuxImgDesc, configuration );
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = RegisterMemory( *pPriImgDesc, priMem );
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = RegisterMemory( *pAuxImgDesc, auxMem );
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = RegisterMemory( *pDispMapDesc, dispMem );
        }

        if ( ret == QC_STATUS_OK )
        {
            if ( configuration.confidenceOutputEn == true )
            {
                const TensorDescriptor_t *pDispConfDesc = dynamic_cast<const TensorDescriptor_t *>(
                        &frameDesc.GetBuffer( bufferMap.disparityConfidenceMapBufferId ) );
                if ( pDispConfDesc == nullptr )
                {
                    QC_ERROR( "invalid confidence tensor buffer (id %u)",
                              bufferMap.disparityConfidenceMapBufferId );
                    ret = QC_STATUS_FAIL;
                }
                if ( ret == QC_STATUS_OK )
                {
                    ret = RegisterMemory( *pDispConfDesc, confMem );
                }
                if ( ret == QC_STATUS_OK )
                {
                    sOutput.sConfMap.sBuffer = confMem;
                }
            }
        }

        if ( ret == QC_STATUS_OK )
        {
            sInput.sPriImage.sImageInfo = m_imageInfo;
            sInput.sAuxImage.sImageInfo = m_imageInfo;
            sInput.sPriImage.sBuffer = priMem;
            sInput.sAuxImage.sBuffer = auxMem;

            sOutput.sDisparityMap.sBuffer = dispMem;

            status = m_stereoDisparity->SubmitSync( sInput, sOutput, configMap );
            if ( status != Status::SUCCESS )
            {
                QC_ERROR( "DepthFromStereo: DFS_Sync failed: %d", status );
                ret = QC_STATUS_FAIL;
            }
        }
    }
    return ret;
}


QCStatus_e DepthFromStereo::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::SUCCESS;

    if ( m_state != QC_OBJECT_STATE_RUNNING )
    {
        QC_ERROR( "DepthFromStereo: Stop is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_session->Stop();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "DepthFromStereo: Failed to stop Session: %d", ret );
        }

        if ( ret == QC_STATUS_OK )
        {
            m_state = QC_OBJECT_STATE_READY;
        }
    }

    return ret;
}


QCStatus_e DepthFromStereo::DeInitialize()
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::SUCCESS;

    if ( m_state != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "DepthFromStereo: Deinitialize is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_stereoDisparity->Destroy();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "DepthFromStereo: Failed to destroy stereo disparity: %d", ret );
        }

        for ( auto &it : m_memMap )
        {
            Buffer mem = it.second;
            status = BufferDeregister( m_session, mem );
            if ( status != Status::SUCCESS )
            {
                QC_ERROR( "DepthFromStereo: Mem Deregister(%p) failed: %d", mem.pAddress, status );
                ret = QC_STATUS_FAIL;
            }
        }

        m_memMap.clear();
        status = m_session->Destroy();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "DepthFromStereo: Session destroy failed %d", ret );
        }
        m_session = nullptr;
    }

    return ret;
}

}   // namespace Node
}   // namespace QC