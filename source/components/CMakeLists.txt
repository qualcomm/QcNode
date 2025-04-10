
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_COMMON_HEADERS
    ${HEADERS_DIR}/ridehal/component/ComponentIF.hpp
)

set( COMPONENT_COMMON_SOURCES
    ComponentIF.cpp
)

set( TARGET_LIBRARIES RideHalCommon )

add_library( RideHalComponentCommon STATIC ${COMPONENT_COMMON_SOURCES} )

set_property( TARGET RideHalComponentCommon PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalComponentCommon PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalComponentCommon PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalComponentCommon PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_COMMON_HEADERS} DESTINATION include/ridehal/component )

add_subdirectory(C2D)
add_subdirectory(Camera)
add_subdirectory(PostCenterPoint)
add_subdirectory(QnnRuntime)
add_subdirectory(CL2DFlex)
add_subdirectory(Remap)
add_subdirectory(VideoEncoder)
add_subdirectory(VideoDecoder)
add_subdirectory(Voxelization)
add_subdirectory(OpticalFlow)
add_subdirectory(DepthFromStereo)

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
	add_subdirectory(GL2DFlex)
endif()

