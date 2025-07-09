// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_TINYVIZ_TEXT_RENDERER_HPP
#define QC_SAMPLE_TINYVIZ_TEXT_RENDERER_HPP

#include <string>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "Macros.hpp"

namespace QC
{
namespace sample
{

class TextInfo
{
public:
    TextInfo();
    TextInfo( std::string const &_text, SDL_Color _color );
    TextInfo( TextInfo && ) = default;

    bool init( SDL_Renderer *ren, TTF_Font *font );
    bool close();

    size_t getWidth() { return m_Width; }
    size_t getHeight() { return m_Height; }
    SDL_Texture *getTexture() { return m_Texture; }

private:
    SDL_Surface *m_Surface = nullptr;
    SDL_Texture *m_Texture = nullptr;
    int m_Width = 0;
    int m_Height = 0;
    SDL_Color m_Color = COLOR_WHITE;
    std::string m_Text;
};

class NumTextCollect
{
public:
    NumTextCollect() = default;
    bool init( uint32_t range, SDL_Renderer &ren, TTF_Font &font );
    bool release();
    TextInfo *getTextInfo( uint32_t idx );

private:
    std::vector<TextInfo> m_NumTextInfos;
};

}   // namespace sample
}   // namespace QC

#endif   // #ifndef QC_SAMPLE_TINYVIZ_TEXT_RENDERER_HPP
