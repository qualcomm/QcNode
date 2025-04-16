
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( COMPONENT_VIDEOENCODER_HEADERS
    ${HEADERS_DIR}/QC/component/VideoEncoder.hpp
)

set( COMPONENT_VIDEOENCODER_SOURCES
    VideoEncoder.cpp
)

set( TARGET_LIBRARIES QCNodeVideoCodec )

add_library( QCNodeVideoEncoder OBJECT ${COMPONENT_VIDEOENCODER_SOURCES} )

set_property( TARGET QCNodeVideoEncoder PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeVideoEncoder PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeVideoEncoder PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeVideoEncoder PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${COMPONENT_VIDEOENCODER_HEADERS} DESTINATION include/QC/component )

