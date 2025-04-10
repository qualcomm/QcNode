
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( OFL_HEADERS
    ${HEADERS_DIR}/ridehal/component/OpticalFlow.hpp
)

set( OFL_SOURCES OpticalFlow.cpp )

list( APPEND TARGET_LIBRARIES RideHalComponentCommon evaEpl evaPlatform devioClient )

add_library( RideHalOpticalFlow OBJECT ${OFL_SOURCES} )

set_property( TARGET RideHalOpticalFlow PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalOpticalFlow PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalOpticalFlow PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalOpticalFlow PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${OFL_HEADERS} DESTINATION include/ridehal/component )
