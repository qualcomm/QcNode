// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_TINYVIZ_MACROS_HPP
#define RIDEHAL_SAMPLE_TINYVIZ_MACROS_HPP

#include <SDL2/SDL.h>

namespace ridehal
{
namespace sample
{

constexpr SDL_Color COLOR_BLACK = { 0, 0, 0, 0xFF };
constexpr SDL_Color COLOR_RED = { 0xFF, 0, 0, 0xFF };
constexpr SDL_Color COLOR_GREEN = { 0, 0xFF, 0, 0xFF };
constexpr SDL_Color COLOR_BLUE = { 0, 0, 0xFF, 0xFF };
constexpr SDL_Color COLOR_LIGHT_BLUE = { 0, 0xFF, 0xFF, 0xFF };
constexpr SDL_Color COLOR_DARK_GREEN = { 0, 0x88, 0, 0xFF };
constexpr SDL_Color COLOR_PURPLE = { 0xCC, 0x99, 0xFF, 0xFF };
constexpr SDL_Color COLOR_WHITE = { 0xFF, 0xFF, 0xFF, 0xFF };
constexpr SDL_Color COLOR_YELLOW = { 0xFF, 0xFF, 0, 0xFF };
constexpr SDL_Color COLOR_PINK = { 0xFF, 0, 0xFF, 0xFF };
constexpr SDL_Color COLOR_LIGHTGRAY = { 0xCC, 0xCC, 0xCC, 0xFF };

}   // namespace sample
}   // namespace ridehal

#endif   // #ifndef RIDEHAL_SAMPLE_TINYVIZ_MACROS_HPP
