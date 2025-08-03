// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.

#include "QC/Infras/Memory/BufferDescriptor.hpp"

namespace QC
{
namespace Memory
{

BufferDescriptor &BufferDescriptor::operator=( BufferDescriptor &other )
{
    if ( this != &other )
    {
        this->name = other.name;
        this->pBuf = other.pBuf;
        this->size = other.size;
        this->type = other.type;
        this->pBufBase = other.pBufBase;
        this->dmaHandle = other.dmaHandle;
        this->dmaSize = other.dmaSize;
        this->offset = other.offset;
        this->id = other.id;
        this->pid = other.pid;
        this->usage = other.usage;
        this->cache = other.cache;
    }
    return *this;
}

}   // namespace Memory

}   // namespace QC
