if( NOT TARGET QCNodeOpticalFlow )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeOpticalFlow gtest_OpticalFlow.cpp )
target_include_directories( gtest_NodeOpticalFlow PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeOpticalFlow gtest QCNodeComponentCommon QCNode QCNodeTestUtils)
install(TARGETS gtest_NodeOpticalFlow DESTINATION bin)
