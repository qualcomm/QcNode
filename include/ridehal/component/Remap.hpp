// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_REMAP_HPP
#define RIDEHAL_REMAP_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "FadasRemap.hpp"
#include "fadas.h"
#include "ridehal/component/ComponentIF.hpp"

using namespace ridehal::common;
using namespace ridehal::libs::FadasIface;

namespace ridehal
{
namespace component
{

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief remap tables for input images */
typedef struct
{
    RideHal_SharedBuffer_t
            *pMapX; /**<shared buffer for X map, each element is the column coordinate of the mapped
      location in the source image, data size is mapWidth * mapHeight*/
    RideHal_SharedBuffer_t *pMapY; /**<shared buffer for Y map, each element is the row coordinate
                     of the mapped location in the source image, data size is mapWidth * mapHeight*/
} Remap_MapTable_t;

/** @brief remap configuration for each input image*/
typedef struct
{
    RideHal_ImageFormat_e inputFormat; /**<input image format*/
    uint32_t inputWidth;               /**<input format width*/
    uint32_t inputHeight;              /**<input format height*/
    uint32_t mapWidth;                 /**<output map width*/
    uint32_t mapHeight;                /**<output map height*/
    Remap_MapTable_t remapTable;       /**<remap table, used if enable undistortion*/
    FadasROI_t ROI; /**<region of interest structure work on image after mapping, the ROI width and
                       height should be consistent with output width and height*/
} Remap_InputConfig_t;

/** @brief Remap component configuration */
typedef struct
{
    RideHal_ProcessorType_e processor;                    /**<pipelie processor type*/
    Remap_InputConfig_t inputConfigs[RIDEHAL_MAX_INPUTS]; /**<input images configuration*/
    uint32_t numOfInputs;                                 /**<number of input images*/
    uint32_t outputWidth;                                 /**<output image width*/
    uint32_t outputHeight;                                /**<output image height*/
    RideHal_ImageFormat_e outputFormat;                   /**<output image format*/
    FadasNormlzParams_t normlzR;                          /**<normalize parameter for R channel*/
    FadasNormlzParams_t normlzG;                          /**<normalize parameter for G channel*/
    FadasNormlzParams_t normlzB;                          /**<normalize parameter for B channel*/
    bool bEnableUndistortion;                             /**<enable undistortion or not*/
    bool bEnableNormalize;                                /**<enable normalization or not*/
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
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Do all the initialization work for remap pipeline: initialize the processor and logger,
     * create remap worker, create remap map. Should be called at the beginning of pipeline.
     */
    RideHalError_e Init( const char *pName, const Remap_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Register buffers for remap
     * @param[in] pBuffers a list of buffers to be registeer
     * @param[in] numBuffers number of buffers
     * @param[in] bufferType buffer type, could be IN, OUT, INOUT
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Register buffers for input and output data. This step could be done by user or skipped.
     * If skipped, all the buffers will be registered at execute step.
     */
    RideHalError_e RegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers,
                                    FadasBufType_e bufferType );

    /**
     * @brief Start the remap pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @brief execute the remap pipeline
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Execute the remap pipeline with FastADAS API. The pipeline is remapping multiple input
     * images buffer to single output image buffer.
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutput );

    /**
     * @brief Stop the remap pipeline, empty for now
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @brief Deregister buffers for remap
     * @param[in] pBuffers a list of buffers to be deregister
     * @param[in] numBuffers number of buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Deregister buffers for input and output data. This step could be done by user or
     * skipped. If skipped, all the buffers will be deregistered at deinit step.
     */
    RideHalError_e DeRegisterBuffers( const RideHal_SharedBuffer_t *pBuffers, uint32_t numBuffers );

    /**
     * @brief Deinitialize the remap pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note Do all the deinitialization work for remap pipeline: deinitialize processor and logger,
     * destroy remap worker, destroy remap map. Should be called at the ending of pipeline.
     */
    RideHalError_e Deinit();

private:
    FadasRemap m_fadasRemapObj;
    Remap_Config_t m_config;

};   // class Remap

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_REMAP_HPP
