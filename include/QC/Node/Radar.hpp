// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_NODE_RADAR_HPP
#define QC_NODE_RADAR_HPP

#include "QC/Node/NodeBase.hpp"
#include "QC/component/Radar.hpp"

namespace QC
{
namespace Node
{
using namespace QC::component;

/**
 * @brief Radar Node Configuration Data Structure
 * @param params The QC component Radar configuration data structure.
 * @param bufferIds The indices of buffers in QCNodeInit::buffers provided by the user application
 * for use by Radar. These buffers will be registered into Radar during the initialization stage.
 * @param globalBufferIdMap The global buffer index map used to identify which buffer in
 * QCFrameDescriptorNodeIfs is used for Radar input and output.
 * @param deRegisterAllBuffersWhenStop When the Stop API of the Radar node is called and
 * deRegisterAllBuffersWhenStop is true, deregister all buffers.
 */
typedef struct RadarConfig : public QCNodeConfigBase_t
{
    Radar_Config_t params;
    std::vector<uint32_t> bufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} RadarConfig_t;

class RadarConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief RadarConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by RadarConfigIfs.
     * @param[in] radar A reference to the QC Radar component to be used by RadarConfigIfs.
     * @return None
     */
    RadarConfigIfs( Logger &logger, Radar &radar ) : NodeConfigIfs( logger ), m_radar( radar ) {}

    /**
     * @brief RadarConfigIfs Destructor
     * @return None
     */
    ~RadarConfigIfs() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * @param[in] config The configuration string.
     * @param[out] errors The error string returned if there is an error.
     * @note The config is a JSON string according to the template below:
     *   {
     *     "static": {
     *        "name": "The Node unique name, type: string",
     *        "id": "The Node unique ID, type: uint32_t",
     *        "maxInputBufferSize": "Maximum input buffer size, type: uint32_t",
     *        "maxOutputBufferSize": "Maximum output buffer size, type: uint32_t",
     *        "serviceName": "Processing service name/path, type: string",
     *        "timeoutMs": "Processing timeout in milliseconds, type: uint32_t",
     *        "enablePerformanceLog": "Enable performance logging, type: bool",
     *        "bufferIds": [A list of uint32_t values representing the indices of buffers
     *                      in QCNodeInit::buffers],
     *        "globalBufferIdMap": [
     *           {
     *              "name": "The buffer name, type: string",
     *              "id": "The index to a buffer in QCFrameDescriptorNodeIfs"
     *           }
     *        ],
     *        "deRegisterAllBuffersWhenStop": "Flag to deregister all buffers when stopped,
     *                   type: bool, default: false"
     *     }
     *   }
     * @return QC_STATUS_OK on success, other values on failure.
     */
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors );

    /**
     * @brief Get Configuration Options
     * @return A reference string to the JSON configuration options.
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

private:
    Radar &m_radar;
    std::string m_options;

public:
    RadarConfig_t m_config;
};

typedef struct RadarMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} RadarMonitorConfig_t;

class RadarMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    RadarMonitoringIfs() {}
    ~RadarMonitoringIfs() {}

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
    RadarMonitorConfig_t m_config;
};

class Radar : public NodeBase
{
public:
    /**
     * @brief Radar Constructor
     * @return None
     */
    Radar() : m_configIfs( m_logger, m_radar ){};

    /**
     * @brief Radar Destructor
     * @return None
     */
    ~Radar(){};

    /**
     * @brief Initializes Node Radar.
     * @param[in] config The Node Radar configuration.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node Radar configuration interface.
     * @return A reference to the Node Radar configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node Radar monitoring interface.
     * @return A reference to the Node Radar monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node Radar
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing input/output buffers.
     * @note The configuration globalBufferIdMap determines which buffers are input and output.
     * - The globalBufferIdMap[0].globalBufferId will be input buffer.
     * - The globalBufferIdMap[1].globalBufferId will be output buffer.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the Node Radar
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node Radar
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the Node Radar
     * @return The current state of the Node Radar
     */
    virtual QCObjectState_e GetState()
    {
        return static_cast<QCObjectState_e>( m_radar.GetState() );
    }

private:
    QCStatus_e SetupGlobalBufferIdMap( const RadarConfig_t &cfg );
    void NotifyEvent( QCFrameDescriptorNodeIfs &frameDesc, QCStatus_e status );

private:
    QC::component::Radar m_radar;
    RadarConfigIfs m_configIfs;
    RadarMonitoringIfs m_monitorIfs;
    bool m_bDeRegisterAllBuffersWhenStop = false;

    uint32_t m_inputNum = 1;
    uint32_t m_outputNum = 1;

    std::vector<QCNodeBufferMapEntry_t> m_globalBufferIdMap;
    QCNodeEventCallBack_t m_eventCallback;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_RADAR_HPP
