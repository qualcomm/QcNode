
add_subdirectory(C2D)
add_subdirectory(Camera)
add_subdirectory(common)
add_subdirectory(PointPillar)
add_subdirectory(VideoEncoder)
add_subdirectory(Remap)
add_subdirectory(QnnRuntime)
add_subdirectory(CL2DFlex)
add_subdirectory(OpticalFlow)
add_subdirectory(DepthFromStereo)

if( ENABLE_DEMUXER )
    add_subdirectory(VideoDecoder)
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    add_subdirectory(GL2DFlex)
endif()

set( GTEST_RIDEHAL_SOURCES ${GTEST_RIDEHAL_SOURCES} PARENT_SCOPE )
set( GTEST_RIDEHAL_INCLUDES ${GTEST_RIDEHAL_INCLUDES} PARENT_SCOPE )
set( GTEST_RIDEHAL_LIBS ${GTEST_RIDEHAL_LIBS} PARENT_SCOPE )

