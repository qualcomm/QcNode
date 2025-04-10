add_executable( gtest_Buffer gtest_Buffer.cpp )
target_link_libraries( gtest_Buffer gtest RideHalCommon )
install(TARGETS gtest_Buffer DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_Buffer.cpp
     PARENT_SCOPE )
