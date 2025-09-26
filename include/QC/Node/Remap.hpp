// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_NODE_REMAP_HPP
#define QC_NODE_REMAP_HPP

#include "FadasRemap.hpp"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{
using namespace QC::libs::FadasIface;

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief remap tables for input images */
typedef struct
{
    uint32_t mapXBufferId; /**<buffer ID for X map, in the buffer each element is the column
               coordinate of the mapped location in the source image, data size is mapWidth *
               mapHeight*/
    uint32_t mapYBufferId; /**<buffer ID for Y map, in the buffer each element is the row coordinate
                     of the mapped location in the source image, data size is mapWidth * mapHeight*/
} Remap_MapTable_t;

/** @brief remap configuration for each input image*/
typedef struct
{
    QCImageFormat_e inputFormat; /**<input image format*/
    uint32_t inputWidth;         /**<input format width*/
    uint32_t inputHeight;        /**<input format height*/
    uint32_t mapWidth;           /**<output map width*/
    uint32_t mapHeight;          /**<output map height*/
    Remap_MapTable_t remapTable; /**<remap table, used if enable undistortion*/
    FadasROI_t ROI; /**<region of interest structure work on image after mapping, the ROI width and
                       height should be consistent with output width and height*/
} Remap_InputConfig_t;

/** @brief Remap component configuration */
typedef struct
{
    QCProcessorType_e processor;                     /**<pipelie processor type*/
    Remap_InputConfig_t inputConfigs[QC_MAX_INPUTS]; /**<input images configuration*/
    uint32_t numOfInputs;                            /**<number of input images*/
    uint32_t outputWidth;                            /**<output image width*/
    uint32_t outputHeight;                           /**<output image height*/
    QCImageFormat_e outputFormat;                    /**<output image format*/
    FadasNormlzParams_t normlzR;                     /**<normalize parameter for R channel*/
    FadasNormlzParams_t normlzG;                     /**<normalize parameter for G channel*/
    FadasNormlzParams_t normlzB;                     /**<normalize parameter for B channel*/
    bool bEnableUndistortion;                        /**<enable undistortion or not*/
    bool bEnableNormalize;                           /**<enable normalization or not*/
} Remap_Config_t;

/**
 * @brief Represents the Remap implementation used by RemapConfig, RemapMonitor, and Node
 * Remap.
 *
 * This class encapsulates the specific implementation details of the
 * Remap that are shared across configuration, monitoring,  and Node Remap.
 *
 * It serves as a central reference for components that need to interact with
 * the underlying Remap implementation.
 */
class RemapImpl;

class RemapConfig : public NodeConfigIfs
{
public:
    /**
     * @brief RemapConfig Constructor
     * @param[in] logger A reference to the logger to be shared and used by RemapConfig.
     * @param[in] pRemapImpl A pointer to the RemapImpl object to be used by RemapConfig.
     * @return None
     */
    RemapConfig( Logger &logger, RemapImpl *pRemapImpl )
        : NodeConfigIfs( logger ),
          m_pRemapImpl( pRemapImpl )
    {}

    /**
     * @brief RemapConfig Destructor
     * @return None
     */
    ~RemapConfig() {}

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
     *              "mapXBufferId": "The buffer id of X direction map table, type: uint32_t",
     *              "mapYBufferId": "The buffer id of Y direction map table, type: uint32_t"
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
     * @note: mapXBufferId and mapYBufferId are optional,
     *        only used when bEnableUndistortion is true.
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
    virtual const QCNodeConfigBase_t &Get();

private:
    QCStatus_e VerifyStaticConfig( DataTree &dt, std::string &errors );
    QCStatus_e ParseStaticConfig( DataTree &dt, std::string &errors );

private:
    RemapImpl *m_pRemapImpl;
    std::string m_options;
    uint32_t m_numOfInputs;
};

// TODO: how to handle RemapMonitoring
typedef struct RemapMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bEnablePerf;
} RemapMonitorConfig_t;

class RemapMonitoring : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief RemapMonitor Constructor
     * @param[in] logger A reference to the logger to be shared and used by RemapConfig.
     * @param[in] pRemapImpl A pointer to the RemapImpl object to be used by RemapConfig.
     * @return None
     */
    RemapMonitoring( Logger &logger, RemapImpl *pRemapImpl )
        : m_logger( logger ),
          m_pRemapImpl( pRemapImpl )
    {}
    ~RemapMonitoring() {}

    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors )
    {
        return QC_STATUS_UNSUPPORTED;
    }

    virtual const std::string &GetOptions() { return m_options; }

    virtual const QCNodeMonitoringBase_t &Get() { return m_monitorConfig; }

    virtual uint32_t GetMaximalSize() { return UINT32_MAX; }
    virtual uint32_t GetCurrentSize() { return UINT32_MAX; }


    virtual QCStatus_e Place( void *ptr, uint32_t &size ) { return QC_STATUS_UNSUPPORTED; }

private:
    RemapImpl *m_pRemapImpl;
    Logger &m_logger;
    std::string m_options;
    RemapMonitorConfig_t m_monitorConfig;
};

class Remap : public NodeBase
{
public:
    /**
     * @brief Remap Constructor
     * @return None
     */
    Remap();

    /**
     * @brief Remap Destructor
     * @return None
     */
    ~Remap();

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
    virtual QCObjectState_e GetState();

private:
    RemapImpl *m_pRemapImpl;
    RemapConfig m_configIfs;
    RemapMonitoring m_monitorIfs;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_REMAP_HPP
