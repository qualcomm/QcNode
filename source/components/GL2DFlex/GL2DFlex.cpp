// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include <cinttypes>
#include <cstring>
#include <memory>

#include "QC/component/GL2DFlex.hpp"

namespace QC
{
namespace component
{

static const char *s_pVertShaderText = "#version 320 es\n"
                                       "layout(location = 0) in vec2 pos;\n"
                                       "layout(location = 1) in vec2 texcoord;\n"
                                       "out vec2 v_texcoord;\n"
                                       "void main() {\n"
                                       "    gl_Position = vec4(pos, 1.0, 1.0);\n"
                                       "    v_texcoord = texcoord;\n"
                                       "}\n";

static const char *s_pFragShaderText = "#version 320 es\n"
                                       "#extension GL_OES_EGL_image_external : require\n"
                                       "#extension GL_OES_EGL_image_external_essl3 : require\n"
                                       "precision mediump float;\n"
                                       "in vec2 v_texcoord;\n"
                                       "out vec4 color;\n"
                                       "uniform samplerExternalOES tex;\n"
                                       "void main() {\n"
                                       "  color = texture(tex, v_texcoord);\n"
                                       "}\n";

static const char *s_pFragShaderYUVText = "#version 320 es\n"
                                          "#extension GL_OES_EGL_image_external : require\n"
                                          "#extension GL_OES_EGL_image_external_essl3 : require\n"
                                          "#extension GL_EXT_YUV_target : require\n"
                                          "precision mediump float;\n"
                                          "in vec2 v_texcoord;\n"
                                          "layout(yuv) out vec4 color;\n"
                                          "uniform samplerExternalOES tex;\n"
                                          "void main() {\n"
                                          "  color = texture(tex, v_texcoord);\n"
                                          "}\n";

std::mutex GL2DFlex::s_mutLock;
int GL2DFlex::s_drmDevFd = 0;
struct gbm_device *GL2DFlex::s_gbmDev = nullptr;
uint32_t GL2DFlex::s_devRefCnt = 0;

GL2DFlex::GL2DFlex() {}

GL2DFlex::~GL2DFlex() {}

QCStatus_e GL2DFlex::Init( const char *pName, const GL2DFlex_Config_t *pConfig,
                           Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;

    uint32_t topX = 0;
    uint32_t topY = 0;
    uint32_t roi_width = 0;
    uint32_t roi_height = 0;

    float gl_topX = 0.0f;
    float gl_topY = 0.0f;
    float gl_bottomX = 0.0f;
    float gl_bottomY = 0.0f;

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "ComponentIF::Init failed" );
    }
    else
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        m_numOfInputs = pConfig->numOfInputs;
        if ( m_numOfInputs > QC_MAX_INPUTS )
        {
            ret = QC_STATUS_OUT_OF_BOUND;
            QC_ERROR( "Number of Inputs exceeds maximum limit" );
        }

        if ( QC_STATUS_OK == ret )
        {
            for ( uint32_t i = 0; i < m_numOfInputs; i++ )
            {
                m_inputResolutions[i].width = pConfig->inputConfigs[i].inputResolution.width;
                m_inputResolutions[i].height = pConfig->inputConfigs[i].inputResolution.height;
                m_inputFormats[i] = pConfig->inputConfigs[i].inputFormat;

                topX = pConfig->inputConfigs[i].ROI.topX;
                topY = pConfig->inputConfigs[i].ROI.topY;
                roi_width = pConfig->inputConfigs[i].ROI.width;
                roi_height = pConfig->inputConfigs[i].ROI.height;

                if ( ( topX == 0 ) && ( topY == 0 ) && ( roi_width == 0 ) && ( roi_height == 0 ) )
                {
                    roi_width = m_inputResolutions[i].width;
                    roi_height = m_inputResolutions[i].height;
                }

                if ( ( topX >= 0 ) && ( topX <= m_inputResolutions[i].width ) )
                {
                    gl_topX = (float) topX;
                }
                else
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "ROI topX of input %u is out of range", i );
                    break;
                }

                if ( ( topY >= 0 ) && ( topY <= m_inputResolutions[i].height ) )
                {
                    gl_topY = (float) topY;
                }
                else
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "ROI topY of input %u is out of range", i );
                    break;
                }

                if ( ( roi_width > 0 ) && ( roi_width <= m_inputResolutions[i].width - topX ) )
                {
                    gl_topX = gl_topX / m_inputResolutions[i].width;
                    gl_bottomX = (float) ( topX + roi_width ) / m_inputResolutions[i].width;
                }
                else
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "ROI width of input %u is out of range", i );
                    break;
                }

                if ( ( roi_height > 0 ) && ( roi_height <= m_inputResolutions[i].height - topY ) )
                {
                    gl_topY = gl_topY / m_inputResolutions[i].height;
                    gl_bottomY = (float) ( topY + roi_height ) / m_inputResolutions[i].height;
                }
                else
                {
                    ret = QC_STATUS_BAD_ARGUMENTS;
                    QC_ERROR( "ROI height of input %u is out of range", i );
                    break;
                }

                GLfloat texcoord[4][2] = { { gl_topX, gl_topY },
                                           { gl_bottomX, gl_topY },
                                           { gl_bottomX, gl_bottomY },
                                           { gl_topX, gl_bottomY } };
                memcpy( m_textcoords[i].texcoord, texcoord, sizeof( texcoord ) );
            }

            m_outputFormat = pConfig->outputFormat;
            m_outputResolution.width = pConfig->outputResolution.width;
            m_outputResolution.height = pConfig->outputResolution.height;

            std::lock_guard<std::mutex> l( s_mutLock );

            if ( QC_STATUS_OK == ret )
            {
                if ( nullptr == s_gbmDev )
                {
                    s_drmDevFd = drmOpen( "msm_drm", NULL );
                    if ( s_drmDevFd < 0 )
                    {
                        ret = QC_STATUS_FAIL;
                        QC_ERROR( "drm open failed: %d", s_drmDevFd );
                    }
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                s_gbmDev = gbm_create_device( s_drmDevFd );
                if ( nullptr == s_gbmDev )
                {
                    ret = QC_STATUS_FAIL;
                    QC_ERROR( "gbm create failed" );
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                s_devRefCnt++;
            }

            /* Complete initialization */
            if ( QC_STATUS_OK == ret )
            {
                m_state = QC_OBJECT_STATE_READY;
                QC_INFO( "Component GL2DFlex is initialized" );
            }
        }
    }

    return ret;
}

QCStatus_e GL2DFlex::Start()
{
    QCStatus_e ret = QC_STATUS_OK;
    if ( QC_OBJECT_STATE_READY != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        // DO start
        m_state = QC_OBJECT_STATE_RUNNING;
        QC_INFO( "Component GL2DFlex start to run" );
    }

    return ret;
}

QCStatus_e GL2DFlex::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        // DO stop
        m_state = QC_OBJECT_STATE_READY;
        QC_INFO( "Component GL2DFlex is stopped" );
    }

    return ret;
}

QCStatus_e GL2DFlex::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    EGLBoolean rc = EGL_FALSE;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
    }

    if ( QC_STATUS_OK == ret )
    {
        std::lock_guard<std::mutex> l( s_mutLock );

        if ( s_devRefCnt > 0 )
        {
            s_devRefCnt--;
        }

        for ( auto it = m_inputImageMap.begin(); it != m_inputImageMap.end(); it++ )
        {
            if ( it->second != nullptr )
            {
                if ( it->second->image != nullptr )
                {
                    rc = eglDestroyImageKHR( m_display, it->second->image );
                    if ( EGL_TRUE != rc )
                    {
                        ret = QC_STATUS_FAIL;
                        QC_ERROR( "Failed to destroy ImageKHR for input, error code: 0x%x", rc );
                    }
                }

                if ( it->second->bo != nullptr )
                {
                    gbm_bo_destroy( it->second->bo );
                }
            }
        }
        m_inputImageMap.clear();

        for ( auto it = m_outputImageMap.begin(); it != m_outputImageMap.end(); it++ )
        {
            if ( it->second != nullptr )
            {
                if ( it->second->image != nullptr )
                {
                    rc = eglDestroyImageKHR( m_display, it->second->image );
                    if ( EGL_TRUE != rc )
                    {
                        ret = QC_STATUS_FAIL;
                        QC_ERROR( "Failed to destroy ImageKHR for output, error code: 0x%x", rc );
                    }
                }

                if ( it->second->bo != nullptr )
                {
                    gbm_bo_destroy( it->second->bo );
                }
            }
        }
        m_outputImageMap.clear();

        glDeleteProgram( m_program );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to delete GL program" );
        }

        if ( ( s_devRefCnt == 0 ) && ( s_gbmDev != nullptr ) )
        {
            s_drmDevFd = drmClose( s_drmDevFd );
            if ( s_drmDevFd < 0 )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "drm close failed: %d", s_drmDevFd );
            }

            gbm_device_destroy( s_gbmDev );
        }
    }

    ret = ComponentIF::Deinit();
    if ( QC_STATUS_OK == ret )
    {
        /* Complete deinitialization */
        m_state = QC_OBJECT_STATE_INITIAL;
        QC_INFO( "Component GL2DFlex is deinitialized" );
    }
    else
    {
        QC_ERROR( "ComponentIF::Deinit failed" );
    }

    return ret;
}

QCStatus_e GL2DFlex::Execute( const QCSharedBuffer_t *pInputs, uint32_t numInputs,
                              const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Component GL2DFlex is not in running state" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( nullptr == pInputs )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Input buffer is null" );
        }
        else if ( numInputs != m_numOfInputs )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Number of inputs not correct: %u != %u", m_numOfInputs, numInputs );
        }
        else if ( nullptr == pOutput )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Output buffer is null" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        for ( size_t i = 0; i < m_numOfInputs; i++ )
        {
            if ( pInputs[i].imgProps.format != m_inputFormats[i] )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Input %u format is not correct: %d != %d", i, (int) m_outputFormat,
                          (int) pOutput->imgProps.format );
            }
            else if ( pInputs[i].imgProps.width != m_inputResolutions[i].width )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Input %u width is not correct: %u != %u", i, m_inputResolutions[i].width,
                          pInputs[i].imgProps.width );
            }
            else if ( pInputs[i].imgProps.height != m_inputResolutions[i].height )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Input %u height is not correct: %u != %u", i,
                          m_inputResolutions[i].height, pInputs[i].imgProps.height );
            }
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( pOutput->imgProps.format != m_outputFormat )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Output format is not correct: %d != %d", (int) m_outputFormat,
                      (int) pOutput->imgProps.format );
        }
        else if ( pOutput->imgProps.width != m_outputResolution.width )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Output width is not correct: %u != %u", m_outputResolution.width,
                      pOutput->imgProps.width );
        }
        else if ( pOutput->imgProps.height != m_outputResolution.height )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Output height is not correct: %u != %u", m_outputResolution.height,
                      pOutput->imgProps.height );
        }
        else if ( pOutput->imgProps.batchSize != m_numOfInputs )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Output batch size is not correct, numOfInputs %u != batchSize %u",
                      m_numOfInputs, pOutput->imgProps.batchSize );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        std::shared_ptr<GL_ImageInfo_t> inputInfo = std::make_shared<GL_ImageInfo_t>();
        std::shared_ptr<GL_ImageInfo_t> outputInfo = std::make_shared<GL_ImageInfo_t>();

        for ( size_t i = 0; i < m_numOfInputs; i++ )
        {
            ret = GetInputImageInfo( &pInputs[i], inputInfo );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to get input image info for input %u", i );
                break;
            }

            ret = GetOutputImageInfo( pOutput, outputInfo, i );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to get output image info for output batch %u", i );
                break;
            }

            ret = Draw( inputInfo, outputInfo, i );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to draw EGL image for index %u", i );
                break;
            }
        }
    }

    return ret;
}

QCStatus_e GL2DFlex::RegisterInputBuffers( const QCSharedBuffer_t *pInputBuffers,
                                           uint32_t numOfInputBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    void *bufferAddr = nullptr;
    std::shared_ptr<GL_ImageInfo_t> inputInfo = std::make_shared<GL_ImageInfo_t>();

    for ( size_t i = 0; i < numOfInputBuffers; i++ )
    {
        bufferAddr = pInputBuffers[i].data();
        if ( m_inputImageMap.find( bufferAddr ) == m_inputImageMap.end() )
        {
            QCImageFormat_e format = pInputBuffers[i].imgProps.format;
            uint32_t width = pInputBuffers[i].imgProps.width;
            uint32_t height = pInputBuffers[i].imgProps.height;
            uint32_t stride = pInputBuffers[i].imgProps.stride[0];
            uint32_t handle = pInputBuffers[i].buffer.dmaHandle;
            size_t offset = pInputBuffers[i].offset;

            ret = CreateGLInputImage( bufferAddr, format, width, height, stride, handle, offset,
                                      inputInfo );
            if ( ret != QC_STATUS_OK )
            {
                QC_ERROR( "Failed to create GL input image for input buffer %u", i );
                break;
            }
        }
    }

    return ret;
}

QCStatus_e GL2DFlex::RegisterOutputBuffers( const QCSharedBuffer_t *pOutputBuffers,
                                            uint32_t numOfOutputBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    void *bufferAddr = nullptr;
    size_t outputSize = pOutputBuffers->size / pOutputBuffers->imgProps.batchSize;
    std::shared_ptr<GL_ImageInfo_t> outputInfo = std::make_shared<GL_ImageInfo_t>();

    for ( size_t i = 0; i < numOfOutputBuffers; i++ )
    {
        for ( size_t k = 0; k < m_numOfInputs; k++ )
        {
            bufferAddr = (void *) ( (uint8_t *) pOutputBuffers[i].data() + k * outputSize );
            if ( m_outputImageMap.find( bufferAddr ) == m_outputImageMap.end() )
            {
                QCImageFormat_e format = pOutputBuffers[i].imgProps.format;
                uint32_t width = pOutputBuffers[i].imgProps.width;
                uint32_t height = pOutputBuffers[i].imgProps.height;
                uint32_t stride = pOutputBuffers[i].imgProps.stride[0];
                uint32_t handle = pOutputBuffers[i].buffer.dmaHandle;
                size_t offset = pOutputBuffers[i].offset;

                ret = CreateGLOutputImage( bufferAddr, format, width, height, stride, handle,
                                           offset, outputInfo );
                if ( ret != QC_STATUS_OK )
                {
                    QC_ERROR( "Failed to create GL output image for output buffer %u batch %u", i,
                              k );
                    break;
                }
            }
        }
    }

    return ret;
}

QCStatus_e GL2DFlex::DeregisterInputBuffers( const QCSharedBuffer_t *pInputBuffers,
                                             uint32_t numOfInputBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    void *bufferAddr = nullptr;
    if ( numOfInputBuffers > m_inputImageMap.size() )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "Number of deregister buffers greater than registered buffers " );
    }

    if ( QC_STATUS_OK == ret )
    {
        EGLBoolean rc = EGL_FALSE;
        for ( size_t i = 0; i < numOfInputBuffers; i++ )
        {
            bufferAddr = pInputBuffers[i].data();
            if ( m_inputImageMap.find( bufferAddr ) != m_inputImageMap.end() )
            {
                if ( m_inputImageMap[bufferAddr]->image != nullptr )
                {
                    rc = eglDestroyImageKHR( m_display, m_inputImageMap[bufferAddr]->image );
                    if ( EGL_TRUE != rc )
                    {
                        ret = QC_STATUS_FAIL;
                        QC_ERROR(
                                "Failed to destroy ImageKHR for input buffer %u, error code: 0x%x",
                                i, rc );
                    }
                }

                if ( m_inputImageMap[bufferAddr]->bo != nullptr )
                {
                    gbm_bo_destroy( m_inputImageMap[bufferAddr]->bo );
                }

                (void) m_inputImageMap.erase( bufferAddr );
            }
        }
    }

    return ret;
}

QCStatus_e GL2DFlex::DeregisterOutputBuffers( const QCSharedBuffer_t *pOutputBuffers,
                                              uint32_t numOfOutputBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    void *bufferAddr = nullptr;
    uint32_t outputSize = pOutputBuffers->size / pOutputBuffers->imgProps.batchSize;

    if ( numOfOutputBuffers > m_outputImageMap.size() )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
        QC_ERROR( "Number of deregister buffers greater than registered buffers " );
    }

    if ( QC_STATUS_OK == ret )
    {
        EGLBoolean rc = EGL_FALSE;
        for ( size_t i = 0; i < numOfOutputBuffers; i++ )
        {
            for ( size_t k = 0; k < m_numOfInputs; k++ )
            {
                bufferAddr = (void *) ( (uint8_t *) pOutputBuffers[i].data() + k * outputSize );
                if ( m_outputImageMap.find( bufferAddr ) != m_outputImageMap.end() )
                {
                    if ( m_outputImageMap[bufferAddr]->image != nullptr )
                    {
                        rc = eglDestroyImageKHR( m_display, m_outputImageMap[bufferAddr]->image );
                        if ( EGL_TRUE != rc )
                        {
                            ret = QC_STATUS_FAIL;
                            QC_ERROR( "Failed to destroy ImageKHR for output buffer %u batch "
                                      "%u, error code: 0x%x",
                                      i, k, rc );
                        }
                    }

                    if ( m_outputImageMap[bufferAddr]->bo != nullptr )
                    {
                        gbm_bo_destroy( m_outputImageMap[bufferAddr]->bo );
                    }

                    (void) m_outputImageMap.erase( bufferAddr );
                }
            }
        }
    }

    return ret;
}


QCStatus_e GL2DFlex::EGLInit()
{
    QCStatus_e ret = QC_STATUS_OK;

    EGLint major = -1;
    EGLint minor = -1;
    EGLint num_config = 0;
    EGLBoolean rc = EGL_FALSE;
    std::vector<EGLConfig> configs;
    const EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    EGLint surface_attribs[] = {
            EGL_WIDTH,  (EGLint) m_outputResolution.width,
            EGL_HEIGHT, (EGLint) m_outputResolution.height,
            EGL_NONE,
    };

    if ( !m_bEGLReady )
    {
        m_display = eglGetPlatformDisplay( EGL_PLATFORM_GBM_KHR, NULL, NULL );
        if ( nullptr == m_display )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "Failed to get EGL display" );
        }

        if ( QC_STATUS_OK == ret )
        {
            rc = eglInitialize( m_display, &major, &minor );
            if ( EGL_TRUE != rc )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to initialize EGL: 0x%x", rc );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            rc = eglGetConfigs( m_display, NULL, 0, &num_config );
            if ( ( EGL_TRUE != rc ) || ( num_config <= 0 ) )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to get config number for EGL: 0x%x", rc );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            configs.resize( num_config );
            rc = eglGetConfigs( m_display, configs.data(), num_config, &num_config );
            if ( EGL_TRUE != rc )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to get config for EGL: 0x%x", rc );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_context = eglCreateContext( m_display, configs[0], EGL_NO_CONTEXT, context_attribs );
            if ( nullptr == m_context )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to create EGL context" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_surface = eglCreatePbufferSurface( m_display, configs[0], surface_attribs );
            if ( nullptr == m_surface )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to create EGL surface" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            rc = eglMakeCurrent( m_display, m_surface, m_surface, m_context );
            if ( EGL_TRUE != rc )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to make EGL current: 0x%x", rc );
            }
        }

        m_bEGLReady = true;
    }

    return ret;
}

QCStatus_e GL2DFlex::CreateGLPipeline()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( !m_bGLPipelineReady )
    {
        if ( QC_STATUS_OK == ret )
        {
            m_vertShader = glCreateShader( GL_VERTEX_SHADER );
            if ( 0 == m_vertShader )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to create GL Vertex Shader" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            glShaderSource( m_vertShader, 1, (char **) &s_pVertShaderText, NULL );
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to set GL Vertex Shader source" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            glCompileShader( m_vertShader );
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to compile GL Vertex Shader source" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_fragShader = glCreateShader( GL_FRAGMENT_SHADER );
            if ( 0 == m_fragShader )
            {
                ret = QC_STATUS_FAIL;
                QC_ERROR( "Failed to create GL Fragment Shader" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            if ( ( m_outputFormat == QC_IMAGE_FORMAT_NV12 ) ||
                 ( m_outputFormat == QC_IMAGE_FORMAT_UYVY ) )
            {
                glShaderSource( m_fragShader, 1, (char **) &s_pFragShaderYUVText, NULL );
                ret = GLErrorCheck();
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to set GL Fragment YUV Shader source" );
                }
            }
            else if ( m_outputFormat == QC_IMAGE_FORMAT_RGB888 )
            {
                glShaderSource( m_fragShader, 1, (char **) &s_pFragShaderText, NULL );
                ret = GLErrorCheck();
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to set GL Fragment Shader source" );
                }
            }
            else
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Unsupported output image format" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            glCompileShader( m_fragShader );
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to compile GL Fragment Shader source" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            m_program = glCreateProgram();
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to create GL Program" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            glAttachShader( m_program, m_vertShader );
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to attach GL Vertex Shader" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            glAttachShader( m_program, m_fragShader );
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to attach GL Fragment Shader" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            glLinkProgram( m_program );
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to link GL program" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            glUseProgram( m_program );
            ret = GLErrorCheck();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to use GL program" );
            }
        }

        if ( 0 != m_vertShader )
        {
            glDeleteShader( m_vertShader );
        }

        if ( 0 != m_vertShader )
        {
            glDeleteShader( m_fragShader );
        }

        m_bGLPipelineReady = true;
    }

    return ret;
}


QCStatus_e GL2DFlex::GetInputImageInfo( const QCSharedBuffer_t *pInputBuffer,
                                        std::shared_ptr<GL_ImageInfo_t> &inputInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    void *bufferAddr = pInputBuffer->data();

    if ( m_inputImageMap.find( bufferAddr ) == m_inputImageMap.end() )
    {
        QCImageFormat_e format = pInputBuffer->imgProps.format;
        uint32_t width = pInputBuffer->imgProps.width;
        uint32_t height = pInputBuffer->imgProps.height;
        uint32_t stride = pInputBuffer->imgProps.stride[0];
        uint32_t handle = pInputBuffer->buffer.dmaHandle;
        size_t offset = pInputBuffer->offset;

        ret = CreateGLInputImage( bufferAddr, format, width, height, stride, handle, offset,
                                  inputInfo );
    }
    else
    {
        inputInfo = m_inputImageMap[bufferAddr];
    }

    return ret;
}


QCStatus_e GL2DFlex::GetOutputImageInfo( const QCSharedBuffer_t *pOutputBuffer,
                                         std::shared_ptr<GL_ImageInfo_t> &outputInfo,
                                         uint32_t batchIdx )
{
    QCStatus_e ret = QC_STATUS_OK;

    uint32_t outputSize = pOutputBuffer->size / m_numOfInputs;
    void *bufferAddr = (void *) ( (uint8_t *) pOutputBuffer->data() + batchIdx * outputSize );

    if ( m_outputImageMap.find( bufferAddr ) == m_outputImageMap.end() )
    {
        QCImageFormat_e format = pOutputBuffer->imgProps.format;
        uint32_t width = pOutputBuffer->imgProps.width;
        uint32_t height = pOutputBuffer->imgProps.height;
        uint32_t stride = pOutputBuffer->imgProps.stride[0];
        uint32_t handle = pOutputBuffer->buffer.dmaHandle;
        size_t offset = pOutputBuffer->offset;

        ret = CreateGLOutputImage( bufferAddr, format, width, height, stride, handle, offset,
                                   outputInfo );
    }
    else
    {
        outputInfo = m_outputImageMap[bufferAddr];
    }

    return ret;
}


QCStatus_e GL2DFlex::CreateGLInputImage( void *bufferAddr, QCImageFormat_e format, uint32_t width,
                                         uint32_t height, uint32_t stride, uint32_t handle,
                                         size_t offset, std::shared_ptr<GL_ImageInfo_t> &inputInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    struct gbm_import_fd_data fdData = { (int) handle, width, height, stride,
                                         GetGBMFormatType( format ) };
    inputInfo->bo =
            gbm_bo_import( s_gbmDev, GBM_BO_IMPORT_FD, &fdData, GBM_BO_TRANSFER_READ_WRITE );
    if ( nullptr == inputInfo->bo )
    {
        ret = QC_STATUS_FAIL;
        QC_ERROR( "Failed to import gbm bo for input" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_STATUS_OK == ret )
        {
            ret = EGLInit();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to Init EGL" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = CreateGLPipeline();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to Create GL Pipeline" );
            }
        }

        int fd = gbm_bo_get_fd( inputInfo->bo );
        EGLint eglImageAttribs[] = { EGL_WIDTH,
                                     (EGLint) width,
                                     EGL_HEIGHT,
                                     (EGLint) height,
                                     EGL_LINUX_DRM_FOURCC_EXT,
                                     (EGLint) GetEGLFormatType( format ),
                                     EGL_DMA_BUF_PLANE0_FD_EXT,
                                     (EGLint) fd,
                                     EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                                     (EGLint) offset,
                                     EGL_DMA_BUF_PLANE0_PITCH_EXT,
                                     (EGLint) stride,
                                     EGL_NONE };
        inputInfo->image = eglCreateImageKHR( m_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
                                              NULL, eglImageAttribs );
        if ( nullptr == inputInfo->image )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "Failed to create image for input" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glGenTextures( 1, &inputInfo->texture );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to generate GL textures for input" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, inputInfo->texture );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to bind GL textures for input" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glEGLImageTargetTexture2DOES( GL_TEXTURE_EXTERNAL_OES, inputInfo->image );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to render GL target texture for input" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        inputInfo->handle = handle;
        inputInfo->offset = offset;
        m_inputImageMap[bufferAddr] = inputInfo;
    }

    return ret;
}


QCStatus_e GL2DFlex::CreateGLOutputImage( void *bufferAddr, QCImageFormat_e format, uint32_t width,
                                          uint32_t height, uint32_t stride, uint32_t handle,
                                          size_t offset,
                                          std::shared_ptr<GL_ImageInfo_t> &outputInfo )
{
    QCStatus_e ret = QC_STATUS_OK;

    struct gbm_import_fd_data fdData = { (int) handle, width, height, stride,
                                         GetGBMFormatType( format ) };
    outputInfo->bo =
            gbm_bo_import( s_gbmDev, GBM_BO_IMPORT_FD, &fdData, GBM_BO_TRANSFER_READ_WRITE );
    if ( nullptr == outputInfo->bo )
    {
        ret = QC_STATUS_FAIL;
        QC_ERROR( "Failed to import gbm bo for output" );
    }

    if ( QC_STATUS_OK == ret )
    {
        if ( QC_STATUS_OK == ret )
        {
            ret = EGLInit();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to Init EGL" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            ret = CreateGLPipeline();
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to Create GL Pipeline" );
            }
        }

        int fd = gbm_bo_get_fd( outputInfo->bo );
        EGLint eglImageAttribs[] = { EGL_WIDTH,
                                     (EGLint) width,
                                     EGL_HEIGHT,
                                     (EGLint) height,
                                     EGL_LINUX_DRM_FOURCC_EXT,
                                     (EGLint) GetEGLFormatType( format ),
                                     EGL_DMA_BUF_PLANE0_FD_EXT,
                                     (EGLint) fd,
                                     EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                                     (EGLint) offset,
                                     EGL_DMA_BUF_PLANE0_PITCH_EXT,
                                     (EGLint) stride,
                                     EGL_NONE };

        outputInfo->image = eglCreateImageKHR( m_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT,
                                               NULL, eglImageAttribs );
        if ( nullptr == outputInfo->image )
        {
            ret = QC_STATUS_FAIL;
            QC_ERROR( "Failed to create image for output" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glGenTextures( 1, &outputInfo->texture );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to generate GL texture for output" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glGenFramebuffers( 1, &outputInfo->framebuffer );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to generate GL frame buffers for output" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, outputInfo->texture );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to bind GL texture for output" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glEGLImageTargetTexture2DOES( GL_TEXTURE_EXTERNAL_OES, outputInfo->image );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to render GL target texture for output" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glBindFramebuffer( GL_FRAMEBUFFER, outputInfo->framebuffer );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to bind GL frame buffer for output" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES,
                                outputInfo->texture, 0 );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to attach GL texture to output buffer" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        outputInfo->handle = handle;
        outputInfo->offset = offset;
        m_outputImageMap[bufferAddr] = outputInfo;
    }

    return ret;
}


QCStatus_e GL2DFlex::Draw( std::shared_ptr<GL_ImageInfo_t> &inputInfo,
                           std::shared_ptr<GL_ImageInfo_t> &outputInfo, uint32_t batchIdx )
{
    QCStatus_e ret = QC_STATUS_OK;

    glViewport( 0, 0, (GLint) m_outputResolution.width, (GLint) m_outputResolution.height );
    ret = GLErrorCheck();
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "glViewport failed" );
    }

    if ( QC_STATUS_OK == ret )
    {
        glClearColor( 1.0, 0.0, 0.0, 1.0 );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "glClearColor failed" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glClear( GL_COLOR_BUFFER_BIT );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "glClear failed" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glUniform1i( glGetUniformLocation( m_program, "tex" ), 0 );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "glUniform1i failed" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        // bind input
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, inputInfo->texture );
        glEGLImageTargetTexture2DOES( GL_TEXTURE_EXTERNAL_OES, inputInfo->image );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to bind input" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        // bind output
        glBindFramebuffer( GL_FRAMEBUFFER, outputInfo->framebuffer );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to bind frame buffer" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_EXTERNAL_OES,
                                outputInfo->texture, 0 );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to bind output" );
        }
    }

    GLushort indices[] = { 0, 1, 3, 3, 1, 2 };
    GLfloat pos[4][2] = { { -1.0, -1.0 }, { 1.0, -1.0 }, { 1.0, 1.0 }, { -1.0, 1.0 } };
    if ( QC_STATUS_OK == ret )
    {
        glVertexAttribPointer( glGetAttribLocation( m_program, "pos" ), 2, GL_FLOAT, GL_FALSE, 0,
                               pos );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to set GL vertex pos" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glVertexAttribPointer( glGetAttribLocation( m_program, "texcoord" ), 2, GL_FLOAT, GL_FALSE,
                               0, m_textcoords[batchIdx].texcoord );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to set GL vertex texcoord" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glEnableVertexAttribArray( glGetAttribLocation( m_program, "pos" ) );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to enable GL vertex pos" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glEnableVertexAttribArray( glGetAttribLocation( m_program, "texcoord" ) );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to enable GL vertex texcoord" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glDrawElements( GL_TRIANGLES, sizeof( indices ) / sizeof( indices[0] ), GL_UNSIGNED_SHORT,
                        indices );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to draw GL elements" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        glFlush();
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to flush" );
        }
    }

    if ( QC_STATUS_OK == ret )
    {
        eglSwapBuffers( m_display, m_surface );
        ret = GLErrorCheck();
        if ( QC_STATUS_OK != ret )
        {
            QC_ERROR( "Failed to swap GL buffers" );
        }
    }

    return ret;
}


uint32_t GL2DFlex::GetEGLFormatType( QCImageFormat_e format )
{
    uint32_t eglFormat = (uint32_t) QC_IMAGE_FORMAT_MAX;
    switch ( format )
    {
        case QC_IMAGE_FORMAT_UYVY:
            eglFormat = DRM_FORMAT_UYVY;
            break;
        case QC_IMAGE_FORMAT_NV12:
            eglFormat = DRM_FORMAT_NV12;
            break;
        case QC_IMAGE_FORMAT_P010:
            eglFormat = DRM_FORMAT_P010;
            break;
        case QC_IMAGE_FORMAT_RGB888:
            eglFormat = DRM_FORMAT_RGB888;
            break;
        default:
            QC_ERROR( "Unsupported EGL image format" );
            break;
    }

    return eglFormat;
}


uint32_t GL2DFlex::GetGBMFormatType( QCImageFormat_e format )
{
    uint32_t gbmFormat = (uint32_t) QC_IMAGE_FORMAT_MAX;
    switch ( format )
    {
        case QC_IMAGE_FORMAT_UYVY:
            gbmFormat = GBM_FORMAT_UYVY;
            break;
        case QC_IMAGE_FORMAT_NV12:
            gbmFormat = GBM_FORMAT_NV12;
            break;
        case QC_IMAGE_FORMAT_P010:
            gbmFormat = GBM_FORMAT_P010;
            break;
        case QC_IMAGE_FORMAT_RGB888:
            gbmFormat = GBM_FORMAT_RGB888;
            break;
        default:
            QC_ERROR( "Unsupported GBM image format" );
            break;
    }

    return gbmFormat;
}


inline QCStatus_e GL2DFlex::GLErrorCheck()
{
    QCStatus_e ret = QC_STATUS_OK;
    GLenum glError = glGetError();

    if ( glError != GL_NO_ERROR )
    {
        ret = QC_STATUS_FAIL;
        QC_ERROR( "GLErrorCheck failed, Err value: %u", (int) glError );
    }

    return ret;
}

}   // namespace component
}   // namespace QC
