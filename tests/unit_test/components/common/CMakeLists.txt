
add_executable( gtest_ComponentIF gtest_ComponentIF.cpp )
target_link_libraries( gtest_ComponentIF gtest RideHalComponentCommon )
install(TARGETS gtest_ComponentIF DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_ComponentIF.cpp
     PARENT_SCOPE )
