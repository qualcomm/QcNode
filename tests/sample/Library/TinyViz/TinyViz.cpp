// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// All rights reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.


#include <cmath>
#include <iostream>
#include <unistd.h>

#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_thread.h>

#include "CamInfo.hpp"
#include "HelpWindow.hpp"
#include "TinyViz.hpp"
#include <array>
#include <chrono>
#include <stdlib.h>
#include <unistd.h>

namespace QC
{
namespace sample
{

#define VIZ_SLOPED_LINE_DIVIDER 30

using namespace std::chrono_literals;
constexpr std::chrono::nanoseconds NSEC = 1s;
constexpr uint64_t NSEC_PER_SEC = NSEC.count();

uint32_t TinyViz::GetSDLFormat( QCImageFormat_e format )
{
    uint32_t pixelFormat = 0;
    switch ( format )
    {
        case QC_IMAGE_FORMAT_RGB888:
            pixelFormat = SDL_PIXELFORMAT_RGB24;
            break;
        case QC_IMAGE_FORMAT_BGR888:
            pixelFormat = SDL_PIXELFORMAT_BGR24;
            break;
        case QC_IMAGE_FORMAT_UYVY:
            pixelFormat = SDL_PIXELFORMAT_UYVY;
            break;
        case QC_IMAGE_FORMAT_NV12:
            pixelFormat = SDL_PIXELFORMAT_NV12;
            break;
        default:
            QC_ERROR( "Found unsupported pixel format %d", static_cast<int>( format ) );
            break;
    }

    return pixelFormat;
}

bool TinyViz::init( uint32_t winW, uint32_t winH )
{
    QC_LOGGER_INIT( "TINYVIZ", LOGGER_LEVEL_INFO );

    m_WindowW = winW;
    m_WindowH = winH;

    if ( SDL_Init( SDL_INIT_VIDEO ) != 0 )
    {
        QC_ERROR( "SDL_Init Error: %s", SDL_GetError() );
        return false;
    }

    TTF_Init();

    // SDL_SetHint( SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "0" );

    SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengl" );

    return true;
}

bool TinyViz::start()
{
    m_Win = SDL_CreateWindow( "QRide TinyViz", 0, 0, m_WindowW, m_WindowH,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED );
    if ( !m_Win )
    {
        QC_ERROR( "SDL_CreateWindow Error: %s", SDL_GetError() );
        return false;
    }

    m_RendererThread = std::make_unique<std::thread>( &TinyViz::rendererThread, this );

    return true;
}

bool TinyViz::stop()
{
    m_Stop = true;
    if ( m_RendererThread ) m_RendererThread->join();

    m_CamInfoMap.clear();

    SDL_DestroyWindow( m_Win );
    TTF_Quit();
    SDL_Quit();

    QC_INFO( "TinyViz stopped" );
    QC_LOGGER_DEINIT();

    return true;
}

bool TinyViz::addCamera( const std::string camName )
{
    m_CamInfoMap.emplace( camName, CamInfo( camName ) );
    m_CamNameList.push_back( camName );

    // Give it an initial black background frame
    auto &camInfo = m_CamInfoMap[camName];

    QC_INFO( "Added camera %s.", camName.c_str() );

    if ( 1 == m_CamNameList.size() )
    {
        m_WindowCol = 1;
        m_WindowRow = 1;
    }
    else if ( m_CamNameList.size() <= 4 )
    {
        m_WindowCol = 2;
        m_WindowRow = 2;
    }
    else if ( m_CamNameList.size() <= 6 )
    {
        m_WindowCol = 2;
        m_WindowRow = 3;
    }
    else if ( m_CamNameList.size() <= 9 )
    {
        m_WindowCol = 3;
        m_WindowRow = 3;
    }
    else if ( m_CamNameList.size() <= 12 )
    {
        m_WindowCol = 3;
        m_WindowRow = 4;
    }
    else
    {
        m_WindowCol = 4;
        m_WindowRow = 4;
    }

    QC_INFO( "TinyViz: %dx%d.", m_WindowCol, m_WindowRow );

    return true;
}

bool TinyViz::addData( const std::string camName, DataFrame_t &data )
{
    QC_DEBUG( "Adding frame for %s %" PRIu64 ", timestamp %" PRIu64, camName.c_str(), data.frameId,
              data.timestamp );

    auto &camInfo = m_CamInfoMap[camName];
    std::lock_guard<std::mutex> camInfoGuard( *camInfo.mutex );
    updateFPS( camName, camInfo.camPTSs, data.timestamp );

    camInfo.camFrame = std::move( data );
    camInfo.setActive();
    return true;
}


bool TinyViz::addData( const std::string camName, Road2DObjects_t &data )
{
    QC_DEBUG( "Adding road objects for %s frame %" PRIu64 ".timestamp %" PRIu64, camName.c_str(),
              data.frameId, data.timestamp );

    auto &camInfo = m_CamInfoMap[camName];
    std::lock_guard<std::mutex> guard( *camInfo.mutex );
    updateFPS( camName, camInfo.rosPTSs, data.timestamp );

    auto &entry = camInfo.roadObjectQueue[data.timestamp];
    entry.emplace_back( std::move( data ) );
    return true;
}

bool TinyViz::isCamSelected( size_t id )
{
    // single-view
    if ( m_WindowCol == 1 )
    {
        if ( id != m_ActiveCamIDX ) return false;
        return true;
    }

    // multi-view
    return ( m_ActiveMultiViewIDX + 1 ) * 4 > id && id >= m_ActiveMultiViewIDX * 4;
}

void TinyViz::rendererThread()
{
    SDL_Renderer *ren =
            SDL_CreateRenderer( m_Win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
    if ( !ren )
    {
        QC_ERROR( "SDL_CreateRenderer Error %s", SDL_GetError() );
        return;
    }

    printRendererInfo( ren );

    // Other textureacceess option: SDL_TEXTUREACCESS_STREAMING ( doesn't work quite as good based
    // on the test )
    constexpr size_t TexWidth = 1920;
    constexpr size_t TexHeight = 1020;
    SDL_Texture *tex = SDL_CreateTexture( ren, SDL_PIXELFORMAT_NV12, SDL_TEXTUREACCESS_TARGET,
                                          TexWidth, TexHeight );
    if ( !tex )
    {
        QC_ERROR( "SDL_CreateTextureFromSurface Error: %s", SDL_GetError() );
        return;
    }

    // init texts
    TTF_Font *font =
            TTF_OpenFont( "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 64 );
    if ( !font ) font = TTF_OpenFont( "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 64 );
    if ( !font ) font = TTF_OpenFont( "/data/LiberationSans-Regular.ttf", 64 );
    if ( !font ) font = TTF_OpenFont( "lib/runtime/LiberationSans-Regular.ttf", 64 );
    for ( auto &cInfo : m_CamInfoMap )
    {
        cInfo.second.initText( ren, font );
    }

    // HelpWindowInfo helpWindow;
    // helpWindow.init( ren, font );

    m_NumTextures.init( 400, *ren, *font );

    SDL_Surface *screenSurface = nullptr;
    const char *envStr = getenv( "QC_TINYVIZ_CAPTURE" );
    int numScreenShot = 0;
    if ( nullptr != envStr )
    {
        numScreenShot = atoi( envStr );
    }

    if ( numScreenShot > 0 )
    {
        screenSurface = SDL_CreateRGBSurface( 0, m_WindowW, m_WindowH, 32, 0x00ff0000, 0x0000ff00,
                                              0x000000ff, 0xff000000 );
        if ( nullptr == screenSurface )
        {
            QC_WARN( "Can't create RGB surface to capture screenshot %s", SDL_GetError() );
            numScreenShot = 0;
        }
    }

    int captureId = 0;
    auto captureTime = std::chrono::high_resolution_clock::now();

    while ( !m_Stop )
    {
        SDL_SetRenderDrawColor( ren, 0x0, 0x0, 0x0, 0xFF );
        // SDL_RenderClear( ren );

        bool isActive = false;

        if ( m_CamInfoMap.empty() )
        {
            SDL_Delay( 10000 );
            continue;
        }
        auto start = std::chrono::high_resolution_clock::now();
        // TODO: mutex lock
        if ( m_WindowCol == 1 )
        {
            isActive = renderCam( m_CamInfoMap[m_CamNameList[m_ActiveCamIDX]], ren );
        }
        else
        {
            size_t multiViewIdxShift = m_ActiveMultiViewIDX * m_WindowCol * m_WindowRow;
            for ( size_t idx = 0; idx < (size_t) m_WindowCol * m_WindowRow; idx++ )
            {
                if ( ( idx + multiViewIdxShift ) >= m_CamNameList.size() )
                {
                    // no camera, render black
                    renderBlack( ren, tex, idx );
                    continue;
                }

                // update display buffer
                auto targetCamName = m_CamNameList[idx + multiViewIdxShift];
                auto &camInfo = m_CamInfoMap[targetCamName];
                isActive |= renderCam( camInfo, ren, idx );
            }
        }   // if( m_WindowCol == 1 )

        // m_ShowHelp ? helpWindow.render( ren ) : helpWindow.renderF1( ren );

        if ( isActive && !m_PauseRenderer )
        {
            SDL_RenderPresent( ren );
        }
        auto end = std::chrono::high_resolution_clock::now();
        float cost = (float) std::chrono::duration_cast<std::chrono::microseconds>( end - start )
                             .count() /
                     1000.0;
        QC_DEBUG( "render cost %.2f ms", cost );
        m_LastRenderFPS = m_LastRenderFPS * 7 / 8 + 1000 / ( cost + 33 ) / 8;
        // TODO: mutex unlock

        // TODO: switch to usleep
        SDL_Delay( 33 );

        if ( captureId < numScreenShot )
        {
            auto now = std::chrono::high_resolution_clock::now();
            if ( ( now - captureTime ) > std::chrono::microseconds( 1000 ) )
            {
                SDL_RenderReadPixels( ren, NULL, SDL_PIXELFORMAT_ARGB8888, screenSurface->pixels,
                                      screenSurface->pitch );
                std::string path = "/tmp/screenshot" + std::to_string( captureId ) + ".bmp";
                SDL_SaveBMP( screenSurface, path.c_str() );
                captureId++;
                captureTime = std::chrono::high_resolution_clock::now();
            }
        }
    }

    if ( nullptr != screenSurface )
    {
        SDL_FreeSurface( screenSurface );
    }

    m_NumTextures.release();
    // helpWindow.close();
    for ( auto &cInfo : m_CamInfoMap )
    {
        cInfo.second.closeText();
    }
    TTF_CloseFont( font );

    SDL_DestroyTexture( tex );
    SDL_DestroyRenderer( ren );

    QC_DEBUG( "TinyViz thread exited" );
}

void TinyViz::setAllCamActive()
{
    for ( auto &cInfo : m_CamInfoMap ) cInfo.second.setActive();
}

void TinyViz::printRendererInfo( SDL_Renderer *ren )
{
    SDL_RendererInfo rendererInfo;
    if ( SDL_GetRendererInfo( ren, &rendererInfo ) )
    {
        QC_ERROR( "SDL_GetRenererInfo Error %s", SDL_GetError() );
        return;
    }

    bool printRendererInfo = false;
    if ( printRendererInfo )
    {
        QC_INFO( "SDK_RendererInfo: name: %s, "
                 "renderer driver: %s"
                 "software: %d"
                 "accelerated: %d"
                 "presentvsync: %d",
                 rendererInfo.name, SDL_GetCurrentVideoDriver(),
                 ( rendererInfo.flags & SDL_RENDERER_SOFTWARE ),
                 ( rendererInfo.flags & SDL_RENDERER_ACCELERATED ),
                 ( rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC ) );
    }

    QC_INFO( "Available video driver: " );
    for ( int idx = 0; idx < SDL_GetNumVideoDrivers(); idx++ )
    {
        std::string isSelected =
                std::string( SDL_GetVideoDriver( idx ) ) == SDL_GetCurrentVideoDriver()
                        ? " (selected)"
                        : "";
        QC_INFO( "%d%s: %s", idx, isSelected.c_str(), SDL_GetVideoDriver( idx ) );
    }

    QC_INFO( "Available renderer driver: " );
    for ( int idx = 0; idx < SDL_GetNumRenderDrivers(); idx++ )
    {
        SDL_RendererInfo rendererInfoByIdx;
        SDL_GetRenderDriverInfo( idx, &rendererInfoByIdx );
        std::string isSelected =
                std::string( rendererInfoByIdx.name ) == rendererInfo.name ? " (selected)" : "";
        QC_INFO( "%d%s: %s", idx, isSelected.c_str(), rendererInfoByIdx.name );
    }
}

bool TinyViz::renderBlack( SDL_Renderer *ren, SDL_Texture *tex, size_t idx )
{
    SDL_Rect DestR;
    DestR.w = m_WindowW / m_WindowRow;
    DestR.h = m_WindowH / m_WindowCol;
    DestR.x = ( idx % m_WindowRow ) * DestR.w;
    DestR.y = ( idx / m_WindowRow ) * DestR.h;
    SDL_SetRenderDrawColor( ren, 0x0, 0x0, 0x0, 0xFF );
    SDL_RenderFillRect( ren, &DestR );

    return true;
}

bool TinyViz::renderCam( CamInfo &camInfo, SDL_Renderer *ren, size_t idx )
{
    SDL_Rect DestR;

    if ( !camInfo.isActive() ) return false;

    SDL_Texture *tex = nullptr;
    uint64_t pts = 0;
    {
        std::lock_guard<std::mutex> guard( *camInfo.mutex );
        if ( nullptr == camInfo.data() )
        {
            QC_DEBUG( "%s: display buffer is not yet available. Skipped one frame.",
                      camInfo.camName.c_str() );
            return false;
        }

        tex = camInfo.tex;
        if ( nullptr == tex )
        {
            tex = SDL_CreateTexture( ren, GetSDLFormat( camInfo.format() ),
                                     SDL_TEXTUREACCESS_TARGET, camInfo.width(), camInfo.height() );
            if ( !tex )
            {
                QC_ERROR( "SDL_CreateTextureFromSurface Error: %s", SDL_GetError() );
                return false;
            }
            camInfo.tex = tex;
        }

        pts = camInfo.camFrame.timestamp;

        // render frame
        DestR.w = m_WindowW / m_WindowRow;
        DestR.h = m_WindowH / m_WindowCol;
        DestR.x = ( idx % m_WindowRow ) * DestR.w;
        DestR.y = ( idx / m_WindowRow ) * DestR.h;
        uint32_t col = 1;
        if ( 1 == camInfo.batch() )
        {
            col = 1;
        }
        else if ( camInfo.batch() <= 4 )
        {
            col = 2;
        }
        else if ( camInfo.batch() <= 9 )
        {
            col = 3;
        }
        else
        {
            col = 4;
        }
        for ( uint32_t i = 0; ( i < camInfo.batch() ) && ( i < ( col * col ) ); i++ )
        {
            SDL_Rect DestRB;
            DestRB.w = DestR.w / col;
            DestRB.h = DestR.h / col;
            DestRB.x = DestR.x + ( i % col ) * DestRB.w;
            DestRB.y = DestR.y + ( i / col ) * DestRB.h;
            SDL_UpdateTexture( tex, nullptr, camInfo.data( i ), camInfo.stride() );
            SDL_RenderCopy( ren, tex, nullptr, &DestRB );
        }
        for ( uint32_t i = camInfo.batch(); i < ( col * col ); i++ )
        {
            SDL_Rect DestRB;
            DestRB.w = DestR.w / col;
            DestRB.h = DestR.h / col;
            DestRB.x = DestR.x + ( i % col ) * DestRB.w;
            DestRB.y = DestR.y + ( i / col ) * DestRB.h;
            SDL_SetRenderDrawColor( ren, 0x0, 0x0, 0x0, 0xFF );
            SDL_RenderFillRect( ren, &DestRB );
        }
    }

    if ( camInfo.lastFPSUpdate < pts - NSEC_PER_SEC / 2 )
    {
        std::lock_guard<std::mutex> camInfoGuard( *camInfo.mutex );
        updateFPS( camInfo );
        camInfo.lastFPSUpdate = pts;
    }

    SDL_SetRenderDrawColor( ren, camInfo.color.r, camInfo.color.g, camInfo.color.b,
                            camInfo.color.a );

    {
        std::lock_guard<std::mutex> guard( *camInfo.mutex );
        float scaleX = static_cast<float>( m_WindowW ) / camInfo.width() / m_WindowRow;
        float scaleY = static_cast<float>( m_WindowH ) / camInfo.height() / m_WindowCol;

        auto widow = 5 * camInfo.lastCamFPS /
                     ( ( camInfo.lastRodFPS > 0 ) ? camInfo.lastRodFPS : camInfo.lastCamFPS );
        auto historyWindow = NSEC_PER_SEC / m_LastRenderFPS * widow;
        renderBB( pts, historyWindow, camInfo.roadObjectQueue, ren, DestR, scaleX, scaleY,
                  COLOR_RED );

        // render status bar
        SDL_Rect statusBarR = DestR;
        statusBarR.h = 25;
        statusBarR.y = DestR.y + DestR.h - statusBarR.h;

        SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_BLEND );
        SDL_SetRenderDrawColor( ren, 0x0, 0x0, 0x0, 0xBB );
        SDL_RenderFillRect( ren, &statusBarR );

        SDL_SetRenderDrawColor( ren, 0xFF, 0xFF, 0xFF, 0xAA );
        SDL_RenderDrawRect( ren, &statusBarR );
        SDL_SetRenderDrawBlendMode( ren, SDL_BLENDMODE_NONE );

        // render cam name and static text
        SDL_Rect textR;
        float scale = 0.22;
        textR.w = camInfo.camNameText.getWidth() * scale;
        textR.h = camInfo.camNameText.getHeight() * scale;
        textR.x = DestR.x + 4;
        textR.y = DestR.y + DestR.h - 4 - textR.h;
        SDL_RenderCopy( ren, camInfo.camNameText.getTexture(), NULL, &textR );

        if ( m_EnableCamFPSCap )
        {
            // render cam name and static text
            textR.w = camInfo.statusBarText.getWidth() * scale;
            textR.h = camInfo.statusBarText.getHeight() * scale;
            textR.x = DestR.x + DestR.w - textR.w;
            textR.y = DestR.y + DestR.h - 4 - textR.h;
            SDL_RenderCopy( ren, camInfo.statusBarText.getTexture(), NULL, &textR );

            // render individual FPS
            renderFPS( ren, DestR, DestR.w - textR.w + 82, camInfo.lastCamFPS );
            renderFPS( ren, DestR, DestR.w - textR.w + 172, camInfo.lastRodFPS );
            renderFPS( ren, DestR, DestR.w - textR.w + 262, camInfo.lastLanFPS );
            renderFPS( ren, DestR, DestR.w - textR.w + 348, camInfo.lastTfsFPS );
        }
    }

    camInfo.markRendered();
    return true;
}

void TinyViz::renderFPS( SDL_Renderer *ren, const SDL_Rect &DestR, uint32_t xShfit, float &fps )
{
    // The FPS texture is pre-calculated with 400 entries
    auto *fpsTexture = m_NumTextures.getTextInfo( fps * 10 );
    if ( !fpsTexture ) return;

    // render fps
    SDL_Rect textR;
    float scale = 0.22;
    textR.w = fpsTexture->getWidth() * scale;
    textR.h = fpsTexture->getHeight() * scale;
    textR.x = DestR.x + xShfit;
    textR.y = DestR.y + DestR.h - textR.h - 4;

    SDL_RenderCopy( ren, fpsTexture->getTexture(), NULL, &textR );
}

void TinyViz::updateFPS( const std::string &camName, std::deque<uint64_t> &queue,
                         uint64_t timestamp )
{
    // note: there could be multiple objects belong to the same timestamp
    if ( queue.empty() || queue[queue.size() - 1] != timestamp ) queue.push_back( timestamp );
    if ( queue.size() > 100 ) queue.pop_front();

    QC_DEBUG( "Updated %s FPS info. queue size now %zu. timestamp:[%" PRIu64 ", %" PRIu64 "]",
              camName.c_str(), queue.size(), queue[queue.size() - 1], queue[0] );
}

void TinyViz::updateFPS( CamInfo &camInfo )
{
    constexpr size_t SIZE = 4;
    std::array<const std::deque<uint64_t> *, SIZE> queues{ &camInfo.camPTSs, &camInfo.lanPTSs,
                                                           &camInfo.rosPTSs, &camInfo.tfsPTSs };
    std::array<float *, SIZE> FPSs{ &camInfo.lastCamFPS, &camInfo.lastLanFPS, &camInfo.lastRodFPS,
                                    &camInfo.lastTfsFPS };

    for ( size_t idx = 0; idx < SIZE; idx++ )
    {
        auto &queue = *( queues[idx] );
        *FPSs[idx] = queue.empty() ? 0
                                   : queue.size() / ( ( queue[queue.size() - 1] - queue[0] ) /
                                                      static_cast<double>( NSEC_PER_SEC ) );

        QC_DEBUG( "%s: queue size:%zu, timestamp (%" PRIu64 ", %" PRIu64 "), ",
                  camInfo.camName.c_str(), queue.size(),
                  queue.empty() ? 0 : queue[queue.size() - 1], queue.empty() ? 0 : queue[0] );
    }
}

void TinyViz::drawLines( SDL_Renderer *ren, std::vector<SDL_Point> &points )
{
    for ( size_t i = 0; i < ( points.size() - 1 ); i++ )
    {
        SDL_Point &pt1 = points[i];
        SDL_Point &pt2 = points[i + 1];
        if ( ( pt1.x == pt2.x ) || ( pt1.y == pt2.y ) )
        {
            SDL_RenderDrawLine( ren, pt1.x, pt1.y, pt2.x, pt2.y );
        }
        else
        { /* draw sloped line */
            float w = pt2.x - pt1.x;
            float h = pt2.y - pt1.y;
            float dx = w / VIZ_SLOPED_LINE_DIVIDER;
            float dy = h / VIZ_SLOPED_LINE_DIVIDER;
            float x1 = pt1.x;
            float y1 = pt1.y;
            float x2, y2;
            for ( int i = 0; i < VIZ_SLOPED_LINE_DIVIDER; i++ )
            {
                x2 = x1 + dx;
                y2 = y1 + dy;
                SDL_RenderDrawLine( ren, std::round( x1 ), std::round( y1 ), std::round( x2 ),
                                    std::round( y2 ) );
                x1 = x2;
                y1 = y2;
            }
        }
    }
}

void TinyViz::renderBB( const uint64_t targetPTS, const uint64_t historyWindow,
                        std::map<uint64_t, std::list<Road2DObjects_t>> &queue, SDL_Renderer *ren,
                        const SDL_Rect &DestR, const float scaleX, const float scaleY,
                        const SDL_Color &color )
{
    // move to the closest entry
    while ( queue.size() >= 2 && std::next( queue.begin() )->first <= targetPTS )
        queue.erase( queue.begin() );

    // remove entry if outdated
    if ( queue.size() == 1 && queue.begin()->first < ( targetPTS - historyWindow ) ) queue.clear();

    if ( queue.empty() || queue.begin()->first > targetPTS ) return;

    float thickness = 3.0;
    SDL_RenderSetScale( ren, thickness, thickness );
    auto &mapItem = *queue.begin();
    for ( auto &l : mapItem.second )
    {
        auto &objs = l.objs;
        for ( auto &obj : objs )
        {
            std::vector<SDL_Point> points;

            SDL_Point point;

            for ( int i = 0; i < ROAD_2D_OBJECT_NUM_POINTS; i++ )
            {
                point.x = ( obj.points[i].x * scaleX + DestR.x ) / thickness;
                point.y = ( obj.points[i].y * scaleY + DestR.y ) / thickness;
                points.push_back( point );
            }

            point.x = ( obj.points[0].x * scaleX + DestR.x ) / thickness;
            point.y = ( obj.points[0].y * scaleY + DestR.y ) / thickness;
            points.push_back( point );

            SDL_SetRenderDrawColor( ren, color.r, color.g, color.b, color.a );
            drawLines( ren, points );
        }
    }
    SDL_RenderSetScale( ren, 1.0, 1.0 );
}

}   // namespace sample
}   // namespace QC
