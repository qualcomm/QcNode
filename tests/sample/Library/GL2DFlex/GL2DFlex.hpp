// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_GL2DFLEX_HPP
#define QC_GL2DFLEX_HPP

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

#if defined( __QNXNTO__ )
#define EGLIMAGE_WITH_UVA
// defination copied from eglextQCOM.h
#define EGL_FORMAT_UYVY_QCOM 0x3125
#define EGL_FORMAT_RGB_888_QCOM 0x31C8
#define EGL_IMAGE_FORMAT_QCOM 0x3121
#define EGL_IMAGE_EXT_BUFFER_BASE_ADDR_LOW_QCOM 0x341A
#define EGL_IMAGE_EXT_BUFFER_BASE_ADDR_HIGH_QCOM 0x341B
#define EGL_IMAGE_EXT_BUFFER_DESCRIPTOR_LOW_QCOM 0x3428
#define EGL_IMAGE_EXT_BUFFER_DESCRIPTOR_HIGH_QCOM 0x3429
#define EGL_IMAGE_EXT_BUFFER_MEMORY_TYPE_QCOM 0x3424
#define EGL_IMAGE_EXT_BUFFER_MEMORY_TYPE_ION_QCOM 0x3427
#define EGL_IMAGE_EXT_BUFFER_STRIDE_QCOM 0x341D
#define EGL_IMAGE_EXT_BUFFER_SIZE_QCOM 0x341C
#define EGL_NEW_IMAGE_QCOM 0x3120
#else
#include <drm/drm_fourcc.h>
#include <gbm.h>
#include <gbm_priv.h>
#include <xf86drm.h>
#endif

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
    QCImageFormat_e inputFormat;                /**< Image format of Input frame */
    GL2DFlex_ImageResolution_t inputResolution; /**< Image Resolution of Input frame */
    GL2DFlex_ROIConfig_t ROI;                   /**< Reigion of Interest in Input frame */
} GL2DFlex_InputConfig_t;

/** @brief GL2DFlex Component Initialization Configuration*/
typedef struct
{
    uint32_t numOfInputs; /**< Number of Input Images in each processing */
    GL2DFlex_InputConfig_t inputConfigs[QC_MAX_INPUTS]; /**< Array of Input Configurations */
    GL2DFlex_ImageResolution_t outputResolution;        /**< Image Resolution of Output frame */
    QCImageFormat_e outputFormat;                       /**< Image format of Output frame */
} GL2DFlex_Config_t;


/**
 * @brief Component GL2DFlex
 * @brief GL2DFlex converts camera frames to another image format and do cropping and resizing
 */
class GL2DFlex
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    GL2DFlex( Logger &logger );
    ~GL2DFlex();

    /**
     * @cond GL2DFlex::Init @endcond
     * @brief Initialize the GL2DFlex component
     * @param[in] name the component unique instance name
     * @param[in] pConfig the GL2DFlex configuration paramaters
     * @param[in] level the logger message level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const GL2DFlex_Config_t *pConfig );

    /**
     * @cond GL2DFlex::Start @endcond
     * @brief Start the GL2DFlex pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @cond GL2DFlex::Stop @endcond
     * @brief stop the GL2DFlex pipeline
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @cond GL2DFlex::Deinit @endcond
     * @brief deinitialize the GL2DFlex component
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

    /**
     * @cond GL2DFlex::Execute @endcond
     * @brief Execute the GL2DFlex pipeline
     * @param[in] pInputs the input shared buffers
     * @param[in] numInputs the number of the input shared buffers
     * @param[out] pOutput the output shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( const ImageDescriptor_t *pInputs, uint32_t numInputs,
                        const ImageDescriptor_t *pOutput );

    /**
     * @cond GL2DFlex::RegisterInputBuffers @endcond
     * @brief Register shared buffers for each input
     * @param[in] pInputBuffers the input shared buffers array
     * @param[in] numOfInputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note This API is optional but recommended to call after input buffers allocation
     * finished. If skip to do this, the Execute API will register input buffers automatically.
     * This API need to be called in the same thread with Execute API
     */
    QCStatus_e RegisterInputBuffers( const ImageDescriptor_t *pInputBuffers,
                                     uint32_t numOfInputBuffers );

    /**
     * @cond GL2DFlex::RegisterOutputBuffers @endcond
     * @brief Register shared buffers for output
     * @param[in] pOutputBuffers the output shared buffers array
     * @param[in] numOfOutputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     * @note This API is optional but recommended to call after output buffers allocation
     * finished. If skip to do this, the Execute API will register output buffers automatically.
     * This API need to be called in the same thread with Execute API
     */
    QCStatus_e RegisterOutputBuffers( const ImageDescriptor_t *pOutputBuffers,
                                      uint32_t numOfOutputBuffers );

    /**
     * @cond GL2DFlex::DeregisterInputBuffers @endcond
     * @brief Deregister shared buffers for each input
     * @param[in] pInputBuffers the input shared buffers array
     * @param[in] numOfInputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeregisterInputBuffers( const ImageDescriptor_t *pInputBuffers,
                                       uint32_t numOfInputBuffers );

    /**
     * @cond GL2DFlex::DeregisterOutputBuffers @endcond
     * @brief Deregister shared buffers for output
     * @param[in] pOutputBuffers the output shared buffers
     * @param[in] numOfOutputBuffers the number of shared buffers
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeregisterOutputBuffers( const ImageDescriptor_t *pOutputBuffers,
                                        uint32_t numOfOutputBuffers );


private:
    typedef struct
    {
#if defined( __QNXNTO__ )
#else
        gbm_bo *bo;
#endif
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

    QCStatus_e EGLInit();

    QCStatus_e CreateGLPipeline();


    QCStatus_e GetInputImageInfo( const ImageDescriptor_t *pInputBuffer,
                                  std::shared_ptr<GL_ImageInfo_t> &inputInfo );

    QCStatus_e GetOutputImageInfo( const ImageDescriptor_t *pOutputBuffer,
                                   std::shared_ptr<GL_ImageInfo_t> &outputInfo, uint32_t batchIdx );

    QCStatus_e CreateGLInputImage( const ImageDescriptor_t *pBuffer,
                                   std::shared_ptr<GL_ImageInfo_t> &inputInfo );

    QCStatus_e CreateGLOutputImage( const ImageDescriptor_t *pBuffer,
                                    std::shared_ptr<GL_ImageInfo_t> &outputInfo );

    QCStatus_e Draw( std::shared_ptr<GL_ImageInfo_t> &inputInfo,
                     std::shared_ptr<GL_ImageInfo_t> &outputInfo, uint32_t batchIdx );

    uint32_t GetEGLFormatType( QCImageFormat_e format );
#if !defined( __QNXNTO__ )
    uint32_t GetGBMFormatType( QCImageFormat_e format );
#endif

    inline QCStatus_e GLErrorCheck();


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

    GL2DFlex_ImageResolution_t m_inputResolutions[QC_MAX_INPUTS];
    QCImageFormat_e m_inputFormats[QC_MAX_INPUTS];
    GL_TexCoord_t m_textcoords[QC_MAX_INPUTS];

    GL2DFlex_ImageResolution_t m_outputResolution;
    QCImageFormat_e m_outputFormat;

    static std::mutex s_mutLock;
    static int s_drmDevFd;
    static struct gbm_device *s_gbmDev;
    static uint32_t s_devRefCnt;

    std::unordered_map<void *, std::shared_ptr<GL_ImageInfo_t>> m_inputImageMap;
    std::unordered_map<void *, std::shared_ptr<GL_ImageInfo_t>> m_outputImageMap;

    Logger &m_logger;
    QCObjectState_e m_state = QC_OBJECT_STATE_INITIAL;

};   // class GL2DFLEX

}   // namespace sample
}   // namespace QC

#endif   // QC_GL2DFLEX_HPP
