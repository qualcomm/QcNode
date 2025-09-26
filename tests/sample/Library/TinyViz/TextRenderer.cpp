// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause



#include "TextRenderer.hpp"

namespace QC
{
namespace sample
{

TextInfo::TextInfo() {}

TextInfo::TextInfo( std::string const &_text, SDL_Color _color )
    : m_Color( _color ),
      m_Text( _text )
{}

bool TextInfo::init( SDL_Renderer *ren, TTF_Font *font )
{

    if ( !ren || !font )
    {
        return false;
    }
    m_Surface = TTF_RenderText_Solid( font, m_Text.c_str(), m_Color );
    m_Texture = SDL_CreateTextureFromSurface( ren, m_Surface );
    SDL_QueryTexture( m_Texture, NULL, NULL, &m_Width, &m_Height );

    return true;
}

bool TextInfo::close()
{
    if ( m_Texture ) SDL_DestroyTexture( m_Texture );
    if ( m_Surface ) SDL_FreeSurface( m_Surface );

    return true;
}

bool NumTextCollect::init( uint32_t range, SDL_Renderer &ren, TTF_Font &font )
{
    for ( uint32_t idx = 0; idx < range; idx++ )
    {
        TextInfo info( std::to_string( idx / 10 ) + "." + std::to_string( idx % 10 ), COLOR_WHITE );
        info.init( &ren, &font );
        m_NumTextInfos.emplace_back( std::move( info ) );
    }

    // saftey guard for anything larger than [0..range]
    TextInfo info( "inv", COLOR_WHITE );
    info.init( &ren, &font );
    m_NumTextInfos.emplace_back( std::move( info ) );

    return true;
}

bool NumTextCollect::release()
{
    for ( auto &t : m_NumTextInfos ) t.close();

    return true;
}

TextInfo *NumTextCollect::getTextInfo( uint32_t idx )
{
    // the last entry is the over-boundary warning
    if ( idx >= m_NumTextInfos.size() - 1 )
    {
        return &m_NumTextInfos[m_NumTextInfos.size() - 1];
    }

    return &m_NumTextInfos[idx];
}

}   // namespace sample
}   // namespace QC
