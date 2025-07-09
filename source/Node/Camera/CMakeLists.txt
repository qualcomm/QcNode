if( NOT TARGET QCNodeCamera )
    message( STATUS "build without Camera" )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_CAMERA_HEADERS
    ${HEADERS_DIR}/QC/Node/Camera.hpp
)

set( QC_NODE_CAMERA_SOURCES
    Camera.cpp
)

set( TARGET_LIBRARIES QCNodeCamera QCNodeNodeBase )

add_library( QCNodeNodeCamera OBJECT ${QC_NODE_CAMERA_SOURCES} )

set_property( TARGET QCNodeNodeCamera PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeCamera PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeCamera PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeCamera PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_CAMERA_HEADERS} DESTINATION include/QC/Node )


