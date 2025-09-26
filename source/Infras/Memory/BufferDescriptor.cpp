// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


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
        this->dmaHandle = other.dmaHandle;
        this->validSize = other.validSize;
        this->offset = other.offset;
        this->id = other.id;
        this->pid = other.pid;
        this->allocatorType = other.allocatorType;
        this->cache = other.cache;
    }
    return *this;
}

}   // namespace Memory

}   // namespace QC
