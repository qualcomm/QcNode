if( NOT TARGET QCNodeNodeRadar )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeRadar gtest_NodeRadar.cpp )
target_include_directories( gtest_NodeRadar PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeRadar gtest QCNodeComponentCommon QCNode QCNodeTestUtils)
install(TARGETS gtest_NodeRadar DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_NodeRadar.cpp
     PARENT_SCOPE )