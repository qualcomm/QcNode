// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause


#include "QC/Infras/Memory/CameraFrameDescriptor.hpp"
#include "QC/Infras/Log/Logger.hpp"

namespace QC
{
namespace Memory
{

CameraFrameDescriptor &CameraFrameDescriptor::operator=( const QCBufferDescriptorBase &other )
{
    if ( this != &other )
    {
        this->name = other.name;
        this->pBuf = other.pBuf;
        this->size = other.size;
        this->type = QC_BUFFER_TYPE_IMAGE;
        this->dmaHandle = other.dmaHandle;
        this->cache = other.cache;
        this->pid = other.pid;
        this->allocatorType = other.allocatorType;
        const ImageDescriptor *pImgFrame = dynamic_cast<const ImageDescriptor *>( &other );
        if ( pImgFrame != nullptr )
        {
            this->id = pImgFrame->id;
            this->validSize = pImgFrame->validSize;
            this->offset = pImgFrame->offset;
            this->format = pImgFrame->format;
            this->batchSize = pImgFrame->batchSize;
            this->width = pImgFrame->width;
            this->height = pImgFrame->height;
            this->numPlanes = pImgFrame->numPlanes;
            for ( uint32_t i = 0; i < this->numPlanes; i++ )
            {
                this->stride[i] = pImgFrame->stride[i];
                this->actualHeight[i] = pImgFrame->actualHeight[i];
                this->planeBufSize[i] = pImgFrame->planeBufSize[i];
            }
        }
    }
    return *this;
}
}   // namespace Memory
}   // namespace QC

