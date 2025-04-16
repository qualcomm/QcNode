
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( POST_CENTER_POINT_HEADERS
    ${HEADERS_DIR}/QC/component/PostCenterPoint.hpp
)

set( POST_CENTER_POINT_SOURCES PostCenterPoint.cpp )

list( APPEND TARGET_LIBRARIES QCNodeComponentCommon FadasIface )

add_library( QCNodePostCenterPoint OBJECT ${POST_CENTER_POINT_SOURCES} )

set_property( TARGET QCNodePostCenterPoint PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodePostCenterPoint PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodePostCenterPoint PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodePostCenterPoint PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${POST_CENTER_POINT_HEADERS} DESTINATION include/QC/component )
