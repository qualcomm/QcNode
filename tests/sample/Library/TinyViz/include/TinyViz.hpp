// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause-Clear


#ifndef QC_SAMPLE_TINYVIZ_HPP
#define QC_SAMPLE_TINYVIZ_HPP
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "CamInfo.hpp"
#include "HelpWindow.hpp"
#include "TinyVizIF.hpp"

#include "QC/Infras/Log/Logger.hpp"

using namespace QC;

namespace QC
{
namespace sample
{

class TinyViz : public TinyVizIF
{
public:
    TinyViz() = default;
    TinyViz( const TinyViz & ) = delete;
    TinyViz &operator=( const TinyViz & ) = delete;
    ~TinyViz() override = default;

    bool init( uint32_t winW = 1920, uint32_t winH = 1024 ) override;
    bool start() override;
    bool stop() override;

    bool addCamera( const std::string camName ) override;
    bool addData( const std::string camName, DataFrame_t & ) override;
    bool addData( const std::string camName, Road2DObjects_t & ) override;

private:
    uint32_t GetSDLFormat( QCImageFormat_e format );
    bool isCamSelected( size_t id );
    void rendererThread();
    void setAllCamActive();
    void printRendererInfo( SDL_Renderer *ren );
    bool renderBlack( SDL_Renderer *ren, SDL_Texture *tex2M, size_t idx );
    bool renderCam( CamInfo &camInfo, SDL_Renderer *ren, size_t idx = 0 );

    void renderFPS( SDL_Renderer *ren, const SDL_Rect &DestR, uint32_t xShift, float &fps );
    void updateFPS( const std::string &camName, std::deque<uint64_t> &queue, uint64_t timestamp );
    void updateFPS( CamInfo &camInfo );

    void renderBB( const uint64_t targetPTS, const uint64_t historyWindow,
                   std::map<uint64_t, std::list<Road2DObjects_t>> &queue, SDL_Renderer *ren,
                   const SDL_Rect &DestR, const float scaleX, const float scaleY,
                   const SDL_Color &color );
    void drawLines( SDL_Renderer *ren, std::vector<SDL_Point> &points );

    std::map<std::string, CamInfo> m_CamInfoMap;
    std::vector<std::string> m_CamNameList;
    float m_LastRenderFPS = 20;

    NumTextCollect m_NumTextures;

    SDL_Window *m_Win = nullptr;

    uint32_t m_WindowW = 1920, m_WindowH = 1080;
    int m_WindowCol = 1;
    int m_WindowRow = 1;

    size_t m_ActiveCamIDX = 0;
    size_t m_ActiveMultiViewIDX = 0;

    bool m_Stop = false;
    bool m_ShowHelp = false;
    bool m_PauseRenderer = false;
    bool m_EnableCamFPSCap = true;

    std::unique_ptr<std::thread> m_RendererThread;

    QC_DECLARE_LOGGER();
};

}   // namespace sample
}   // namespace QC

#endif   // #ifndef QC_SAMPLE_TINYVIZ_HPP
