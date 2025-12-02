
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_NODE_DEPTH_FROM_STEREO_HPP
#define QC_NODE_DEPTH_FROM_STEREO_HPP

#include "svStereoDisparity.h"
#include "svUtils.h"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{

using namespace SV;

/** @brief The QCNode Depth from Stereo Version */
#define QCNODE_DFS_VERSION_MAJOR 2U
#define QCNODE_DFS_VERSION_MINOR 0U
#define QCNODE_DFS_VERSION_PATCH 0U

#define QCNODE_DFS_VERSION                                                                         \
    ( ( QCNODE_DFS_VERSION_MAJOR << 16U ) | ( QCNODE_DFS_VERSION_MINOR << 8U ) |                   \
      QCNODE_DFS_VERSION_PATCH )

typedef enum
{
    QC_NODE_DFS_PRIMARY_IMAGE_BUFF_ID = 0,
    QC_NODE_DFS_AUXILARY_IMAGE_BUFF_ID = 1,
    QC_NODE_DFS_DISPARITY_MAP_BUFF_ID = 2,
    QC_NODE_DFS_DISPARITY_CONFIDANCE_MAP_BUFF_ID = 3,
    QC_NODE_DFS_LAST_BUFF_ID = 4
} QCNodeDepthFromStereoBufferId_e;


typedef struct DepthFromStereoBufferMapping
{
    DepthFromStereoBufferMapping()
        : primaryImageBufferId( QC_NODE_DFS_PRIMARY_IMAGE_BUFF_ID ),
          auxilaryImageBufferId( QC_NODE_DFS_AUXILARY_IMAGE_BUFF_ID ),
          disparityMapBufferId( QC_NODE_DFS_DISPARITY_MAP_BUFF_ID ),
          disparityConfidenceMapBufferId( QC_NODE_DFS_DISPARITY_CONFIDANCE_MAP_BUFF_ID ){};

    uint32_t primaryImageBufferId;
    uint32_t auxilaryImageBufferId;
    uint32_t disparityMapBufferId;
    uint32_t disparityConfidenceMapBufferId;
} DepthFromStereoBufferMapping_t;

/** @brief DFS output map format. */
typedef enum : uint8_t
{
    DISP_FORMAT_P012_LA_Y_ONLY = 0,
    DISP_FORMAT_MAX
} DisparityFormat_e;

/** @brief DFS processing mode. */
typedef enum : uint8_t
{
    PROCESSING_MODE_AUTO = 0,
    PROCESSING_MODE_DL,
    PROCESSING_MODE_SGM,
    PROCESSING_MODE_MAX
} ProcessingMode_e;

/** @brief Disparity map precision type. */
typedef enum : uint8_t
{
    DISP_MAP_PRECISION_FRAC_6BIT = 0,
    DISP_MAP_PRECISION_FRAC_4BIT,
    DISP_MAP_PRECISION_INT,
    DISP_MAP_PRECISION_MAX
} DispMapPrecision_e;

/** @brief DFS refinement level. */
typedef enum : uint8_t
{
    REFINEMENT_LEVEL_NONE = 0,
    REFINEMENT_LEVEL_REFINED_L1,
    REFINEMENT_LEVEL_REFINED_L2,
    REFINEMENT_LEVEL_MAX
} RefinementLevel_e;

/** @brief DFS search direction. */
typedef enum : uint8_t
{
    SEARCH_DIRECTION_L2R = 0,
    SEARCH_DIRECTION_R2L,
    SEARCH_DIRECTION_MAX
} SearchDirection_e;


/** @brief DepthFromStereo component configuration */
typedef struct DepthFromStereo_Config : public QCNodeConfigBase_t
{
    QCImageFormat_e imageFormat;
    DisparityFormat_e disparityFormat;
    uint32_t width;
    uint32_t height;
    uint32_t frameRate;
    bool confidenceOutputEn;
    ProcessingMode_e processingMode;
    bool isFirstRequest;
    float32_t noiseOffsetPri;
    float32_t noiseOffsetAux;
    uint8_t modelType;
    uint8_t modelSwitchFrameCount;
    float32_t prevDisparityFactor;
    DispMapPrecision_e disparityMapPrecision;
    RefinementLevel_e refinementLevel;
    bool occlusionOutputEn;
    bool disparityStatsEn;
    bool rectificationErrorStatsEn;
    bool chromaProcEN;
    bool maskLowTextureEn;
    uint32_t confidenceThreshold;
    uint32_t disparityThreshold;
    float32_t noiseScalePri;
    float32_t noiseScaleAux;
    float32_t rectificationErrTolerance;
    float32_t segmentationThreshold;
    float32_t textureThreshold;
    SearchDirection_e searchDirection;
    float32_t edgePenalty;
    float32_t initialPenalty;
    float32_t neighborPenalty;
    float32_t smoothnessPenalty;
    float32_t imageSharpnessThreshold;
    float32_t farAwayDisparityLimit;
    float32_t disparityEdgeThreshold;
    uint32_t matchingCostMetric;
    uint32_t textureMetric;
    uint32_t edgeAlignMetric;
    uint32_t disparityVarianceMetric;
    uint32_t occlusionMetric;
    float32_t disparityVarianceTolerance;
    float32_t occlusionTolerance;
    float32_t refinementThreshold;
    uint32_t maxDisparityRange;
    DepthFromStereoBufferMapping_t bufferMap;

public:
    DepthFromStereo_Config();
    DepthFromStereo_Config( const DepthFromStereo_Config &rhs );
    DepthFromStereo_Config &operator=( const DepthFromStereo_Config &rhs );

} DepthFromStereo_Config_t;


class DepthFromStereoConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief NodeDepthFromStereoConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by
     * NodeDepthFromStereoConfigIfs.
     * @return None
     */
    DepthFromStereoConfigIfs( Logger &logger ) : NodeConfigIfs( logger ) {}

    /**
     * @brief NodeDepthFromStereoConfigIfs Destructor
     * @return None
     */
    ~DepthFromStereoConfigIfs() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * @param[in] config The configuration string.
     * @param[out] errors The error string returned if there is an error.
     * @note The config is a JSON string according to the templates below.
     *   1. Static configuration used for initialization:
     *   {
     *     "static": {
     *        "name": "The Node unique name, type: string, default: 'unnamed'",
     *        "id": "The Node unique ID, type: uint32_t, default: 0",
     *        "width": "The width in pixels of the I/O frame, type: uint32_t, default: 0",
     *        "height": "The height in pixels of the I/O frame, type: uint32_t, default: 0",
     *        "frameRate": "Input frames per second, type: uint32_t, default: 30",
     *        "format": "Input image format (e.g., NV12, NV12_UBWC, P010), type: uint32_t, default:nv12",
     *        "confidenceOutputEn": "Enable confidence output, type: bool, default: false",
     *        "processingMode": "Processing mode selector, type: ProcessingMode_e, default: PROCESSING_MODE_AUTO",
     *        "isFirstRequest": "Indicates whether this is the first request, type: bool, default:true",
     *        "noiseOffsetPrimary": "Primary noise offset, type: float32_t, default: 0.0f",
     *        "noiseOffsetAux": "Auxiliary noise offset, type: float32_t, default: 0.0f",
     *        "modelType": "Model type used for disparity computation (0-4), type: uint8_t, default:1",
     *        "modelSwitchFrameCount": "Minimum frames before model switch, type: uint8_t, default:10",
     *        "prevDisparityFactor": "Previous disparity factor for smoothing, type: float32_t, default: 1.0f",
     *        "disparityMapPrecision": "Precision level for disparity map, type: DispMapPrecision_e, default: DISP_MAP_PRECISION_FRAC_6BIT",
     *        "refinementLevel": "Refinement level applied during disparity estimation, type: RefinementLevel_e, default: REFINEMENT_LEVEL_REFINED_L2",
     *        "occlusionOutputEn": "Enable occlusion output, type: bool, default: false",
     *        "disparityStatsEn": "Enable disparity statistics tracking, type: bool, default: false",
     *        "rectificationErrorStatsEn": "Enable rectification error statistics, type: bool, default: false",
     *        "chromaProcEN": "Enable chrominance processing, type: bool, default: false",
     *        "maskLowTextureEn": "Enable masking of low-texture regions, type: bool, default: false",
     *        "confidenceThreshold": "Minimum confidence threshold, type: uint32_t, default: 210",
     *        "disparityThreshold": "Maximum disparity threshold, type: uint32_t, default: 64",
     *        "noiseScalePri": "Primary noise scaling factor, type: float32_t, default: 1.0f",
     *        "noiseScaleAux": "Auxiliary noise scaling factor, type: float32_t, default: 1.0f",
     *        "rectificationErrTolerance": "Rectification error tolerance, type: float32_t, default: 0.29",
     *        "segmentationThreshold": "Segmentation threshold for region detection, type: float32_t, default: 0.5f",
     *        "textureThreshold": "Texture threshold for feature detection, type: float32_t, default: 0.167f",
     *        "searchDirection": "Search direction for stereo matching (0=L2R, 1=R2L), type: SearchDirection_e, default: SEARCH_DIRECTION_L2R",
     *        "edgePenalty": "Penalty applied to edges during matching, type: float32_t, default: 0.0f",
     *        "initialPenalty": "Initial penalty for disparity propagation, type: float32_t, default: 0.0f",
     *        "neighborPenalty": "Penalty based on neighboring disparities, type: float32_t, default: 0.0f",
     *        "smoothnessPenalty": "Smoothness constraint penalty, type: float32_t, default: 0.0f",
     *        "imageSharpnessThreshold": "Threshold for image sharpness checks, type: float32_t, default: 0.4f",
     *        "farAwayDisparityLimit": "Maximum disparity allowed for distant objects, type: float32_t, default: 0.0f",
     *        "disparityEdgeThreshold": "Threshold for disparity edge detection, type: float32_t, default: 0.43f",
     *        "matchingCostMetric": "Cost metric used in matching algorithm, type: uint32_t, default: 100",
     *        "textureMetric": "Texture-based cost metric, type: uint32_t, default: 100",
     *        "edgeAlignMetric": "Edge alignment metric used in SGM, type: uint32_t, default: 100",
     *        "disparityVarianceMetric": "Variance metric used for disparity consistency, type: uint32_t, default: 100",
     *        "occlusionMetric": "Occlusion detection metric, type: uint32_t, default: 100",
     *        "disparityVarianceTolerance": "Tolerance for disparity variance checks, type: float32_t, default: 0.36f",
     *        "occlusionTolerance": "Tolerance for occlusion detection, type: float32_t, default: 0.5f",
     *        "refinementThreshold": "Threshold for refinement steps, type: float32_t, default: 0.75f",
     *        "maxDisparityRange": "Maximum disparity range supported, type: uint32_t, default: 64" 
     *     }
     *   }
     * @return QC_STATUS_OK on success, other values on failure.
     */

    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get Configuration Options
     * @return A reference string to the JSON configuration options.
     * @note
     * TODO: Provide a more detailed introduction about the JSON configuration options.
     */
    virtual const std::string &GetOptions();

    /**
     * @brief Get the Configuration Structure.
     * @return A reference to the Configuration Structure.
     */
    virtual const QCNodeConfigBase_t &Get() { return m_config; };

private:
    QCStatus_e VerifyStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ApplyDynamicConfig( DataTree &dt, std::string &errors );

    DepthFromStereo_Config_t m_config;
    std::string m_options;
};


// TODO: how to handle NodeDepthFromStereoMonitorConfig
typedef struct DepthFromStereoMonitorConfig : public QCNodeMonitoringBase_t
{

} DepthFromStereoMonitorConfig_t;

// TODO: how to handle NodeDepthFromStereoMonitoringIfs
class DepthFromStereoMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    DepthFromStereoMonitoringIfs() {}
    ~DepthFromStereoMonitoringIfs() {}

    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    virtual const std::string &GetOptions() { return m_options; }

    virtual const QCNodeMonitoringBase_t &Get() { return m_config; };

    virtual uint32_t GetMaximalSize() { return UINT32_MAX; }
    virtual uint32_t GetCurrentSize() { return UINT32_MAX; }

    virtual QCStatus_e Place( void *ptr, uint32_t &size ) { return QC_STATUS_UNSUPPORTED; }

private:
    std::string m_options;
    DepthFromStereoMonitorConfig_t m_config;
};

class DepthFromStereo : public NodeBase
{
public:
    /**
     * @brief NodeDepthFromStereo Constructor
     * @return None
     */
    DepthFromStereo()
        : m_configIfs( m_logger ),
          m_callback( nullptr ),
          m_imageInfo(),
          sessionConfigMap(),
          configMap(),
          m_session( nullptr ),
          m_stereoDisparity( nullptr ){};

    /**
     * @brief NodeDepthFromStereo Destructor
     * @return None
     */
    ~DepthFromStereo(){};

    /**
     * @brief Initializes Node Depth From Stereo.
     * @param[in] config The Node Depth From Stereo configuration.
     * @note QCNodeInit::config - Refer to the comments of the API
     * NodeDepthFromStereoConfigIfs::VerifyAndSet.
     * @note QCNodeInit::callback - The user application callback to notify the status of
     * the API ProcessFrameDescriptor. This is optional. If provided, the API ProcessFrameDescriptor
     * will be asynchronous; otherwise, the API ProcessFrameDescriptor will be synchronous.
     * @note QCNodeInit::buffers - Buffers provided by the user application. The buffers can be
     * provided for two purposes:
     * - 1. A buffer provided to store the decrypted DepthFromStereo context binary (see
     *      NodeDepthFromStereoConfig::contextBufferId).
     * - 2. Shared buffers to be registered into Depth From Stereo (see the
     * NodeDepthFromStereoConfig::bufferIds class).
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node Depth From Stereo configuration interface.
     * @return A reference to the Node Depth From Stereo configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node Depth From Stereo monitoring interface.
     * @return A reference to the Node Depth From Stereo monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node Depth From Stereo
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the Node Depth From Stereo
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node Depth From Stereo
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the Node Depth From Stereo
     * @return The current state of the Node Depth From Stereo
     */
    virtual QCObjectState_e GetState() { return m_state; }

private:
    DepthFromStereoConfigIfs m_configIfs;
    QCNodeEventCallBack_t m_callback;
    DepthFromStereoMonitoringIfs m_monitorIfs;
    ImageInfo m_imageInfo;
    Session::ConfigMap sessionConfigMap;
    StereoDisparity::ConfigMap configMap;
    Session *m_session;
    StereoDisparity *m_stereoDisparity;
    QCObjectState_e m_state = QC_OBJECT_STATE_INITIAL;
    std::map<void *, Buffer> m_memMap;
    StereoDisparity::Penalties penalties;
    StereoDisparity::FeatureNoiseToleranceOffset noiseToleranceOffset;
    StereoDisparity::FeatureNoiseToleranceScale noiseToleranceScale;
    PixelFormat GetInputImageFormat( QCImageFormat_e imageFormat );
    PixelFormat GetDisparityMapFormat( DisparityFormat_e disparityFormat );
    void UpdateIconfig( StereoDisparity::ConfigMap &configMap,
                        DepthFromStereo_Config_t configuration );
    QCStatus_e ValidateImageDesc( const ImageDescriptor_t &imgDesc,
                                  const DepthFromStereo_Config_t &config );
    void SetInitialFrameConfig( StereoDisparity::ConfigMap &configMapFrame,
                                DepthFromStereo_Config_t configuration );
    QCStatus_e RegisterMemory( const BufferDescriptor_t &bufferDesc, Buffer &pBuff );
};


}   // namespace Node
}   // namespace QC


#endif   // QC_NODE_DEPTH_FROM_STEREO_HPP
