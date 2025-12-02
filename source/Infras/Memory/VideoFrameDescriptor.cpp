// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear

#include "QC/Infras/Memory/VideoFrameDescriptor.hpp"
#include "QC/Infras/Log/Logger.hpp"

namespace QC::Memory
{
VideoFrameDescriptor &VideoFrameDescriptor::operator=( const QCBufferDescriptorBase &other )
{
    if ( this != &other )
    {
        const ImageDescriptor_t *pImg = static_cast<const ImageDescriptor_t*>(&other);
        if ( nullptr != pImg )
        {
            ImageDescriptor::operator=(*pImg);
        }
        const VideoFrameDescriptor *otherVD = dynamic_cast<const VideoFrameDescriptor *>( &other );
        if (otherVD != nullptr)
        {
            timestampNs = otherVD->timestampNs;
            appMarkData = otherVD->appMarkData;
            frameType = otherVD->frameType;
            frameFlag = otherVD->frameFlag;
        }
    }
    return *this;
}
VideoFrameDescriptor &VideoFrameDescriptor::operator=( const QCSharedBuffer_t &other )
{
    ImageDescriptor::operator=(static_cast<const QCSharedBuffer_t&>(other));
    return *this;
}
}   // namespace QC::Memory
