
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( GL2DFLEX_HEADERS
    ${HEADERS_DIR}/QC/component/GL2DFlex.hpp
)

set( TARGET_LIBRARIES QCNodeComponentCommon )
list( APPEND TARGET_LIBRARIES EGL GLESv2)
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
list( APPEND TARGET_LIBRARIES GLESv2_adreno drm gbm  )
endif()
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/drm )
list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/libdrm )

set( COMPONENT_GL2DFLEX_SOURCES
     GL2DFlex.cpp
)

add_library( QCNodeGL2DFlex OBJECT ${COMPONENT_GL2DFLEX_SOURCES} )

set_property( TARGET QCNodeGL2DFlex PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeGL2DFlex PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeGL2DFlex PUBLIC ${HEADERS_DIR} )
target_include_directories( QCNodeGL2DFlex PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( QCNodeGL2DFlex PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${GL2DFLEX_HEADERS} DESTINATION include/QC/component )
