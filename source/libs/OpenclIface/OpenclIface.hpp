// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#ifndef QC_OPENCL_IFACE_HPP
#define QC_OPENCL_IFACE_HPP

#include <CL/cl.h>
#include <CL/cl_ext.h>
#include <CL/cl_ext_qcom.h>
#include <map>
#include <vector>

#include "QC/common/Types.hpp"
#include "QC/infras/logger/Logger.hpp"
#include "QC/infras/memory/SharedBuffer.hpp"
using namespace QC::common;

namespace QC
{
namespace libs
{
namespace OpenclIface
{

/*=================================================================================================
** Typedefs
=================================================================================================*/

/** @brief OpenCL performance priority level */
typedef enum
{
    OPENCLIFACE_PERF_HIGH,   /**request highest priority for all submissions for any command*/
    OPENCLIFACE_PERF_NORMAL, /**request balanced priority for all submissions for any command, the
                                default setting*/
    OPENCLIFACE_PERF_LOW     /**request lower priority for all submissions for any command*/
} OpenclIfcae_Perf_e;

/** @brief OpenCL execute arguments structure */
typedef struct
{
    void *pArg;     /**argument pointer*/
    size_t argSize; /**argument size*/
} OpenclIfcae_Arg_t;

/** @brief OpenCL execute work parameters structure */
typedef struct
{
    size_t *pGlobalWorkOffset; /**global work offset*/
    size_t *pGlobalWorkSize;   /**global work size*/
    size_t *pLocalWorkSize;    /**local work size*/
    size_t workDim;            /**work dimensions*/
} OpenclIface_WorkParams_t;

/** @brief OpenCL memory info structure */
typedef struct
{
    cl_mem clMem; /**OpenCL memory buffer*/
} OpenclIface_MemInfo_t;

class OpenclSrv
{

    /*=================================================================================================
    ** API Functions
    =================================================================================================*/

public:
    /**
     * @brief Initialize the OpenclIface object
     * @param[in] pName the OpenclIface unique instance name
     * @param[in] level the logger message level
     * @param[in] priority the desired performance priority level for this OpenCL context
     * @return QC_STATUS_OK on success, others on failure
     * @note Do all the initialization work for a OpenclIface object: get platform ID, get device
     * ID, create context, create command queue, get device info. Must be called at the beginning of
     * pipeline.
     */
    QCStatus_e Init( const char *pName, Logger_Level_e level,
                     OpenclIfcae_Perf_e priority = OPENCLIFACE_PERF_NORMAL );

    /**
     * @brief Load OpenCL program from source file
     * @param[in] pSourceFile the OpenCL program source file string pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Create and build OpenCL program from source file, print build log if compilation
     * fail. In this way the kernel code is easy to read and modify. Must be called after Init.
     */
    QCStatus_e LoadFromSource( const char *pSourceFile );

    /**
     * @brief Load OpenCL program from binary file
     * @param[in] pBinaryFile the OpenCL program binary file string pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Create and build OpenCL program from prebuilt binary file. In this way the compilation
     * time is saved. Must be called after Init.
     */
    QCStatus_e LoadFromBinary( const unsigned char *pBinaryFile );

    /**
     * @brief Create OpenCL kernel from OpenCL program
     * @param[in] pKernel the OpenCL kernel pointer
     * @param[in] pKernelName the OpenCL kernel name string pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Create OpenCL kernel from OpenCL program and store to kernel map. Can be called
     * multiple times to create multiple kernels from a single program.
     */
    QCStatus_e CreateKernel( cl_kernel *pKernel, const char *pKernelName );

    /**
     * @brief Deinitialize the OpenclIface object
     * @return QC_STATUS_OK on success, others on failure
     * @note Do all the deinitialization work for a OpenclIface object: release kernel, release
     * program, release context, release command queue, release memory map. Must be called at the
     * ending of pipeline.
     */
    QCStatus_e Deinit();

    /**
     * @brief Register the OpenclIface buffer
     * @param[in] pBuffer the QC buffer pointer to register
     * @param[in] pBufferCL the OpenCL memory buffer pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Create an OpenCL memory buffer and register the host QC buffer to it in zero
     * memory copy method. Then store the OpenCL memory buffer to buffer map, so the same host
     * buffer would not be registered twice.
     */
    QCStatus_e RegBuf( const QCBuffer_t *pBuffer, cl_mem *pBufferCL );

    /**
     * @brief Register the OpenclIface buffer in image format
     * @param[in] pData the image data pointer to register
     * @param[in] dmaHandle the dma handle of image buffer
     * @param[in] pBufferCL the OpenCL memory buffer pointer
     * @param[in] pFormat the cl image format pointer
     * @param[in] pDesc the cl image descriptor pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Create an OpenCL memory buffer in image format and register the host QC buffer to
     * it in zero memory copy method.
     */
    QCStatus_e RegImage( void *pData, uint64_t dmaHandle, cl_mem *pBufferCL,
                         cl_image_format *pFormat, cl_image_desc *pDesc );
    /**
     * @brief Register a single plane of image buffer
     * @param[in] pData the image data pointer to register
     * @param[in] pBufferCL the OpenCL memory buffer pointer
     * @param[in] pFormat the cl image format pointer
     * @param[in] pDesc the cl image descriptor pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Create an OpenCL memory buffer in image format and register a single plane of image
     * such as Y or UV plane of NV12 image to it.
     */
    QCStatus_e RegPlane( void *pData, cl_mem *pBufferCL, cl_image_format *pFormat,
                         cl_image_desc *pDesc );

    /**
     * @brief Deregister the OpenclIface buffer
     * @param[in] pBuffer the QC buffer pointer to deregister
     * @return QC_STATUS_OK on success, others on failure
     * @note Release the OpenCL memory buffer corresponding to the host QC buffer and
     * erase it in the buffer map.
     */
    QCStatus_e DeregBuf( const QCBuffer_t *pBuffer );

    /**
     * @brief Deregister the OpenclIface image
     * @param[in] pData the image data pointer to deregister
     * @return QC_STATUS_OK on success, others on failure
     * @note Release the OpenCL memory buffer of image.
     */
    QCStatus_e DeregImage( void *pData );

    /**
     * @brief Deregister the OpenclIface plane
     * @param[in] pData the image data pointer to deregister
     * @param[in] pFormat the cl image format pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Release the OpenCL memory buffer of single plane.
     */
    QCStatus_e DeregPlane( void *pData, cl_image_format *pFormat );

    /**
     * @brief Execute the OpenclIface kernel
     * @param[in] pKernel the OpenCL kernel pointer
     * @param[in] pArgs the arguments structure pointer for certain kernel
     * @param[in] numOfArgs the arguments number
     * @param[in] pWorkParam the work parameters structure pointer
     * @return QC_STATUS_OK on success, others on failure
     * @note Set arguments for an OpenCL kernel and execute the kernel in GPU device. The number of
     * arguments must correspond to kernel codes.
     */
    QCStatus_e Execute( const cl_kernel *pKernel, const OpenclIfcae_Arg_t *pArgs, size_t numOfArgs,
                        const OpenclIface_WorkParams_t *pWorkParam );


private:
    cl_platform_id m_platformID;                         /**OpenCL platform ID*/
    cl_device_id m_deviceID;                             /**OpenCL device ID*/
    cl_command_queue m_commandQueue;                     /**OpenCL command queue*/
    cl_context m_context;                                /**OpenCL context*/
    cl_program m_program;                                /**OpenCL program*/
    std::map<void *, OpenclIface_MemInfo_t> m_bufferMap; /**OpenCL buffer memory map*/
    std::map<void *, OpenclIface_MemInfo_t> m_imageMap;  /**OpenCL image memory map*/
    std::map<std::pair<void *, uint32_t>, OpenclIface_MemInfo_t>
            m_planeMap;                           /**OpenCL plane memory map*/
    std::map<std::string, cl_kernel> m_kernelMap; /**OpenCL kernel map*/

public:
    cl_sampler m_sampler; /**OpenCL sampler*/

protected:
    QC_DECLARE_LOGGER();
};

}   // namespace OpenclIface
}   // namespace libs
}   // namespace QC

#endif   // QC_OPENCL_IFACE_HPP
