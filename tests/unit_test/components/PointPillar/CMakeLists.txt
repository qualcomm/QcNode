
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
add_executable( gtest_PointPillar gtest_PointPillar.cpp )
target_include_directories( gtest_PointPillar PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_PointPillar gtest RideHalVoxelization RideHalPostCenterPoint )
install(TARGETS gtest_PointPillar DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_PointPillar.cpp
     PARENT_SCOPE )
