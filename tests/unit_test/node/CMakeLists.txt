
add_subdirectory(NodeBase)
add_subdirectory(QNN)
add_subdirectory(CL2DFlex)
add_subdirectory(Camera)
add_subdirectory(VideoEncoder)
add_subdirectory(PointPillar)
if ( ENABLE_FADAS )
add_subdirectory(Remap)
endif()

set( GTEST_QCNODE_SOURCES ${GTEST_QCNODE_SOURCES} PARENT_SCOPE )
set( GTEST_QCNODE_INCLUDES ${GTEST_QCNODE_INCLUDES} PARENT_SCOPE )
set( GTEST_QCNODE_LIBS ${GTEST_QCNODE_LIBS} PARENT_SCOPE )
