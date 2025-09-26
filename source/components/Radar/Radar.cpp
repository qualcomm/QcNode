// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/component/Radar.hpp"

namespace QC
{
namespace component
{

Radar::Radar() {}

Radar::~Radar() {}

QCStatus_e Radar::Init( const char *pName, const Radar_Config_t *pConfig, Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;
    bool bIFInitOK = false;
    bool bRadarIfaceInitOK = false;

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "ComponentIF::Init failed" );
    }
    else
    {
        m_state = QC_OBJECT_STATE_INITIALIZING;
        bIFInitOK = true;

        if ( nullptr == pConfig )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Invalid configuration" );
        }
        else
        {
            if ( pConfig->maxInputBufferSize == 0 || pConfig->maxOutputBufferSize == 0 )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Invalid buffer sizes in configuration" );
            }
        }

        if ( QC_STATUS_OK == ret )
        {
            if ( pConfig->serviceConfig.serviceName.empty() )
            {
                ret = QC_STATUS_BAD_ARGUMENTS;
                QC_ERROR( "Service name cannot be empty" );
            }
        }

        // Store configuration
        if ( QC_STATUS_OK == ret )
        {
            m_config = *pConfig;
        }

        // Initialize RadarIface
        if ( QC_STATUS_OK == ret )
        {
            ret = m_radarIface.Initialize( m_config.serviceConfig.serviceName.c_str() );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to initialize RadarIface with device: %s",
                          m_config.serviceConfig.serviceName.c_str() );
            }
            else
            {
                bRadarIfaceInitOK = true;
            }
        }

        // Error cleanup
        if ( QC_STATUS_OK != ret )
        {
            if ( bRadarIfaceInitOK )
            {
                (void) m_radarIface.Deinitialize();
            }
            if ( bIFInitOK )
            {
                (void) ComponentIF::Deinit();
            }
        }
        else
        {
            m_state = QC_OBJECT_STATE_READY;
            QC_INFO( "Component Radar is initialized with service: %s",
                     m_config.serviceConfig.serviceName.c_str() );
        }
    }

    return ret;
}

QCStatus_e Radar::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in ready state" );
    }
    else
    {
        // Check if RadarIface is properly initialized
        if ( !m_radarIface.IsInitialized() )
        {
            ret = QC_STATUS_BAD_STATE;
            QC_ERROR( "RadarIface not initialized" );
        }
        else
        {
            m_state = QC_OBJECT_STATE_RUNNING;
            QC_INFO( "Component Radar started" );
        }
    }

    return ret;
}

QCStatus_e Radar::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in running state" );
    }
    else
    {
        // Clear registered buffers
        m_registeredInputBuffers.clear();
        m_registeredOutputBuffers.clear();

        m_state = QC_OBJECT_STATE_READY;
        QC_INFO( "Component Radar stopped" );
    }

    return ret;
}

QCStatus_e Radar::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;
    // The error state arises from the fact that the radar service is not present,
    // and Deinit should proceed in that case to allow reattempts
    if ( QC_OBJECT_STATE_READY == m_state || QC_OBJECT_STATE_ERROR == m_state )
    {
        // Deinitialize RadarIface
        QCStatus_e ret2 = m_radarIface.Deinitialize();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "Failed to deinitialize RadarIface" );
            ret = ret2;
        }

        ret2 = ComponentIF::Deinit();
        if ( QC_STATUS_OK != ret2 )
        {
            QC_ERROR( "ComponentIF::Deinit failed" );
            ret = ret2;
        }
    }
    else
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in ready state for deinitialization" );
    }

    return ret;
}

QCStatus_e Radar::Execute( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in running state" );
    }
    else
    {
        if ( nullptr == pInput || nullptr == pOutput )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Input or output buffer is null" );
        }
        else
        {
            // Validate buffers
            ret = ValidateBuffer( pInput, true );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Input buffer validation failed" );
            }
            else
            {
                ret = ValidateBuffer( pOutput, false );
                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Output buffer validation failed" );
                }
                else
                {
                    // Check if RadarIface is properly initialized
                    if ( !m_radarIface.IsInitialized() )
                    {
                        ret = QC_STATUS_BAD_STATE;
                        QC_ERROR( "RadarIface not initialized" );
                    }
                    else
                    {
                        // Use RadarIface to execute processing with buffer pointers and sizes
                        uint8_t *pInputData = static_cast<uint8_t *>( pInput->data() );
                        uint8_t *pOutputData = static_cast<uint8_t *>( pOutput->data() );
                        size_t inputSize = pInput->size;
                        size_t outputSize = pOutput->size;

                        ret = m_radarIface.Execute( pInputData, inputSize, pOutputData,
                                                    outputSize );
                        if ( QC_STATUS_OK != ret )
                        {
                            QC_ERROR( "RadarIface execution failed" );
                        }
                        else
                        {
                            QC_DEBUG( "Radar processing completed successfully" );
                        }
                    }
                }
            }
        }
    }

    return ret;
}

QCStatus_e Radar::RegisterInputBuffer( const QCSharedBuffer_t *pInputBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in ready or running status!" );
    }
    else
    {
        if ( nullptr == pInputBuffer )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Input buffer is null" );
        }
        else
        {
            void *bufferAddr = pInputBuffer->data();
            if ( m_registeredInputBuffers.find( bufferAddr ) != m_registeredInputBuffers.end() )
            {
                QC_DEBUG( "Input buffer already registered" );
            }
            else
            {
                // Register buffer with service (implementation depends on service interface)
                uint64_t dmaHandle = pInputBuffer->buffer.dmaHandle;
                m_registeredInputBuffers[bufferAddr] = dmaHandle;

                QC_DEBUG( "Registered input buffer at %p with handle %" PRIu64, bufferAddr,
                          dmaHandle );
            }
        }
    }
    return ret;
}

QCStatus_e Radar::RegisterOutputBuffer( const QCSharedBuffer_t *pOutputBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in ready or running status!" );
    }
    else
    {
        if ( nullptr == pOutputBuffer )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
            QC_ERROR( "Output buffer is null" );
        }
        else
        {
            void *bufferAddr = pOutputBuffer->data();
            if ( m_registeredOutputBuffers.find( bufferAddr ) != m_registeredOutputBuffers.end() )
            {
                QC_DEBUG( "Output buffer already registered" );
            }
            else
            {
                // Register buffer with service
                uint64_t dmaHandle = pOutputBuffer->buffer.dmaHandle;
                m_registeredOutputBuffers[bufferAddr] = dmaHandle;

                QC_DEBUG( "Registered output buffer at %p with handle %" PRIu64, bufferAddr,
                          dmaHandle );
            }
        }
    }

    return ret;
}

QCStatus_e Radar::DeregisterInputBuffer( const QCSharedBuffer_t *pInputBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in ready or running status!" );
    }
    else
    {
        if ( nullptr == pInputBuffer )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            void *bufferAddr = pInputBuffer->data();
            auto it = m_registeredInputBuffers.find( bufferAddr );
            if ( it != m_registeredInputBuffers.end() )
            {
                m_registeredInputBuffers.erase( it );
                QC_DEBUG( "Deregistered input buffer at %p", bufferAddr );
            }
        }
    }

    return ret;
}

QCStatus_e Radar::DeregisterOutputBuffer( const QCSharedBuffer_t *pOutputBuffer )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        ret = QC_STATUS_BAD_STATE;
        QC_ERROR( "Radar component not in ready or running status!" );
    }
    else
    {
        if ( nullptr == pOutputBuffer )
        {
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            void *bufferAddr = pOutputBuffer->data();
            auto it = m_registeredOutputBuffers.find( bufferAddr );
            if ( it != m_registeredOutputBuffers.end() )
            {
                m_registeredOutputBuffers.erase( it );
                QC_DEBUG( "Deregistered output buffer at %p", bufferAddr );
            }
        }
    }

    return ret;
}


QCStatus_e Radar::ValidateBuffer( const QCSharedBuffer_t *pBuffer, bool isInput )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pBuffer->buffer.pData )
    {
        ret = QC_STATUS_INVALID_BUF;
        QC_ERROR( "Buffer data pointer is null" );
    }
    else
    {
        if ( pBuffer->buffer.size == 0 )
        {
            ret = QC_STATUS_INVALID_BUF;
            QC_ERROR( "Buffer size is zero" );
        }
        else
        {
            if ( pBuffer->buffer.dmaHandle == 0 )
            {
                ret = QC_STATUS_INVALID_BUF;
                QC_ERROR( "Invalid DMA handle" );
            }
            else
            {
                uint32_t maxSize =
                        isInput ? m_config.maxInputBufferSize : m_config.maxOutputBufferSize;
                if ( pBuffer->size > maxSize )
                {
                    ret = QC_STATUS_INVALID_BUF;
                    QC_ERROR( "%s buffer size (%zu) exceeds maximum (%u)",
                              isInput ? "Input" : "Output", pBuffer->size, maxSize );
                }
                else
                {
                    if ( pBuffer->type == QC_BUFFER_TYPE_RAW )
                    {
                        QC_DEBUG( "Processing RAW buffer type for radar data" );
                    }
                    else if ( pBuffer->type == QC_BUFFER_TYPE_TENSOR )
                    {
                        QC_DEBUG( "Processing TENSOR buffer type for radar data" );
                    }
                    else if ( pBuffer->type == QC_BUFFER_TYPE_IMAGE )
                    {
                        QC_DEBUG( "Processing IMAGE buffer type for radar data" );
                    }
                    else
                    {
                        QC_WARN( "Unexpected buffer type %d for radar processing", pBuffer->type );
                    }
                }
            }
        }
    }

    return ret;
}

}   // namespace component
}   // namespace QC
