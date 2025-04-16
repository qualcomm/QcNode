
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( DFS_HEADERS
    ${HEADERS_DIR}/QC/component/DepthFromStereo.hpp
)

set( DFS_SOURCES DepthFromStereo.cpp )

list( APPEND TARGET_LIBRARIES QCNodeComponentCommon evaEpl evaPlatform devioClient )

add_library( QCNodeDepthFromStereo OBJECT ${DFS_SOURCES} )

set_property( TARGET QCNodeDepthFromStereo PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeDepthFromStereo PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeDepthFromStereo PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeDepthFromStereo PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${DFS_HEADERS} DESTINATION include/QC/component )
