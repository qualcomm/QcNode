
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_C2D_HEADERS
    ${HEADERS_DIR}/ridehal/component/C2D.hpp
)

set( TARGET_LIBRARIES RideHalComponentCommon )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" AND "${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64" )
    list( APPEND TARGET_LIBRARIES C2D2 )
    list( APPEND HEADERS_DIR ${CMAKE_SYSROOT}/usr/include/WF )
endif()

set( COMPONENT_C2D_SOURCES
    C2D.cpp
)

add_library( RideHalC2D OBJECT ${COMPONENT_C2D_SOURCES} )

set_property( TARGET RideHalC2D PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalC2D PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalC2D PUBLIC ${HEADERS_DIR} )
target_include_directories( RideHalC2D PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

target_link_libraries( RideHalC2D PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_C2D_HEADERS} DESTINATION include/ridehal/component )
