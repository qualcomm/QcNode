// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "RadarIface.hpp"
#include "RadarIf.hxx"
#include <errno.h>

namespace QC
{
namespace Library
{

/**
 * @brief Implementation class for RadarIface using PIMPL pattern
 */
class RadarIface::Impl
{
public:
    Impl() : m_radar( nullptr ), m_initialized( false ) {}

    ~Impl() { Deinitialize(); }

    QCStatus_e Initialize( const char *devicePath )
    {
        QCStatus_e status = QC_STATUS_OK;

        // Only initialize once
        if ( m_initialized )
        {
            status = QC_STATUS_OK;
        }
        else if ( devicePath == nullptr )
        {
            status = QC_STATUS_BAD_ARGUMENTS;
        }
        else
        {
            try
            {
                m_radar = new q::interface::Radar( devicePath );
                if ( m_radar == nullptr )
                {
                    status = QC_STATUS_BAD_STATE;
                }
                else
                {
                    if ( !m_radar->is_open() )
                    {
                        delete m_radar;
                        m_radar = nullptr;
                        status = QC_STATUS_BAD_STATE;
                    }
                    else
                    {
                        m_initialized = true;
                        status = QC_STATUS_OK;
                    }
                }
            }
            catch ( ... )
            {
                status = QC_STATUS_BAD_STATE;
            }
        }
        return status;
    }

    QCStatus_e Deinitialize()
    {
        if ( m_radar && m_initialized )
        {
            delete m_radar;
            m_radar = nullptr;
            m_initialized = false;
        }
        return QC_STATUS_OK;
    }

    QCStatus_e Execute( uint8_t *pInput, size_t inputSize, uint8_t *pOutput, size_t outputSize )
    {
        QCStatus_e status = QC_STATUS_OK;
        if ( !m_initialized || !m_radar )
        {
            status = QC_STATUS_BAD_STATE;
        }
        else
        {
            if ( !pInput || !pOutput || inputSize == 0 || outputSize == 0 )
            {
                status = QC_STATUS_BAD_ARGUMENTS;
            }
            else
            {
                int result = m_radar->execute( pInput, inputSize, pOutput, outputSize );
                status = ( result == EOK ) ? QC_STATUS_OK : QC_STATUS_FAIL;
            }
        }
        return status;
    }

    bool IsInitialized() const { return m_initialized; }

private:
    q::interface::Radar *m_radar;
    bool m_initialized;
};

// RadarIface implementation
RadarIface::RadarIface() : m_pImpl( new Impl() ) {}

RadarIface::~RadarIface()
{
    delete m_pImpl;
}

QCStatus_e RadarIface::Initialize( const char *devicePath )
{
    return m_pImpl->Initialize( devicePath );
}

QCStatus_e RadarIface::Deinitialize()
{
    return m_pImpl->Deinitialize();
}

QCStatus_e RadarIface::Execute( uint8_t *pInput, size_t inputSize, uint8_t *pOutput,
                                size_t outputSize )
{
    return m_pImpl->Execute( pInput, inputSize, pOutput, outputSize );
}

bool RadarIface::IsInitialized() const
{
    return m_pImpl->IsInitialized();
}

}   // namespace Library
}   // namespace QC
