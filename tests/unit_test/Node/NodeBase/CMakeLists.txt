
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeBase gtest_NodeBase.cpp gtest_DataTree.cpp )
target_include_directories( gtest_NodeBase PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeBase gtest QCNodeComponentCommon QCNode QCNodeTestUtils BufferManager )
install(TARGETS gtest_NodeBase DESTINATION bin)
