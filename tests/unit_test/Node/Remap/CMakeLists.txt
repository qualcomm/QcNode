if( NOT TARGET QCNodeRemap )
    return()
endif()
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include
                 ${PROJECT_SOURCE_DIR}/tests/utils 
)
add_executable( gtest_NodeRemap gtest_NodeRemap.cpp )
target_include_directories( gtest_NodeRemap PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeRemap gtest QCNode QCNodeTestUtils BufferManager)
install(TARGETS gtest_NodeRemap DESTINATION bin)
