// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_REMAP_HPP
#define QC_REMAP_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "QC/Node/Remap.hpp"

namespace QC
{
namespace Node
{

/**
 * @brief Remap Node Configuration Data Structure
 * @param params The QC component Remap configuration data structure.
 * @param bufferIds The indices of buffers in QCNodeInit::buffers provided by the user application
 * for use by Remap. These buffers will be registered into Remap during the initialization stage.
 * @note bufferIds are optional and can be empty, in which case the buffers will be registered into
 * Remap when the API ProcessFrameDescriptor is called.
 * @param globalBufferIdMap The global buffer index map used to identify which buffer in
 * QCFrameDescriptorNodeIfs is used for Remap input(s) and output(s).
 * @note globalBufferIdMap is optional and can be empty, in which case a default buffer index map
 * will be applied for Remap input(s) and output(s). For now Remap only support multiple inputs to
 * single output
 * - The index 0 of QCFrameDescriptorNodeIfs will be input 0.
 * - The index 1 of QCFrameDescriptorNodeIfs will be input 1.
 * - ...
 * - The index N-1 of QCFrameDescriptorNodeIfs will be input N-1.
 * - The index N of QCFrameDescriptorNodeIfs will be output.
 * @param bDeRegisterAllBuffersWhenStop When the Stop API of the Remap node is called and
 * bDeRegisterAllBuffersWhenStop is true, deregister all buffers.
 */
typedef struct RemapImplConfig : public QCNodeConfigBase_t
{
    Remap_Config_t params;
    std::vector<uint32_t> bufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} RemapImplConfig_t;

class RemapPipelineBase; /**<pipeline base class*/

class RemapImpl
{

public:
    RemapImpl( QCNodeID_t &nodeId, Logger &logger )
        : m_nodeId( nodeId ),
          m_logger( logger ),
          m_state( QC_OBJECT_STATE_INITIAL ) {};
    RemapImplConfig_t &GetConifg() { return m_config; }

    QCStatus_e Initialize( std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );
    QCStatus_e Start();
    QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );
    QCStatus_e Stop();
    QCStatus_e DeInitialize();
    QCObjectState_e GetState();

private:
    QCStatus_e SetupGlobalBufferIdMap();

private:
    QCNodeID_t &m_nodeId;
    Logger &m_logger;
    RemapImplConfig_t m_config;
    QCObjectState_e m_state;
    FadasRemap m_fadasRemapObj;

};   // class RemapImpl

}   // namespace Node
}   // namespace QC

#endif   // QC_REMAP_HPP
