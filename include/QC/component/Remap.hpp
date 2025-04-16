// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_REMAP_HPP
#define QC_REMAP_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "FadasRemap.hpp"
#include "QC/component/ComponentIF.hpp"
#include "fadas.h"

using namespace QC::common;
using namespace QC::libs::FadasIface;

namespace QC
{
namespace component
{

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief remap tables for input images */
typedef struct
{
    QCSharedBuffer_t *pMapX; /**<shared buffer for X map, each element is the column coordinate of
               the mapped location in the source image, data size is mapWidth * mapHeight*/
    QCSharedBuffer_t *pMapY; /**<shared buffer for Y map, each element is the row coordinate
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

class Remap : public ComponentIF
{
    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    Remap();
    ~Remap();

    /**
     * @brief Initialize the remap pipeline
     * @param[in] pName the remap unique instance name
     * @param[in] pConfig the remap configuration paramaters
     * @param[in] level the logger message level
     * @return QC_STATUS_OK on success, others on failure
     * @note Do all the initialization work for remap pipeline: initialize the processor and logger,
     * create remap worker, create remap map. Should be called at the beginning of pipeline.
     */
    QCStatus_e Init( const char *pName, const Remap_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for remap
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @param[in] bufferType buffer type, could be IN, OUT, INOUT
     * @return QC_STATUS_OK on success, others on failure
     * @note Register buffers for input and output data. This step could be done by user or skipped.
     * If skipped, all the buffers will be registered at execute step.
     */
    QCStatus_e RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers,
                                FadasBufType_e bufferType );

    /**
     * @brief Start the remap pipeline, empty for now
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief execute the remap pipeline
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return QC_STATUS_OK on success, others on failure
     * @note Execute the remap pipeline with FastADAS API. The pipeline is remapping multiple input
     * images buffer to single output image buffer.
     */
    QCStatus_e Execute( const QCSharedBuffer_t *pInputs, uint32_t numInputs,
                        const QCSharedBuffer_t *pOutput );

    /**
     * @brief Stop the remap pipeline, empty for now
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief Deregister buffers for remap
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note Deregister buffers for input and output data. This step could be done by user or
     * skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    QCStatus_e DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the remap pipeline
     * @return QC_STATUS_OK on success, others on failure
     * @note Do all the deinitialization work for remap pipeline: deinitialize processor and logger,
     * destroy remap worker, destroy remap map. Should be called at the ending of pipeline.
     */
    QCStatus_e Deinit();

private:
    FadasRemap m_fadasRemapObj;
    Remap_Config_t m_config;

};   // class Remap

}   // namespace component
}   // namespace QC

#endif   // QC_REMAP_HPP
