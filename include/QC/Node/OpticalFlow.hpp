// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_OPTICAL_FLOW_HPP
#define QC_NODE_OPTICAL_FLOW_HPP

#include "QC/component/OpticalFlow.hpp"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{
using namespace QC::component;

typedef enum
{
    QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID = 0,
    QC_NODE_OF_CURRENT_IMAGE_BUFF_ID = 1,
    QC_NODE_OF_MOTION_VECTORS_BUFF_ID = 2,
    QC_NODE_OF_CONFIDENCE_BUFF_ID = 3,
    QC_NODE_OF_LAST_BUFF_ID = 4
} QCOpticalFlowBufferId_e;

typedef struct OpticalFlowBufferMapping
{
    OpticalFlowBufferMapping()
        : referenceImageBufferId( QC_NODE_OF_REFERENCE_IMAGE_BUFF_ID ),
          currentImageBufferId( QC_NODE_OF_CURRENT_IMAGE_BUFF_ID ),
          monionVectorsBufferId( QC_NODE_OF_MOTION_VECTORS_BUFF_ID ),
          confidenceBufferId( QC_NODE_OF_CONFIDENCE_BUFF_ID ){};

    uint32_t referenceImageBufferId;
    uint32_t currentImageBufferId;
    uint32_t monionVectorsBufferId;
    uint32_t confidenceBufferId;
} OpticalFlowBufferMapping_t;

typedef struct OpticalFlowConfig : public QCNodeConfigBase_t
{
    OpticalFlow_Config_t config;
    OpticalFlowBufferMapping_t bufferMap;
} OpticalFlowConfig_t;

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
     * 1. Static configuration used for initialization:
     *   {
     *     "static": {
     *        "name": "The Node unique name, type: string, default: 'unnamed'",
     *        "id": "The Node unique ID, type: uint32_t, default: 0",
     *        "width": "The width in pixels of the input frame, type: uint32_t, default: 0",
     *        "height": "The height in pixels of the input frame, type: uint32_t, default: 0",
     *        "step_size": ", type: uint32_t, default: 1",
     *        "direction": "The direction of motion of the input frame, type: string,
     *                     options: [forward, backward], default: forward",
     *        "fps": "The input frame rate in frames per second, type: uint32_t, default: 0",
     *        "eva_mode": "The EVA mode, type: string, options: [cpu, dsp, disable],
     *                   default: cpu",
     *        "format": "input format support nv12, nv12_ubwc, p010, type: uint32_t,
     *                   default: nv12"
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

    OpticalFlowConfig_t m_config;
};

// TODO: how to handle OpticalFlowMonitorConfig
typedef struct OpticalFlowMonitorConfig : public QCNodeMonitoringBase_t
{

} OpticalFlowMonitorConfig_t;

// TODO: how to handle OpticalFlowMonitoringIfs
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
    virtual QCObjectState_e GetState() { return static_cast<QCObjectState_e>( m_of.GetState() ); }

private:
    QC::component::OpticalFlow m_of;
    OpticalFlowConfigIfs m_configIfs;
    OpticalFlowMonitoringIfs m_monitorIfs;

    QCNodeEventCallBack_t m_callback = nullptr;
    std::mutex m_lock;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_OPTICAL_FLOW_HPP
