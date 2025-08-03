// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_BASES_HPP
#define QC_NODE_BASES_HPP

#include "QC/Node/Ifs/QCFrameDescriptorNodeIfs.hpp"
#include "QC/Node/Ifs/QCNodeConfigIfs.hpp"
#include "QC/Node/Ifs/QCNodeDefs.hpp"
#include "QC/Node/Ifs/QCNodeMonitoringIfs.hpp"
#include <functional>

namespace QC
{

typedef struct QCNodeEventInfo
{
    QCNodeEventInfo() = delete;
    QCNodeEventInfo( QCFrameDescriptorNodeIfs &frameDesc, const QCNodeID_t &node, QCStatus_e status,
                     QCObjectState_e state )
        : frameDesc( frameDesc ),
          node( node ),
          status( status ),
          state( state ){};
    QCFrameDescriptorNodeIfs &frameDesc;
    QCNodeID_t node;
    QCStatus_e status;
    QCObjectState_e state;
} QCNodeEventInfo_t;

// define call back function type
typedef std::function<void( const QCNodeEventInfo_t & )> QCNodeEventCallBack_t;

typedef struct QCNodeInit
{
    std::string config;
    QCNodeEventCallBack_t callback;
    std::vector<std::reference_wrapper<QCBufferDescriptorBase_t>> buffers;
} QCNodeInit_t;

class QCNodeIfs
{

public:
    virtual QCStatus_e Initialize( QCNodeInit_t &config ) = 0;
    virtual QCStatus_e DeInitialize() = 0;
    virtual QCStatus_e Start() = 0;
    virtual QCStatus_e Stop() = 0;

    // put frame descriptor in the node queue
    virtual QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc ) = 0;

    // getter functions
    virtual QCObjectState_e GetState() = 0;

    virtual QCNodeConfigIfs &GetConfigurationIfs() = 0;
    virtual QCNodeMonitoringIfs &GetMonitoringIfs() = 0;

private:
};

}   // namespace QC

#endif   // QC_NODE_BASES_HPP
