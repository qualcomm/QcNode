
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( VOXELICATION_HEADERS
    ${HEADERS_DIR}/ridehal/component/Voxelization.hpp
)

set( PLRPRE_SOURCES Voxelization.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon FadasIface OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/components/Voxelization )
add_library( RideHalVoxelization OBJECT ${PLRPRE_SOURCES} )

set_property( TARGET RideHalVoxelization PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalVoxelization PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalVoxelization PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalVoxelization PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${VOXELICATION_HEADERS} DESTINATION include/ridehal/component )
