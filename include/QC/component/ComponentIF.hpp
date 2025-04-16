// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_OBJECT_IF_HPP
#define QC_OBJECT_IF_HPP

#include <string>

#include "QC/common/Types.hpp"
#include "QC/infras/logger/Logger.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"

/** @brief The QC Version */
#define QC_VERSION_MAJOR 1
#define QC_VERSION_MINOR 9
#define QC_VERSION_PATCH 0

namespace QC
{
namespace component
{

/**
 * @brief ComponentIF
 * Component Interface
 */
class ComponentIF
{
public:
    ComponentIF() = default;
    ~ComponentIF() = default;

    /**
     * @brief Start the component
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Start() = 0;

    /**  @brief Stop the component
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Stop() = 0;

    /**
     * @brief deinitialize the component
     * @return QC_STATUS_OK on success, others on failure
     */
    virtual QCStatus_e Deinit();

    /**
     * @brief get the current state of the component
     * @return the current state of the component
     */
    QCObjectState_e GetState();

    /**
     * @brief get the name of the component
     * @return the name of the component
     */
    const char *GetName();

protected:
    /**
     * @brief Initialize the component
     * @param[in] pName the component unique instance name
     * @param[in] level the logger message level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

protected:
    std::string m_name;
    QC_DECLARE_LOGGER();
    QCObjectState_e m_state = QC_OBJECT_STATE_INITIAL;

};   // class ComponentIF

}   // namespace component
}   // namespace QC

#endif   // QC_OBJECT_IF_HPP
