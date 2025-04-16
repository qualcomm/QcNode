
if( NOT TARGET QCNodeQnnRuntime )
    message( STATUS "build gtest without QNN" )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
add_executable( gtest_QnnRuntime gtest_QnnRuntime.cpp )
target_include_directories( gtest_QnnRuntime PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_QnnRuntime gtest QCNodeComponentCommon QCNodeQnnRuntime QCNodeTestUtils)
install(TARGETS gtest_QnnRuntime DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_QnnRuntime.cpp
     PARENT_SCOPE )
