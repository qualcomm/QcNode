
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( DFS_HEADERS
    ${HEADERS_DIR}/ridehal/component/DepthFromStereo.hpp
)

set( DFS_SOURCES DepthFromStereo.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon evaEpl evaPlatform devioClient )

add_library( RideHalDepthFromStereo OBJECT ${DFS_SOURCES} )

set_property( TARGET RideHalDepthFromStereo PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalDepthFromStereo PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalDepthFromStereo PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalDepthFromStereo PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${DFS_HEADERS} DESTINATION include/ridehal/component )
