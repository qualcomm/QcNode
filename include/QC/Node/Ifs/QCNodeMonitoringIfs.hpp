// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_MONITORING_IFS_HPP
#define QC_NODE_MONITORING_IFS_HPP

#include "QC/Common/QCDefs.hpp"

namespace QC
{

typedef struct QCNodeMonitoringBase
{
    uint32_t numOfEntries;
} QCNodeMonitoringBase_t;

class QCNodeMonitoringIfs
{

public:
    // Rare execution methods
    ////////////////////////////////////
    // verify, migrate to structure and set internal data member
    // stringConfig is std:string to be able to work with different textual encoding (josn, yaml,
    // custom)
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors ) = 0;

    // Get details of parameters and ranges needed for essential functionality in run time
    // The returned is std:string to be able to work with different textual encoding (json, yaml,
    // custom) Convey the monitoring options supported by the node (including the common base
    // options
    // )
    virtual const std::string &GetOptions() = 0;

    // Get configuration structure reference
    virtual const QCNodeMonitoringBase_t &Get() = 0;

    // return Monitoring information maximal size in bytes
    virtual inline uint32_t GetMaximalSize() = 0;

    // return Monitoring information currently configured data size in Bytes
    virtual inline uint32_t GetCurrentSize() = 0;

    // place monitoring information in a provided pointer
    // buffer size variable is provided as input to indicate the maximal buffer size available for
    // writing same parameter is used to return the actual size of data written
    virtual QCStatus_e Place( void *pData, uint32_t &size ) = 0;

protected:
    // In implementation a data member will be placed here from a type
    // which inherits "QCNodeMonitoringBase_t" as base
};

}   // namespace QC

#endif   // QC_NODE_MONITORING_IFS_HPP
