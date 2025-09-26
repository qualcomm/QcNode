// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_C2D_HPP
#define QC_C2D_HPP

#include <c2d2.h>
#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <vector>

#include "QC/Infras/Log/Logger.hpp"
#include "QC/Infras/Memory/ImageDescriptor.hpp"

namespace QC
{
namespace sample
{

using namespace QC::Memory;

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief C2D Image resolution */
typedef struct
{
    uint32_t width;  /**< Image width */
    uint32_t height; /**< Image height */
} C2D_ImageResolution_t;

/** @brief C2D ROI Config*/
typedef struct
{
    uint32_t topX;   /**< X coordinate of top left point */
    uint32_t topY;   /**< Y coordinate of top left point */
    uint32_t width;  /**< ROI width */
    uint32_t height; /**< ROI height */
} C2D_ROIConfig_t;

/** @brief C2D Input Configurations*/
typedef struct
{
    QCImageFormat_e inputFormat;           /**< Image format of Input frame */
    C2D_ImageResolution_t inputResolution; /**< Image Resolution of Input frame */
    C2D_ROIConfig_t ROI;                   /**< Reigion of Interest in Input frame */
} C2D_InputConfig_t;

/** @brief C2D Component Initialization Configurations*/
typedef struct
{
    uint32_t numOfInputs;                          /**< Number of Input Images in each processing */
    C2D_InputConfig_t inputConfigs[QC_MAX_INPUTS]; /**< Array of Input Configurations */
} C2D_Config_t;


/**
 * @brief Component C2D
 * @brief C2D convert 1 camera frame into another format normalize
 */
class C2D
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    C2D( Logger &logger );
    ~C2D();

    /**
     * @cond C2D::Init @endcond
     * @brief Initialize the C2D component
     * @param[in] name the component unique instance name
     * @param[in] pConfig the C2D configuration paramaters
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const C2D_Config_t *pConfig );

    /**
     * @cond C2D::Start @endcond
     * @brief Start the C2D pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @cond C2D::Stop @endcond
     * @brief stop the C2D pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @cond C2D::Deinit @endcond
     * @brief deinitialize the C2D component
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

    /**
     * @cond C2D::Execute @endcond
     * @brief Execute the C2D pipeline
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of the input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( const ImageDescriptor_t *pInputs, uint32_t numInputs,
                        const ImageDescriptor_t *pOutput );

    /**
     * @cond C2D::RegisterInputBuffers @endcond
     * @brief Register shared buffers for each input
     * @param[in] pInputBuffer the input shared buffers array
     * @param[in] numOfInputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note This API is optional but recommended to call after input buffers allocation finished.
     * If skip to do this, the Execute API will register input buffers automatically.
     */
    QCStatus_e RegisterInputBuffers( const ImageDescriptor_t *pInputBuffer,
                                     uint32_t numOfInputBuffers );

    /**
     * @cond C2D::RegisterOutputBuffers @endcond
     * @brief Register shared buffers for output
     * @param[in] pOutputBuffer the output shared buffer
     * @param[in] numOfOutputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note This API is optional but recommended to call after output buffers allocation finished.
     * If skip to do this, the Execute API will register output buffers automatically.
     */
    QCStatus_e RegisterOutputBuffers( const ImageDescriptor_t *pOutputBuffer,
                                      uint32_t numOfOutputBuffers );

    /**
     * @cond C2D::DeregisterInputBuffers @endcond
     * @brief Deregister shared buffers for each input
     * @param[in] pInputBuffer the input shared buffers array
     * @param[in] numOfInputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeregisterInputBuffers( const ImageDescriptor_t *pInputBuffer,
                                       uint32_t numOfInputBuffers );

    /**
     * @cond C2D::DeregisterOutputBuffers @endcond
     * @brief Deregister shared buffers for output
     * @param[in] pOutputBuffer the output shared buffer
     * @param[in] numOfOutputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeregisterOutputBuffers( const ImageDescriptor_t *pOutputBuffer,
                                        uint32_t numOfOutputBuffers );

private:
    /**
     * @cond C2D::GetSourceSurface @endcond
     * @brief Get the source C2D surface from map. If not found in map, this function will create a
     * new surface
     * @param[in] pSharedBuffer the input shared buffer
     * @param[in] inputIdx the index of input in input list
     * @param[out] c2dObject the blit object structure that specifies the surface
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetSourceSurface( const ImageDescriptor_t *pSharedBuffer, uint32_t inputIdx,
                                 C2D_OBJECT &c2dObject );
    /**
     * @cond C2D::GetTargetSurface @endcond
     * @brief Get the target C2D surface from map. If not found in map, this function will create a
     * new surface
     * @param[in] pSharedBuffer the output shared buffer
     * @param[in] batchIdx the index of output frame in shared buffer
     * @param[out] surfaceId the field that identifies the surface index
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e GetTargetSurface( const ImageDescriptor_t *pSharedBuffer, uint32_t batchIdx,
                                 uint32_t *surfaceId );
    /**
     * @cond C2D::CreateSourceSurface @endcond
     * @brief Create source C2D surface for input
     * @param[in] pSharedBuffer the input shared buffer
     * @param[in] inputIdx the index of input in input list
     * @param[out] c2dObject the blit object structure that specifies the surface
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e CreateSourceSurface( const ImageDescriptor_t *pSharedBuffer, uint32_t inputIdx,
                                    C2D_OBJECT &c2dObject );
    /**
     * @cond C2D::CreateTargetSurface @endcond
     * @brief Create target C2D surface for output
     * @param[in] pSharedBuffer the output shared buffer
     * @param[in] batchIdx the index of output frame in shared buffer
     * @param[out] surfaceId the field that identifies the surface index
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e CreateTargetSurface( const ImageDescriptor_t *pSharedBuffer, uint32_t batchIdx,
                                    uint32_t *surfaceId );
    /**
     * @cond C2D::CreateYUVSurface @endcond
     * @brief Create YUV source/target surface
     * @param[in] pSharedBuffer the shared buffer
     * @param[out] surfaceId the field that identifies the surface index
     * @param[in] format the format of image in shared buffer
     * @param[in] width the width of image in shared buffer
     * @param[in] height the height of image in shared buffer
     * @param[in] stride0 the stride of 1st plane of image
     * @param[in] stride1 the stride of 2nd plane of image
     * @param[in] actualHeight0 the height with padding of 1st plane of image
     * @param[in] isSource identifies source/target surface
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e CreateYUVSurface( void *bufferAddr, uint32_t *surfaceId, QCImageFormat_e format,
                                 uint32_t width, uint32_t height, uint32_t stride0,
                                 uint32_t stride1, uint32_t planeBufSize0, bool isSource );
    /**
     * @cond C2D::CreateRGBSurface @endcond
     * @brief Create RGB source/target surface
     * @param[in] pSharedBuffer the shared buffer
     * @param[out] surfaceId the field that identifies the surface index
     * @param[in] format the format of image in shared buffer
     * @param[in] width the width of image in shared buffer
     * @param[in] height the height of image in shared buffer
     * @param[in] stride the stride of 1st plane of image
     * @param[in] isSource identifies source/target surface
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e CreateRGBSurface( void *bufferAddr, uint32_t *surfaceId, QCImageFormat_e format,
                                 uint32_t width, uint32_t height, uint32_t stride, bool isSource );
    /**
     * @cond C2D::GetC2DFormatType @endcond
     * @brief Convert image format to C2D format type
     * @param[in] format the format of image in shared buffer
     * @return C2D format on success, QC_IMAGE_FORMAT_MAX on failure
     */
    uint32_t GetC2DFormatType( QCImageFormat_e format );

private:
    uint32_t m_numOfInputs = 1;
    C2D_ImageResolution_t m_inputResolutions[QC_MAX_INPUTS];
    QCImageFormat_e m_inputFormats[QC_MAX_INPUTS];
    C2D_ROIConfig_t m_rois[QC_MAX_INPUTS];

    std::unordered_map<void *, C2D_OBJECT> m_inputBufferSurfaceMap;
    std::unordered_map<void *, uint32_t> m_outputBufferSurfaceMap;

    Logger &m_logger;
    QCObjectState_e m_state = QC_OBJECT_STATE_INITIAL;

};   // class C2D

}   // namespace sample
}   // namespace QC

#endif   // QC_C2D_HPP
