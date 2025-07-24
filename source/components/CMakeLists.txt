
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_COMMON_HEADERS
    ${HEADERS_DIR}/QC/component/ComponentIF.hpp
)

set( COMPONENT_COMMON_SOURCES
    ComponentIF.cpp
)

set( TARGET_LIBRARIES QCNodeCommon )

add_library( QCNodeComponentCommon STATIC ${COMPONENT_COMMON_SOURCES} )

set_property( TARGET QCNodeComponentCommon PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeComponentCommon PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeComponentCommon PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeComponentCommon PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_COMMON_HEADERS} DESTINATION include/QC/component )
if ( ENABLE_C2D )
  add_subdirectory(C2D)
endif()
add_subdirectory(Camera)
if ( ENABLE_FADAS )
add_subdirectory(PostCenterPoint)
add_subdirectory(Remap)
add_subdirectory(Voxelization)
endif()
add_subdirectory(VideoEncoder)
add_subdirectory(VideoDecoder)
if ( ENABLE_EVA )
  add_subdirectory(OpticalFlow)
  add_subdirectory(DepthFromStereo)
endif()
add_subdirectory(GL2DFlex)


