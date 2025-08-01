set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( RADAR_IFACE_HEADERS
    RadarIface.hpp
    RadarIf.hxx
)

set( RADAR_IFACE_SOURCES
    RadarIface.cpp
)

set( TARGET_LIBRARIES )

# Platform-specific libraries
if( NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES c )
endif()

add_library( QCNodeRadarIface STATIC ${RADAR_IFACE_SOURCES} )

set_property( TARGET QCNodeRadarIface PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeRadarIface PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeRadarIface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( QCNodeRadarIface PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeRadarIface PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${RADAR_IFACE_HEADERS} DESTINATION include/QC/Library/RadarIface )
