
if( NOT TARGET RideHalQnnRuntime )
    message( STATUS "build gtest without QNN" )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
add_executable( gtest_QnnRuntime gtest_QnnRuntime.cpp )
target_include_directories( gtest_QnnRuntime PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_QnnRuntime gtest RideHalComponentCommon RideHalQnnRuntime RideHalTestUtils)
install(TARGETS gtest_QnnRuntime DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_QnnRuntime.cpp
     PARENT_SCOPE )
