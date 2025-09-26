// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_NODE_VOXELIZATION_HPP
#define QC_NODE_VOXELIZATION_HPP

#include "QC/component/Voxelization.hpp"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{

/**
 * @brief Voxelization Node Configuration Data Structure
 * @param params The QC component Voxelization configuration data structure.
 */
typedef struct VoxelizationConfig : public QCNodeConfigBase_t
{
    Voxelization_Config_t params;
    std::vector<uint32_t> inputBufferIds;
    std::vector<uint32_t> outputPlrBufferIds;
    std::vector<uint32_t> outputFeatureBufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} VoxelizationConfig_t;

/**
 * @brief Voxelization Node Monitor Data Structure
 * @param bPerfEnabled The flag to enable Voxelization Node performance monitoring.
 */
typedef struct VoxelizationMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} VoxelizationMonitorConfig_t;


/**
 * @brief Interface for Node Voxelization Configuration.
 *
 * This class provides an interface for configuring Voxelization nodes. It extends the NodeConfigIfs
 * class and includes additional functionality specific to Voxelization configuration.
 */
class VoxelizationConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief Constructor for VoxelizationConfigIfs.
     * Initializes the VoxelizationConfigIfs with a logger and a Voxelization component.
     * @param[in] logger A reference to the logger to be shared and used by VoxelizationConfigIfs.
     * @param[in] voxel A reference to the QC Voxelization component to be used by
     * VoxelizationConfigIfs.
     */
    VoxelizationConfigIfs( Logger &logger, Voxelization &voxel )
        : NodeConfigIfs( logger ),
          m_voxel( voxel )
    {}

    /**
     * @brief Destructor for VoxelizationConfigIfs.
     */
    ~VoxelizationConfigIfs() {}

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
     *         "inputBufferIds": "[ A list of uint32_t values representing the indices of input
     *                           buffers in QCNodeInit::buffers ],
     *         "outputPlrBufferIds": "[ A list of uint32_t values representing the indices of output
     *                               pillar buffers in QCNodeInit::buffers ],
     *         "outputFeatureBufferIds": "[ A list of uint32_t values representing the indices of
     *                                   output feature buffers in QCNodeInit::buffers ],
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
    Voxelization_InputMode_e GetInputMode( std::string &mode );

private:
    Voxelization &m_voxel;
    VoxelizationConfig_t m_config;
    std::string m_options;
};


/**
 * @brief Interface for Node Voxelization Monitoring.
 * This class provides an interface for monitoring Voxelization nodes. It extends the
 * QCNodeMonitoringIfs class and includes additional functionality specific to Voxelization
 * monitoring.
 */
class VoxelizationMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief Constructor for VoxelizationMonitoringIfs.
     * Initializes the VoxelizationMonitoringIfs with a logger and a Voxelization component.
     * @param[in] logger A reference to the logger to be shared and used by
     * VoxelizationMonitoringIfs.
     * @param[in] cam A reference to the QC Voxelization component to be
     * used by VoxelizationMonitoringIfs.
     */
    VoxelizationMonitoringIfs( Logger &logger, Voxelization &voxel )
        : m_logger( logger ),
          m_voxel( voxel )
    {}

    /**
     * @brief Destructor for VoxelizationMonitoringIfs.
     */
    ~VoxelizationMonitoringIfs() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * This method verifies the provided configuration string and sets the configuration structure.
     * Currently, this functionality is unsupported.
     * @param[in] config The configuration string in JSON format.
     * @param[out] errors The error string returned if there is an error.
     * @return QC_STATUS_UNSUPPORTED as this functionality is not supported.
     */
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    /**
     * @brief Get the QCNode monitoring options as a string.
     * @return A reference to the QCNode monitoring options string.
     */
    virtual const std::string &GetOptions() { return m_options; }

    /**
     * @brief Get the base QCNode monitoring structure.
     * @return A reference to the base QCNode monitoring structure
     */
    virtual const QCNodeMonitoringBase_t &Get() { return m_config; }

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
    Voxelization &m_voxel;
    Logger &m_logger;
    std::string m_options;
    VoxelizationMonitorConfig_t m_config;
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
     * Initializes the Voxelization with configuration and monitoring interfaces.
     */
    Voxelization() : m_configIfs( m_logger, m_voxel ), m_monitorIfs( m_logger, m_voxel ) {}

    /**
     * @brief Destructor for Voxelization.
     */
    ~Voxelization() {}

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
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

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
     * The 2nd output is stacked pillar tensor with size of maxPlrNum x maxPointNumPerPlr x
     * outputFeatureDimNum x sizeof(float).
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
    virtual QCObjectState_e GetState()
    {
        return static_cast<QCObjectState_e>( m_voxel.GetState() );
    }

private:
    QC::component::Voxelization m_voxel;
    VoxelizationConfigIfs m_configIfs;
    VoxelizationMonitoringIfs m_monitorIfs;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_VOXELIZATION_HPP
