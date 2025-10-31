set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_VOXELIZATION_HEADERS
    ${HEADERS_DIR}/QC/Node/Voxelization.hpp
)

set( QC_NODE_VOXELIZATION_SOURCES
    Voxelization.cpp
    VoxelizationImpl.cpp
    VoxelizationConfig.cpp
    VoxelizationMonitor.cpp
)

set( TARGET_LIBRARIES QCNodeBase )

list( APPEND TARGET_LIBRARIES FadasIface OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Library/OpenclIface )
add_library( QCNodeVoxelization OBJECT ${QC_NODE_VOXELIZATION_SOURCES} )

set_property( TARGET QCNodeVoxelization PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeVoxelization PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeVoxelization PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeVoxelization PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_VOXELIZATION_HEADERS} DESTINATION include/QC/Node )
