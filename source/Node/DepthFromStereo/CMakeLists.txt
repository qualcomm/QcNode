
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_DEPTH_FROM_STEREO_HEADERS ${HEADERS_DIR}/QC/Node/DepthFromStereo.hpp)
set( QC_NODE_DEPTH_FROM_STEREO_SOURCES DepthFromStereo.cpp)

set( TARGET_LIBRARIES QCNodeDepthFromStereo QCNodeBase )

add_library( QCNodeNodeDepthFromStereo OBJECT ${QC_NODE_DEPTH_FROM_STEREO_SOURCES} )

set_property( TARGET QCNodeNodeDepthFromStereo PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeDepthFromStereo PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeDepthFromStereo PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeDepthFromStereo PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_DEPTH_FROM_STEREO_HEADERS} DESTINATION include/QC/Node )
