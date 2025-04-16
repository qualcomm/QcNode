
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils )

add_executable( gtest_C2D gtest_C2D.cpp )
target_link_libraries( gtest_C2D gtest QCNodeC2D QCNodeTestUtils )
install(TARGETS gtest_C2D DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_C2D.cpp
     PARENT_SCOPE )
