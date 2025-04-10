// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_COMPONENT_IF_HPP
#define RIDEHAL_COMPONENT_IF_HPP

#include <string>

#include "ridehal/common/Logger.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/common/Types.hpp"

using namespace ridehal::common;

/** @brief The RideHal Version */
#define RIDEHAL_VERSION_MAJOR 1
#define RIDEHAL_VERSION_MINOR 9
#define RIDEHAL_VERSION_PATCH 0

namespace ridehal
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
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    virtual RideHalError_e Start() = 0;

    /**  @brief Stop the component
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    virtual RideHalError_e Stop() = 0;

    /**
     * @brief deinitialize the component
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    virtual RideHalError_e Deinit();

    /**
     * @brief get the current state of the component
     * @return the current state of the component
     */
    RideHal_ComponentState_t GetState();

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
     * @return RIDEHAL_ERROR_NONE on success, others on failure
     */
    RideHalError_e Init( const char *pName, Logger_Level_e level = LOGGER_LEVEL_ERROR );

protected:
    std::string m_name;
    RIDEHAL_DECLARE_LOGGER();
    RideHal_ComponentState_t m_state = RIDEHAL_COMPONENT_STATE_INITIAL;

};   // class ComponentIF

}   // namespace component
}   // namespace ridehal

#endif   // RIDEHAL_COMPONENT_IF_HPP
