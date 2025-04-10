
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( CAMERA_HEADERS
    ${HEADERS_DIR}/ridehal/component/Camera.hpp
)

set( CAMERA_SOURCES
    Camera.cpp
)

set( TARGET_LIBRARIES RideHalComponentCommon qcxclient qcxosal )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES qcxplatform cutils )
endif()

add_library( RideHalCamera OBJECT ${CAMERA_SOURCES} )

set_property( TARGET RideHalCamera PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalCamera PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalCamera PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalCamera PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${CAMERA_HEADERS} DESTINATION include/ridehal/component )
