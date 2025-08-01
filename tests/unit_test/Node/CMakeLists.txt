
add_subdirectory(NodeBase)
add_subdirectory(QNN)
add_subdirectory(CL2DFlex)
add_subdirectory(Camera)
add_subdirectory(VideoEncoder)
if ( ENABLE_FADAS )
add_subdirectory(Remap)
add_subdirectory(PointPillar)
endif()

if ( ENABLE_RADAR AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" )
add_subdirectory(Radar)
endif()
