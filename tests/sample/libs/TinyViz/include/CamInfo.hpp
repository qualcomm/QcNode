// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#ifndef RIDEHAL_SAMPLE_TINYVIZ_CAM_INFO_HPP
#define RIDEHAL_SAMPLE_TINYVIZ_CAM_INFO_HPP

#include "TextRenderer.hpp"
#include "ridehal/common/SharedBuffer.hpp"
#include "ridehal/sample/DataTypes.hpp"
#include <deque>
#include <list>
#include <map>
#include <mutex>
#include <vector>

using namespace ridehal::sample;

namespace ridehal
{
namespace sample
{

class CamInfo
{
public:
    CamInfo();   // dummy constructor is needed for map initialization
    CamInfo( std::string _camName );
    CamInfo( const CamInfo &copy ) = default;
    CamInfo( CamInfo &&copy ) = default;
    CamInfo &operator=( const CamInfo &copy ) = default;
    CamInfo &operator=( CamInfo &&copy ) = default;

    bool initText( SDL_Renderer *ren, TTF_Font *font );
    bool closeText();
    void clearQueue();

    void setActive() { inactiveCount = 0; }
    bool isActive() { return inactiveCount < 30; }
    void markRendered() { inactiveCount++; }

    uint8_t *data( uint32_t batch = 0 );
    size_t size();
    uint32_t batch();
    uint32_t width();
    uint32_t height();
    uint32_t stride();
    RideHal_ImageFormat_e format();

    std::string camName;
    SDL_Color color;

    // text
    TextInfo camNameText;
    TextInfo statusBarText;

    uint64_t lastFPSUpdate = 0;
    std::deque<uint64_t> camPTSs;
    float lastCamFPS = 0;
    std::deque<uint64_t> lanPTSs;
    float lastLanFPS = 0;
    std::deque<uint64_t> rosPTSs;
    float lastRodFPS = 0;
    std::deque<uint64_t> tfsPTSs;
    float lastTfsFPS = 0;

    // For protecting display buffer & bbBoxes
    // wrap with unique_ptr is needed for map initialization (mutex is not move-able)
    std::unique_ptr<std::mutex> mutex;
    DataFrame_t camFrame;

    SDL_Texture *tex = nullptr;

    std::map<uint64_t, std::list<Road2DObjects_t>> roadObjectQueue;

private:
    size_t inactiveCount = 0;
};

}   // namespace sample
}   // namespace ridehal

#endif   // #ifndef RIDEHAL_SAMPLE_TINYVIZ_CAM_INFO_HPP
