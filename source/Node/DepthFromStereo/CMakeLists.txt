set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_DEPTH_FROM_STEREO_HEADERS ${HEADERS_DIR}/QC/Node/DepthFromStereo.hpp)
set( QC_NODE_DEPTH_FROM_STEREO_SOURCES DepthFromStereo.cpp)

set( TARGET_LIBRARIES QCNodeBase svplatform svcl devioClient)

add_library( QCNodeDepthFromStereo OBJECT ${QC_NODE_DEPTH_FROM_STEREO_SOURCES} )

set_property( TARGET QCNodeDepthFromStereo PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeDepthFromStereo PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeDepthFromStereo PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeDepthFromStereo PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_DEPTH_FROM_STEREO_HEADERS} DESTINATION include/QC/Node )
