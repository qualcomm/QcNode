// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_CL2DFLEX_HPP
#define QC_NODE_CL2DFLEX_HPP

#include "QC/component/CL2DFlex.hpp"
#include "QC/node/NodeBase.hpp"

namespace QC
{
namespace node
{
using namespace QC::component;

/**
 * @brief CL2D Node Configuration Data Structure
 * @param params The QC component CL2D configuration data structure.
 * @param bufferIds The indices of buffers in QCNodeInit::buffers provided by the user application
 * for use by CL2D. These buffers will be registered into CL2D during the initialization stage.
 * @note bufferIds are optional and can be empty, in which case the buffers will be registered into
 * CL2D when the API ProcessFrameDescriptor is called.
 * @param globalBufferIdMap The global buffer index map used to identify which buffer in
 * QCFrameDescriptorNodeIfs is used for CL2D input(s) and output(s).
 * @note globalBufferIdMap is optional and can be empty, in which case a default buffer index map
 * will be applied for CL2D input(s) and output(s). For now CL2D only support multiple inputs to
 * single output
 * - The index 0 of QCFrameDescriptorNodeIfs will be input 0.
 * - The index 1 of QCFrameDescriptorNodeIfs will be input 1.
 * - ...
 * - The index N-1 of QCFrameDescriptorNodeIfs will be input N-1.
 * - The index N of QCFrameDescriptorNodeIfs will be output.
 * @param bDeRegisterAllBuffersWhenStop When the Stop API of the CL2DFlex node is called and
 * bDeRegisterAllBuffersWhenStop is true, deregister all buffers.
 */
typedef struct CL2DFlexConfig : public QCNodeConfigBase_t
{
    CL2DFlex_Config_t params;
    std::vector<uint32_t> bufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} CL2DFlexConfig_t;

class CL2DFlexConfigIfs : public NodeConfigIfs
{
public:
    /**
     * @brief CL2DFlexConfigIfs Constructor
     * @param[in] logger A reference to the logger to be shared and used by CL2DFlexConfigIfs.
     * @param[in] cl2d A reference to the QC CL2D component to be used by CL2DFlexConfigIfs.
     * @return None
     */
    CL2DFlexConfigIfs( Logger &logger, CL2DFlex &cl2d ) : NodeConfigIfs( logger ), m_cl2d( cl2d ) {}

    /**
     * @brief CL2DFlexConfigIfs Destructor
     * @return None
     */
    ~CL2DFlexConfigIfs() {}

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
     *        "outputWidth": "The output width, type: uint32_t",
     *        "outputHeight": "The output height, type: uint32_t",
     *        "outputFormat": "The output format, type: string,
     *                         options: [rgb, bgr, uyvy, nv12, nv12_ubwc]",
     *        "inputs": [
     *           {
     *              "inputWidth": "The input width, type: uint32_t",
     *              "inputHeight": "The input height, type: uint32_t",
     *              "inputFormat": "The input format, type: string,
     *                  options: [rgb, bgr, uyvy, nv12, nv12_ubwc]",
     *              "roiX": "The input roiX, type: uint32_t",
     *              "roiY": "The input roiY, type: uint32_t",
     *              "roiWidth": "The input roiWidth, type: uint32_t",
     *              "roiHeight": "The input roiHeight, type: uint32_t",
     *              "workMode": "The input work mode, type: string,
     *                          options: [convert, resize_nearest, letterbox_nearest, convert_ubwc,
     *                          letterbox_nearest_multiple, resize_nearest_multiple, remap_nearest]"
     *              "mapX": "The buffer id of X direction map table, type: uint32_t",
     *              "mapY": "The buffer id of Y direction map table, type: uint32_t"
     *           }
     *       ],
     * @note: mapX and mapY are optional, only used for remap_nearest work mode.
     *        "ROIs": [
     *           {
     *              "roiX": "The input roiX, type: uint32_t",
     *              "roiY": "The input roiY, type: uint32_t",
     *              "roiWidth": "The input roiWidth, type: uint32_t",
     *              "roiHeight": "The input roiHeight, type: uint32_t",
     *           }
     *       ],
     * @note: ROIs configuration is optional, only used for resize_nearest_multiple and
     *        letterbox_nearest_multiple work mode.
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
    CL2DFlex &m_cl2d;
    std::string m_options;

public:
    CL2DFlexConfig_t m_config;
    uint32_t m_mapXBufferIds[QC_MAX_INPUTS];
    uint32_t m_mapYBufferIds[QC_MAX_INPUTS];
    CL2DFlex_ROIConfig_t m_ROIs[QC_CL2DFLEX_ROI_NUMBER_MAX];
    uint32_t m_numOfInputs;
    uint32_t m_numOfROIs;
};

// TODO: how to handle CL2DFlexMonitorConfig
typedef struct CL2DFlexMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bPerfEnabled;
} CL2DFlexMonitorConfig_t;

// TODO: how to handle CL2DFlexMonitoringIfs
class CL2DFlexMonitoringIfs : public QCNodeMonitoringIfs
{
public:
    CL2DFlexMonitoringIfs() {}
    ~CL2DFlexMonitoringIfs() {}

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
    CL2DFlexMonitorConfig_t m_config;
};

class CL2DFlex : public NodeBase
{
public:
    /**
     * @brief CL2DFlex Constructor
     * @return None
     */
    CL2DFlex() : m_configIfs( m_logger, m_cl2d ) {};

    /**
     * @brief CL2DFlex Destructor
     * @return None
     */
    ~CL2DFlex() {};

    /**
     * @brief Initializes Node CL2D.
     * @param[in] config The Node CL2D configuration.
     * @note QCNodeInit::config - Refer to the comments of the API CL2DFlexConfigIfs::VerifyAndSet.
     * @note QCNodeInit::buffers - Buffers provided by the user application. The buffers can be
     * provided for the following purposes:
     * - 1. A buffer provided to store the mapping tables for remap work mode.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node CL2D configuration interface.
     * @return A reference to the Node CL2D configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node CL2D monitoring interface.
     * @return A reference to the Node CL2D monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node CL2D
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note The configuration globalBufferIdMap determines which buffers are input and which
     * are output.
     * @example For a CL2D model with N inputs:
     * - The globalBufferIdMap[0].globalBufferId of QCFrameDescriptorNodeIfs will be input 0.
     * - The globalBufferIdMap[1].globalBufferId of QCFrameDescriptorNodeIfs will be input 1.
     * - ...
     * - The globalBufferIdMap[N-1].globalBufferId of QCFrameDescriptorNodeIfs will be input N-1.
     * - The globalBufferIdMap[N].globalBufferId of QCFrameDescriptorNodeIfs will be output.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the Node CL2D
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node CL2D
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the Node CL2D
     * @return The current state of the Node CL2D
     */
    virtual QCObjectState_e GetState() { return static_cast<QCObjectState_e>( m_cl2d.GetState() ); }

private:
    QCStatus_e SetupGlobalBufferIdMap( const CL2DFlexConfig_t &cfg );

private:
    QC::component::CL2DFlex m_cl2d;
    CL2DFlexConfigIfs m_configIfs;
    CL2DFlexMonitoringIfs m_monitorIfs;
    bool m_bDeRegisterAllBuffersWhenStop = false;

    uint32_t m_inputNum;
    uint32_t m_outputNum = 1;

    std::vector<QCNodeBufferMapEntry_t> m_globalBufferIdMap;
};

}   // namespace node
}   // namespace QC

#endif   // QC_NODE_CL2DFLEX_HPP
