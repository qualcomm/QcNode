// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/Node/NodeBase.hpp"

namespace QC
{
namespace Node
{

const std::string NodeConfigIfs::s_QC_STATUS_UNSUPPORTED = "QC_STATUS_UNSUPPORTED";

QCStatus_e NodeConfigIfs::VerifyAndSet( const std::string config, std::string &errors )
{
    QCStatus_e status = QC_STATUS_OK;
    std::string name = "";
    Logger_Level_e loggerLevel = LOGGER_LEVEL_ERROR;

    status = m_dataTree.Load( config, errors );
    if ( ( QC_STATUS_OK == status ) && ( false == m_bLoggerInit ) )
    {
        name = m_dataTree.Get<std::string>( "static.name", "" );
        if ( "" == name )
        {
            errors += "no name specified, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }

        std::string levelStr = m_dataTree.Get<std::string>( "static.logLevel", "ERROR" );
        loggerLevel = LOGGER_LEVEL_ERROR;
        if ( levelStr == "VERBOSE" )
        {
            loggerLevel = LOGGER_LEVEL_VERBOSE;
        }
        else if ( levelStr == "DEBUG" )
        {
            loggerLevel = LOGGER_LEVEL_DEBUG;
        }
        else if ( levelStr == "INFO" )
        {
            loggerLevel = LOGGER_LEVEL_INFO;
        }
        else if ( levelStr == "WARN" )
        {
            loggerLevel = LOGGER_LEVEL_WARN;
        }
        else if ( levelStr == "ERROR" )
        {
            loggerLevel = LOGGER_LEVEL_ERROR;
        }
        else
        {
            errors += "invalid logLevel:<" + levelStr + ">, ";
            status = QC_STATUS_BAD_ARGUMENTS;
        }

        if ( QC_STATUS_OK == status )
        {
            status = (QCStatus_e) m_logger.Init( name.c_str(), loggerLevel );
            if ( QC_STATUS_OK == status )
            {
                m_bLoggerInit = true;
            }
        }
    }

    QC_DEBUG( "config: %s", config.c_str() );

    return status;
}

QCStatus_e NodeBase::Init( QCNodeID_t nodeId, Logger_Level_e level )
{
    QCStatus_e status = QC_STATUS_OK;
    m_nodeId = nodeId;
    status = (QCStatus_e) m_logger.Init( m_nodeId.name.c_str(), level );
    if ( QC_STATUS_BAD_STATE == status )
    { /* logger used by QCNodeConfigIfs and already initialized */
        status = QC_STATUS_OK;
    }
    QC_LOG_DEBUG( "Init NodeBase %s(%u) type (%d) status=%d", m_nodeId.name.c_str(), m_nodeId.id,
                  m_nodeId.type, status );
    return status;
}

QCStatus_e NodeBase::DeInitialize()
{
    QCStatus_e status = QC_STATUS_OK;
    status = (QCStatus_e) m_logger.Deinit();
    QC_LOG_DEBUG( "DeInit NodeBase %s(%u) type(%d) status=%d", m_nodeId.name.c_str(), m_nodeId.id,
                  m_nodeId.type, status );
    return status;
}

}   // namespace Node
}   // namespace QC
