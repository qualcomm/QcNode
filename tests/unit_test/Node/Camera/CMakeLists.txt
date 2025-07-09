if( NOT TARGET QCNodeCamera )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeCamera gtest_NodeCamera.cpp )
target_include_directories( gtest_NodeCamera PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeCamera gtest QCNodeComponentCommon QCNode QCNodeTestUtils)
install(TARGETS gtest_NodeCamera DESTINATION bin)
