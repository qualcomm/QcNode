
add_executable( gtest_VideoEncoder gtest_VideoEncoder.cpp )
target_link_libraries( 
     gtest_VideoEncoder
     gtest
     QCNodeVideoEncoder
)

install(TARGETS gtest_VideoEncoder DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_VideoEncoder.cpp
     PARENT_SCOPE )

