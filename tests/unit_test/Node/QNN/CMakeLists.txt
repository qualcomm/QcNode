if( NOT TARGET QCNodeQNN )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include
                 ${PROJECT_SOURCE_DIR}/tests/utils 
                 ${PROJECT_SOURCE_DIR}/source/Node/QNN
)

add_executable( gtest_NodeQnn gtest_NodeQnn.cpp )
target_include_directories( gtest_NodeQnn PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeQnn gtest QCNodeComponentCommon QCNode QCNodeTestUtils BufferManager)
install(TARGETS gtest_NodeQnn DESTINATION bin)

