
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_NODE_OPTICAL_FLOW_HPP
#define QC_NODE_OPTICAL_FLOW_HPP

#include "svLme.h"
#include "svUtils.h"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{

using namespace SV;

/** @brief The QCNode OpticalFlow Version */
#define QCNODE_OFL_VERSION_MAJOR 2U
#define QCNODE_OFL_VERSION_MINOR 0U
#define QCNODE_OFL_VERSION_PATCH 0U

#define QCNODE_OFL_VERSION                                                                         \
    ( ( QCNODE_OFL_VERSION_MAJOR << 16U ) | ( QCNODE_OFL_VERSION_MINOR << 8U ) |                   \
      QCNODE_OFL_VERSION_PATCH )

typedef enum
{
    QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID = 0,
    QC_NODE_OF_CURRENT_IMAGE_BUFF_ID = 1,
    QC_NODE_OF_FWD_MOTION_BUFF_ID = 2,
    QC_NODE_OF_BWD_MOTION_BUFF_ID = 3,
    QC_NODE_OF_FWD_CONF_BUFF_ID = 4,
    QC_NODE_OF_BWD_CONF_BUFF_ID = 5,
    QC_NODE_OF_LAST_BUFF_ID = 6
} QCOpticalFlowBufferId_e;


typedef struct OpticalFlowBufferMapping
{
    OpticalFlowBufferMapping()
        : referenceImageBufferId( QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID ),
          currentImageBufferId( QC_NODE_OF_CURRENT_IMAGE_BUFF_ID ),
          fwdMotionBufferId( QC_NODE_OF_FWD_MOTION_BUFF_ID ),
          bwdMotionBufferId( QC_NODE_OF_BWD_MOTION_BUFF_ID ),
          fwdConfBufferId( QC_NODE_OF_FWD_CONF_BUFF_ID ),
          bwdConfBufferId( QC_NODE_OF_BWD_CONF_BUFF_ID ){};

    uint32_t referenceImageBufferId;
    uint32_t currentImageBufferId;
    uint32_t fwdMotionBufferId;
    uint32_t bwdMotionBufferId;
    uint32_t fwdConfBufferId;
    uint32_t bwdConfBufferId;
} OpticalFlowBufferMapping_t;


/** @brief Optical flow output format. */
typedef enum : uint8_t
{
    MOTION_FORMAT_12_LA = 0,
    MOTION_FORMAT_MAX
} MotionMapFormat_e;

/** @brief Motion upscale format. */
typedef enum : uint8_t
{
    MOTION_MAP_UPSCALE_NONE = 0,
    MOTION_MAP_UPSCALE_2,
    MOTION_MAP_UPSCALE_4,
    MOTION_MAP_UPSCALE_MAX
} MotionMapUpscale_e;

/** @brief Motion map direction. */
typedef enum : uint8_t
{
    MOTION_DIRECTION_FORWARD = 0,
    MOTION_DIRECTION_BACKWARD,
    MOTION_DIRECTION_BIDIRECTIONAL,
    MOTION_DIRECTION_MAX
} MotionDirection_e;

/** @brief Motion map step size. */
typedef enum : uint8_t
{
    MOTION_MAP_STEP_SIZE_1 = 0,
    MOTION_MAP_STEP_SIZE_2,
    MOTION_MAP_STEP_SIZE_4,
    MOTION_MAP_STEP_SIZE_MAX
} MotionMapStepSize_e;

/** @brief Refinement level. */
typedef enum : uint8_t
{
    REFINEMENT_LEVEL_NONE = 0,
    REFINEMENT_LEVEL_REFINED_L1,
    REFINEMENT_LEVEL_MAX
} RefinementLevel_e;

/** @brief Computation Accuracy. */
typedef enum : uint8_t
{
    COMPUTATION_ACCURACY_LOW = 0,
    COMPUTATION_ACCURACY_MEDIUM,
    COMPUTATION_ACCURACY_HIGH,
    COMPUTATION_ACCURACY_MAX
} ComputationAccuracy_e;

/** @brief Lighting condition. */
typedef enum : uint8_t
{
    LIGHTING_CONDITION_LOW = 0,
    LIGHTING_CONDITION_HIGH,
    LIGHTING_CONDITION_MAX
} LightingCondition_e;


/** @brief Optical Flow component configuration */
typedef struct OpticalFlow_Config : public QCNodeConfigBase_t
{
    QCImageFormat_e imageFormat;
    MotionMapFormat_e motionMapFormat;
    uint32_t width;
    uint32_t height;
    uint32_t frameRate;
    bool motionMapFracEn;
    MotionMapUpscale_e motionMapUpscale;
    MotionDirection_e motionDirection;
    MotionMapStepSize_e motionMapStepSize;
    bool confidenceOutputEn;
    RefinementLevel_e refinementLevel;
    bool chromaProcEn;
    bool maskLowTextureEn;
    ComputationAccuracy_e computationAccuracy;
    float32_t noiseScaleSrc;
    float32_t noiseScaleDst;
    float32_t noiseOffsetSrc;
    float32_t noiseOffsetDst;
    float32_t motionVarianceTolerance;
    float32_t occlusionTolerance;
    float32_t edgePenalty;
    float32_t initialPenalty;
    float32_t neighborPenalty;
    float32_t smoothnessPenalty;
    uint32_t textureMetric;
    uint32_t edgeAlignMetric;
    uint32_t motionVarianceMetric;
    uint32_t occlusionMetric;
    float32_t segmentationThreshold;
    float32_t globalMotionDetailThreshold;
    float32_t imageSharpnessThreshold;
    float32_t mvEdgeThreshold;
    float32_t refinementThreshold;
    float32_t textureThreshold;
    LightingCondition_e lightingCondition;
    bool isFirstRequest;
    uint64_t requestId;
    OpticalFlowBufferMapping_t bufferMap;

public:
    OpticalFlow_Config();
    OpticalFlow_Config( const OpticalFlow_Config &rhs );
    OpticalFlow_Config &operator=( const OpticalFlow_Config &rhs );

} OpticalFlow_Config_t;

class OpticalFlowConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief OpticalFlowConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by
     * OpticalFlowConfigIfs.
     * @return None
     */
    OpticalFlowConfigIfs( Logger &logger ) : NodeConfigIfs( logger ) {}

    /**
     * @brief OpticalFlowConfigIfs Destructor
     * @return None
     */
    ~OpticalFlowConfigIfs() {}

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
     *        "format": "Input image format (e.g., NV12, NV12_UBWC, P010), type: uint32_t, default: nv12",
     *        "motionMapFracEn": "Enable fractional bits of motion vector values, type: bool, default: true",
     *        "motionMapUpscale": "Config to decide upscaling of final motion vector output, type: uint8_t, default: MOTION_MAP_UPSCALE_NONE",
     *        "motionDirection": "Config to specify direction of local motion estimation, type: uint8_t, default: MOTION_DIRECTION_FORWARD",
     *        "motionMapStepSize": "Config to decide the sampling for motion map, type: uint8_t, default: MOTION_MAP_STEP_SIZE_1",
     *        "confidenceOutputEn": "Enable confidence map output, type: bool, default: false",
     *        "refinementLevel": "Refinement level for output motion map, type: uint8_t, default: REFINEMENT_LEVEL_REFINED_L1",
     *        "chromaProcEn": "Enable chrominance processing, type: bool, default: false",
     *        "maskLowTextureEn": "Enable masking of low-texture regions, type: bool, default: false",
     *        "computationAccuracy": "Defines the underlying implementation to use, type: uint8_t, default: COMPUTATION_ACCURACY_MEDIUM",
     *        "noiseScaleSrc": "Source image noise scaling factor, type: float32_t, default: 1.0f",
     *        "noiseScaleDst": "Destination image noise scaling factor, type: float32_t, default: 1.0f",
     *        "noiseOffsetSrc": "Source image noise offset, type: float32_t, default: 0.0f",
     *        "noiseOffsetDst": "Destination image noise offset, type: float32_t, default: 0.0f",
     *        "motionVarianceTolerance": "Motion variance tolerance, type: float32_t, default: 0.36f",
     *        "occlusionTolerance": "Occlusion tolerance, type: float32_t, default: 0.5f",
     *        "edgePenalty": "Edge penalty for motion calculation, type: float32_t, default: 0.0f",
     *        "initialPenalty": "Initial penalty for motion propagation, type: float32_t, default: 0.0f",
     *        "neighborPenalty": "Neighbor penalty for motion calculation, type: float32_t, default: 0.0f",
     *        "smoothnessPenalty": "Smoothness constraint penalty, type: float32_t, default: 0.0f",
     *        "textureMetric": "Texture quality importance metric, type: uint32_t, default: 100",
     *        "edgeAlignMetric": "Edge alignment importance metric, type: uint32_t, default: 100",
     *        "motionVarianceMetric": "Motion variance importance metric, type: uint32_t, default: 100",
     *        "occlusionMetric": "Occlusion consistency importance metric, type: uint32_t, default: 100",
     *        "segmentationThreshold": "Segmentation bias threshold, type: float32_t, default: 0.5f",
     *        "globalMotionDetailThreshold": "Global motion threshold for textured regions, type: float32_t, default: 0.66f",
     *        "imageSharpnessThreshold": "Image sharpness threshold for edge detection, type: float32_t, default: 0.4f",
     *        "mvEdgeThreshold": "Motion vector edge selection threshold, type: float32_t, default: 0.43f",
     *        "refinementThreshold": "Refinement threshold for motion vectors, type: float32_t, default: 0.75f",
     *        "textureThreshold": "Texture threshold for low-texture region detection, type: float32_t, default: 0.167f",
     *        "lightingCondition": "Lighting condition for automotive platform, type: uint8_t, default: LIGHTING_CONDITION_HIGH",
     *        "isFirstRequest": "Indicates whether this is the first request, type: bool, default: false",
     *        "requestId": "Unique identifier for the request, type: uint64_t, default: 0"
     *     }
     *   }
     *  @return QC_STATUS_OK on success, other values on failure.
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

    OpticalFlow_Config_t m_config;
    std::string m_options;
};

// TODO: how to handle OpticalFlowMonitorConfig
typedef struct OpticalFlowMonitorConfig : public QCNodeMonitoringBase_t
{

} OpticalFlowMonitorConfig_t;


class OpticalFlowMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    OpticalFlowMonitoringIfs() {}
    ~OpticalFlowMonitoringIfs() {}

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
    OpticalFlowMonitorConfig_t m_config;
};

class OpticalFlow : public NodeBase
{
public:
    /**
     * @brief OpticalFlow Constructor
     * @return None
     */
    OpticalFlow() : m_configIfs( m_logger ){};

    /**
     * @brief OpticalFlow Destructor
     * @return None
     */
    ~OpticalFlow(){};

    /**
     * @brief Initializes Node Optical Flow.
     * @param[in] config The Node Optical Flow configuration.
     * @note QCNodeInit::config - Refer to the comments of the API
     * OpticalFlowConfigIfs::VerifyAndSet.
     * @note QCNodeInit::callback - The user application callback to notify the status of
     * the API ProcessFrameDescriptor. This is optional. If provided, the API ProcessFrameDescriptor
     * will be asynchronous; otherwise, the API ProcessFrameDescriptor will be synchronous.
     * @note QCNodeInit::buffers - Buffers provided by the user application. The buffers can be
     * provided for shared buffers to be registered into Optical Flow (see
     * OpticalFlowBufferMapping_t).
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node Optical Flow configuration interface.
     * @return A reference to the Node Optical Flow configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node Optical Flow monitoring interface.
     * @return A reference to the Node Optical Flow monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node Optical Flow
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of several input buffers.
     * @note ProcessFrameDescriptor call will return immediately after executing the EVA engine on
     * these four buffers.
     * @note There are four channels (buffers IDs):
     * o) QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID for the reference input image.
     * o) QC_NODE_OF_CURRENT_IMAGE_BUFF_ID for the current input image.
     * o) QC_NODE_OF_MOTION_VECTORS_BUFF_ID for motion vector output image.
     * o) QC_NODE_OF_CONFIDENCE_BUFF_ID for output.
     * The function passes the required buffer ID to GetBuffer() to get a handle to the required
     * channel. So to get the input and output buffers the following pattern can be used:
     *     auto &refBufIn = GetBuffer( QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID )
     *     auto &currBufIn = GetBuffer( QC_NODE_OF_CURRENT_IMAGE_BUFF_ID );
     *     auto &mvBufOut = GetBuffer( QC_NODE_OF_MOTION_VECTORS_BUFF_ID )
     *     auto &mvConfBufOut = GetBuffer( QC_NODE_OF_CONFIDENCE_BUFF_ID );
     *
     * @note This API is not thread-safe. Avoid calling the ProcessFrameDescriptor API
     * on the same instance from multiple threads simultaneously.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the Node Optical Flow
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node Optical Flow
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the Node Optical Flow
     * @return The current state of the Node Optical Flow
     */
    virtual QCObjectState_e GetState() { return m_state; }

private:
    OpticalFlowConfigIfs m_configIfs;
    OpticalFlowMonitoringIfs m_monitorIfs;
    ImageInfo m_imageInfo;
    Session::ConfigMap m_sessionConfigMap;
    LME::ConfigMap m_configMap;
    Session *m_session;
    LME *m_Lme;
    QCObjectState_e m_state = QC_OBJECT_STATE_INITIAL;
    std::map<void *, Buffer> m_memMap;
    LME::FeatureNoiseTolerances noiseTolerances;
    LME::Penalties penalties;
    PixelFormat GetInputImageFormat( QCImageFormat_e imageFormat );
    PixelFormat GetMotionMapFormat( MotionMapFormat_e motionFormat );
    void UpdateIconfig( LME::ConfigMap &configMap, OpticalFlow_Config_t configuration );
    QCStatus_e ValidateImageDesc( const ImageDescriptor_t &imgDesc,
                                  const OpticalFlow_Config_t &config );
    void SetInitialFrameConfig( LME::ConfigMap &configMapFrame,
                                OpticalFlow_Config_t configuration );
    QCStatus_e RegisterMemory( const BufferDescriptor_t &bufferDesc, Buffer &pBuff );
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_OPTICAL_FLOW_HPP
