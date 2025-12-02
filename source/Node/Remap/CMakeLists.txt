set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_REMAP_HEADERS
    ${HEADERS_DIR}/QC/Node/Remap.hpp
)

set( QC_NODE_REMAP_SOURCES
    Remap.cpp
    RemapConfig.cpp
    RemapImpl.cpp
)

set( TARGET_LIBRARIES QCNodeBase )

add_library( QCNodeRemap OBJECT ${QC_NODE_REMAP_SOURCES} )
list( APPEND TARGET_LIBRARIES FadasIface)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Library/FadasIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Node/Remap )

set_property( TARGET QCNodeRemap PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeRemap PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeRemap PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeRemap PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_REMAP_HEADERS} DESTINATION include/QC/Node )
