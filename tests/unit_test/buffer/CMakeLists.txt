add_executable( gtest_Buffer gtest_Buffer.cpp )
target_link_libraries( gtest_Buffer gtest QCNodeCommon )
install(TARGETS gtest_Buffer DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_Buffer.cpp
     PARENT_SCOPE )
