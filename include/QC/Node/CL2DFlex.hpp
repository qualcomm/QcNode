// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_CL2DFLEX_HPP
#define QC_NODE_CL2DFLEX_HPP

#include "OpenclIface.hpp"
#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{

/** @brief The QCNode CL2DFLEX Version */
#define QCNODE_CL2DFLEX_VERSION_MAJOR 2U
#define QCNODE_CL2DFLEX_VERSION_MINOR 0U
#define QCNODE_CL2DFLEX_VERSION_PATCH 0U

#define QCNODE_CL2DFLEX_VERSION                                                                    \
    ( ( QCNODE_CL2DFLEX_VERSION_MAJOR << 12U ) | ( QCNODE_CL2DFLEX_VERSION_MINOR << 8U ) |         \
      QCNODE_CL2DFLEX_VERSION_PATCH )

using namespace QC::libs::OpenclIface;

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief CL2DFlex valid work modes */
typedef enum
{
    CL2DFLEX_WORK_MODE_CONVERT, /**<color convert only, roi.width should be equal to output width
                                  and roi.height should be equal to output height*/
    CL2DFLEX_WORK_MODE_RESIZE_NEAREST,    /**<color convert and resize use nearest point*/
    CL2DFLEX_WORK_MODE_RESIZE_BILINEAR,   /**<color convert and resize use bilinear interpolation*/
    CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST, /**<color convert and letterbox with fixed height/width
                                            ratio use nearest point*/
    CL2DFLEX_WORK_MODE_LETTERBOX_NEAREST_MULTIPLE, /**<color convert and letterbox with fixed
                                                      height/width ratio use nearest point, execute
                                                      on multiple batches with different ROI
                                                      paramters*/
    CL2DFLEX_WORK_MODE_RESIZE_NEAREST_MULTIPLE,    /**<color convert and resize use nearest point,
                                                      execute on multiple batches with different ROI
                                                      paramters*/
    CL2DFLEX_WORK_MODE_CONVERT_UBWC,  /**<convert from ubwc compress format to normal format*/
    CL2DFLEX_WORK_MODE_REMAP_NEAREST, /**<color convert and remap using nearest point in map table
                                         to do undistortion*/
    CL2DFLEX_WORK_MODE_MAX
} CL2DFlex_Work_Mode_e;

/** @brief remap tables for input images */
typedef struct
{
    uint32_t mapXBufferId; /**<tensor buffer ID for X map, in the buffer each element is the column
coordinate of the mapped location in the source image, data size is mapWidth * mapHeight*/
    uint32_t mapYBufferId; /**<tensor buffer ID for Y map, in the buffer each element is the row
coordinate of the mapped location in the source image, data size is mapWidth * mapHeight*/

} CL2DFlex_MapTable_t;

/** @brief CL2DFlex input images ROI configuration */
typedef struct
{
    uint32_t x;      /**<ROI beginnning x coordinate*/
    uint32_t y;      /**<ROI beginnning y coordinate*/
    uint32_t width;  /**<ROI width, x+width must be smaller than
                        input width*/
    uint32_t height; /**<ROI height, y+height must be smaller
                        than input height*/
} CL2DFlex_ROIConfig_t;

/** @brief CL2DFlex component configuration */
typedef struct
{
    uint32_t numOfInputs;                          /**<number of input images*/
    CL2DFlex_Work_Mode_e workModes[QC_MAX_INPUTS]; /**<work mode for each input*/
    QCImageFormat_e inputFormats[QC_MAX_INPUTS];   /**<input image format for each batch*/
    uint32_t inputWidths[QC_MAX_INPUTS];           /**<input image width for each batch, must be
                                                         an integer   multiple of 2*/
    uint32_t inputHeights[QC_MAX_INPUTS];          /**<input image height for each batch, must be an
                                                         integer  multiple of 2*/
    CL2DFlex_ROIConfig_t ROIs[QC_MAX_INPUTS];      /**<ROI configurations for each batch*/
    QCImageFormat_e outputFormat;                  /**<output image format*/
    uint32_t outputWidth;                          /**<output image width*/
    uint32_t outputHeight;                         /**<output image height*/
    uint32_t letterboxPaddingValue =
            0; /**<the padding value for letterbox resize with fixed height/width ratio, uint32
                  value in which the lower 24 bits are composed of three 8 bits values representing
                  R,G,B respectively, default set to 0 which means all black */
    CL2DFlex_MapTable_t remapTable[QC_MAX_INPUTS]; /**<remap table, used for remap work mode*/
    uint32_t numOfROIs;    /**<number of output ROIs for multiple ROI work modes*/
    uint32_t ROIsBufferId; /**<ROIs buffer ID for multiple ROI work modes*/
    OpenclIfcae_Perf_e priority =
            OPENCLIFACE_PERF_NORMAL; /**<OpenCL performance priority level, default set to normal*/
    uint32_t deviceId = 0;           /**<OpenCL device ID, default set to 0*/

} CL2DFlex_Config_t;

/**
 * @brief Represents the CL2DFlex implementation used by CL2DFlexConfig, CL2DFlexMonitor, and Node
 * CL2DFlex.
 *
 * This class encapsulates the specific implementation details of the
 * CL2DFlex that are shared across configuration, monitoring,  and Node CL2DFlex.
 *
 * It serves as a central reference for components that need to interact with
 * the underlying CL2DFlex implementation.
 */
class CL2DFlexImpl;

class CL2DFlexConfig : public NodeConfigIfs
{
public:
    /**
     * @brief CL2DFlexConfig Constructor
     * @param[in] logger A reference to the logger to be shared and used by CL2DFlexConfig.
     * @param[in] pCL2DFlexImpl A pointer to the CL2DFlexImpl object to be used by CL2DFlexConfig.
     * @return None
     */
    CL2DFlexConfig( Logger &logger, CL2DFlexImpl *pCL2DFlexImpl )
        : NodeConfigIfs( logger ),
          m_pCL2DFlexImpl( pCL2DFlexImpl )
    {}

    /**
     * @brief CL2DFlexConfig Destructor
     * @return None
     */
    ~CL2DFlexConfig() {}

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
     *        "logLevel": "The message log level, type: string,
     *                     options: [VERBOSE, DEBUG, INFO, WARN, ERROR],
     *                     default: ERROR"
     *        "priority": "The performance priority level, type: string,
     *                     options: [low, normal, high]",
     *        "deviceId": "The device ID for OpenCL platform, type: uint32_t",
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
     *              "mapXBufferId": "The buffer id of X direction map table, type: uint32_t",
     *              "mapYBufferId": "The buffer id of Y direction map table, type: uint32_t"
     *           }
     *       ],
     *        "numOfROIs": "The number of ROIs for multiple ROIs work mode, type: uint32_t",
     *        "ROIsBufferId": "The ROIs buffer ID for multiple ROIs work mode, type: uint32_t",
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
     * @note: priority is optional, default set to normal.
     *        deviceId is optional, default set to 0.
     *        mapXBufferId and mapYBufferId are optional, only used for remap_nearest work mode.
     *        numOfROIs and ROIsBufferId are optional, only used for resize_nearest_multiple and
     *        letterbox_nearest_multiple work mode.
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
    CL2DFlexImpl *m_pCL2DFlexImpl;
    std::string m_options;
};

// TODO: how to handle CL2DFlexMonitoring
typedef struct CL2DFlexMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bEnablePerf;
} CL2DFlexMonitorConfig_t;

class CL2DFlexMonitoring : public QCNodeMonitoringIfs
{
public:
    /**
     * @brief CL2DFlexMonitor Constructor
     * @param[in] logger A reference to the logger to be shared and used by CL2DFlexConfig.
     * @param[in] pCL2DFlexImpl A pointer to the CL2DFlexImpl object to be used by CL2DFlexConfig.
     * @return None
     */
    CL2DFlexMonitoring( Logger &logger, CL2DFlexImpl *pCL2DFlexImpl )
        : m_logger( logger ),
          m_pCL2DFlexImpl( pCL2DFlexImpl )
    {}
    ~CL2DFlexMonitoring() {}

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
    CL2DFlexImpl *m_pCL2DFlexImpl;
    Logger &m_logger;
    std::string m_options;
    CL2DFlexMonitorConfig_t m_monitorConfig;
};

class CL2DFlex : public NodeBase
{
public:
    /**
     * @brief CL2DFlex Constructor
     * @return None
     */
    CL2DFlex();

    /**
     * @brief CL2DFlex Destructor
     * @return None
     */
    ~CL2DFlex();

    /**
     * @brief Initializes Node CL2DFlex.
     * @param[in] config The Node CL2DFlex configuration.
     * @note QCNodeInit::config - Refer to the comments of the API CL2DFlexConfig::VerifyAndSet.
     * @note QCNodeInit::buffers - Buffers provided by the user application. The buffers can be
     * provided for the following purposes:
     * - 1. A buffer provided to store the mapping tables for remap work mode.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e Initialize( QCNodeInit_t &config );

    /**
     * @brief Get the Node CL2DFlex configuration interface.
     * @return A reference to the Node CL2DFlex configuration interface.
     */
    virtual QCNodeConfigIfs &GetConfigurationIfs() { return m_configIfs; }

    /**
     * @brief Get the Node CL2DFlex monitoring interface.
     * @return A reference to the Node CL2DFlex monitoring interface.
     */
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() { return m_monitorIfs; }

    /**
     * @brief Start the Node CL2DFlex
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start();

    /**
     * @brief Processes the Frame Descriptor.
     * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
     * @note The configuration globalBufferIdMap determines which buffers are input and which
     * are output.
     * @example For a CL2DFlex pipeline with N inputs:
     * - The globalBufferIdMap[0].globalBufferId of QCFrameDescriptorNodeIfs will be input 0.
     * - The globalBufferIdMap[1].globalBufferId of QCFrameDescriptorNodeIfs will be input 1.
     * - ...
     * - The globalBufferIdMap[N-1].globalBufferId of QCFrameDescriptorNodeIfs will be input N-1.
     * - The globalBufferIdMap[N].globalBufferId of QCFrameDescriptorNodeIfs will be output.
     * @return QC_STATUS_OK on success, or an error code on failure.
     */
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the Node CL2DFlex
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop();

    /**
     * @brief De-initialize Node CL2DFlex
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e DeInitialize();

    /**
     * @brief Get the current state of the Node CL2DFlex
     * @return The current state of the Node CL2DFlex
     */
    virtual QCObjectState_e GetState();

private:
    CL2DFlexImpl *m_pCL2DFlexImpl;
    CL2DFlexConfig m_configIfs;
    CL2DFlexMonitoring m_monitorIfs;
};

}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_CL2DFLEX_HPP
