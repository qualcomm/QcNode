
add_subdirectory(common)
add_subdirectory(components)
add_subdirectory(libs)

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

file( WRITE ${CMAKE_BINARY_DIR}/null.cpp "" )

set( RIDEHAL_SOURCES ${CMAKE_BINARY_DIR}/null.cpp )

set( TARGET_LIBRARIES
    RideHalCommon
    RideHalC2D
    RideHalCamera
    RideHalRemap
    RideHalVideoEncoder
    RideHalVideoDecoder
    RideHalVoxelization
    RideHalPostCenterPoint
    RideHalCL2DFlex
    RideHalOpticalFlow
    RideHalDepthFromStereo
)

if( NOT TARGET RideHalQnnRuntime )
  message(STATUS "build RideHal without QNN")
else()
  list( APPEND TARGET_LIBRARIES RideHalQnnRuntime )
endif()

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND TARGET_LIBRARIES RideHalGL2DFlex )
endif()

add_library( RideHal SHARED ${RIDEHAL_SOURCES} )

set_property( TARGET RideHal PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHal PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHal PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHal PUBLIC ${TARGET_LIBRARIES} )

install( TARGETS RideHal DESTINATION lib )
