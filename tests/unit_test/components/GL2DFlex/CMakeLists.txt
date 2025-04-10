
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils )

add_executable( gtest_GL2DFlex gtest_GL2DFlex.cpp )
target_include_directories( gtest_GL2DFlex PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_GL2DFlex gtest RideHalComponentCommon RideHalGL2DFlex RideHalTestUtils )

install(TARGETS gtest_GL2DFlex DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_GL2DFlex.cpp PARENT_SCOPE )


