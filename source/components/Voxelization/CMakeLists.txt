
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( VOXELICATION_HEADERS
    ${HEADERS_DIR}/QC/component/Voxelization.hpp
)

set( PLRPRE_SOURCES Voxelization.cpp )

list( APPEND TARGET_LIBRARIES QCNodeComponentCommon FadasIface OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/Library/OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/components/Voxelization )
add_library( QCNodeVoxelization OBJECT ${PLRPRE_SOURCES} )

set_property( TARGET QCNodeVoxelization PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeVoxelization PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeVoxelization PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeVoxelization PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${VOXELICATION_HEADERS} DESTINATION include/QC/component )
