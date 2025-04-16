// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include <SDL2/SDL_opengl.h>
#include <algorithm>   // for_each

#include "HelpWindow.hpp"

namespace QC
{
namespace sample
{

bool HelpWindowInfo::init( SDL_Renderer *ren, TTF_Font *font )
{
    std::vector<std::string> helpTextSet = { "[F1] HELP menu",
                                             "[ESC] Quit TinyViz",
                                             "[SPACE] Pause / Resume",
                                             "[TAB] Go to next view",
                                             "[M] Toggle Multi-view",
                                             "[I] Toggle FPS info",
                                             "==============================",
                                             "Object color:" };
    for ( auto &text : helpTextSet )
    {
        TextInfo textInfo( text, COLOR_WHITE );
        textInfo.init( ren, font );
        textInfoList.emplace_back( std::move( textInfo ) );
    }

    return true;
}

bool HelpWindowInfo::render( SDL_Renderer *ren )
{
    constexpr double scale = 0.25;
    constexpr int textGap = 6;
    SDL_Rect targetWindow = window;
    for_each( textInfoList.begin(), textInfoList.end(), [&]( TextInfo &textInfo ) {
        if ( textInfo.getWidth() * scale + 40 > targetWindow.w )
            targetWindow.w = textInfo.getWidth() * scale + 40;
        targetWindow.h += textInfo.getHeight() * scale + textGap;
    } );

    // draw border
    // glLineWidth( 3 );
    SDL_SetRenderDrawColor( ren, borderColor.r, borderColor.g, borderColor.b, borderColor.a );
    SDL_RenderDrawRect( ren, &targetWindow );
    // glLineWidth( 1 );

    // draw background color
    SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_BLEND );
    SDL_SetRenderDrawColor( ren, bgColor.r, bgColor.g, bgColor.b, bgColor.a );
    SDL_RenderFillRect( ren, &targetWindow );
    SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_NONE );

    size_t idx = 0;
    int nextX = 20;
    int nextY = 20;
    for ( auto &textInfo : textInfoList )
    {
        SDL_Rect textR;
        textR.w = textInfo.getWidth() * scale;
        textR.h = textInfo.getHeight() * scale;
        textR.x = window.x + nextX;
        textR.y = window.y + nextY;
        SDL_RenderCopy( ren, textInfo.getTexture(), NULL, &textR );
        nextY += textR.h + textGap;
        idx++;
    }

    return true;
}

bool HelpWindowInfo::renderF1( SDL_Renderer *ren )
{
    TextInfo &textInfo = textInfoList[0];
    constexpr double scale = 0.22;
    SDL_Rect targetWindow = window;
    targetWindow.w = textInfo.getWidth() * scale + 10;
    targetWindow.h = textInfo.getHeight() * scale + 10;

    // draw border
    // glLineWidth( 3 );
    SDL_SetRenderDrawColor( ren, borderColor.r, borderColor.g, borderColor.b, borderColor.a );
    SDL_RenderDrawRect( ren, &targetWindow );
    // glLineWidth( 1 );

    // draw background color
    SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_BLEND );
    SDL_SetRenderDrawColor( ren, bgColor.r, bgColor.g, bgColor.b, bgColor.a );
    SDL_RenderFillRect( ren, &targetWindow );
    SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_NONE );

    SDL_Rect textR;
    textR.w = textInfo.getWidth() * scale;
    textR.h = textInfo.getHeight() * scale;
    textR.x = targetWindow.x + 5;
    textR.y = targetWindow.y + 5;
    SDL_RenderCopy( ren, textInfo.getTexture(), NULL, &textR );

    return true;
}

bool HelpWindowInfo::close()
{
    for ( auto &textInfo : textInfoList )
    {
        textInfo.close();
    }

    return true;
}

}   // namespace sample
}   // namespace QC
