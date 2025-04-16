
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( OFL_HEADERS
    ${HEADERS_DIR}/QC/component/OpticalFlow.hpp
)

set( OFL_SOURCES OpticalFlow.cpp )

list( APPEND TARGET_LIBRARIES QCNodeComponentCommon evaEpl evaPlatform devioClient )

add_library( QCNodeOpticalFlow OBJECT ${OFL_SOURCES} )

set_property( TARGET QCNodeOpticalFlow PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeOpticalFlow PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeOpticalFlow PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeOpticalFlow PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${OFL_HEADERS} DESTINATION include/QC/component )
