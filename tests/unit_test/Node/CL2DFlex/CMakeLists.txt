if( NOT TARGET QCNodeCL2DFlex )
    return()
endif()
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include
                 ${PROJECT_SOURCE_DIR}/tests/utils 
)
add_executable( gtest_NodeCL2DFlex gtest_NodeCL2DFlex.cpp )
target_include_directories( gtest_NodeCL2DFlex PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeCL2DFlex gtest QCNodeComponentCommon QCNode QCNodeTestUtils BufferManager)
install(TARGETS gtest_NodeCL2DFlex DESTINATION bin)
