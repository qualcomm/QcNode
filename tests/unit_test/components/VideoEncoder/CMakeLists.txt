
add_executable( gtest_VideoEncoder gtest_VideoEncoder.cpp )
target_link_libraries( 
     gtest_VideoEncoder
     gtest
     RideHalVideoEncoder
)

install(TARGETS gtest_VideoEncoder DESTINATION bin)

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES}
     ${CMAKE_CURRENT_SOURCE_DIR}/gtest_VideoEncoder.cpp
     PARENT_SCOPE )

