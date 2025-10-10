// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#include "QC/Infras/Memory/TensorDescriptor.hpp"
#include "QC/Infras/Log/Logger.hpp"

namespace QC
{
namespace Memory
{

TensorDescriptor &TensorDescriptor::operator=( const BufferDescriptor &other )
{
    if ( this != &other )
    {
        this->name = other.name;
        this->pBuf = other.pBuf;
        this->size = other.size;
        this->type = QC_BUFFER_TYPE_TENSOR;
        this->dmaHandle = other.dmaHandle;
        this->validSize = other.validSize;
        this->offset = other.offset;
        this->id = other.id;
        this->pid = other.pid;
        this->allocatorType = other.allocatorType;
        this->cache = other.cache;

        const TensorDescriptor_t *pTensor = dynamic_cast<const TensorDescriptor_t *>( &other );
        if ( nullptr != pTensor )
        {
            this->tensorType = pTensor->tensorType;
            uint32_t numDims = std::min( pTensor->numDims, (uint32_t) QC_NUM_TENSOR_DIMS );
            std::copy( pTensor->dims, pTensor->dims + numDims, this->dims );
            this->numDims = pTensor->numDims;
        }
    }
    return *this;
}

TensorDescriptor &TensorDescriptor::operator=( const QCSharedBuffer_t &other )
{
    static const QCMemoryAllocator_e s_Usage2Allocator[] = {
            QC_MEMORY_ALLOCATOR_DMA,        /* QC_BUFFER_USAGE_DEFAULT */
            QC_MEMORY_ALLOCATOR_DMA_CAMERA, /* QC_BUFFER_USAGE_CAMERA */
            QC_MEMORY_ALLOCATOR_DMA_GPU,    /* QC_BUFFER_USAGE_GPU */
            QC_MEMORY_ALLOCATOR_DMA_VPU,    /* QC_BUFFER_USAGE_VPU */
            QC_MEMORY_ALLOCATOR_DMA_EVA,    /* QC_BUFFER_USAGE_EVA */
            QC_MEMORY_ALLOCATOR_DMA_HTP,    /* QC_BUFFER_USAGE_HTP */
    };
    this->pBuf = other.buffer.pData;
    this->validSize = other.size;
    this->type = QC_BUFFER_TYPE_TENSOR;
    this->dmaHandle = other.buffer.dmaHandle;
    this->size = other.buffer.size;
    this->offset = other.offset;
    this->id = other.buffer.id;
    this->pid = other.buffer.pid;
    this->allocatorType = s_Usage2Allocator[other.buffer.usage];
    this->cache = QC_CACHEABLE;
    this->tensorType = other.tensorProps.type;
    std::copy( other.tensorProps.dims, other.tensorProps.dims + other.tensorProps.numDims,
               this->dims );
    this->numDims = other.tensorProps.numDims;

    QC_LOG_DEBUG( "Tensor %s = %u [%u %u %u %u]", this->name.c_str(), this->numDims, this->dims[0],
                  this->dims[1], this->dims[2], this->dims[3] );
    return *this;
}

}   // namespace Memory

}   // namespace QC
