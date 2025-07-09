
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeBase gtest_NodeBase.cpp )
target_include_directories( gtest_NodeBase PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeBase gtest QCNodeComponentCommon QCNode QCNodeTestUtils)
install(TARGETS gtest_NodeBase DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_NodeBase.cpp
     PARENT_SCOPE )
