// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_CONFIG_IFS_HPP
#define QC_NODE_CONFIG_IFS_HPP

#include "QC/Node/Ifs/QCNodeDefs.hpp"

namespace QC
{

typedef struct QCNodeConfigBase
{
    virtual ~QCNodeConfigBase() = default;
    QCNodeID_t nodeId;
    uint32_t numOfEntries;
} QCNodeConfigBase_t;

class QCNodeConfigIfs
{

public:
    // Single lifetime execution methods
    ////////////////////////////////////
    // verify, migrate to structure and set internal data member
    // stringConfig is std:string to be able to work with different textual encoding (josn, yaml,
    // custom)
    virtual QCStatus_e VerifyAndSet( const std::string config, std::string &errors ) = 0;

    // Get details of parameters and ranges needed for essential functionality in run time
    // The returned is std:string to be able to work with different textual encoding (json, yaml,
    // custom) Information for pipeline builder (examples and rules )
    virtual const std::string &GetOptions() = 0;

    // Get configuration structure reference
    virtual const QCNodeConfigBase_t &Get() = 0;

protected:
    // In implementation a data member will be placed here from a type
    // which inherits "QCNodeConfigBase_t" as base
};

}   // namespace QC

#endif   // QC_NODE_CONFIG_IFS_HPP