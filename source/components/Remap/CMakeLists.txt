
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( REMAP_HEADERS
    ${HEADERS_DIR}/QC/component/Remap.hpp
)

set( REMAP_SOURCES Remap.cpp )

list( APPEND TARGET_LIBRARIES QCNodeComponentCommon FadasIface FadasIfaceStub)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Library/FadasIface )

add_library( QCNodeRemap OBJECT ${REMAP_SOURCES} )

set_property( TARGET QCNodeRemap PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeRemap PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeRemap PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeRemap PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${REMAP_HEADERS} DESTINATION include/QC/component )
