if( DEFINED ENV{QNN_SDK_ROOT} )
    set( QNN_SDK_ROOT $ENV{QNN_SDK_ROOT} )
endif()

if ( NOT DEFINED QNN_SDK_ROOT )
    message( WARNING "QNN_SDK_ROOT is not defined" )
    return()
endif()

if (EXISTS "${QNN_SDK_ROOT}/examples/QNN/SampleApp/SampleApp")
    set( QNN_SAMPLEAPP_DIR ${QNN_SDK_ROOT}/examples/QNN/SampleApp/SampleApp )
else()
    set( QNN_SAMPLEAPP_DIR ${QNN_SDK_ROOT}/examples/QNN/SampleApp )
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QC_NODE_QNN_HEADERS
    ${HEADERS_DIR}/QC/Node/QNN.hpp
)

set( QC_NODE_QNN_SOURCES
    QNN.cpp
    QnnImpl.cpp
    QnnConfig.cpp
    QnnMonitor.cpp
)

set( TARGET_LIBRARIES QCNodeBase cdsprpc )

add_library( QCNodeQNN OBJECT ${QC_NODE_QNN_SOURCES} )

set_property( TARGET QCNodeQNN PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeQNN PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeQNN PUBLIC 
    ${HEADERS_DIR}
    ${QNN_SAMPLEAPP_DIR}/src
    ${QNN_SAMPLEAPP_DIR}/src/WrapperUtils
    ${QNN_SDK_ROOT}/include/QNN
)

if( ENABLE_GCOV OR ENABLE_CTC )
target_compile_definitions( QCNodeQNN PRIVATE DISABLE_QC_LOG )
endif()

target_link_libraries( QCNodeQNN PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QC_NODE_QNN_HEADERS} DESTINATION include/QC/Node )
