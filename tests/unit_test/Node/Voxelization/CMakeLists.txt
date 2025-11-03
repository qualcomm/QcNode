if( NOT TARGET QCNodeVoxelization )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include
                 ${PROJECT_SOURCE_DIR}/tests/utils 
                 ${PROJECT_SOURCE_DIR}/source/Node/Voxelization
)

add_executable( gtest_NodeVoxelization gtest_NodeVoxelization.cpp )
target_include_directories( gtest_NodeVoxelization PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodeVoxelization gtest QCNode QCNodeTestUtils BufferManager )
install(TARGETS gtest_NodeVoxelization DESTINATION bin)

