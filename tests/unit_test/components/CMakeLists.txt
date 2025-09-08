add_subdirectory(Common)
add_subdirectory(VideoEncoder)
if ( ENABLE_EVA )
  add_subdirectory(OpticalFlow)
  add_subdirectory(DepthFromStereo)
endif()

if( ENABLE_DEMUXER )
    add_subdirectory(VideoDecoder)
endif()

if ( ENABLE_FADAS )
  add_subdirectory(PointPillar)
endif()

if ( ENABLE_RADAR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
  add_subdirectory(Radar)
endif()

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES} PARENT_SCOPE )
set( GTEST_QCNODE_INCLUDES ${GTEST_QCNODE_INCLUDES} PARENT_SCOPE )
set( GTEST_QCNODE_LIBS ${GTEST_QCNODE_LIBS} PARENT_SCOPE )
