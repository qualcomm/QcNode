// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#ifndef QC_NODE_VOXELIZATION_HPP
#define QC_NODE_VOXELIZATION_HPP

#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{

/** @brief The QCNode Voxelization Version */
#define QCNODE_VOXELIZATION_VERSION_MAJOR 2U
#define QCNODE_VOXELIZATION_VERSION_MINOR 0U
#define QCNODE_VOXELIZATION_VERSION_PATCH 0U

#define QCNODE_VOXELIZATION_VERSION                                                                \
    ( ( QCNODE_VOXELIZATION_VERSION_MAJOR << 16U ) | ( QCNODE_VOXELIZATION_VERSION_MINOR << 8U ) | \
      QCNODE_VOXELIZATION_VERSION_PATCH )

/**
 * @brief Represents the Voxelization implementation used by NodeVoxelization
 *
 * This class encapsulates the specific implementation details of the
 * NodeVoxelization that are shared across configuration,
 * monitoring, and Node Voxelization.
 *
 * It serves as a central reference for nodes that need to interact with
 * the underlying Voxelization implementation.
 */
class VoxelizationImpl;

/**
 * @brief Interface for Node Voxelization Configuration.
 *
 * This class provides an interface for configuring Voxelization nodes. It extends the NodeConfigIfs
 * class and includes additional functionality specific to Voxelization configuration.
 */
class VoxelizationConfig : public NodeConfigIfs
{
public:
    /**
     * @brief Constructor for VoxelizationConfig.
     * Initializes the VoxelizationConfig with a logger and a VoxelizationImpl object.
     * @param[in] logger A reference to the logger to be shared and used by VoxelizationConfig.
     * @param[in] pVoxelImpl A reference to the VoxelizationImpl object to be used by
     * VoxelizationConfig.
     */
    VoxelizationConfig( Logger &logger, VoxelizationImpl *pVoxelImpl )
        : NodeConfigIfs( logger ),
          m_pVoxelImpl( pVoxelImpl )
    {}

    /**
     * @brief Destructor for VoxelizationConfig.
     */
    ~VoxelizationConfig() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * This method verifies the provided configuration string and sets the configuration structure.
     * The configuration string is expected to be in JSON format.
     * @param[in] config The configuration string in JSON format.
     * @param[out] errors The error string returned if there is an error.
     * @return QC_STATUS_OK on success, other values on failure.
     * @note The config is a JSON string according to the templates below.
     * Example of static configuration used for initialization:
     * @code
     * {
     *     "static": {
     *         "name": "The Node unique name, type: string",
     *         "id": "The Node unique ID, type: uint32_t",
     *         "processorType": "The processor type, type: string, options: [htp0, htp1, cpu, gpu],
     *                          default: htp0",
     *         "Xsize": "Pillar size in X direction in meters, type: float",
     *         "Ysize": "Pillar size in Y direction in meters, type: float",
     *         "Zsize": "Pillar size in Z direction in meters, type: float",
     *         "Xmin": "Minimum range value in X direction, type: float",
     *         "Ymin": "Minimum range value in Y direction, type: float",
     *         "Zmin": "Minimum range value in Z direction, type: float",
     *         "Xmax": "Maximum range value in X direction, type: float",
     *         "Ymax": "Maximum range value in Y direction, type: float",
     *         "Zmax": "Maximum range value in Z direction, type: float",
     *         "maxPointNum": "Maximum number of points in input point cloud, type: uint32_t",
     *         "maxPlrNum": "Maximum number of point pillars that can be created, type: uint32_t",
     *         "maxPointNumPerPlr": "Maximum number of points to map to each pillar,
     *                              type: uint32_t",
     *         "inputMode": "Voxelization input pointclouds type, type: string,
     *                       options: [xyzr, xyzrt], default: xyzr",
     *         "outputFeatureDimNum": "Number of features for each point in output point pillars,
     *                                 type: uint32_t",
     *         "outputPlrBufferIds": "[ A list of uint32_t values representing the indices of output
     *                                pillar buffers in QCNodeInit::buffers ]",
     *         "outputFeatureBufferIds": "[ A list of uint32_t values representing the indices of
     *                                    output feature buffers in QCNodeInit::buffers ]",
     *         "plrPointsBufferId": "The index of buffer for maximal pliiar point number in
     *                              QCNodeInit::buffers, type: uint32_t",
     *         "coordToPlrIdxBufferId": "The index of buffer to store coordinate to pillar point
     *                                   transform indices  in QCNodeInit::buffers, type: uint32_t",
     *         "globalBufferIdMap": [
     *            {
     *               "name": "The buffer name, type: string",
     *               "id": "The index to a buffer in QCFrameDescriptorNodeIfs, type: uint32_t"
     *            }
     *         ],
     *         "deRegisterAllBuffersWhenStop": "Flag to deregister all buffers when stopped,
     *                                         type: bool, default: false"
     *     }
     * }
     * @endcode
     *
     * @note: 
     * plrPointsBufferId and coordToPlrIdxBufferId is only needed while the processorType is gpu.
     * globalBufferIdMap is optional. If not set, this config will be set to default.
     */
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get the configuration options as a string.
     * @return A reference to the configuration options string.
     */
    virtual const std::string &GetOptions();

    /**
     * @brief Get the base configuration structure.
     * @return A reference to the base configuration structure
     */
    virtual const QCNodeConfigBase_t &Get();

private:
    QCStatus_e VerifyStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e GetInputMode( std::string &mode );

private:
    VoxelizationImpl *m_pVoxelImpl;
    std::string m_options;
};


/**
 * @brief Interface for Node Voxelization Monitor.
 * This class provides an interface for monitoring Voxelization nodes. It extends the
 * QCNodeMonitoringIfs class and includes additional functionality specific to VoxelizationMonitor.
 */
class VoxelizationMonitor : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief Constructor for VoxelizationMonitor.
     * Initializes the VoxelizationMonitor with a logger and a VoxelizationImpl object.
     * @param[in] logger A reference to the logger to be shared and used by
     * VoxelizationMonitor.
     * @param[in] cam A reference to the VoxelizationImpl object to be
     * used by VoxelizationMonitor.
     */
    VoxelizationMonitor( Logger &logger, VoxelizationImpl *pVoxelImpl )
        : m_logger( logger ),
          m_pVoxelImpl( pVoxelImpl )
    {}

    /**
     * @brief Destructor for VoxelizationMonitor.
     */
    ~VoxelizationMonitor() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * This method verifies the provided configuration string and sets the configuration structure.
     * Currently, this functionality is unsupported.
     * @param[in] config The configuration string in JSON format.
     * @param[out] errors The error string returned if there is an error.
     * @return QC_STATUS_UNSUPPORTED as this functionality is not supported.
     */
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get the QCNode monitoring options as a string.
     * @return A reference to the QCNode monitoring options string.
     */
    virtual const std::string &GetOptions();

    /**
     * @brief Get the base QCNode monitoring structure.
     * @return A reference to the base QCNode monitoring structure
     */
    virtual const QCNodeMonitoringBase_t &Get();

    /**
     * @brief Get the maximal size of the monitoring data in bytes.
     * @return The maximal size of the monitoring data in bytes.
     */
    virtual inline uint32_t GetMaximalSize() { return 0; }

    /**
     * @brief Get the current size of the monitoring data in bytes.
     * @return The current size of the monitoring data in bytes.
     */
    virtual inline uint32_t GetCurrentSize() { return 0; }

    /**
     * @brief Place monitoring data.
     * This method places the monitoring data. Currently, this functionality is unsupported.
     * @param[in] pData Pointer to the data to be placed.
     * @param[in, out] size The size of the data to be placed.
     * @return QC_STATUS_UNSUPPORTED as this functionality is not supported.
     */
    virtual QCStatus_e Place( void *pData, uint32_t &size ) { return QC_STATUS_UNSUPPORTED; }

private:
    VoxelizationImpl *m_pVoxelImpl;
    Logger &m_logger;
    std::string m_options;
};

/**
 * @brief Node Voxelization Interface.
 * This class provides an interface for Voxelization nodes. It extends the NodeBase class and
 * includes functionality for initializing, starting, processing frames, and stopping the
 * Voxelization node.
 */
class Voxelization : public NodeBase
{
public:
    /**
     * @brief Constructor for Voxelization.
     * Initializes the Voxelization object.
     */
    Voxelization();

    /**
     * @brief Destructor for Voxelization.
     */
    ~Voxelization();

    /**
     * @brief Initialize the voxelization node.
     * This method initializes the Voxelization node with the provided configuration.
     * @param[in] config The configuration structure for node initialization.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the configuration interface.
     * @return A reference to the configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the monitoring interface.
     * @return A reference to the monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitor; }

    /**
     * @brief Start the voxelization node.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e Start();

    /**
     * @brief Process the provided frame descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note The voxelization pipeline has 1 input and 2 outputs.
     * The input is point cloud with size of maxPointNum x inputFeatureDimNum x sizeof(float).
     * The 1st output is pillar index tensor with size of 4 x maxPlrNum x sizeof(float).
     * The 2nd output is stacked pillar tensor with size of maxPlrNum * maxPointNumPerPlr *
     * outputFeatureDimNum * sizeof(float).
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the voxelization node.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e Stop();

    /**
     * @brief Deinitialize the voxelization node.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the voxelization node.
     * @return The current state of the voxelization node.
     */
    virtual QCObjectState_e GetState();

private:
    VoxelizationImpl *m_pVoxelImpl;
    VoxelizationConfig m_configIfs;
    VoxelizationMonitor m_monitor;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_VOXELIZATION_HPP

