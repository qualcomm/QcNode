if( NOT TARGET QCNodeVoxelization )
    message( STATUS "build without Voxelization" )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_VOXELIZATION_HEADERS
    ${HEADERS_DIR}/QC/Node/Voxelization.hpp
)

set( QC_NODE_VOXELIZATION_SOURCES
    Voxelization.cpp
)

set( TARGET_LIBRARIES QCNodeVoxelization QCNodeNodeBase )

add_library( QCNodeNodeVoxelization OBJECT ${QC_NODE_VOXELIZATION_SOURCES} )

set_property( TARGET QCNodeNodeVoxelization PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeVoxelization PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeVoxelization PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeVoxelization PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_VOXELIZATION_HEADERS} DESTINATION include/QC/Node )


