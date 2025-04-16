
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_DepthFromStereo gtest_DepthFromStereo.cpp )
target_include_directories( gtest_DepthFromStereo PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_DepthFromStereo gtest QCNodeComponentCommon QCNodeDepthFromStereo QCNodeTestUtils)
install(TARGETS gtest_DepthFromStereo DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_DepthFromStereo.cpp
     PARENT_SCOPE )
