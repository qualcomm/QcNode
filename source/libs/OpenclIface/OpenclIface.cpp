// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include "OpenclIface.hpp"

namespace ridehal
{
namespace libs
{
namespace OpenclIface
{

RideHalError_e OpenclSrv::Init( const char *pName, Logger_Level_e level,
                                OpenclIfcae_Perf_e priority )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    ret = RIDEHAL_LOGGER_INIT( pName, level );
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to create logger for OpenclSrv %s: ret = %d\n",
                        pName, ret );
        ret = RIDEHAL_ERROR_NONE; /* ignore logger init error */
    }

    retCL = clGetPlatformIDs( 1, &m_platformID, NULL );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to get Platforms, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    if ( CL_SUCCESS == retCL )
    {
        retCL = clGetDeviceIDs( m_platformID, CL_DEVICE_TYPE_GPU, 1, &m_deviceID, NULL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to get OpenCL compatible GPU device, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        cl_context_properties properties[] = { CL_CONTEXT_PRIORITY_HINT_QCOM,
                                               CL_PRIORITY_HINT_NORMAL_QCOM, 0 };
        if ( OPENCLIFACE_PERF_HIGH == priority )
        {
            properties[1] = CL_PRIORITY_HINT_HIGH_QCOM;
        }
        else if ( OPENCLIFACE_PERF_NORMAL == priority )
        {
            properties[1] = CL_PRIORITY_HINT_NORMAL_QCOM;
        }
        else if ( OPENCLIFACE_PERF_LOW == priority )
        {
            properties[1] = CL_PRIORITY_HINT_LOW_QCOM;
        }
        else
        {
            RIDEHAL_ERROR( "Invalid performance priority argument setting" );
            ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
        }
        m_context = clCreateContext( properties, 1, &m_deviceID, NULL, NULL, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create context, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        m_commandQueue = clCreateCommandQueueWithProperties( m_context, m_deviceID, NULL, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create command queue, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        size_t versionSize = 0;
        (void) clGetDeviceInfo( m_deviceID, CL_DEVICE_VERSION, 0, NULL, &versionSize );
        std::vector<char> version( versionSize );
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_VERSION, versionSize, version.data(), NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL version is %s", version.data() );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device version info, retCL = %d", retCL );
        }

        size_t extensionSize = 0;
        (void) clGetDeviceInfo( m_deviceID, CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize );
        std::vector<char> extensions( extensionSize );
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_EXTENSIONS, extensionSize, extensions.data(),
                                 NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL extension is %s", extensions.data() );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device extensions info, retCL = %d", retCL );
        }

        cl_uint unit;
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof( cl_uint ), &unit,
                                 NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL max compute unit is %d\n", unit );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device max compute units info, retCL = %d", retCL );
        }

        size_t workSizes[3];
        retCL = clGetDeviceInfo( m_deviceID, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof( size_t ) * 3,
                                 workSizes, NULL );
        if ( CL_SUCCESS == retCL )
        {
            RIDEHAL_INFO( "CL max work item sizes is {%d, %d, %d}", workSizes[0], workSizes[1],
                          workSizes[2] );
        }
        else
        {
            RIDEHAL_ERROR( "Unable to get device max work item sizes info, retCL = %d", retCL );
        }
    }

    if ( CL_SUCCESS == retCL )
    {
        const cl_sampler_properties samplerProperties[] = { CL_SAMPLER_NORMALIZED_COORDS,
                                                            CL_FALSE,
                                                            CL_SAMPLER_ADDRESSING_MODE,
                                                            CL_ADDRESS_CLAMP_TO_EDGE,
                                                            CL_SAMPLER_FILTER_MODE,
                                                            CL_FILTER_NEAREST,
                                                            0 };
        m_sampler = clCreateSamplerWithProperties( m_context, samplerProperties, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create sampler, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::LoadFromSource( const char *pSourceFile )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    m_program =
            clCreateProgramWithSource( m_context, 1, (const char **) &pSourceFile, NULL, &retCL );
    if ( retCL != CL_SUCCESS )
    {
        RIDEHAL_ERROR( "Unable to create program with source, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }
    else
    {
        retCL = clBuildProgram( m_program, 1, &m_deviceID, "-cl-fast-relaxed-math", NULL, NULL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to build program, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
            size_t len;
            (void) clGetProgramBuildInfo( m_program, m_deviceID, CL_PROGRAM_BUILD_LOG, 0, NULL,
                                          &len );
            std::vector<char> logs;
            logs.resize( len );
            char *pBuffer = logs.data();
            if ( nullptr != pBuffer )
            {
                (void) clGetProgramBuildInfo( m_program, m_deviceID, CL_PROGRAM_BUILD_LOG, len,
                                              pBuffer, NULL );
                RIDEHAL_ERROR( "error build log:\n %s\n", pBuffer );
            }
            else
            {
                RIDEHAL_ERROR( "Unable to get build log!" );
            }
            logs.clear();
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::LoadFromBinary( const unsigned char *pBinaryFile )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    m_program = clCreateProgramWithBinary( m_context, 1, &m_deviceID, NULL, &pBinaryFile, NULL,
                                           &retCL );
    if ( retCL != CL_SUCCESS )
    {
        RIDEHAL_ERROR( "Unable to create program with binary, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    return ret;
}

RideHalError_e OpenclSrv::CreateKernel( cl_kernel *pKernel, const char *pKernelName )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    std::string kernelString = pKernelName;
    auto it = m_kernelMap.find( kernelString );
    if ( it == m_kernelMap.end() )
    {
        *pKernel = clCreateKernel( m_program, pKernelName, &retCL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to create kernel, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            m_kernelMap[kernelString] = (cl_kernel) *pKernel;
        }
    }
    else
    {
        *pKernel = it->second;
    }

    return ret;
}

RideHalError_e OpenclSrv::Deinit()
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    for ( auto &it : m_kernelMap )
    {
        retCL = clReleaseKernel( it.second );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to release kernel, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }
    m_kernelMap.clear();

    retCL = clReleaseProgram( m_program );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release program, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    retCL = clReleaseCommandQueue( m_commandQueue );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release command queue, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    retCL = clReleaseContext( m_context );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release context, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }

    for ( auto &it : m_bufferMap )
    {
        retCL = clReleaseMemObject( it.second.clMem );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to deregister buffer %d", it.first );
        }
    }
    m_bufferMap.clear();

    for ( auto &it : m_imageMap )
    {
        retCL = clReleaseMemObject( it.second.clMem );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to deregister image %d", it.first );
        }
    }
    m_imageMap.clear();

    for ( auto &it : m_planeMap )
    {
        retCL = clReleaseMemObject( it.second.clMem );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to deregister plane %d", it.first );
        }
    }
    m_planeMap.clear();

    retCL = clReleaseSampler( m_sampler );
    if ( CL_SUCCESS != retCL )
    {
        RIDEHAL_ERROR( "Unable to release sampler, retCL = %d", retCL );
        ret = RIDEHAL_ERROR_FAIL;
    }


    ret = RIDEHAL_LOGGER_DEINIT();
    if ( RIDEHAL_ERROR_NONE != ret )
    {
        (void) fprintf( stderr, "WARINING: failed to deinit logger for OpenclSrv: ret = %d\n",
                        ret );
        ret = RIDEHAL_ERROR_NONE; /* ignore logger deinit error */
    }

    return ret;
}

RideHalError_e OpenclSrv::RegBuf( const RideHal_Buffer_t *pBuffer, cl_mem *pBufferCL )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    if ( nullptr == pBuffer )
    {
        RIDEHAL_ERROR( "null host buffer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        auto it = m_bufferMap.find( pBuffer->pData );
        if ( it == m_bufferMap.end() )
        {
#if defined( __QNXNTO__ )
            cl_mem_pmem_host_ptr clBufHostPtr = { 0 };
            clBufHostPtr.pmem_handle = (uintptr_t) pBuffer->dmaHandle;
            clBufHostPtr.ext_host_ptr.allocation_type = CL_MEM_PMEM_HOST_PTR_QCOM;
            clBufHostPtr.ext_host_ptr.host_cache_policy = CL_MEM_HOST_IOCOHERENT_QCOM;
            clBufHostPtr.pmem_hostptr = pBuffer->pData;
            cl_mem bufferCL =
                    clCreateBuffer( m_context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM,
                                    pBuffer->size, &clBufHostPtr, &retCL );
#else
            cl_mem_dmabuf_host_ptr clBufHostPtr = { 0 };
            clBufHostPtr.dmabuf_filedesc = (int) pBuffer->dmaHandle;
            clBufHostPtr.ext_host_ptr.allocation_type = CL_MEM_DMABUF_HOST_PTR_QCOM;
            clBufHostPtr.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
            clBufHostPtr.dmabuf_hostptr = pBuffer->pData;
            cl_mem bufferCL =
                    clCreateBuffer( m_context, CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM,
                                    pBuffer->size, &clBufHostPtr, &retCL );
#endif

            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to create CL buffer, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                m_bufferMap[pBuffer->pData] = { bufferCL };
                *pBufferCL = bufferCL;
            }
        }
        else
        {
            *pBufferCL = it->second.clMem;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::RegImage( void *pData, uint64_t dmaHandle, cl_mem *pBufferCL,
                                    cl_image_format *pFormat, cl_image_desc *pDesc )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    if ( nullptr == pData )
    {
        RIDEHAL_ERROR( "null data pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        auto it = m_imageMap.find( pData );
        if ( it == m_imageMap.end() )
        {
#if defined( __QNXNTO__ )
            cl_mem_pmem_host_ptr clBufHostPtr = { { 0 } };
            clBufHostPtr.pmem_handle = (uintptr_t) 0;
            clBufHostPtr.ext_host_ptr.allocation_type = CL_MEM_PMEM_HOST_PTR_QCOM;
            clBufHostPtr.ext_host_ptr.host_cache_policy = CL_MEM_HOST_IOCOHERENT_QCOM;
            clBufHostPtr.pmem_hostptr = pData;
            cl_mem bufferCL = clCreateImage(
                    m_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM,
                    pFormat, pDesc, &clBufHostPtr, &retCL );
#else
            cl_mem_dmabuf_host_ptr clBufHostPtr = { { 0 } };
            clBufHostPtr.dmabuf_filedesc = (int) dmaHandle;
            clBufHostPtr.ext_host_ptr.allocation_type = CL_MEM_DMABUF_HOST_PTR_QCOM;
            clBufHostPtr.ext_host_ptr.host_cache_policy = CL_MEM_HOST_IOCOHERENT_QCOM;
            clBufHostPtr.dmabuf_hostptr = pData;
            cl_mem bufferCL = clCreateImage(
                    m_context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM,
                    pFormat, pDesc, &clBufHostPtr, &retCL );
#endif

            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to create CL image, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                m_imageMap[pData] = { bufferCL };
                *pBufferCL = bufferCL;
            }
        }
        else
        {
            *pBufferCL = it->second.clMem;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::RegPlane( void *pData, cl_mem *pBufferCL, cl_image_format *pFormat,
                                    cl_image_desc *pDesc )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    std::pair<void *, uint32_t> key;
    key.first = pData;
    if ( nullptr == key.first )
    {
        RIDEHAL_ERROR( "null image CL data pointer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        key.second = pFormat->image_channel_order;
        auto it = m_planeMap.find( key );
        if ( it == m_planeMap.end() )
        {
            cl_mem bufferCL =
                    clCreateImage( m_context, CL_MEM_READ_WRITE, pFormat, pDesc, nullptr, &retCL );

            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to create CL image, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                m_planeMap[key] = { bufferCL };
                *pBufferCL = bufferCL;
            }
        }
        else
        {
            *pBufferCL = it->second.clMem;
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::DeregBuf( const RideHal_Buffer_t *pBuffer )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    if ( nullptr == pBuffer )
    {
        RIDEHAL_ERROR( "null host buffer!" );
        ret = RIDEHAL_ERROR_BAD_ARGUMENTS;
    }
    else
    {
        auto it = m_bufferMap.find( pBuffer->pData );
        if ( it != m_bufferMap.end() )
        {
            retCL = clReleaseMemObject( it->second.clMem );
            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to release CL buffer, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
            else
            {
                (void) m_bufferMap.erase( it );
            }
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::DeregImage( void *pData )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    auto it = m_imageMap.find( pData );
    if ( it != m_imageMap.end() )
    {
        retCL = clReleaseMemObject( it->second.clMem );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to release CL image, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            (void) m_imageMap.erase( it );
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::DeregPlane( void *pData, cl_image_format *pFormat )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    std::pair<void *, uint32_t> key;
    key.first = pData;
    key.second = pFormat->image_channel_order;
    auto it = m_planeMap.find( key );
    if ( it != m_planeMap.end() )
    {
        retCL = clReleaseMemObject( it->second.clMem );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to release CL plane, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            (void) m_planeMap.erase( it );
        }
    }

    return ret;
}

RideHalError_e OpenclSrv::Execute( const cl_kernel *pKernel, const OpenclIfcae_Arg_t *pArgs,
                                   size_t numOfArgs, const OpenclIface_WorkParams_t *pWorkParam )
{
    RideHalError_e ret = RIDEHAL_ERROR_NONE;
    cl_int retCL = CL_SUCCESS;

    for ( int i = 0; i < numOfArgs; i++ )
    {
        retCL = clSetKernelArg( *pKernel, i, pArgs[i].argSize, pArgs[i].pArg );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to set number %d argument, retCL = %d", i, retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
    }

    if ( RIDEHAL_ERROR_NONE == ret )
    {
        retCL = clEnqueueNDRangeKernel( m_commandQueue, *pKernel, pWorkParam->workDim,
                                        pWorkParam->pGlobalWorkOffset, pWorkParam->pGlobalWorkSize,
                                        pWorkParam->pLocalWorkSize, 0, NULL, NULL );
        if ( CL_SUCCESS != retCL )
        {
            RIDEHAL_ERROR( "Unable to enqueue range kernel, retCL = %d", retCL );
            ret = RIDEHAL_ERROR_FAIL;
        }
        else
        {
            retCL = clFinish( m_commandQueue );
            if ( CL_SUCCESS != retCL )
            {
                RIDEHAL_ERROR( "Unable to finish command queue, retCL = %d", retCL );
                ret = RIDEHAL_ERROR_FAIL;
            }
        }
    }

    return ret;
}

}   // namespace OpenclIface
}   // namespace libs
}   // namespace ridehal
