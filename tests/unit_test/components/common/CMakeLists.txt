
add_executable( gtest_ComponentIF gtest_ComponentIF.cpp )
target_link_libraries( gtest_ComponentIF gtest QCNodeComponentCommon )
install(TARGETS gtest_ComponentIF DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_ComponentIF.cpp
     PARENT_SCOPE )
