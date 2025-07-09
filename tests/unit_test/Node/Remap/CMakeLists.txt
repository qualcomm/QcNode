if( NOT TARGET QCNodeNodeRemap )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeRemap gtest_NodeRemap.cpp )
target_include_directories( gtest_NodeRemap PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeRemap gtest QCNodeComponentCommon QCNode QCNodeTestUtils)
install(TARGETS gtest_NodeRemap DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_NodeRemap.cpp
     PARENT_SCOPE )
