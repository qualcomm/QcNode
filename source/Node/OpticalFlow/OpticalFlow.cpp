
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/Node/OpticalFlow.hpp"
#include "svBuffer.h"
#include <unistd.h>

namespace QC
{
namespace Node
{

OpticalFlow_Config::OpticalFlow_Config()
{
    this->imageFormat = QC_IMAGE_FORMAT_NV12;
    this->motionMapFormat = MOTION_FORMAT_12_LA;
    this->width = 0;
    this->height = 0;
    this->frameRate = 30;
    this->motionMapFracEn = true;
    this->motionMapUpscale = MOTION_MAP_UPSCALE_NONE;
    this->motionDirection = MOTION_DIRECTION_FORWARD;
    this->motionMapStepSize = MOTION_MAP_STEP_SIZE_1;
    this->confidenceOutputEn = false;
    this->refinementLevel = REFINEMENT_LEVEL_REFINED_L1;
    this->chromaProcEn = false;
    this->maskLowTextureEn = false;
    this->computationAccuracy = COMPUTATION_ACCURACY_MEDIUM;
    this->noiseScaleSrc = 0.0f;
    this->noiseScaleDst = 0.0f;
    this->noiseOffsetSrc = 0.0f;
    this->noiseOffsetDst = 0.0f;
    this->motionVarianceTolerance = 0.36f;
    this->occlusionTolerance = 0.5f;
    this->edgePenalty = 0.08f;
    this->initialPenalty = 0.19f;
    this->neighborPenalty = 0.22f;
    this->smoothnessPenalty = 0.41f;
    this->textureMetric = 100;
    this->edgeAlignMetric = 100;
    this->motionVarianceMetric = 100;
    this->occlusionMetric = 100;
    this->segmentationThreshold = 0.5f;
    this->globalMotionDetailThreshold = 0.66f;
    this->imageSharpnessThreshold = this->chromaProcEn ? 0.0f : 0.4f;
    this->mvEdgeThreshold = 0.43f;
    this->refinementThreshold = 0.75f;
    this->textureThreshold = 0.167f;
    this->lightingCondition = LIGHTING_CONDITION_HIGH;
    this->isFirstRequest = false;
    this->requestId = 0;
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

QCStatus_e OpticalFlowConfigIfs::VerifyStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;

    if ( ( dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 ) != QC_IMAGE_FORMAT_NV12 ) and
         ( dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 ) != QC_IMAGE_FORMAT_NV12_UBWC ) )
    {
        errors += "invalid image format, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    if ( static_cast<MotionMapFormat_e>(
                 dt.Get<uint8_t>( "motionMapFormat", MOTION_FORMAT_12_LA ) ) >= MOTION_FORMAT_MAX )
    {
        errors += "invalid motion map format, ";
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


    MotionMapUpscale_e motionMapUpscale = static_cast<MotionMapUpscale_e>(
            dt.Get<uint8_t>( "motionMapUpscale", MOTION_MAP_UPSCALE_NONE ) );
    if ( motionMapUpscale >= MOTION_MAP_UPSCALE_MAX )
    {
        errors += "motion map up scale out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    MotionDirection_e motionDirection = static_cast<MotionDirection_e>(
            dt.Get<uint8_t>( "motionDirection", MOTION_DIRECTION_FORWARD ) );
    if ( motionDirection >= MOTION_DIRECTION_MAX )
    {
        errors += "motion direction out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    MotionMapStepSize_e motionMapStepSize = static_cast<MotionMapStepSize_e>(
            dt.Get<uint8_t>( "motionMapStepSize", MOTION_MAP_STEP_SIZE_1 ) );
    if ( motionMapStepSize >= MOTION_MAP_STEP_SIZE_MAX )
    {
        errors += "motion map step size out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    RefinementLevel_e refinementLevel = static_cast<RefinementLevel_e>(
            dt.Get<uint8_t>( "refinementLevel", REFINEMENT_LEVEL_REFINED_L1 ) );
    if ( refinementLevel >= REFINEMENT_LEVEL_MAX )
    {
        errors += "refinement level out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    ComputationAccuracy_e computationAccuracy = static_cast<ComputationAccuracy_e>(
            dt.Get<uint8_t>( "computationAccuracy", COMPUTATION_ACCURACY_MEDIUM ) );
    if ( computationAccuracy >= COMPUTATION_ACCURACY_MAX )
    {
        errors += "computation accuracy out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t noiseScaleSrc = dt.Get<float32_t>( "noiseScaleSrc", 0.0f );
    if ( noiseScaleSrc < -1.0f or noiseScaleSrc > 1.0f )
    {
        errors += "feature noise tolerance for source image mulitplied by luma value, for when "
                  "noise increases along with luma out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t noiseScaleDst = dt.Get<float32_t>( "noiseScaleDst", 0.0f );
    if ( noiseScaleDst < -1.0f or noiseScaleDst > 1.0f )
    {
        errors += "feature noise tolerance for destination image mulitplied by luma value, for "
                  "when noise increases along with luma out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t noiseOffsetSrc = dt.Get<float32_t>( "noiseOffsetSrc", 0.0f );
    if ( noiseOffsetSrc < -1.0f or noiseOffsetSrc > 1.0f )
    {
        errors += "feature noise tolerance for source image, for when noise is fixed across image "
                  "out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t noiseOffsetDst = dt.Get<float32_t>( "noiseOffsetDst", 0.0f );
    if ( noiseOffsetDst < -1.0f or noiseOffsetDst > 1.0f )
    {
        errors += "feature noise tolerance for destination image, for when noise is fixed across "
                  "image out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t motionVarianceTolerance = dt.Get<float32_t>( "motionVarianceTolerance", 0.36f );
    if ( motionVarianceTolerance < 0.0f or motionVarianceTolerance > 1.0f )
    {
        errors += "motion variance tolerance out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t occlusionTolerance = dt.Get<float32_t>( "occlusionTolerance", 0.5f );
    if ( occlusionTolerance < 0.0f or occlusionTolerance > 1.0f )
    {
        errors += "occlusion tolerance out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t initialPenalty = dt.Get<float32_t>( "initialPenalty", 0.19f );
    if ( initialPenalty < -1.0f or initialPenalty > 1.0f )
    {
        errors += "initial penalty value out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t edgePenalty = dt.Get<float32_t>( "edgePenalty", 0.08f );
    if ( edgePenalty < -1.0f or edgePenalty > 1.0f )
    {
        errors += "edge penalty out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t smoothnessPenalty = dt.Get<float32_t>( "smoothnessPenalty", 0.41f );
    if ( smoothnessPenalty < -1.0f or smoothnessPenalty > 1.0f )
    {
        errors += "smoothness penalty out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t neighborPenalty = dt.Get<float32_t>( "neighborPenalty", 0.22f );
    if ( neighborPenalty < -1.0f or neighborPenalty > 1.0f )
    {
        errors += "neightbour penalty out of range, ";
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

    uint32_t motionVarianceMetric = dt.Get<uint32_t>( "motionVarianceMetric", 100 );
    if ( motionVarianceMetric < 0 or motionVarianceMetric > 100 )
    {
        errors += "motion variance metric out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    uint32_t occlusionMetric = dt.Get<uint32_t>( "occlusionMetric", 100 );
    if ( occlusionMetric < 0 or occlusionMetric > 100 )
    {
        errors += "occlusion metric out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t segmentationThreshold = dt.Get<float32_t>( "segmentationThreshold", 0.5f );
    if ( segmentationThreshold < 0.0f or segmentationThreshold > 1.0f )
    {
        errors += "segmentation threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t imageSharpnessThreshold = dt.Get<float32_t>( "imageSharpnessThreshold", 0.4f );
    if ( imageSharpnessThreshold < 0.0f or imageSharpnessThreshold > 1.0f )
    {
        errors += "image sharpness threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t mvEdgeThreshold = dt.Get<float32_t>( "mvEdgeThreshold", 0.43f );
    if ( mvEdgeThreshold < 0.0f or mvEdgeThreshold > 1.0f )
    {
        errors += "mv edge threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t refinementThreshold = dt.Get<float32_t>( "refinementThreshold", 0.75f );
    if ( refinementThreshold < 0.0f or refinementThreshold > 1.0f )
    {
        errors += "refinement threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    float32_t textureThreshold = dt.Get<float32_t>( "textureThreshold", 0.167f );
    if ( textureThreshold < 0.0f or textureThreshold > 1.0f )
    {
        errors += "texture threshold out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }


    LightingCondition_e lightingCondition = static_cast<LightingCondition_e>(
            dt.Get<uint8_t>( "lightingCondition", LIGHTING_CONDITION_HIGH ) );
    if ( lightingCondition >= LIGHTING_CONDITION_MAX )
    {
        errors += "lighting condition out of range, ";
        status = QC_STATUS_BAD_ARGUMENTS;
    }

    return status;
}


QCStatus_e OpticalFlowConfigIfs::ParseStaticConfig( DataTree &dt, std::string &errors )
{
    QCStatus_e status = VerifyStaticConfig( dt, errors );

    if ( QC_STATUS_OK == status )
    {
        m_config.imageFormat = dt.GetImageFormat( "format", QC_IMAGE_FORMAT_NV12 );
        m_config.motionMapFormat = static_cast<MotionMapFormat_e>(
                dt.Get<uint8_t>( "motionMapFormat", MOTION_FORMAT_12_LA ) );
        m_config.width = dt.Get<uint32_t>( "width", 0 );
        m_config.height = dt.Get<uint32_t>( "height", 0 );
        m_config.frameRate = dt.Get<uint32_t>( "fps", 30 );
        m_config.motionMapFracEn = dt.Get<bool>( "motionMapFracEn", true );
        m_config.motionMapUpscale = static_cast<MotionMapUpscale_e>(
                dt.Get<uint8_t>( "motionMapUpscale", MOTION_MAP_UPSCALE_NONE ) );
        m_config.motionDirection = static_cast<MotionDirection_e>(
                dt.Get<uint8_t>( "motionDirection", MOTION_DIRECTION_FORWARD ) );
        m_config.motionMapStepSize = static_cast<MotionMapStepSize_e>(
                dt.Get<uint8_t>( "motionMapStepSize", MOTION_MAP_STEP_SIZE_1 ) );
        m_config.confidenceOutputEn = dt.Get<bool>( "confidenceOutputEn", false );
        m_config.refinementLevel = static_cast<RefinementLevel_e>(
                dt.Get<uint8_t>( "refinementLevel", REFINEMENT_LEVEL_REFINED_L1 ) );
        m_config.chromaProcEn = dt.Get<bool>( "chromaProcEn", false );
        m_config.maskLowTextureEn = dt.Get<bool>( "maskLowTextureEn", false );
        m_config.computationAccuracy = static_cast<ComputationAccuracy_e>(
                dt.Get<uint8_t>( "computationAccuracy", COMPUTATION_ACCURACY_MEDIUM ) );
        m_config.noiseScaleSrc = dt.Get<float32_t>( "noiseScaleSrc", 0.0f );
        m_config.noiseScaleDst = dt.Get<float32_t>( "noiseScaleDst", 0.0f );
        m_config.noiseOffsetSrc = dt.Get<float32_t>( "noiseOffsetSrc", 0.0f );
        m_config.noiseOffsetDst = dt.Get<float32_t>( "noiseOffsetDst", 0.0f );
        m_config.motionVarianceTolerance = dt.Get<float32_t>( "motionVarianceTolerance", 0.36f );
        m_config.occlusionTolerance = dt.Get<float32_t>( "occlusionTolerance", 0.5f );
        m_config.initialPenalty = dt.Get<float32_t>( "initialPenalty", 0.19f );
        m_config.edgePenalty = dt.Get<float32_t>( "edgePenalty", 0.08f );
        m_config.smoothnessPenalty = dt.Get<float32_t>( "smoothnessPenalty", 0.41f );
        m_config.neighborPenalty = dt.Get<float32_t>( "neighborPenalty", 0.22f );
        m_config.textureMetric = dt.Get<uint32_t>( "textureMetric", 100 );
        m_config.edgeAlignMetric = dt.Get<uint32_t>( "edgeAlignMetric", 100 );
        m_config.motionVarianceMetric = dt.Get<uint32_t>( "motionVarianceMetric", 100 );
        m_config.occlusionMetric = dt.Get<uint32_t>( "occlusionMetric", 100 );
        m_config.segmentationThreshold = dt.Get<float32_t>( "segmentationThreshold", 0.5f );
        m_config.imageSharpnessThreshold = dt.Get<float32_t>( "imageSharpnessThreshold", 0.4f );
        m_config.mvEdgeThreshold = dt.Get<float32_t>( "mvEdgeThreshold", 0.43f );
        m_config.refinementThreshold = dt.Get<float32_t>( "refinementThreshold", 0.75f );
        m_config.textureThreshold = dt.Get<float32_t>( "textureThreshold", 0.167f );
        m_config.lightingCondition = static_cast<LightingCondition_e>(
                dt.Get<uint8_t>( "lightingCondition", LIGHTING_CONDITION_HIGH ) );
        m_config.isFirstRequest = dt.Get<bool>( "isFirstRequest", false );
        m_config.requestId = dt.Get<uint64_t>( "requestId", 0 );
    }

    return status;
}


QCStatus_e OpticalFlowConfigIfs::ApplyDynamicConfig( DataTree &dt, std::string &errors )
{
    // TBD in phase 2
    QCStatus_e status = QC_STATUS_OK;
    return status;
}

QCStatus_e OpticalFlowConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
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

const std::string &OpticalFlowConfigIfs::GetOptions()
{
    return NodeConfigIfs::s_QC_STATUS_UNSUPPORTED;
}


PixelFormat OpticalFlow::GetInputImageFormat( QCImageFormat_e imageFormat )
{
    PixelFormat format = SV::PixelFormat::NV12;

    switch ( imageFormat )
    {
        case QC_IMAGE_FORMAT_NV12:
        {
            format = SV::PixelFormat::NV12;
            break;
        }
        case QC_IMAGE_FORMAT_NV12_UBWC:
        {
            format = SV::PixelFormat::NV12_UBWC;
            break;
        }
        default:
        {
            QC_ERROR( "OpticalFlow: Unsupport color format: %d. Setting defalut color format",
                      imageFormat );
            break;
        }
    }

    return format;
}


PixelFormat OpticalFlow::GetMotionMapFormat( MotionMapFormat_e motionMapFormat )
{
    PixelFormat format = PixelFormat::MOTION_MAP_12_LA;

    switch ( motionMapFormat )
    {
        case MOTION_FORMAT_12_LA:
        {
            format = PixelFormat::MOTION_MAP_12_LA;
            break;
        }
        case MOTION_FORMAT_12_LA_UBWC:
        {
            format = PixelFormat::MOTION_MAP_12_LA_UBWC;
            break;
        }
        case MOTION_FORMAT_12_LA_PLANAR:
        {
            format = PixelFormat::MOTION_MAP_12_LA_PLANAR;
            break;
        }
        case MOTION_FORMAT_12_LA_PLANAR_UBWC:
        {
            format = PixelFormat::MOTION_MAP_12_LA_PLANAR_UBWC;
            break;
        }
        case MOTION_FORMAT_12_MA:
        {
            format = PixelFormat::MOTION_MAP_12_MA;
            break;
        }
        case MOTION_FORMAT_12_MA_UBWC:
        {
            format = PixelFormat::MOTION_MAP_12_MA_UBWC;
            break;
        }
        case MOTION_FORMAT_12_MA_PLANAR:
        {
            format = PixelFormat::MOTION_MAP_12_MA_PLANAR;
            break;
        }
        case MOTION_FORMAT_12_MA_PLANAR_UBWC:
        {
            format = PixelFormat::MOTION_MAP_12_MA_PLANAR_UBWC;
            break;
        }
        case MOTION_FORMAT_16:
        {
            format = PixelFormat::MOTION_MAP_16;
            break;
        }
        case MOTION_FORMAT_16_UBWC:
        {
            format = PixelFormat::MOTION_MAP_16_UBWC;
            break;
        }
        default:
        {
            QC_ERROR( "OpticalFlow: Unsupport color format: %d. Setting default color format",
                      motionMapFormat );
            break;
        }
    }

    return format;
}


void OpticalFlow::UpdateIconfig( LME::ConfigMap &configMap, OpticalFlow_Config_t configuration )
{

    configMap.Set( LME::ConfigId::AVERAGE_FPS, configuration.frameRate );
    configMap.Set( LME::ConfigId::SRC_IMAGE_INFO, &m_imageInfo );
    configMap.Set( LME::ConfigId::DST_IMAGE_INFO, &m_imageInfo );
    configMap.Set( LME::ConfigId::MOTION_MAP_FORMAT,
                   GetMotionMapFormat( configuration.motionMapFormat ) );
    configMap.Set( LME::ConfigId::MOTION_MAP_FRAC_EN, configuration.motionMapFracEn );

    if ( configuration.motionMapUpscale == MOTION_MAP_UPSCALE_NONE )
    {
        configMap.Set( LME::ConfigId::MOTION_MAP_UPSCALE, LME::MotionMapUpscale::NONE );
    }
    else if ( configuration.motionMapUpscale == MOTION_MAP_UPSCALE_2 )
    {
        configMap.Set( LME::ConfigId::MOTION_MAP_UPSCALE, LME::MotionMapUpscale::UPSCALE_2 );
    }
    else
    {
        configMap.Set( LME::ConfigId::MOTION_MAP_UPSCALE, LME::MotionMapUpscale::UPSCALE_4 );
    }

    if ( configuration.motionMapStepSize == MOTION_MAP_STEP_SIZE_1 )
    {
        configMap.Set( LME::ConfigId::MOTION_MAP_STEP_SIZE, LME::MotionMapStepSize::STEP_1 );
    }
    else if ( configuration.motionMapStepSize == MOTION_MAP_STEP_SIZE_2 )
    {
        configMap.Set( LME::ConfigId::MOTION_MAP_STEP_SIZE, LME::MotionMapStepSize::STEP_2 );
    }
    else
    {
        configMap.Set( LME::ConfigId::MOTION_MAP_STEP_SIZE, LME::MotionMapStepSize::STEP_4 );
    }

    if ( configuration.motionDirection == MOTION_DIRECTION_FORWARD )
    {
        configMap.Set( LME::ConfigId::MOTION_DIRECTION, LME::MotionDirection::FORWARD );
    }
    else if ( configuration.motionDirection == MOTION_DIRECTION_BACKWARD )
    {
        configMap.Set( LME::ConfigId::MOTION_DIRECTION, LME::MotionDirection::BACKWARD );
    }
    else
    {
        configMap.Set( LME::ConfigId::MOTION_DIRECTION, LME::MotionDirection::BIDIRECTIONAL );
    }

    configMap.Set( LME::ConfigId::CONFIDENCE_OUTPUT_EN, configuration.confidenceOutputEn );

    if ( configuration.refinementLevel == REFINEMENT_LEVEL_NONE )
    {
        configMap.Set( LME::ConfigId::REFINEMENT_LEVEL, LME::RefinementLevel::NONE );
    }
    else
    {
        configMap.Set( LME::ConfigId::REFINEMENT_LEVEL, LME::RefinementLevel::REFINED_L1 );
    }

    configMap.Set( LME::ConfigId::CHROMA_PROC_EN, configuration.chromaProcEn );
    configMap.Set( LME::ConfigId::MASK_LOW_TEXTURE_EN, configuration.maskLowTextureEn );

    if ( configuration.computationAccuracy == COMPUTATION_ACCURACY_LOW )
    {
        configMap.Set( LME::ConfigId::COMPUTATION_ACCURACY, LME::ComputationAccuracy::LOW );
    }
    else if ( configuration.computationAccuracy == COMPUTATION_ACCURACY_MEDIUM )
    {
        configMap.Set( LME::ConfigId::COMPUTATION_ACCURACY, LME::ComputationAccuracy::MEDIUM );
    }
    else
    {
        configMap.Set( LME::ConfigId::COMPUTATION_ACCURACY, LME::ComputationAccuracy::HIGH );
    }
}

QCStatus_e OpticalFlow::ValidateImageDesc( const ImageDescriptor_t &imgDesc,
                                           const OpticalFlow_Config_t &config )
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

void OpticalFlow::SetInitialFrameConfig( LME::ConfigMap &configMapFrame,
                                         OpticalFlow_Config_t configuration )
{

    noiseTolerances.nScaleSrc = configuration.noiseScaleSrc;
    noiseTolerances.nScaleDst = configuration.noiseScaleDst;
    noiseTolerances.nOffsetSrc = configuration.noiseOffsetSrc;
    noiseTolerances.nOffsetDst = configuration.noiseOffsetDst;
    penalties.nInitialPenalty = configuration.initialPenalty;
    penalties.nEdgePenalty = configuration.edgePenalty;
    penalties.nNeighborPenalty = configuration.neighborPenalty;
    penalties.nSmoothnessPenalty = configuration.smoothnessPenalty;
    configMapFrame.Set( LME::ConfigId::NOISE_TOLERANCES, &noiseTolerances );
    configMapFrame.Set( LME::ConfigId::MOTION_VARIANCE_TOLERANCE,
                        configuration.motionVarianceTolerance );
    configMapFrame.Set( LME::ConfigId::OCCLUSION_TOLERANCE, configuration.occlusionTolerance );
    configMapFrame.Set( LME::ConfigId::PENALTIES, &penalties );
    configMapFrame.Set( LME::ConfigId::TEXTURE_METRIC, configuration.textureMetric );
    configMapFrame.Set( LME::ConfigId::EDGE_ALIGN_METRIC, configuration.edgeAlignMetric );
    configMapFrame.Set( LME::ConfigId::MOTION_VARIANCE_METRIC, configuration.motionVarianceMetric );
    configMapFrame.Set( LME::ConfigId::OCCLUSION_METRIC, configuration.occlusionMetric );
    configMapFrame.Set( LME::ConfigId::SEGMENTATION_THRESHOLD,
                        configuration.segmentationThreshold );
    configMapFrame.Set( LME::ConfigId::IMAGE_SHARPNESS_THRESHOLD,
                        configuration.imageSharpnessThreshold );
    configMapFrame.Set( LME::ConfigId::MV_EDGE_THRESHOLD, configuration.mvEdgeThreshold );
    configMapFrame.Set( LME::ConfigId::REFINEMENT_THRESHOLD, configuration.refinementThreshold );
    configMapFrame.Set( LME::ConfigId::TEXTURE_THRESHOLD, configuration.textureThreshold );
    if ( configuration.lightingCondition == LIGHTING_CONDITION_LOW )
    {
        configMapFrame.Set( LME::ConfigId::LIGHTING_CONDITION, LME::LightingCondition::LOW );
    }
    else
    {
        configMapFrame.Set( LME::ConfigId::LIGHTING_CONDITION, LME::LightingCondition::HIGH );
    }
    configMapFrame.Set( LME::ConfigId::IS_FIRST_REQUEST, configuration.isFirstRequest );
    configMapFrame.Set( LME::ConfigId::REQUEST_ID, configuration.requestId );
}

QCStatus_e OpticalFlow::RegisterMemory( const BufferDescriptor_t &bufferDesc, Buffer &pBuff )
{
    QCStatus_e ret = QC_STATUS_OK;
    Status eStatus = Status::EFAIL;
    Buffer buff;

    if ( bufferDesc.GetDataPtr() == nullptr )
    {
        QC_ERROR( "OpticalFlow: Buffer data ptr is nullptr" );
        ret = QC_STATUS_INVALID_BUF;
    }
    else
    {
        auto it = m_memMap.find( bufferDesc.GetDataPtr() );
        if ( it == m_memMap.end() )
        {
            QC_DEBUG( "OpticalFlow: Buffer not found in map" );
            QC_DEBUG( "OpticalFlow: Buffer size %d", bufferDesc.GetDataSize() );
            QC_DEBUG( "OpticalFlow: Buffer offset %d", bufferDesc.offset );
            QC_DEBUG( "OpticalFlow: Buffer dmahandle %lu", bufferDesc.dmaHandle );

            buff.nSize = bufferDesc.GetDataSize();
            buff.nOffset = bufferDesc.offset;
            buff.pAddress = bufferDesc.GetDataPtr();
            buff.hHandle = (BufferHandle) bufferDesc.dmaHandle;

            eStatus = BufferRegister( m_session, buff );
            if ( eStatus != Status::SUCCESS )
            {
                QC_ERROR( "OpticalFlow: memory register(%p) failed: %d", bufferDesc.GetDataPtr(),
                          eStatus );
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

QCStatus_e OpticalFlow::Initialize( QCNodeInit_t &config )
{
    QCStatus_e ret = QC_STATUS_OK;

    std::string errors;
    Status eStatus = Status::SUCCESS;
    PixelFormat format;
    const OpticalFlow_Config_t &configuration =
            dynamic_cast<const OpticalFlow_Config_t &>( GetConfigurationIfs().Get() );

    ret = GetConfigurationIfs().VerifyAndSet( config.config, errors );

    if ( ret == QC_STATUS_OK )
    {
        ret = NodeBase::Init( GetConfigurationIfs().Get().nodeId );
    }
    else
    {
        QC_ERROR( "OpticalFlow: config error: %s", errors.c_str() );
    }

    if ( ret == QC_STATUS_OK )
    {
        std::string sessionName{ "OPTICAL_FLOW" };
        m_sessionConfigMap.Set( Session::ConfigId::NAME, &sessionName );
        m_sessionConfigMap.Set( Session::ConfigId::TYPE, Session::Type::CV );
        m_sessionConfigMap.Set( Session::ConfigId::PRIORITY, Session::Priority::REALTIME_MED );

        format = GetInputImageFormat( configuration.imageFormat );

        eStatus = ImageInfoQuery( format, configuration.width, configuration.height, m_imageInfo );
        if ( eStatus != Status::SUCCESS )
        {
            QC_ERROR( "OpticalFlow: Failed to get image info: %d", eStatus );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "OpticalFlow: Successfully retreived image info" );
        }
    }
    else
    {
        QC_ERROR( "OpticalFlow: Failure to initialize node" );
    }

    if ( ret == QC_STATUS_OK )
    {
        m_session = Session::Create( m_sessionConfigMap, nullptr, nullptr );
        if ( m_session == nullptr )
        {
            QC_ERROR( "OpticalFlow: Failed to create session" );
            ret = QC_STATUS_FAIL;
        }
        else
        {
            QC_DEBUG( "OpticalFlow: Successfully created session %d", ret );
        }
    }

    if ( ret == QC_STATUS_OK )
    {
        UpdateIconfig( m_configMap, configuration );
        m_state = QC_OBJECT_STATE_READY;
    }


    if ( ret == QC_STATUS_OK )
    {
        m_Lme = LME::Create( m_session, m_configMap );
        if ( m_Lme == nullptr )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "OpticalFlow: LME creation failed %d", ret );
            Status status = m_session->Destroy();
            if ( status != Status::SUCCESS )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "OpticalFlow: Session destroy failed %d", ret );
            }
            m_session = nullptr;
        }
    }

    return ret;
}

QCStatus_e OpticalFlow::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::SUCCESS;
    const OpticalFlow_Config_t &configuration =
            dynamic_cast<const OpticalFlow_Config_t &>( GetConfigurationIfs().Get() );

    if ( m_state != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "OpticalFlow: Start is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_session->Start();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "OpticalFlow: Session start failed %d", ret );
        }
        if ( ret == QC_STATUS_OK )
        {
            SetInitialFrameConfig( m_configMap, configuration );
            m_state = QC_OBJECT_STATE_RUNNING;
        }
    }

    return ret;
}

QCStatus_e OpticalFlow::ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc )
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::EFAIL;
    Buffer refMem;
    Buffer curMem;
    Buffer fwdMvMem;
    Buffer bwdMvMem;
    Buffer fwdConfMem;
    Buffer bwdConfMem;
    LME::Output sOutputFrwd;
    LME::Output sOutputBkwd;
    LME::Input sInput;

    if ( m_state != QC_OBJECT_STATE_RUNNING )
    {
        QC_ERROR( "OpticalFlow: Execute is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        const OpticalFlow_Config_t &configuration =
                dynamic_cast<const OpticalFlow_Config_t &>( GetConfigurationIfs().Get() );
        const OpticalFlowBufferMapping_t &bufferMap = configuration.bufferMap;

        const ImageDescriptor_t *pRefImgDesc = dynamic_cast<const ImageDescriptor_t *>(
                &frameDesc.GetBuffer( bufferMap.referenceImageBufferId ) );
        const ImageDescriptor_t *pCurImgDesc = dynamic_cast<const ImageDescriptor_t *>(
                &frameDesc.GetBuffer( bufferMap.currentImageBufferId ) );
        const TensorDescriptor_t *pFwdMvDesc = dynamic_cast<const TensorDescriptor_t *>(
                &frameDesc.GetBuffer( bufferMap.fwdMotionBufferId ) );
        const TensorDescriptor_t *pBwdMvDesc = dynamic_cast<const TensorDescriptor_t *>(
                &frameDesc.GetBuffer( bufferMap.bwdMotionBufferId ) );

        if ( pRefImgDesc == nullptr )
        {
            QC_ERROR( "invalid reference image buffer (id %u)", bufferMap.referenceImageBufferId );
            ret = QC_STATUS_FAIL;
        }

        if ( pCurImgDesc == nullptr )
        {
            QC_ERROR( "invalid current image buffer (id %u)", bufferMap.currentImageBufferId );
            ret = QC_STATUS_FAIL;
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = ValidateImageDesc( *pRefImgDesc, configuration );
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = ValidateImageDesc( *pCurImgDesc, configuration );
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = RegisterMemory( *pRefImgDesc, refMem );
        }

        if ( ret == QC_STATUS_OK )
        {
            ret = RegisterMemory( *pCurImgDesc, curMem );
        }

        if ( ret == QC_STATUS_OK )
        {
            if ( ( pFwdMvDesc != nullptr ) and
                 ( ( configuration.motionDirection == MOTION_DIRECTION_FORWARD ) or
                   ( configuration.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) ) )
            {
                ret = RegisterMemory( *pFwdMvDesc, fwdMvMem );
                if ( ret == QC_STATUS_OK )
                {
                    sOutputFrwd.sMotionMap.sBuffer = fwdMvMem;
                }
            }
        }

        if ( ret == QC_STATUS_OK )
        {
            if ( ( pBwdMvDesc != nullptr ) and
                 ( ( configuration.motionDirection == MOTION_DIRECTION_BACKWARD ) or
                   ( configuration.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) ) )
            {
                ret = RegisterMemory( *pBwdMvDesc, bwdMvMem );
                if ( ret == QC_STATUS_OK )
                {
                    sOutputBkwd.sMotionMap.sBuffer = bwdMvMem;
                }
            }
        }

        if ( ret == QC_STATUS_OK )
        {
            if ( configuration.confidenceOutputEn == true )
            {
                const TensorDescriptor_t *pFwdConfDesc = dynamic_cast<const TensorDescriptor_t *>(
                        &frameDesc.GetBuffer( bufferMap.fwdConfBufferId ) );
                const TensorDescriptor_t *pBwdConfDesc = dynamic_cast<const TensorDescriptor_t *>(
                        &frameDesc.GetBuffer( bufferMap.bwdConfBufferId ) );
                if ( ( configuration.motionDirection == MOTION_DIRECTION_FORWARD ) or
                     ( configuration.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
                {
                    if ( pFwdConfDesc == nullptr )
                    {
                        QC_ERROR( "invalid fwd confidence tensor buffer (id %u)",
                                  bufferMap.fwdConfBufferId );
                        ret = QC_STATUS_FAIL;
                    }
                    else
                    {
                        ret = RegisterMemory( *pFwdConfDesc, fwdConfMem );
                        if ( ret == QC_STATUS_OK )
                        {
                            sOutputFrwd.sConfMap.sBuffer = fwdConfMem;
                        }
                    }
                }
                if ( ( configuration.motionDirection == MOTION_DIRECTION_BACKWARD ) or
                     ( configuration.motionDirection == MOTION_DIRECTION_BIDIRECTIONAL ) )
                {
                    if ( pBwdConfDesc == nullptr )
                    {
                        QC_ERROR( "invalid bwd confidence tensor buffer (id %u)",
                                  bufferMap.bwdConfBufferId );
                        ret = QC_STATUS_FAIL;
                    }
                    else
                    {
                        ret = RegisterMemory( *pBwdConfDesc, bwdConfMem );
                        if ( ret == QC_STATUS_OK )
                        {
                            sOutputBkwd.sConfMap.sBuffer = bwdConfMem;
                        }
                    }
                }
            }
        }

        if ( ret == QC_STATUS_OK )
        {
            sInput.sSrcImage.sImageInfo = m_imageInfo;
            sInput.sDstImage.sImageInfo = m_imageInfo;
            sInput.sSrcImage.sBuffer = refMem;
            sInput.sDstImage.sBuffer = curMem;


            status = m_Lme->SubmitSync( sInput, sOutputFrwd, sOutputBkwd, m_configMap );
            if ( status != Status::SUCCESS )
            {
                QC_ERROR( "OpticalFlow: LME_Sync failed: %d", status );
                ret = QC_STATUS_FAIL;
            }
        }
    }
    return ret;
}

QCStatus_e OpticalFlow::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::SUCCESS;

    if ( m_state != QC_OBJECT_STATE_RUNNING )
    {
        QC_ERROR( "OpticalFlow: Stop is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_session->Stop();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "OpticalFlow: Failed to stop Session: %d", ret );
        }

        if ( ret == QC_STATUS_OK )
        {
            m_state = QC_OBJECT_STATE_READY;
        }
    }

    return ret;
}

QCStatus_e OpticalFlow::DeInitialize()
{
    QCStatus_e ret = QC_STATUS_OK;
    Status status = Status::SUCCESS;

    if ( m_state != QC_OBJECT_STATE_READY )
    {
        QC_ERROR( "OpticalFlow: Deinitialize is not allowed when in state %d!", m_state );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        status = m_Lme->Destroy();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "OpticalFlow: Failed to destroy stereo disparity: %d", ret );
        }
        for ( auto &it : m_memMap )
        {
            Buffer mem = it.second;
            status = BufferDeregister( m_session, mem );
            if ( status != Status::SUCCESS )
            {
                QC_ERROR( "OpticalFlow: Mem Deregister(%p) failed: %d", mem.pAddress, status );
                ret = QC_STATUS_FAIL;
            }
        }

        m_memMap.clear();
        status = m_session->Destroy();
        if ( status != Status::SUCCESS )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "OpticalFlow: Session destroy failed %d", ret );
        }
        m_session = nullptr;
    }

    return ret;
}

}   // namespace Node
}   // namespace QC