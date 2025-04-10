
set( HEADERS_DIR
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

include(FindPkgConfig)
pkg_check_modules(SDL2 SDL2_gfx SDL2_ttf)

if( NOT SDL2_FOUND )
    message(STATUS "Skipped TinyViz lib. Target SDL2_gfx or SDL2_ttf not found.")
    return ()
endif()

add_library(TinyViz STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/TinyViz.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/HelpWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/TextRenderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CamInfo.cpp
)

target_include_directories(TinyViz PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_PREFIX_PATH}/include
)

if( NOT TARGET RideHal )
  find_package(RideHal)
  if( NOT TARGET RideHal )
    message(FATAL_ERROR "RideHal SDK not found")
  endif()
endif()

target_link_libraries(TinyViz 
    ${SDL2_LIBRARIES}
    freetype
    RideHal
)

target_link_directories(TinyViz PUBLIC ${CMAKE_PREFIX_PATH}/lib)

target_include_directories(TinyViz PUBLIC ${HEADERS_DIR})

set_property( TARGET TinyViz PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET TinyViz PROPERTY POSITION_INDEPENDENT_CODE ON )

install(TARGETS TinyViz DESTINATION lib)

