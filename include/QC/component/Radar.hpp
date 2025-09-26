// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#ifndef QC_RADAR_HPP
#define QC_RADAR_HPP

#include <cinttypes>
#include <cmath>
#include <inttypes.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include "QC/component/ComponentIF.hpp"
#include "RadarIface.hpp"

namespace QC
{
namespace component
{

/** @brief Radar Processing Service Configuration */
typedef struct
{
    std::string serviceName;    /**< Name of the processing service */
    uint32_t timeoutMs;         /**< Timeout for processing in milliseconds */
    bool bEnablePerformanceLog; /**< Enable performance logging */
} Radar_ServiceConfig_t;

/** @brief Radar Component Initialization Configuration */
typedef struct
{
    uint32_t maxInputBufferSize;         /**< Maximum expected input buffer size */
    uint32_t maxOutputBufferSize;        /**< Maximum expected output buffer size */
    Radar_ServiceConfig_t serviceConfig; /**< Processing service configuration */
} Radar_Config_t;

/**
 * @brief Component Radar
 * Radar component that interfaces with external processing service
 */
class Radar final : public ComponentIF
{
public:
    Radar();
    ~Radar();

    /**
     * @brief Initialize the Radar component
     * @param[in] pName the component unique instance name
     * @param[in] pConfig the Radar configuration parameters
     * @param[in] level the logger message level
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Init( const char *pName, const Radar_Config_t *pConfig,
                     Logger_Level_e level = LOGGER_LEVEL_ERROR );

    /**
     * @brief Start the Radar component
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Start();

    /**
     * @brief Stop the Radar component
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Stop();

    /**
     * @brief Deinitialize the Radar component
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinit();

    /**
     * @brief Execute radar processing
     * @param[in] pInput the input shared buffer
     * @param[out] pOutput the output shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput );

    /**
     * @brief Register input buffer with processing service
     * @param[in] pInputBuffer the input shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterInputBuffer( const QCSharedBuffer_t *pInputBuffer );

    /**
     * @brief Register output buffer with processing service
     * @param[in] pOutputBuffer the output shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e RegisterOutputBuffer( const QCSharedBuffer_t *pOutputBuffer );

    /**
     * @brief Deregister input buffer
     * @param[in] pInputBuffer the input shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeregisterInputBuffer( const QCSharedBuffer_t *pInputBuffer );

    /**
     * @brief Deregister output buffer
     * @param[in] pOutputBuffer the output shared buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e DeregisterOutputBuffer( const QCSharedBuffer_t *pOutputBuffer );

private:
    /**
     * @brief Validate buffer compatibility with service requirements
     * @param[in] pBuffer the shared buffer to validate
     * @param[in] isInput true for input buffer, false for output buffer
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e ValidateBuffer( const QCSharedBuffer_t *pBuffer, bool isInput );

private:
    Radar_Config_t m_config;
    QC::Library::RadarIface m_radarIface; /**< Radar processing interface */
    std::unordered_map<void *, uint64_t> m_registeredInputBuffers;
    std::unordered_map<void *, uint64_t> m_registeredOutputBuffers;
};

}   // namespace component
}   // namespace QC

#endif   // QC_RADAR_HPP
