// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "QC/sample/shared_ring/SharedRing.hpp"
#if defined( __QNXNTO__ )
#include "QC/Infras/Memory/PMEMUtils.hpp"
#else
#include "QC/Infras/Memory/DMABUFFUtils.hpp"
#endif
#include "QC/sample/shared_ring/SpinLock.hpp"


#if defined( __QNXNTO__ )
using MemUtils = QC::Memory::PMEMUtils;
#else
using MemUtils = QC::Memory::DMABUFFUtils;
#endif

namespace QC
{
namespace sample
{
namespace shared_ring
{

#define SPINLOCK_TIMEOUT 100

QCStatus_e SharedRing_Ring::Push( uint16_t idx )
{
    QCStatus_e ret = QC_STATUS_OK;

    ret = QCSpinLock( &spinlock, SPINLOCK_TIMEOUT );
    if ( QC_STATUS_OK == ret )
    {
        ring[writeIdx % SHARED_RING_NUM_DESC] = idx;
        writeIdx++;
        ret = QCSpinUnLock( &spinlock );
    }

    if ( QC_STATUS_OK != ret )
    {
        QC_LOG_ERROR( "ring push %d to %s failed", idx, name );
    }

    return ret;
}

QCStatus_e SharedRing_Ring::Pop( uint16_t &idx )
{
    QCStatus_e ret = QC_STATUS_OK;
    QCStatus_e ret2;

    ret = QCSpinLock( &spinlock, SPINLOCK_TIMEOUT );
    if ( QC_STATUS_OK == ret )
    {
        if ( readIdx != writeIdx )
        {
            idx = ring[readIdx % SHARED_RING_NUM_DESC];
            readIdx++;
        }
        else
        {
            ret = QC_STATUS_OUT_OF_BOUND;
        }
        ret2 = QCSpinUnLock( &spinlock );
        if ( QC_STATUS_OK != ret2 )
        {
            QC_LOG_ERROR( "ring pop %d from %s unlock failed", idx, name );
            ret = QC_STATUS_FAIL;
        }
    }
    else
    {
        QC_LOG_ERROR( "ring pop from %s lock failed", name );
    }

    return ret;
}

uint32_t SharedRing_Ring::Size()
{
    return ( writeIdx - readIdx ) & 0xFFFF;
}

void SharedRing_DataFrame::SetName( std::string name )
{
    (void) snprintf( this->name, sizeof( this->name ), "%s", name.c_str() );
}

void SharedRing_Ring::SetName( std::string name )
{
    (void) snprintf( this->name, sizeof( this->name ), "%s", name.c_str() );
}

void SharedRing_Memory::SetName( std::string name )
{
    (void) snprintf( this->name, sizeof( this->name ), "%s", name.c_str() );
}


SharedRing_BufferDesc::SharedRing_BufferDesc( const QCBufferDescriptorBase_t &other )
{
    const TensorDescriptor_t *pTensor = dynamic_cast<const TensorDescriptor_t *>( &other );
    const ImageDescriptor_t *pImage = dynamic_cast<const ImageDescriptor_t *>( &other );

    (void) snprintf( this->name, sizeof( this->name ), "%s", other.name.c_str() );
    this->size = other.size;
    this->alignment = other.alignment;
    this->cache = other.cache;
    this->allocatorType = other.allocatorType;
    this->type = other.type;
    this->dmaHandle = other.dmaHandle;
    this->pid = other.pid;
    this->validSize = other.size;
    this->offset = 0;

    if ( nullptr != pTensor )
    {
        this->validSize = pTensor->validSize;
        this->offset = pTensor->offset;
        this->tensorType = pTensor->tensorType;
        std::copy( pTensor->dims, pTensor->dims + pTensor->numDims, this->dims );
        this->numDims = pTensor->numDims;
    }

    if ( nullptr != pImage )
    {
        this->validSize = pImage->validSize;
        this->offset = pImage->offset;
        this->format = pImage->format;
        this->width = pImage->width;
        this->height = pImage->height;
        this->batchSize = pImage->batchSize;
        std::copy( pImage->stride, pImage->stride + pImage->numPlanes, this->stride );
        std::copy( pImage->actualHeight, pImage->actualHeight + pImage->numPlanes,
                   this->actualHeight );
        std::copy( pImage->planeBufSize, pImage->planeBufSize + pImage->numPlanes,
                   this->planeBufSize );
        this->numPlanes = pImage->numPlanes;
    }
}

QCStatus_e SharedRing_BufferDesc::Import( SharedBuffer_t &sharedBuffer )
{
    QCStatus_e status = QC_STATUS_OK;
    MemUtils memUtils;
    QCBufferDescriptorBase_t org;
    QCBufferDescriptorBase_t mapped;

    org.dmaHandle = this->dmaHandle;
    org.size = this->size;
    org.alignment = this->alignment;
    org.cache = this->cache;
    org.allocatorType = this->allocatorType;
    org.type = this->type;
    org.pid = this->pid;

    status = memUtils.MemoryMap( org, mapped );
    if ( QC_STATUS_OK != status )
    {
        QC_LOG_ERROR( "Unable to map memory %s - %d", this->name, status );
    }
    else
    {
        if ( QC_BUFFER_TYPE_IMAGE == this->type )
        {
            sharedBuffer.imgDesc.name = this->name;
            sharedBuffer.imgDesc.pBuf = mapped.pBuf;
            sharedBuffer.imgDesc.dmaHandle = mapped.dmaHandle;
            sharedBuffer.imgDesc.size = this->size;
            sharedBuffer.imgDesc.alignment = this->alignment;
            sharedBuffer.imgDesc.cache = this->cache;
            sharedBuffer.imgDesc.allocatorType = this->allocatorType;
            sharedBuffer.imgDesc.type = this->type;
            sharedBuffer.imgDesc.pid = this->pid;
            sharedBuffer.imgDesc.validSize = this->validSize;
            sharedBuffer.imgDesc.offset = this->offset;
            sharedBuffer.imgDesc.format = this->format;
            sharedBuffer.imgDesc.width = this->width;
            sharedBuffer.imgDesc.height = this->height;
            sharedBuffer.imgDesc.batchSize = this->batchSize;
            std::copy( this->stride, this->stride + this->numPlanes, sharedBuffer.imgDesc.stride );
            std::copy( this->actualHeight, this->actualHeight + this->numPlanes,
                       sharedBuffer.imgDesc.actualHeight );
            std::copy( this->planeBufSize, this->planeBufSize + this->numPlanes,
                       sharedBuffer.imgDesc.planeBufSize );
            sharedBuffer.imgDesc.numPlanes = this->numPlanes;
            sharedBuffer.buffer = sharedBuffer.imgDesc;
            sharedBuffer.sharedBuffer = sharedBuffer.imgDesc;
        }
        else if ( QC_BUFFER_TYPE_TENSOR == this->type )
        {
            sharedBuffer.tensorDesc.name = this->name;
            sharedBuffer.tensorDesc.pBuf = mapped.pBuf;
            sharedBuffer.tensorDesc.dmaHandle = mapped.dmaHandle;
            sharedBuffer.tensorDesc.size = this->size;
            sharedBuffer.tensorDesc.alignment = this->alignment;
            sharedBuffer.tensorDesc.cache = this->cache;
            sharedBuffer.tensorDesc.allocatorType = this->allocatorType;
            sharedBuffer.tensorDesc.type = this->type;
            sharedBuffer.tensorDesc.pid = this->pid;
            sharedBuffer.tensorDesc.validSize = this->validSize;
            sharedBuffer.tensorDesc.offset = this->offset;
            sharedBuffer.tensorDesc.tensorType = this->tensorType;
            std::copy( this->dims, this->dims + this->numDims, sharedBuffer.tensorDesc.dims );
            sharedBuffer.tensorDesc.numDims = this->numDims;
            sharedBuffer.buffer = sharedBuffer.tensorDesc;
            sharedBuffer.sharedBuffer = sharedBuffer.tensorDesc;
        }
        else
        {
            QC_LOG_ERROR( "Not support buffer type %d for %s", this->type, this->name );
            status = QC_STATUS_UNSUPPORTED;
        }
    }

    return status;
}

}   // namespace shared_ring
}   // namespace sample
}   // namespace QC
