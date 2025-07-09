if( NOT TARGET QCNodeQnnRuntime )
    message( STATUS "build without QNN" )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_QNN_HEADERS
    ${HEADERS_DIR}/QC/Node/QNN.hpp
)

set( QC_NODE_QNN_SOURCES
    QNN.cpp
)

set( TARGET_LIBRARIES QCNodeQnnRuntime QCNodeNodeBase )

add_library( QCNodeNodeQNN OBJECT ${QC_NODE_QNN_SOURCES} )

set_property( TARGET QCNodeNodeQNN PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeNodeQNN PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeNodeQNN PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeNodeQNN PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_QNN_HEADERS} DESTINATION include/QC/Node )
