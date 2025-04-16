
set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( CAMERA_HEADERS
    ${HEADERS_DIR}/QC/component/Camera.hpp
)

set( CAMERA_SOURCES
    Camera.cpp
)

set( TARGET_LIBRARIES QCNodeComponentCommon qcxclient qcxosal )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES qcxplatform cutils )
endif()

add_library( QCNodeCamera OBJECT ${CAMERA_SOURCES} )

set_property( TARGET QCNodeCamera PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeCamera PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeCamera PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeCamera PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${CAMERA_HEADERS} DESTINATION include/QC/component )
