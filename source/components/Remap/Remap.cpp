// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "QC/component/Remap.hpp"

#include <map>
#include <vector>

namespace QC
{
namespace component
{

Remap::Remap() {}

Remap::~Remap() {}

QCStatus_e Remap::Start()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY == m_state )
    {
        m_state = QC_OBJECT_STATE_RUNNING;
    }
    else
    {
        QC_ERROR( "Remap component start failed due to wrong state!" );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Remap::Stop()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_RUNNING == m_state )
    {
        m_state = QC_OBJECT_STATE_READY;
    }
    else
    {
        QC_ERROR( "Remap component stop failed due to wrong state!" );
        ret = QC_STATUS_BAD_STATE;
    }

    return ret;
}

QCStatus_e Remap::Init( const char *pName, const Remap_Config_t *pConfig, Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = ComponentIF::Init( pName, level );
    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to init component!" );
    }
    else
    {
        if ( nullptr == pConfig )
        {
            QC_ERROR( "Empty config pointer!" );
            QCStatus_e retVal = ComponentIF::Deinit();
            if ( QC_STATUS_OK != retVal )
            {
                QC_ERROR( "Deinit ComponentIF failed!" );
            }
            ret = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            m_state = QC_OBJECT_STATE_INITIALIZING;
            m_config = *pConfig;
            ret = m_fadasRemapObj.Init( m_config.processor, pName, level );
            if ( QC_STATUS_OK != ret )
            {
                QC_ERROR( "Failed to init fadas remap!" );
            }
            else
            {
                ret = m_fadasRemapObj.SetRemapParams(
                        m_config.numOfInputs, m_config.outputWidth, m_config.outputHeight,
                        m_config.outputFormat, m_config.normlzR, m_config.normlzG, m_config.normlzB,
                        m_config.bEnableUndistortion, m_config.bEnableNormalize );

                if ( QC_STATUS_OK != ret )
                {
                    QC_ERROR( "Failed to set parameters!" );
                }
                else
                {
                    for ( uint32_t inputId = 0; inputId < m_config.numOfInputs; inputId++ )
                    {
                        ret = m_fadasRemapObj.CreateRemapWorker(
                                inputId, m_config.inputConfigs[inputId].inputFormat,
                                m_config.inputConfigs[inputId].inputWidth,
                                m_config.inputConfigs[inputId].inputHeight,
                                m_config.inputConfigs[inputId].ROI );
                        if ( QC_STATUS_OK != ret )
                        {
                            QC_ERROR( "Create worker fail at inputId = %d", inputId );
                            QCStatus_e retVal;
                            retVal = m_fadasRemapObj.DestroyMap();
                            if ( QC_STATUS_OK != retVal )
                            {
                                QC_ERROR( "Destroy map failed!" );
                            }
                            retVal = m_fadasRemapObj.DestroyWorkers();
                            if ( QC_STATUS_OK != retVal )
                            {
                                QC_ERROR( "Destroy worker failed!" );
                            }
                            break;
                        }

                        ret = m_fadasRemapObj.CreatRemapTable(
                                inputId, m_config.inputConfigs[inputId].mapWidth,
                                m_config.inputConfigs[inputId].mapHeight,
                                m_config.inputConfigs[inputId].remapTable.pMapX,
                                m_config.inputConfigs[inputId].remapTable.pMapY );
                        if ( QC_STATUS_OK != ret )
                        {
                            QC_ERROR( "Create remap table fail at inputId = %d", inputId );
                            QCStatus_e retVal;
                            retVal = m_fadasRemapObj.DestroyMap();
                            if ( QC_STATUS_OK != retVal )
                            {
                                QC_ERROR( "Destroy map failed!" );
                            }
                            retVal = m_fadasRemapObj.DestroyWorkers();
                            if ( QC_STATUS_OK != retVal )
                            {
                                QC_ERROR( "Destroy worker failed!" );
                            }
                            break;
                        }
                    }
                }
            }

            if ( QC_STATUS_OK == ret )
            {
                m_state = QC_OBJECT_STATE_READY;
            }
            else
            {
                m_state = QC_OBJECT_STATE_INITIAL;
                QCStatus_e retVal;
                retVal = m_fadasRemapObj.Deinit();
                if ( QC_STATUS_OK != retVal )
                {
                    QC_ERROR( "Deinit fadas remap failed!" );
                }
                retVal = ComponentIF::Deinit();
                if ( QC_STATUS_OK != retVal )
                {
                    QC_ERROR( "Deinit ComponentIF failed!" );
                }
            }
        }
    }

    return ret;
}

QCStatus_e Remap::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( QC_OBJECT_STATE_READY != m_state )
    {
        QC_ERROR( "Remap component not in ready status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        QCStatus_e retVal = QC_STATUS_OK;

        retVal = m_fadasRemapObj.DestroyMap();
        if ( QC_STATUS_OK != retVal )
        {
            QC_ERROR( "Destroy map failed!" );
            ret = QC_STATUS_FAIL;
        }
        retVal = m_fadasRemapObj.DestroyWorkers();
        if ( QC_STATUS_OK != retVal )
        {
            QC_ERROR( "Destroy worker failed!" );
            ret = QC_STATUS_FAIL;
        }
        retVal = m_fadasRemapObj.Deinit();
        if ( QC_STATUS_OK != retVal )
        {
            QC_ERROR( "Deinit fadas remap failed!" );
            ret = QC_STATUS_FAIL;
        }
        retVal = ComponentIF::Deinit();
        if ( QC_STATUS_OK != retVal )
        {
            QC_ERROR( "Deinit ComponentIF failed!" );
            ret = QC_STATUS_FAIL;
        }
    }

    return ret;
}

QCStatus_e Remap::RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers,
                                   FadasBufType_e bufferType )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "Remap component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < numBuffers; inputId++ )
        {
            const QCSharedBuffer_t *input = &pBuffers[inputId];
            int32_t fd = m_fadasRemapObj.RegBuf( input, bufferType );
            if ( 0 > fd )
            {
                QC_ERROR( "Failed to register buffer for inputId = %d!", inputId );
                ret = QC_STATUS_FAIL;
                break;
            }
        }
    }

    return ret;
}

QCStatus_e Remap::DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "Remap component not in ready or running status!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( nullptr == pBuffers )
    {
        QC_ERROR( "Empty buffers pointer!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        for ( uint32_t inputId = 0; inputId < numBuffers; inputId++ )
        {
            m_fadasRemapObj.DeregBuf( pBuffers[inputId].data() );
        }
    }

    return ret;
}

QCStatus_e Remap::Execute( const QCSharedBuffer_t *pInputs, uint32_t numInputs,
                           const QCSharedBuffer_t *pOutput )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( ( QC_OBJECT_STATE_READY != m_state ) && ( QC_OBJECT_STATE_RUNNING != m_state ) )
    {
        QC_ERROR( "Remap component not initialized!" );
        ret = QC_STATUS_BAD_STATE;
    }
    else if ( m_config.numOfInputs != numInputs )
    {
        QC_ERROR( "Number of input buffers not equal to config value!" );
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else
    {
        ret = m_fadasRemapObj.RemapRun( pInputs, pOutput );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_ERROR( "Failed to run remap component!" );
    }

    return ret;
}

}   // namespace component
}   // namespace QC