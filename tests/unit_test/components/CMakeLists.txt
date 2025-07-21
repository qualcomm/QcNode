if ( ENABLE_C2D )
  add_subdirectory(C2D)
endif()
add_subdirectory(Camera)
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
  add_subdirectory(Remap)
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    add_subdirectory(GL2DFlex)
endif()

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES} PARENT_SCOPE )
set( GTEST_QCNODE_INCLUDES ${GTEST_QCNODE_INCLUDES} PARENT_SCOPE )
set( GTEST_QCNODE_LIBS ${GTEST_QCNODE_LIBS} PARENT_SCOPE )

