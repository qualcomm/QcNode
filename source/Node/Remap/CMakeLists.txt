set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_REMAP_HEADERS
    ${HEADERS_DIR}/QC/Node/Remap.hpp
)

set( QC_NODE_REMAP_SOURCES
    Remap.cpp
)

set( TARGET_LIBRARIES QCNodeRemap QCNodeNodeBase )

add_library( QCNodeNodeRemap OBJECT ${QC_NODE_REMAP_SOURCES} )

set_property( TARGET QCNodeNodeRemap PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeRemap PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeRemap PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeRemap PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_REMAP_HEADERS} DESTINATION include/QC/Node )
