// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear



#include "CamInfo.hpp"
#include "Macros.hpp"

namespace QC
{
namespace sample
{

CamInfo::CamInfo() : mutex( new std::mutex )
{
    camFrame.timestamp = 0;
}

CamInfo::CamInfo( std::string _camName )
    : camName( _camName ),
      camNameText( camName, COLOR_WHITE ),
      statusBarText( "FPS:{ Cam:       /s, ROD:        /s, LAN:        /s, TFS:        /s }",
                     COLOR_LIGHTGRAY ),
      mutex( new std::mutex )
{
    camFrame.timestamp = 0;
}

bool CamInfo::initText( SDL_Renderer *ren, TTF_Font *font )
{
    camNameText.init( ren, font );
    statusBarText.init( ren, font );
    return true;
}

bool CamInfo::closeText()
{
    camNameText.close();
    statusBarText.close();
    return true;
}

uint8_t *CamInfo::data( uint32_t batch )
{
    uint8_t *pData = nullptr;
    if ( nullptr != camFrame.buffer )
    {
        QCBufferDescriptorBase_t &bufDesc = camFrame.GetBuffer();
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            uint32_t sizeOne = pImage->GetDataSize() / pImage->batchSize;
            pData = ( (uint8_t *) pImage->GetDataPtr() ) + sizeOne * batch;
        }
    }
    return pData;
}

size_t CamInfo::size()
{
    size_t sz = 0;
    if ( nullptr != camFrame.buffer )
    {
        QCBufferDescriptorBase_t &bufDesc = camFrame.GetBuffer();
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            sz = pImage->GetDataSize();
        }
    }
    return sz;
}

uint32_t CamInfo::batch()
{
    uint32_t batchSize = 1;
    if ( nullptr != camFrame.buffer )
    {
        QCBufferDescriptorBase_t &bufDesc = camFrame.GetBuffer();
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            batchSize = pImage->batchSize;
        }
    }
    return batchSize;
}

uint32_t CamInfo::width()
{
    uint32_t w = 0;

    if ( nullptr != camFrame.buffer )
    {
        QCBufferDescriptorBase_t &bufDesc = camFrame.GetBuffer();
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            w = pImage->width;
        }
    }

    return w;
}

uint32_t CamInfo::height()
{
    uint32_t h = 0;

    if ( nullptr != camFrame.buffer )
    {
        QCBufferDescriptorBase_t &bufDesc = camFrame.GetBuffer();
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            h = pImage->actualHeight[0];
        }
    }

    return h;
}


uint32_t CamInfo::stride()
{
    uint32_t st = 0;

    if ( nullptr != camFrame.buffer )
    {
        QCBufferDescriptorBase_t &bufDesc = camFrame.GetBuffer();
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            st = pImage->stride[0];
        }
    }

    return st;
}

QCImageFormat_e CamInfo::format()
{
    QCImageFormat_e fmt = QC_IMAGE_FORMAT_MAX;

    if ( nullptr != camFrame.buffer )
    {
        QCBufferDescriptorBase_t &bufDesc = camFrame.GetBuffer();
        ImageDescriptor_t *pImage = dynamic_cast<ImageDescriptor_t *>( &bufDesc );
        if ( nullptr != pImage )
        {
            fmt = pImage->format;
        }
    }

    return fmt;
}


}   // namespace sample
}   // namespace QC
