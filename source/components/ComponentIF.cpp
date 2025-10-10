// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "QC/component/ComponentIF.hpp"
#include <stdio.h>

namespace QC
{
namespace component
{

QCStatus_e ComponentIF::Init( const char *pName, Logger_Level_e level )
{
    QCStatus_e ret = QC_STATUS_OK;

    if ( nullptr == pName )
    {
        ret = QC_STATUS_BAD_ARGUMENTS;
    }
    else if ( QC_OBJECT_STATE_INITIAL != m_state )
    {
        ret = QC_STATUS_BAD_STATE;
    }
    else
    {
        m_name = pName;
        ret = QC_LOGGER_INIT( pName, level );
        if ( QC_STATUS_OK != ret )
        {
            (void) fprintf( stderr,
                            "WARINING: failed to create logger for component %s: ret = %d\n", pName,
                            ret );
        }
        ret = QC_STATUS_OK;
        QC_INFO( "QC version %d.%d.%d", QC_VERSION_MAJOR, QC_VERSION_MINOR, QC_VERSION_PATCH );
    }

    return ret;
}

QCStatus_e ComponentIF::Deinit()
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = QC_LOGGER_DEINIT();
    if ( QC_STATUS_OK != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to deinit logger for component %s: ret = %d\n",
                        GetName(), ret );
    }
    ret = QC_STATUS_OK; /* ignore logger init error */

    m_state = QC_OBJECT_STATE_INITIAL;

    return ret;
}


QCObjectState_e ComponentIF::GetState()
{
    return m_state;
}

const char *ComponentIF::GetName()
{
    return m_name.c_str();
}

}   // namespace component
}   // namespace QC
