
set( GTEST_QCNODE_SOURCES gtest_QCNode.cpp )
set( GTEST_QCNODE_INCLUDES ${PROJECT_SOURCE_DIR}/include )
set( GTEST_QCNODE_LIBS gtest QCNode QCNodeTestUtils QCNodeSample)

add_subdirectory(buffer)
add_subdirectory(components)
add_subdirectory(logger)

add_executable( gtest_QCNode ${GTEST_QCNODE_SOURCES} )
target_link_libraries( gtest_QCNode ${GTEST_QCNODE_LIBS} )
target_compile_definitions( gtest_QCNode PUBLIC GTEST_QCNODE )
target_include_directories( gtest_QCNode PUBLIC ${GTEST_QCNODE_INCLUDES} )

install(TARGETS gtest_QCNode DESTINATION bin)
