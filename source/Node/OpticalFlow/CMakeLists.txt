
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_OPTICAL_FLOW_HEADERS ${HEADERS_DIR}/QC/Node/OpticalFlow.hpp)
set( QC_NODE_OPTICAL_FLOW_SOURCES OpticalFlow.cpp)

set( TARGET_LIBRARIES QCNodeBase svplatform svcl devioClient)

add_library( QCNodeOpticalFlow OBJECT ${QC_NODE_OPTICAL_FLOW_SOURCES} )

set_property( TARGET QCNodeOpticalFlow PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeOpticalFlow PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeOpticalFlow PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeOpticalFlow PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_OPTICAL_FLOW_HEADERS} DESTINATION include/QC/Node )
