set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_Radar gtest_Radar.cpp )
target_include_directories( gtest_Radar PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_Radar gtest QCNodeComponentCommon QCNodeRadar QCNodeTestUtils)
install(TARGETS gtest_Radar DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_Radar.cpp
     PARENT_SCOPE )
