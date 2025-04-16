
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils )

add_executable( gtest_VideoDecoder gtest_VideoDecoder.cpp )
target_link_libraries( 
     gtest_VideoDecoder
     gtest
     QCNodeVideoDecoder
     VidcDemuxer
     QCNodeTestUtils
)

install(TARGETS gtest_VideoDecoder DESTINATION bin)

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_VideoDecoder.cpp
     PARENT_SCOPE )


