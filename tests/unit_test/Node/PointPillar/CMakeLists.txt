if( NOT TARGET QCNodeVoxelization )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils)
add_executable( gtest_NodePointPillar gtest_NodePointPillar.cpp )
target_include_directories( gtest_NodePointPillar PUBLIC ${HEADERS_DIR})
target_link_libraries( gtest_NodePointPillar gtest QCNodeComponentCommon QCNode QCNodeTestUtils)
install(TARGETS gtest_NodePointPillar DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_NodePointPillar.cpp
     PARENT_SCOPE )


