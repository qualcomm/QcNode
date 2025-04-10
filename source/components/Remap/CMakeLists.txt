
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( REMAP_HEADERS
    ${HEADERS_DIR}/ridehal/component/Remap.hpp
)

set( REMAP_SOURCES Remap.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon FadasIface FadasIfaceStub)
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/FadasIface )

add_library( RideHalRemap OBJECT ${REMAP_SOURCES} )

set_property( TARGET RideHalRemap PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalRemap PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalRemap PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalRemap PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${REMAP_HEADERS} DESTINATION include/ridehal/component )
