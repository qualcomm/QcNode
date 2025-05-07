
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeVideoEncoder gtest_NodeVideoEncoder.cpp )
target_include_directories( gtest_NodeVideoEncoder PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeVideoEncoder gtest QCNodeComponentCommon QCNode QCNodeTestUtils)
install(TARGETS gtest_NodeVideoEncoder DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_NodeVideoEncoder.cpp
     PARENT_SCOPE )
