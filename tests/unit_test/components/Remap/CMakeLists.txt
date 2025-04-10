
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_Remap gtest_Remap.cpp )
target_include_directories( gtest_Remap PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_Remap gtest RideHalComponentCommon RideHalRemap RideHalTestUtils)
install(TARGETS gtest_Remap DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_Remap.cpp
     PARENT_SCOPE )
