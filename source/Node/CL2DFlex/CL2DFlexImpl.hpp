// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_CL2DFLEX_HPP
#define QC_CL2DFLEX_HPP

#include <cinttypes>
#include <inttypes.h>
#include <memory>
#include <unistd.h>

#include "OpenclIface.hpp"
#include "QC/Node/CL2DFlex.hpp"

#define QC_CL2DFLEX_ROI_NUMBER_MAX 100 /**<max number of roi parameters for ExecuteWithROI API*/

namespace QC
{
namespace Node
{
using namespace QC::libs::OpenclIface;

/**
 * @brief CL2D Node Configuration Data Structure
 * @param params The QC component CL2D configuration data structure.
 * @param bufferIds The indices of buffers in QCNodeInit::buffers provided by the user application
 * for use by CL2D. These buffers will be registered into CL2D during the initialization stage.
 * @note bufferIds are optional and can be empty, in which case the buffers will be registered into
 * CL2D when the API ProcessFrameDescriptor is called.
 * @param globalBufferIdMap The global buffer index map used to identify which buffer in
 * QCFrameDescriptorNodeIfs is used for CL2D input(s) and output(s).
 * @note globalBufferIdMap is optional and can be empty, in which case a default buffer index map
 * will be applied for CL2D input(s) and output(s). For now CL2D only support multiple inputs to
 * single output
 * - The index 0 of QCFrameDescriptorNodeIfs will be input 0.
 * - The index 1 of QCFrameDescriptorNodeIfs will be input 1.
 * - ...
 * - The index N-1 of QCFrameDescriptorNodeIfs will be input N-1.
 * - The index N of QCFrameDescriptorNodeIfs will be output.
 * @param bDeRegisterAllBuffersWhenStop When the Stop API of the CL2DFlex node is called and
 * bDeRegisterAllBuffersWhenStop is true, deregister all buffers.
 */
typedef struct CL2DFlexImplConfig : public QCNodeConfigBase_t
{
    CL2DFlex_Config_t params;
    std::vector<uint32_t> bufferIds;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} CL2DFlexImplConfig_t;

class CL2DPipelineBase; /**<pipeline base class*/

class CL2DFlexImpl
{

public:
    CL2DFlexImpl( QCNodeID_t &nodeId, Logger &logger )
        : m_nodeId( nodeId ),
          m_logger( logger ),
          m_state( QC_OBJECT_STATE_INITIAL ) {};
    CL2DFlexImplConfig_t &GetConifg() { return m_config; }

    QCStatus_e Initialize( std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );
    QCStatus_e Start();
    QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );
    QCStatus_e Stop();
    QCStatus_e DeInitialize();
    QCObjectState_e GetState();

private:
    QCStatus_e RegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );
    QCStatus_e DeRegisterBuffers( const QCSharedBuffer_t *pBuffers, uint32_t numBuffers );
    QCStatus_e Execute( const QCSharedBuffer_t *pInputs, const uint32_t numInputs,
                        const QCSharedBuffer_t *pOutput );
    QCStatus_e ExecuteWithROI( const QCSharedBuffer_t *pInput, const QCSharedBuffer_t *pOutput,
                               const CL2DFlex_ROIConfig_t *pROIs, const uint32_t numROIs );
    QCStatus_e SetupGlobalBufferIdMap();

private:
    QCNodeID_t &m_nodeId;
    Logger &m_logger;
    CL2DFlexImplConfig_t m_config;
    QCObjectState_e m_state;

    OpenclSrv m_OpenclSrvObj;
    CL2DPipelineBase *m_pCL2DPipeline[QC_MAX_INPUTS] = { nullptr };
    cl_kernel m_kernel[QC_MAX_INPUTS];

    uint32_t m_inputNum;
    uint32_t m_outputNum = 1;

public:
    uint32_t m_mapXBufferIds[QC_MAX_INPUTS];
    uint32_t m_mapYBufferIds[QC_MAX_INPUTS];
    CL2DFlex_ROIConfig_t m_ROIs[QC_CL2DFLEX_ROI_NUMBER_MAX];
    uint32_t m_numOfROIs;

};   // class CL2DFlexImpl

}   // namespace Node
}   // namespace QC

#endif   // QC_CL2DFLEX_HPP
