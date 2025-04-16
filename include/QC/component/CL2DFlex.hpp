// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2DFLEX_HPP
#define QC_CL2DFLEX_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "OpenclIface.hpp"
#include "QC/component/ComponentIF.hpp"

#define QC_CL2DFLEX_ROI_NUMBER_MAX 100 /**<max number of roi parameters for ExecuteWithROI API*/

namespace QC
{
namespace component
{

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
    QCSharedBuffer_t
            *pMapX; /**<shared buffer for X map, each element is the column coordinate of the mapped
                       location in the source image, data size is mapWidth * mapHeight*/
    QCSharedBuffer_t
            *pMapY; /**<shared buffer for Y map, each element is the row coordinate of the mapped
                       location in the source image, data size is mapWidth * mapHeight*/
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
    QCImageFormat_e outputFormat;                  /**<output image format*/
    uint32_t outputWidth;                          /**<output image width*/
    uint32_t outputHeight;                         /**<output image height*/
    CL2DFlex_ROIConfig_t ROIs[QC_MAX_INPUTS];      /**<ROI configurations for each batch*/
    uint32_t letterboxPaddingValue =
            0; /**<the padding value for letterbox resize with fixed height/width ratio, uint32
                  value in which the lower 24 bits are composed of three 8 bits values representing
                  R,G,B respectively, default set to 0 which means all black */
    CL2DFlex_MapTable_t remapTable[QC_MAX_INPUTS]; /**<remap table, used for remap work mode*/
    OpenclIfcae_Perf_e priority =
            OPENCLIFACE_PERF_NORMAL; /**<OpenCL performance priority level, default set to normal*/
} CL2DFlex_Config_t;

class CL2DPipelineBase; /**<pipeline base class*/

class CL2DFlex : public ComponentIF
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    CL2DFlex();
    ~CL2DFlex();

    /**
     * @brief Initialize the CL2DFlex pipeline
     * @param[in] pName the CL2DFlex unique instance name
     * @param[in] pConfig the CL2DFlex configuration paramaters
     * @param[in] level the logger message level
     * @return QC_STATUS_OK on success, others on failure
     * @note Do all the initialization work for a CL2DFlex pipeline: parse configuration parameters,
     * setup OpenCL command queue and context, load OpenCL kernel and build OpenCL program. Must be
     * called at the beginning of pipeline.
     */
    QCStatus_e Init( const char *pName, const CL2DFlex_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for CL2DFlex
     * @param[in] pBuffers buffers to be registered
     * @param[in] numBuffers number of buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note Register device buffers from host buffers for input and output data. This step can be
     * done by user or skipped. If skipped, all the buffers will be registered at execute step.
     */
    QCStatus_e RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Start the CL2DFlex pipeline, empty for now
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief Execute the CL2DFlex pipeline normally
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return QC_STATUS_OK on success, others on failure
     * @note Execute the CL2DFlex pipeline. Currently support color conversion and resize of
     * multiple image inputs to single output. The supported color conversion pipelines are NV12 to
     * RGB, UYVY to RGB, UYVY to NV12.
     */
    QCStatus_e Execute( const QCSharedBuffer_t *pInputs, const uint32_t numInputs,
                        const QCSharedBuffer_t *pOutput );

    /**
     * @brief Execute the CL2DFlex pipeline with ROI parameters
     * @param[in] pInput the input shared buffer
     * @param[out] pOutput the output shared buffer
     * @param[in] pROIs the ROI configurations for each execution
     * @param[in] numROIs the number of ROI configuration parameters, also equal to the batch number
     * of output buffer, must be not bigger than QC_CL2DFLEX_ROI_NUMBER_MAX
     * @return QC_STATUS_OK on success, others on failure
     * @note Execute the CL2DFlex pipeline with ROI parameters, one input buffer to one output
     * buffer with multiple batches, used for cases such as traffic light detection.
     */
    QCStatus_e ExecuteWithROI( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput,
                               const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );

    /**
     * @brief Stop the CL2DFlex pipeline, empty for now
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief Deregister buffers for CL2DFlex
     * @param[in] pBuffers buffers to be deregistered
     * @param[in] numBuffers number of buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note Deregister device buffers from host buffers for input and output data. This step can
     * be done by user or skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    QCStatus_e DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the CL2DFlex pipeline
     * @return QC_STATUS_OK on success, others on failure
     * @note Do all the deinitialization work for a CL2DFlex pipeline: release OpenCL context,
     * command queue, kernel and program, deregister all the OpenCL buffers remained. Must be
     * called at the ending of pipeline.
     */
    QCStatus_e Deinit();

private:
    CL2DFlex_Config_t m_config;
    OpenclSrv m_OpenclSrvObj;
    CL2DPipelineBase *m_pCL2DPipeline[QC_MAX_INPUTS] = { nullptr };
    cl_kernel m_kernel[QC_MAX_INPUTS];

};   // class CL2DFlex

}   // namespace component
}   // namespace QC

#endif   // QC_CL2DFLEX_HPP
