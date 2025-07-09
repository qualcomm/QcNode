// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_REMAP_HPP
#define QC_NODE_REMAP_HPP

#include "QC/component/Remap.hpp"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{
using namespace QC::component;

/**
 * @brief Remap Node Configuration Data Structure
 * @param params The QC component Remap configuration data structure.
 * @param bufferIds The indices of buffers in QCNodeInit::buffers provided by the user application
 * for use by Remap. These buffers will be registered into Remap during the initialization stage.
 * @note bufferIds are optional and can be empty, in which case the buffers will be registered into
 * Remap when the API ProcessFrameDescriptor is called.
 * @param globalBufferIdMap The global buffer index map used to identify which buffer in
 * QCFrameDescriptorNodeIfs is used for Remap input(s) and output(s).
 * @note globalBufferIdMap is optional and can be empty, in which case a default buffer index map
 * will be applied for Remap input(s) and output(s). For now Remap only support multiple inputs to
 * single output
 * - The index 0 of QCFrameDescriptorNodeIfs will be input 0.
 * - The index 1 of QCFrameDescriptorNodeIfs will be input 1.
 * - ...
 * - The index N-1 of QCFrameDescriptorNodeIfs will be input N-1.
 * - The index N of QCFrameDescriptorNodeIfs will be output.
 * @param bDeRegisterAllBuffersWhenStop When the Stop API of the Remap node is called and
 * bDeRegisterAllBuffersWhenStop is true, deregister all buffers.
 */
typedef struct RemapConfig : public QCNodeConfigBase_t
{
    Remap_Config_t params;
    std::vector<uint32_t> bufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} RemapConfig_t;

class RemapConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief RemapConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by RemapConfigIfs.
     * @param[in] remap A reference to the QC Remap component to be used by RemapConfigIfs.
     * @return None
     */
    RemapConfigIfs( Logger &logger, Remap &remap ) : NodeConfigIfs( logger ), m_remap( remap ) {}

    /**
     * @brief RemapConfigIfs Destructor
     * @return None
     */
    ~RemapConfigIfs() {}

    /**
     * @brief Verify the configuration string and set the configuration structure.
     * @param[in] config The configuration string.
     * @param[out] errors The error string returned if there is an error.
     * @note The config is a JSON string according to the templates below.
     * 1. Static configuration used for initialization:
     *   {
     *     "static": {
     *        "name": "The Node unique name, type: string",
     *        "id": "The Node unique ID, type: uint32_t",
     *        "processorType": "The processor type, type: string",
     *                      options: [cpu, gpu, htp0, htp1]",
     *        "outputWidth": "The output width, type: uint32_t",
     *        "outputHeight": "The output height, type: uint32_t",
     *        "outputFormat": "The output format, type: string,
     *                         options: [rgb, bgr]",
     *        "bEnableUndistortion": "Enable undistortion or not, type: bool",
     *        "bEnableNormalize": "Enable normalization or not, type: bool",
     *        "RAdd": "The normalize parameter for R channel add, type: float",
     *        "RMul": "The normalize parameter for R channel mul, type: float",
     *        "RSub": "The normalize parameter for R channel sub, type: float",
     *        "GAdd": "The normalize parameter for G channel add, type: float",
     *        "GMul": "The normalize parameter for G channel mul, type: float",
     *        "GSub": "The normalize parameter for G channel sub, type: float",
     *        "BAdd": "The normalize parameter for B channel add, type: float",
     *        "BMul": "The normalize parameter for B channel mul, type: float",
     *        "BSub": "The normalize parameter for B channel sub, type: float",
     *        "inputs": [
     *           {
     *              "inputWidth": "The input width, type: uint32_t",
     *              "inputHeight": "The input height, type: uint32_t",
     *              "inputFormat": "The input format, type: string,
     *                  options: [rgb, uyvy, nv12, nv12_ubwc]",
     *              "mapWidth": "The map width, type: uint32_t",
     *              "mapHeight": "The map height, type: uint32_t",
     *              "roiX": "The input roiX, type: uint32_t",
     *              "roiY": "The input roiY, type: uint32_t",
     *              "roiWidth": "The input roiWidth, type: uint32_t",
     *              "roiHeight": "The input roiHeight, type: uint32_t",
     *              "mapX": "The buffer id of X direction map table, type: uint32_t",
     *              "mapY": "The buffer id of Y direction map table, type: uint32_t"
     *           }
     *       ],
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
     * @note: mapX and mapY are optional, only used when bEnableUndistortion is true.
     *        The quantization and normalization parameters are optional,
     *        only used when bEnableNormalize is true.
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

private:
    Remap &m_remap;
    std::string m_options;

public:
    RemapConfig_t m_config;
    uint32_t m_mapXBufferIds[QC_MAX_INPUTS];
    uint32_t m_mapYBufferIds[QC_MAX_INPUTS];
    uint32_t m_numOfInputs;
};

// TODO: how to handle RemapMonitorConfig
typedef struct RemapMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} RemapMonitorConfig_t;

// TODO: how to handle RemapMonitoringIfs
class RemapMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    RemapMonitoringIfs() {}
    ~RemapMonitoringIfs() {}

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
    RemapMonitorConfig_t m_config;
};

class Remap : public NodeBase
{
public:
    /**
     * @brief Remap Constructor
     * @return None
     */
    Remap() : m_configIfs( m_logger, m_remap ) {};

    /**
     * @brief Remap Destructor
     * @return None
     */
    ~Remap() {};

    /**
     * @brief Initializes Node Remap.
     * @param[in] config The Node Remap configuration.
     * @note QCNodeInit::config - Refer to the comments of the API RemapConfigIfs::VerifyAndSet.
     * @note QCNodeInit::buffers - Buffers provided by the user application. The buffers can be
     * provided for the following purposes:
     * - 1. A buffer provided to store the mapping tables for remap work mode.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node Remap configuration interface.
     * @return A reference to the Node Remap configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node Remap monitoring interface.
     * @return A reference to the Node Remap monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node Remap
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note The configuration globalBufferIdMap determines which buffers are input and which
     * are output.
     * @example For a Remap pipeline with N inputs:
     * - The globalBufferIdMap[0].globalBufferId of QCFrameDescriptorNodeIfs will be input 0.
     * - The globalBufferIdMap[1].globalBufferId of QCFrameDescriptorNodeIfs will be input 1.
     * - ...
     * - The globalBufferIdMap[N-1].globalBufferId of QCFrameDescriptorNodeIfs will be input N-1.
     * - The globalBufferIdMap[N].globalBufferId of QCFrameDescriptorNodeIfs will be output.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the Node Remap
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node Remap
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the Node Remap
     * @return The current state of the Node Remap
     */
    virtual QCObjectState_e GetState()
    {
        return static_cast<QCObjectState_e>( m_remap.GetState() );
    }

private:
    QCStatus_e SetupGlobalBufferIdMap( const RemapConfig_t &cfg );

private:
    QC::component::Remap m_remap;
    RemapConfigIfs m_configIfs;
    RemapMonitoringIfs m_monitorIfs;
    bool m_bDeRegisterAllBuffersWhenStop = false;

    uint32_t m_inputNum;
    uint32_t m_outputNum = 1;

    std::vector<QCNodeBufferMapEntry_t> m_globalBufferIdMap;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_REMAP_HPP
