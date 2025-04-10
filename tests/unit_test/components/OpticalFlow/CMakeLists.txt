
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_OpticalFlow gtest_OpticalFlow.cpp )
target_include_directories( gtest_OpticalFlow PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_OpticalFlow gtest RideHalComponentCommon RideHalOpticalFlow RideHalTestUtils)
install(TARGETS gtest_OpticalFlow DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_OpticalFlow.cpp
     PARENT_SCOPE )
