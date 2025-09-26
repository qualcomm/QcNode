// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#ifndef QC_RADAR_IFACE_HPP
#define QC_RADAR_IFACE_HPP

#include "QC/Common/Types.hpp"

namespace QC
{
namespace Library
{

/**
 * @brief QC wrapper for the radar interface
 * This class wraps the q::interface::Radar to provide QC-style interface
 */
class RadarIface
{
public:
    RadarIface();
    ~RadarIface();

    /**
     * @brief Initialize the radar interface
     * @param[in] devicePath Path to the radar device
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Initialize( const char *devicePath );

    /**
     * @brief Deinitialize the radar interface
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Deinitialize();

    /**
     * @brief Execute radar processing
     * @param[in] pInput Pointer to input buffer
     * @param[in] inputSize Size of input buffer in bytes
     * @param[in] pOutput Pointer to output buffer
     * @param[in] outputSize Size of output buffer in bytes
     * @return QC_STATUS_OK on success, others on failure
     */
    QCStatus_e Execute( uint8_t *pInput, size_t inputSize, uint8_t *pOutput, size_t outputSize );

    /**
     * @brief Check if radar interface is initialized
     * @return true if initialized, false otherwise
     */
    bool IsInitialized() const;

private:
    class Impl;
    Impl *m_pImpl;
};

}   // namespace Library
}   // namespace QC

#endif   // QC_RADAR_IFACE_HPP
