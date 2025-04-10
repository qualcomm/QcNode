
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/tests/utils )

add_executable( gtest_VideoDecoder gtest_VideoDecoder.cpp )
target_link_libraries( 
     gtest_VideoDecoder
     gtest
     RideHalVideoDecoder
     VidcDemuxer
     RideHalTestUtils
)

install(TARGETS gtest_VideoDecoder DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_VideoDecoder.cpp
     PARENT_SCOPE )


