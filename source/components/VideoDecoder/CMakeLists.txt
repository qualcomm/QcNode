set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_VIDEODECODER_HEADERS
    ${HEADERS_DIR}/ridehal/component/VideoDecoder.hpp
)

set( COMPONENT_VIDEODECODER_SOURCES
    VideoDecoder.cpp
)

set( TARGET_LIBRARIES RideHalVideoCodec )

add_library( RideHalVideoDecoder OBJECT ${COMPONENT_VIDEODECODER_SOURCES} )

set_property( TARGET RideHalVideoDecoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalVideoDecoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalVideoDecoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalVideoDecoder PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_VIDEODECODER_HEADERS} DESTINATION include/ridehal/component )

