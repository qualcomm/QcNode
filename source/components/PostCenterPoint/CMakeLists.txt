
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( POST_CENTER_POINT_HEADERS
    ${HEADERS_DIR}/ridehal/component/PostCenterPoint.hpp
)

set( POST_CENTER_POINT_SOURCES PostCenterPoint.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon FadasIface )

add_library( RideHalPostCenterPoint OBJECT ${POST_CENTER_POINT_SOURCES} )

set_property( TARGET RideHalPostCenterPoint PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalPostCenterPoint PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalPostCenterPoint PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalPostCenterPoint PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${POST_CENTER_POINT_HEADERS} DESTINATION include/ridehal/component )
