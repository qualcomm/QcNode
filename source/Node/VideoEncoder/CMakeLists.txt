
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( NODE_VIDEOENCODER_HEADERS
    ${HEADERS_DIR}/QC/Node/VideoEncoder.hpp
)

set( NODE_VIDEOENCODER_SOURCES
    VideoEncoder.cpp
)

set( TARGET_LIBRARIES QCNodeNodeBase QCNodeVideoCodec )

add_library( QCNodeNodeVideoEncoder OBJECT ${NODE_VIDEOENCODER_SOURCES} )

set_property( TARGET QCNodeNodeVideoEncoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeVideoEncoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeVideoEncoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeVideoEncoder PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${NODE_VIDEOENCODER_HEADERS} DESTINATION include/QC/Node )