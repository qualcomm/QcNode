set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_RADAR_HEADERS
    ${HEADERS_DIR}/QC/Node/Radar.hpp
)

set( TARGET_LIBRARIES
    QCNodeBase
    QCNodeRadar
)

set( QC_NODE_RADAR_SOURCES
    Radar.cpp
)

add_library( QCNodeNodeRadar OBJECT ${QC_NODE_RADAR_SOURCES} )

set_property( TARGET QCNodeNodeRadar PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeRadar PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeRadar PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeRadar PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_RADAR_HEADERS} DESTINATION include/QC/Node )
