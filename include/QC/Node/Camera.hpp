// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_CAMERA_HPP
#define QC_NODE_CAMERA_HPP

#include "QC/component/Camera.hpp"
#include "QC/Node/NodeBase.hpp"

#include <condition_variable>
#include <mutex>

namespace QC
{
namespace Node
{

/**
 * @brief Descriptor for QCNode Shared Camera Frame.
 *
 * This structure represents the shared camera frame descriptor for QCNode. It extends the
 * QCSharedBufferDescriptor and includes additional members specific to camera frames.
 *
 * @param timestamp The hardware timestamp of the frame in nanoseconds.
 * @param timestampQGPTP The Generic Precision Time Protocol (GPTP) timestamp in nanoseconds.
 * @param frameIndex The index of the camera frame.
 * @param flags Flags indicating the error state of the buffer.
 * @param streamId The identifier for the Qcarcam buffer list.
 *
 * @note During initialization, the Camera Node creates a QCSharedCameraFrameDescriptor for each
 * buffer of each stream. The QCSharedCameraFrameDescriptor.buffer should be set up with either
 * the buffer provided by the user application or allocated by the camera node itself.
 *
 * @note For phase 2: Update to inherit QCSharedImageDescriptor.
 */
typedef struct QCSharedCameraFrameDescriptor : public QCSharedBufferDescriptor
{
public:
    QCSharedCameraFrameDescriptor() = default;
    virtual ~QCSharedCameraFrameDescriptor() = default;
    uint64_t timestamp;
    uint64_t timestampQGPTP;
    uint32_t frameIndex;
    uint32_t flags;
    uint32_t streamId;
} QCSharedCameraFrameDescriptor_t;

/**
 * @brief Camera Node Configuration Data Structure
 * @param params The QC component Camera configuration data structure.
 */
typedef struct CameraConfig : public QCNodeConfigBase_t
{
    Camera_Config_t params;
    std::vector<std::vector<uint32_t>> streamBufferIds;
} CameraConfig_t;

/**
 * @brief Camera Node Event Data Structure
 * @param eventId The QCarcamera callback event type.
 * @param pPayload The pointer of QCarcamera event payload.
 */
typedef struct
{
    uint32_t eventId;
    const void *pPayload;
} CameraEvent_t;

/**
 * @brief Camera Node Monitor Data Structure
 * @param bPerfEnabled The flag to enable Camera Node performance monitoring.
 */
typedef struct CameraMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} CameraMonitorConfig_t;


/**
 * @brief Interface for Node Camera Configuration.
 *
 * This class provides an interface for configuring camera nodes. It extends the NodeConfigIfs
 * class and includes additional functionality specific to camera configuration.
 */
class CameraConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief Constructor for CameraConfigIfs.
     * Initializes the CameraConfigIfs with a logger and a camera component.
     * @param[in] logger A reference to the logger to be shared and used by CameraConfigIfs.
     * @param[in] cam A reference to the QC Camera component to be used by CameraConfigIfs.
     */
    CameraConfigIfs( Logger &logger, Camera &cam ) : NodeConfigIfs( logger ), m_cam( cam ) {}

    /**
     * @brief Destructor for CameraConfigIfs.
     */
    ~CameraConfigIfs() {}

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
     *              "bufferIds": "buffer index for each buffer in user allocation mode,
     *                           type: array of uint32_t",
     *              "width": "Camera frame width, type: uint32_t",
     *              "height": "Camera frame height, type: uint32_t",
     *              "format": "Camera frame format, type: string, options: ["nv12", nv12_ubwc",
     *                        "uyvy", "rgb", "bgr", "p010", "tp10_ubwc"]",
     *              "submitRequestPattern": "Buffer submit request pattern, type: uint32_t"}
     *         ],
     *         "requestMode": "Flag to set request buffer mode, type: bool",
     *         "primary": "Flag to indicate if the session is primary or not when configured with
     *                    the clientId, type: bool",
     *         "recovery": "Flag to enable self-recovery for the session, type: bool"
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
    Camera &m_cam;
    CameraConfig_t m_config;
    std::string m_options;
};


/**
 * @brief Interface for Node Camera Monitoring.
 * This class provides an interface for monitoring camera nodes. It extends the QCNodeMonitoringIfs
 * class and includes additional functionality specific to camera monitoring.
 */
class CameraMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief Constructor for CameraMonitoringIfs.
     * Initializes the CameraMonitoringIfs with a logger and a camera component.
     * @param[in] logger A reference to the logger to be shared and used by CameraMonitoringIfs.
     * @param[in] cam A reference to the QC Camera component to be
    used by CameraMonitoringIfs.
     */
    CameraMonitoringIfs( Logger &logger, Camera &cam ) : m_logger( logger ), m_cam( cam ) {}

    /**
     * @brief Destructor for CameraMonitoringIfs.
     */
    ~CameraMonitoringIfs() {}

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
    Camera &m_cam;
    Logger &m_logger;
    std::string m_options;
    CameraMonitorConfig_t m_config;
};


/**
 * @brief Node Camera Interface.
 * This class provides an interface for camera nodes. It extends the NodeBase class and includes
 * functionality for initializing, starting, processing frames, and stopping the camera node.
 */
class Camera : public NodeBase
{
public:
    /**
     * @brief Constructor for Camera.
     * Initializes the Camera with configuration and monitoring interfaces.
     */
    Camera() : m_configIfs( m_logger, m_camera ), m_monitorIfs( m_logger, m_camera ) {}

    /**
     * @brief Destructor for Camera.
     */
    ~Camera() {}

    /**
     * @brief Initialize the camera node.
     * This method initializes the camera node with the provided configuration.
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
     * @brief Start the camera node.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e Start();

    /**
     * @brief Process the provided frame descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note This function has two modes: request mode and release mode, which is determined by
     * m_bRequestMode flag.
     * In request mode, it submits requset to QCarCamera for frames and processes frames when it
     * receives frame event.
     * In release mode, it processes frames and then release frame buffer when it receives frame
     * event.
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
    virtual QCObjectState_e GetState()
    {
        return static_cast<QCObjectState_e>( m_camera.GetState() );
    }

private:
    QCStatus_e ProcessFrame( const QCSharedCameraFrameDescriptor_t *pFrame );
    QCStatus_e RegisterBuffer( QCSharedBufferDescriptor_t *pSharedBuffer );

    static void FrameCallback( QC::component::CameraFrame_t *pFrame, void *pPrivData );
    static void EventCallback( const uint32_t eventId, const void *pPayload, void *pPrivData );
    void FrameCallback( QC::component::CameraFrame_t *pFrame );
    void EventCallback( const uint32_t eventId, const void *pPayload );

private:
    QC::component::Camera m_camera;
    CameraConfigIfs m_configIfs;
    CameraMonitoringIfs m_monitorIfs;

    bool m_bRequestMode;
    QCNodeEventCallBack_t m_callback = nullptr;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_CAMERA_HPP
