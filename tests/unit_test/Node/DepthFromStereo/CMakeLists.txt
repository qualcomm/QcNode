if( NOT TARGET QCNodeDepthFromStereo )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodeDepthFromStereo gtest_DepthFromStereo.cpp )
target_include_directories( gtest_NodeDepthFromStereo PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeDepthFromStereo gtest QCNode QCNodeTestUtils BufferManager)
install(TARGETS gtest_NodeDepthFromStereo DESTINATION bin)

