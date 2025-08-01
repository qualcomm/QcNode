set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_RADAR_HEADERS
    ${HEADERS_DIR}/QC/component/Radar.hpp
)

set( TARGET_LIBRARIES QCNodeComponentCommon QCNodeRadarIface )

set( COMPONENT_RADAR_SOURCES
    Radar.cpp
)

add_library( QCNodeRadar OBJECT ${COMPONENT_RADAR_SOURCES} )

set_property( TARGET QCNodeRadar PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeRadar PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeRadar PUBLIC ${HEADERS_DIR} )
target_include_directories( QCNodeRadar PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
target_include_directories( QCNodeRadar PUBLIC ${PROJECT_SOURCE_DIR}/source/Library/RadarIface )

target_link_libraries( QCNodeRadar PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_RADAR_HEADERS} DESTINATION include/QC/component )
