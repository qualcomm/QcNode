
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils )

add_executable( gtest_GL2DFlex gtest_GL2DFlex.cpp )
target_include_directories( gtest_GL2DFlex PUBLIC ${HEADERS_DIR} )
target_link_libraries( gtest_GL2DFlex gtest QCNodeComponentCommon QCNodeGL2DFlex QCNodeTestUtils )

install(TARGETS gtest_GL2DFlex DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_GL2DFlex.cpp PARENT_SCOPE )


