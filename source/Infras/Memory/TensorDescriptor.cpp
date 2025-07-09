// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

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
        this->pBufBase = other.pBufBase;
        this->dmaHandle = other.dmaHandle;
        this->dmaSize = other.dmaSize;
        this->offset = other.offset;
        this->id = other.id;
        this->pid = other.pid;
        this->usage = other.usage;
        this->attr = other.attr;

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
    this->pBuf = other.data();
    this->size = other.size;
    this->type = QC_BUFFER_TYPE_TENSOR;
    this->pBufBase = other.buffer.pData;
    this->dmaHandle = other.buffer.dmaHandle;
    this->dmaSize = other.buffer.size;
    this->offset = other.offset;
    this->id = other.buffer.id;
    this->pid = other.buffer.pid;
    this->usage = other.buffer.usage;
    this->attr = QC_CACHEABLE;
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
