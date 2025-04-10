// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_GL2DFLEX_HPP
#define RIDEHAL_GL2DFLEX_HPP

#include <cinttypes>
#include <memory>
#include <unordered_map>
#include <vector>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl32.h>

#include <drm/drm_fourcc.h>
#include <gbm.h>
#include <gbm_priv.h>
#include <xf86drm.h>

#include "ridehal/component/ComponentIF.hpp"

namespace ridehal
{
namespace component
{

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief GL2DFlex Image resolution */
typedef struct
{
    uint32_t width;  /**< Image width */
    uint32_t height; /**< Image height */
} GL2DFlex_ImageResolution_t;

/** @brief GL2DFlex ROI Configuration*/
typedef struct
{
    uint32_t topX;   /**< X coordinate of top left point */
    uint32_t topY;   /**< Y coordinate of top left point */
    uint32_t width;  /**< ROI width */
    uint32_t height; /**< ROI height */
} GL2DFlex_ROIConfig_t;

/** @brief GL2DFlex Input Configuration*/
typedef struct
{
    RideHal_ImageFormat_e inputFormat;          /**< Image format of Input frame */
    GL2DFlex_ImageResolution_t inputResolution; /**< Image Resolution of Input frame */
    GL2DFlex_ROIConfig_t ROI;                   /**< Reigion of Interest in Input frame */
} GL2DFlex_InputConfig_t;

/** @brief GL2DFlex Component Initialization Configuration*/
typedef struct
{
    uint32_t numOfInputs; /**< Number of Input Images in each processing */
    GL2DFlex_InputConfig_t inputConfigs[RIDEHAL_MAX_INPUTS]; /**< Array of Input Configurations */
    GL2DFlex_ImageResolution_t outputResolution; /**< Image Resolution of Output frame */
    RideHal_ImageFormat_e outputFormat;          /**< Image format of Output frame */
} GL2DFlex_Config_t;


/**
 * @brief Component GL2DFlex
 * @brief GL2DFlex converts camera frames to another image format and do cropping and resizing
 */
class GL2DFlex final : public ComponentIF
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    GL2DFlex();
    ~GL2DFlex();

    /**
     * @cond GL2DFlex::Init @endcond
     * @brief Initialize the GL2DFlex component
     * @param[in] name the component unique instance name
     * @param[in] pConfig the GL2DFlex configuration paramaters
     * @param[in] level the logger message level
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, const GL2DFlex_Config_t *pConfig,
                         Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @cond GL2DFlex::Start @endcond
     * @brief Start the GL2DFlex pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Start();

    /**
     * @cond GL2DFlex::Stop @endcond
     * @brief stop the GL2DFlex pipeline
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Stop();

    /**
     * @cond GL2DFlex::Deinit @endcond
     * @brief deinitialize the GL2DFlex component
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Deinit();

    /**
     * @cond GL2DFlex::Execute @endcond
     * @brief Execute the GL2DFlex pipeline
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of the input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Execute( const RideHal_SharedBuffer_t *pInputs, uint32_t numInputs,
                            const RideHal_SharedBuffer_t *pOutput );

    /**
     * @cond GL2DFlex::RegisterInputBuffers @endcond
     * @brief Register shared buffers for each input
     * @param[in] pInputBuffers the input shared buffers array
     * @param[in] numOfInputBuffers the number of shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note This API is optional but recommended to call after input buffers allocation finished.
     * If skip to do this, the Execute API will register input buffers automatically.
     * This API need to be called in the same thread with Execute API
     */
    RideHalError_e RegisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffers,
                                         uint32_t numOfInputBuffers );

    /**
     * @cond GL2DFlex::RegisterOutputBuffers @endcond
     * @brief Register shared buffers for output
     * @param[in] pOutputBuffers the output shared buffers array
     * @param[in] numOfOutputBuffers the number of shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     * @note This API is optional but recommended to call after output buffers allocation finished.
     * If skip to do this, the Execute API will register output buffers automatically.
     * This API need to be called in the same thread with Execute API
     */
    RideHalError_e RegisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffers,
                                          uint32_t numOfOutputBuffers );

    /**
     * @cond GL2DFlex::DeregisterInputBuffers @endcond
     * @brief Deregister shared buffers for each input
     * @param[in] pInputBuffers the input shared buffers array
     * @param[in] numOfInputBuffers the number of shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeregisterInputBuffers( const RideHal_SharedBuffer_t *pInputBuffers,
                                           uint32_t numOfInputBuffers );

    /**
     * @cond GL2DFlex::DeregisterOutputBuffers @endcond
     * @brief Deregister shared buffers for output
     * @param[in] pOutputBuffers the output shared buffers
     * @param[in] numOfOutputBuffers the number of shared buffers
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e DeregisterOutputBuffers( const RideHal_SharedBuffer_t *pOutputBuffers,
                                            uint32_t numOfOutputBuffers );


private:
    typedef struct
    {
        gbm_bo *bo;
        int fd;
        EGLImageKHR image;
        GLuint texture;
        GLuint framebuffer;
        uint64_t handle;
        int32_t offset;
    } GL_ImageInfo_t;

    typedef struct
    {
        GLfloat texcoord[4][2];
    } GL_TexCoord_t;

    RideHalError_e EGLInit();

    RideHalError_e CreateGLPipeline();


    RideHalError_e GetInputImageInfo( const RideHal_SharedBuffer_t *pInputBuffer,
                                      std::shared_ptr<GL_ImageInfo_t> &inputInfo );

    RideHalError_e GetOutputImageInfo( const RideHal_SharedBuffer_t *pOutputBuffer,
                                       std::shared_ptr<GL_ImageInfo_t> &outputInfo,
                                       uint32_t batchIdx );

    RideHalError_e CreateGLInputImage( void *bufferAddr, RideHal_ImageFormat_e format,
                                       uint32_t width, uint32_t height, uint32_t stride,
                                       uint32_t handle, size_t offset,
                                       std::shared_ptr<GL_ImageInfo_t> &inputInfo );

    RideHalError_e CreateGLOutputImage( void *bufferAddr, RideHal_ImageFormat_e format,
                                        uint32_t width, uint32_t height, uint32_t stride,
                                        uint32_t handle, size_t offset,
                                        std::shared_ptr<GL_ImageInfo_t> &inputInfo );

    RideHalError_e Draw( std::shared_ptr<GL_ImageInfo_t> &inputInfo,
                         std::shared_ptr<GL_ImageInfo_t> &outputInfo, uint32_t batchIdx );

    uint32_t GetEGLFormatType( RideHal_ImageFormat_e format );

    uint32_t GetGBMFormatType( RideHal_ImageFormat_e format );

    inline RideHalError_e GLErrorCheck();


private:
    uint32_t m_numOfInputs = 1;

    bool m_bEGLReady = false;
    bool m_bGLPipelineReady = false;

    EGLDisplay m_display = nullptr;
    EGLContext m_context = nullptr;
    EGLSurface m_surface = nullptr;
    GLuint m_vertShader = 0;
    GLuint m_fragShader = 0;
    GLuint m_program = 0;

    GL2DFlex_ImageResolution_t m_inputResolutions[RIDEHAL_MAX_INPUTS];
    RideHal_ImageFormat_e m_inputFormats[RIDEHAL_MAX_INPUTS];
    GL_TexCoord_t m_textcoords[RIDEHAL_MAX_INPUTS];

    GL2DFlex_ImageResolution_t m_outputResolution;
    RideHal_ImageFormat_e m_outputFormat;

    static std::mutex s_mutLock;
    static int s_drmDevFd;
    static struct gbm_device *s_gbmDev;
    static uint32_t s_devRefCnt;

    std::unordered_map<void *, std::shared_ptr<GL_ImageInfo_t>> m_inputImageMap;
    std::unordered_map<void *, std::shared_ptr<GL_ImageInfo_t>> m_outputImageMap;

};   // class GL2DFLEX

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_GL2DFLEX_HPP

