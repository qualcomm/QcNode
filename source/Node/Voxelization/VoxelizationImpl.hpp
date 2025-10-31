// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_NODE_VOXELIZATION_IMPL_HPP
#define QC_NODE_VOXELIZATION_IMPL_HPP

#include <cinttypes>
#include <cmath>
#include <memory>
#include <string>
#include <unistd.h>
#include <unordered_map>

#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Infras/NodeTrace/NodeTrace.hpp"
#include "QC/Node/Voxelization.hpp"

#include "FadasPlr.hpp"
#include "OpenclIface.hpp"

namespace QC
{
namespace Node
{

using namespace QC::Memory;
using namespace QC::libs::FadasIface;
using namespace QC::libs::OpenclIface;

/**
 * @brief Enumeration of input pointclouds type used for OpenCL implementation
 *
 * @enum VOXELIZATION_INPUT_XYZR
 * In this mode, the input cloud point data has 4 feature dimensions: [x,y,z,r], the output
 * point data has 10 feature dimensions:
 * [x,y,z,r,x-xMean,y-yMean,z-zMean,x-xPillar,y-yPillar,z-zPillar].
 * @enum VOXELIZATION_INPUT_XYZRT
 * In this mode, the input cloud point data has 5 feature dimensions: [x,y,z,r,t], the
 * output point data has 10 feature dimensions:
 * [x,y,z,r,x-xMean,y-yMean,z-zMean,x-xPillar,y-yPillar,z-zPillar].
 */
typedef enum
{
    VOXELIZATION_INPUT_MODE_XYZR,
    VOXELIZATION_INPUT_MODE_XYZRT,
    VOXELIZATION_INPUT_MODE_MAX
} Voxelization_InputMode_e;

/** @brief Configuration structure for Voxelization function parameters
 *
 * @param processor                 Processor type
 * @param coreIds                   A list of core IDs representing the target hardware processors
 * @param pillarXSize               Pillar size in x direction in meters
 * @param pillarYSize               Pillar size in y direction in meters
 * @param pillarZSize               Pillar size in z direction in meters
 * @param minXRange                 Minimum range value in x direction
 * @param minYRange                 Minimum range value in y direction
 * @param minZRange                 Minimum range value in z direction
 * @param maxXRange                 Maximum range value in x direction
 * @param maxYRange                 Maximum range value in y direction
 * @param maxZRange                 Maximum range value in z direction
 * @param numInFeatureDim           Number of features for each point in the input point cloud data
 * For e.g., if cloud data point contains (x, y, z, r) features, then the numInFeatureDim is 4
 * @param numOutFeatureDim          Number of features for each point in point pillars
 * @param maxNumInPts               Maximum number of points in input point cloud
 * @param maxNumPlrs                Maximum number of point pillars that can be created
 * @param maxNumPtsPerPlr           Maximum number of points to map to each pillar
 * @param inputMode                 Voxelization input pointclouds type
 */
typedef struct Voxelization_Config_t
{
    QCProcessorType_e processor;
    std::vector<uint32_t> coreIds;

    float pillarXSize;
    float pillarYSize;
    float pillarZSize;
    float minXRange;
    float minYRange;
    float minZRange;
    float maxXRange;
    float maxYRange;
    float maxZRange;
    uint32_t numInFeatureDim;
    uint32_t numOutFeatureDim;
    uint32_t maxNumInPts;
    uint32_t maxNumPlrs;
    uint32_t maxNumPtsPerPlr;
    std::string inputMode;
} Voxelization_Config_t;


/** @brief Configuration structure for Voxelization Node
 *
 * @param voxelConfig               Configuration structure for Voxelization function parameters
 * @param outputPlrBufferIds        The indices of buffers for output pillar tensors
 * @param outputFeatureBufferIds    The indices of buffers for output feature tensors
 * @param plrPointsBufferId         The index of buffer for maximal pliiar point number
 * @param coordToPlrIdxBufferId     The index of buffer to store coordinate to pillar point
 * transform indices
 * @param globalBufferIdMap         Mapping of global buffer indices used to identify which buffers
 * in QCFrameDescriptorNodeIfs correspond to Voxelization inputs and outputs.
 * @note This field is optional. If empty, a default mapping is applied:
 * - Index 0 to (inputBufferIds.size - 1) : Input buffers
 * - Index inputBufferIds.size to (inputBufferIds.size + outputPlrBufferIds - 1):
 * Output pillar buffers
 * - Index (inputBufferIds.size + outputPlrBufferIds)  to (inputBufferIds.size +
 * outputPlrBufferIds.size + outputFeatureBufferIds.size - 1): Output feature buffers
 * - Index inputBufferIds.size + outputPlrBufferIds.size + outputFeatureBufferIds.size :
 * Internal (for maximal pillar points buffer)
 * - Index inputBufferIds.size + outputPlrBufferIds.size + outputFeatureBufferIds.size + 1 :
 * Internal (for coordinate to pillar buffer)
 * @param bDeRegisterAllBuffersWhenStop Flag to deregister all buffers when stopped
 */
typedef struct VoxelizationImplConfig : public QCNodeConfigBase_t
{
    Voxelization_Config_t voxelConfig;
    std::vector<uint32_t> outputPlrBufferIds;
    std::vector<uint32_t> outputFeatureBufferIds;
    uint32_t plrPointsBufferId;
    uint32_t coordToPlrIdxBufferId;
    std::vector<QCNodeBufferMapEntry_t> globalBufferIdMap;
    bool bDeRegisterAllBuffersWhenStop;
} VoxelizationImplConfig_t;

// TODO
typedef struct VoxelizationImplMonitorConfig : public QCNodeMonitoringBase_t
{
    bool bEnablePerf;
} VoxelizationImplMonitorConfig_t;


class VoxelizationImpl
{
public:
    /**
     * @brief Construct a new VoxelizationImpl object.
     */
    VoxelizationImpl( QCNodeID_t &nodeId, Logger &logger );

    /**
     * @brief Destroy the VoxelizationImpl object.
     */
    ~VoxelizationImpl();

    /**
     * @brief Initialize VoxelizationImpl object.
     *
     * @return QC_STATUS_OK on success, others on failure.
     */
    QCStatus_e Initialize( QCNodeEventCallBack_t callback,
                           std::vector<std::reference_wrapper<QCBufferDescriptorBase>> &buffers );

    /**
     * @brief Start the VoxelizationImpl object.
     *
     * @return QC_STATUS_OK on success, others on failure.
     */
    QCStatus_e Start();

    /**
      * @brief Process the provided frame descriptor.
      * @param[in] frameDesc The frame descriptor containing a vector of input/output buffers.
      * @note The voxelization pipeline has 1 input and 2 outputs.
      * The input is point cloud with size of maxPointNum x inputFeatureDimNum x sizeof(float).
      * The 1st output is pillar index tensor with size of 4 x maxPlrNum x sizeof(float).
      * The 2nd output is stacked pillar tensor with size of maxPlrNum x maxPointNumPerPlr x
      * outputFeatureDimNum x sizeof(float).
      * @return QC_STATUS_OK on success, other values on failure.
      */
    QCStatus_e ProcessFrameDescriptor( QCFrameDescriptorNodeIfs &frameDesc );

    /**
     * @brief Stop the VoxelizationImpl object.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    QCStatus_e Stop();

    /**
     * @brief Deinitialize the VoxelizationImpl object.
     * @return QC_STATUS_OK on success, other values on failure.
     */
    QCStatus_e DeInitialize();

    /**
     * @brief Get the VoxelizationImpl configuration structure object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    VoxelizationImplConfig_t &GetConifg() { return m_config; }

    /**
     * @brief Get the monitor configuration structure object
     *
     * @return QC_STATUS_OK on success, others on failure
     */
    VoxelizationImplMonitorConfig_t &GetMonitorConifg() { return m_monitorConfig; }

    /**
     * @brief Retrieves the current state of the Voxelization Node.
     *
     * This function returns the current operational state of the Voxelization Node,
     * which may indicate whether the node is initialized, running, stopped, or in
     * an error state.
     *
     * @return The current state of the Voxelization Node.
     */
    QCObjectState_e GetState();

private:
    QCStatus_e ProcessCL( TensorDescriptor_t &inputTensorDesc,
                          TensorDescriptor_t &outputPlrTensorDesc,
                          TensorDescriptor_t &outputFeatTensorDesc );

    QCStatus_e RegisterBuffer( QCBufferDescriptorBase_t &buffer, uint32_t bufferId,
                               FadasBufType_e bufferType );
    QCStatus_e DeRegisterBuffer( QCBufferDescriptorBase_t &buffer, uint32_t bufferId );

    QCStatus_e SetupGlobalBufferIdMap();

    Voxelization_InputMode_e GetInputMode( std::string &mode );

    void InitOpenCLArgs();

private:
    QCNodeID_t &m_nodeId;
    Logger &m_logger;
    VoxelizationImplConfig_t m_config;
    VoxelizationImplMonitorConfig_t m_monitorConfig;
    QCObjectState_e m_state;

    Voxelization_InputMode_e m_inputMode;
    QCProcessorType_e m_processor;

    uint32_t m_outputPlrBufferNum;
    uint32_t m_outputFeatureBufferNum;
    size_t m_gridXSize;
    size_t m_gridYSize;

    TensorDescriptor_t m_plrPointsTensor;
    TensorDescriptor_t m_coordToPlrIdxTensor;

    bool m_bufferRegisterOK;
    FadasPlrPreProc m_plrPre;
    OpenclSrv m_openCLSrvObj;

    static constexpr size_t CLUSTER_POINT_KERNEL_ARGS = 19;
    static constexpr size_t FEAT_GATHER_KERNEL_ARGS = 13;
    size_t m_numOfArgs1 = CLUSTER_POINT_KERNEL_ARGS;
    size_t m_numOfArgs2 = FEAT_GATHER_KERNEL_ARGS;
    OpenclIfcae_Arg_t m_openCLArgsClusterPoint[CLUSTER_POINT_KERNEL_ARGS];
    OpenclIfcae_Arg_t m_openCLArgsFeatGather[FEAT_GATHER_KERNEL_ARGS];

    cl_kernel m_clusterPointKernel;
    cl_kernel m_featGatherKernel;

    cl_mem m_clPlrPointsBuffer;
    cl_mem m_clCoordToPlrIdxBuffer;
    std::unordered_map<uint64_t, cl_mem> m_clBufferDescMap;

    QC_DECLARE_NODETRACE();
};


}   // namespace Node
}   // namespace QC

#endif   // QC_NODE_VOXELIZATION_IMPL_HPP

