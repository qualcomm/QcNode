// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef QC_SAMPLE_TINYVIZ_HELP_WINDOW_HPP
#define QC_SAMPLE_TINYVIZ_HELP_WINDOW_HPP

#include <vector>

#include "TextRenderer.hpp"

namespace QC
{
namespace sample
{

class HelpWindowInfo
{
public:
    std::vector<TextInfo> textInfoList;
    SDL_Color color = COLOR_WHITE;
    SDL_Color bgColor = { 0x22, 0x22, 0x22, 0xCC };
    SDL_Color borderColor = COLOR_BLACK;

    SDL_Rect window = { 5, 5, 50, 50 };

    bool init( SDL_Renderer *ren, TTF_Font *font );
    bool render( SDL_Renderer *ren );
    bool renderF1( SDL_Renderer *ren );
    bool close();
};

}   // namespace sample
}   // namespace QC

#endif   // #ifndef QC_SAMPLE_TINYVIZ_HELP_WINDOW_HPP
