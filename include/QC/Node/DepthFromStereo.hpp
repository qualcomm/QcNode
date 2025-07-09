// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_DEPTH_FROM_STEREO_HPP
#define QC_NODE_DEPTH_FROM_STEREO_HPP

#include "QC/component/DepthFromStereo.hpp"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{
using namespace QC::component;

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
          disparityConfidenceMapBufferId( QC_NODE_DFS_DISPARITY_CONFIDANCE_MAP_BUFF_ID ) {};

    uint32_t primaryImageBufferId;
    uint32_t auxilaryImageBufferId;
    uint32_t disparityMapBufferId;
    uint32_t disparityConfidenceMapBufferId;
} DepthFromStereoBufferMapping_t;

typedef struct DepthFromStereoConfig : public QCNodeConfigBase_t
{
    DepthFromStereo_Config_t config;
    DepthFromStereoBufferMapping_t bufferMap;
} DepthFromStereoConfig_t;

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
     * 1. Static configuration used for initialization:
     *   {
     *     "static": {
     *        "name": "The Node unique name, type: string, default: 'unnamed'",
     *        "id": "The Node unique ID, type: uint32_t, default: 0",
     *        "width": "The width in pixels of the I/O frame, type: uint32_t, default: 0",
     *        "height": "The height in pixels of the I/O frame, type: uint32_t, default: 0",
     *        "fps": "The input frames per second, type: uint32_t, default: 0",
     *        "direction": "EVA search direction, either left to right (l2r)
     *                      or right to left (r2l), type: string, default: l2r"
     *        "format": "output format support NV12, NV12_UBWC, P010, type: uint32_t,
     *                   default: nv12",
     *        "bHolefillEnable": "Enable post-processing hole-filling, type: bool,
     *                            default: true",
     *        "holeFillConfig": "Structure for post-processing hole-filling (see eva_dfs.h),
     *                           type: EvaDFSHoleFillConfig_t",
     *        "bAmFilterEnable": "Adaptive P1 penalty", type: bool,
     *                            default: true"
     *        "amFilterConfThreshold": "Enable adaptive median filtering for a final pass
     *                                  of filtering on low confidence pixels,
     *                                 type: uint8_t, default: 200"
     *        "p1Config": "Defines penalty parameters for disparity changes of +/-1 pixel in
     *                     SGM path propagation (see eva_dfs.h),
     *                     type: EvaDFSP1Config_t",
     *        "p2Config": "Defines penalty parameters for disparity changes of >1 pixel in
     *                     SGM path propagation (see eva_dfs.h),
     *                     type: EvaDFSP2Config_t",
     *        "censusConfig": "Census block noise thresholding (see eva_dfs.h),
     *                        type: EvaDFSCensusConfig_t",
     *        "occlusionConf": "Weight used with occlusion estimation to generate
     *                         final confidence map, type: uint8_t [0,32], default: 16
     *        "mvVarConf": "Weight used with local neighborhood motion vector variance
     *                      estimation to generate a final confidence map,
     *                     type: uint8_t [0,32], default: 10",
     *        "mvVarThresh": "Threshold offset for MV variance in confidence generation.
     *                        Value = 0 allows most variance,
     *                       type: uint8_t [0,3], default: 1",
     *        "flatnessConf": "Weight used with flatness estimation to generate final
     *                         confidence map,
     *                        type: uint8_t [0,32], default: 6"
     *        "flatnessThresh": "Threshold for flatness check in the SGM block,
     *                          type: uint8_t [1,7], default: 3"
     *        "bFlatnessOverride": "whether the final confidence value will be overridden
     *                              by the flatness value or not,
     *                             type: bool, default: false"
     *        "rectificationErrorTolerance": "Defines DFS rectification error tolerance range.
     *                                        If 0, DFS vertical search is disabled.
     *                                        If non-zero, +-3 vertical search is enabled,
     *                                       type: float [0,1], default: 0.29"
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

    DepthFromStereoConfig_t m_config;
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
    DepthFromStereo() : m_configIfs( m_logger ) {};

    /**
     * @brief NodeDepthFromStereo Destructor
     * @return None
     */
    ~DepthFromStereo() {};

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
    virtual QCObjectState_e GetState() { return m_dfs.GetState(); }

private:
    QC::component::DepthFromStereo m_dfs;
    DepthFromStereoConfigIfs m_configIfs;
    DepthFromStereoMonitoringIfs m_monitorIfs;

    QCNodeEventCallBack_t m_callback = nullptr;
    std::mutex m_lock;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_DEPTH_FROM_STEREO_HPP
