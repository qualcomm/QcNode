
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( NODE_VIDEODECODER_HEADERS
    ${HEADERS_DIR}/QC/Node/VideoDecoder.hpp
)

set( NODE_VIDEODECODER_SOURCES
    VideoDecoder.cpp
)

set( TARGET_LIBRARIES QCNodeNodeBase QCNodeVideoCodec )

add_library( QCNodeNodeVideoDecoder OBJECT ${NODE_VIDEODECODER_SOURCES} )

set_property( TARGET QCNodeNodeVideoDecoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeVideoDecoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeVideoDecoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeVideoDecoder PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${NODE_VIDEODECODER_HEADERS} DESTINATION include/QC/Node )