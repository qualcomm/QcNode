
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_VIDEOENCODER_HEADERS
    ${HEADERS_DIR}/ridehal/component/VideoEncoder.hpp
)

set( COMPONENT_VIDEOENCODER_SOURCES
    VideoEncoder.cpp
)

set( TARGET_LIBRARIES RideHalVideoCodec )

add_library( RideHalVideoEncoder OBJECT ${COMPONENT_VIDEOENCODER_SOURCES} )

set_property( TARGET RideHalVideoEncoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalVideoEncoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalVideoEncoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalVideoEncoder PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_VIDEOENCODER_HEADERS} DESTINATION include/ridehal/component )

