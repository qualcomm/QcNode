// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_CAMERA_HPP
#define QC_NODE_CAMERA_HPP

#include "QC/Node/NodeBase.hpp"
#include "qcarcam.h"

namespace QC
{
namespace Node
{

/** @brief The QCNode Camera Version */
#define QCNODE_CAMERA_VERSION_MAJOR 2U
#define QCNODE_CAMERA_VERSION_MINOR 0U
#define QCNODE_CAMERA_VERSION_PATCH 1U

/** @brief The maximum stream number of QCNode Camera */
#define QCNODE_CAMERA_MAX_STREAM_NUM 8

/**
 * @brief Represents the Camera implementation used by NodeCamera
 *
 * This class encapsulates the specific implementation details of the
 * NodeCamera that are shared across configuration,
 * monitoring, and Node Camera.
 *
 * It serves as a central reference for nodes that need to interact with
 * the underlying Camera implementation.
 */
class CameraImpl;

/**
 * @brief Interface for Node Camera Configuration.
 *
 * This class provides an interface for configuring camera nodes. It extends the
 * NodeConfigIfs class and includes additional functionality specific to camera
 * configuration.
 */
class CameraConfig : public NodeConfigIfs
{
public:
    /**
     * @brief Constructor for CameraConfig.
     * Initializes the CameraConfig with a logger and a CameraImpl object.
     * @param[in] logger A reference to the logger to be shared and used by CameraConfig.  
     * @param[in] pCamImpl A pointer to the CameraImpl object to be used by CameraConfig.
     */
    CameraConfig( Logger &logger, CameraImpl *pCamImpl )
        : NodeConfigIfs( logger ),
          m_pCamImpl( pCamImpl )
    {}

    /**
     * @brief Destructor for CameraConfig.
     */
    ~CameraConfig() {}

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
     *         "inputId": "Camera input id, type: uint32_t",
     *         "srcId": "Input source identifier, type: uint32_t",
     *         "clientId": "Used for multi-client use case, type: uint32_t",
     *         "inputMode": "The input mode id is the index into #QCarCamInputModes_t pModex,
     *                      type: uint32_t",
     *         "ispUseCase": "ISP use case defined by qcarcam, type: uint32_t",
     *         "camFrameDropPattern": "Frame drop pattern defined by qcarcam. Set to 0 when frame
     *                                drop is not used, type: uint32_t",
     *         "camFrameDropPeriod": "Frame drop period defined by qcarcam, type: uint32_t",
     *         "opMode": "Operation mode defined by qcarcam, type: uint32_t",
     *         "streamConfigs": "Configuration for each camera stream, type: data tree array",
     *         [
     *             {"streamId": "Camera stream id, type: uint32_t",
     *              "bufCnt": "Buffer count set to camera, type: uint32_t",
     *              "width": "Camera frame width, type: uint32_t",
     *              "height": "Camera frame height, type: uint32_t",
     *              "format": "Camera frame format, type: string, options: ["nv12", "nv12_ubwc",
     *                        "uyvy", "rgb", "bgr", "p010", "tp10_ubwc"]",
     *              "submitRequestPattern": "Buffer submit request pattern, type: uint32_t"}
     *         ],
     *         "requestMode": "Flag to set request buffer mode, type: bool",
     *         "primary": "Flag to indicate if the session is primary or not when configured with
     *                    the clientId, type: bool",
     *         "recovery": "Flag to enable self-recovery for the session, type: bool",
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

private:
    CameraImpl *m_pCamImpl;
    std::string m_options;
};

/**
 * @brief Interface for Node Camera CameraMonitor.
 * This class provides an interface for monitoring camera nodes. It extends the
 * QCNodeMonitoringIfs class and includes additional functionality specific to
 * camera monitoring.
 */
class CameraMonitor : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief Constructor for CameraMonitor.
     * Initializes the CameraMonitor with a logger and a camera component.
     * @param[in] logger A reference to the logger to be shared and used by CameraMonitor.
     * @param[in] pCamImpl A pointer to the CameraImpl object to be used by CameraConfig.
     */
    CameraMonitor( Logger &logger, CameraImpl *pCamImpl )
        : m_logger( logger ),
          m_pCamImpl( pCamImpl )
    {}

    /**
     * @brief Destructor for CameraMonitor.
     */
    ~CameraMonitor() {}

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
     * @brief Get the QCNode monitor options as a string.
     * @return A reference to the QCNode monitor options string.
     */
    virtual const std::string &GetOptions();

    /**
     * @brief Get the base QCNode monitor structure.
     * @return A reference to the base QCNode monitor structure
     */
    virtual const QCNodeMonitoringBase_t &Get();

    /**
     * @brief Get the maximal size of the monitor data in bytes.
     * @return The maximal size of the monitor data in bytes.
     */
    virtual inline uint32_t GetMaximalSize() { return 0; }

    /**
     * @brief Get the current size of the monitor data in bytes.
     * @return The current size of the monitor data in bytes.
     */
    virtual inline uint32_t GetCurrentSize() { return 0; }

    /**
     * @brief Place monitor data.
     * This method places the monitor data. Currently, this functionality is unsupported.  
     * @param[in] pData Pointer to the data to be placed.  
     * @param[in, out] size The size of the data to be placed.
     * @return QC_STATUS_UNSUPPORTED as this functionality is not supported.  */
    virtual QCStatus_e Place( void *pData, uint32_t &size ) { return QC_STATUS_UNSUPPORTED; }

private:
    CameraImpl *m_pCamImpl;
    Logger &m_logger;
    std::string m_options;
};

/**
 * @brief Node Camera Interface.
 * This class provides an interface for camera nodes. It extends the NodeBase
 * class and includes functionality for initializing, starting, processing
 * frames, and stopping the camera node.
 */
class Camera : public NodeBase
{
public:
    /**
     * @brief Constructor for Camera.
     * Initializes the Camera with configuration and monitoring interfaces.
     */
    Camera();

    /**
     * @brief Destructor for Camera.
     */
    ~Camera();

    /**
     * @brief Initialize the camera node.
     * This method initializes the camera node with the provided configuration.
     * @param[in] config The configuration structure for node initialization.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the configuration interface.
     * @return A reference to the configuration object.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the monitoring interface.
     * @return A reference to the CameraMonitoring object.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitor; }

    /**
     * @brief Start the camera node.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e Start();

    /**
     * @brief Process the provided frame descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note This function has two modes: request mode and release mode, which is determined
     * by m_bRequestMode flag.
     * In request mode, it submits requset to QCarCamera for frames and processes frames
     * when it receives frame event.
     * In release mode, it processes frames and then release frame buffer when it
     * receives frame event.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the camera node.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e Stop();

    /**
     * @brief Deinitialize the camera node.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the camera node.
     * @return The current state of the camera node.
     */
    virtual QCObjectState_e GetState();

private:
    CameraImpl *m_pCamImpl;
    CameraConfig m_configIfs;
    CameraMonitor m_monitor;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_CAMERA_HPP
