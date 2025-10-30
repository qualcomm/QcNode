if( NOT TARGET QCNodeQNN )
    return()
endif()

add_subdirectory(Mock)

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include
                 ${PROJECT_SOURCE_DIR}/tests/utils 
                 ${PROJECT_SOURCE_DIR}/source/Node/QNN
)

add_library( QnnSdkLibForTest STATIC
    ${PROJECT_SOURCE_DIR}/source/Node/QNN/QNN.cpp
    ${PROJECT_SOURCE_DIR}/source/Node/QNN/QnnConfig.cpp
    ${PROJECT_SOURCE_DIR}/source/Node/QNN/QnnMonitor.cpp
    ${PROJECT_SOURCE_DIR}/source/Node/QNN/QnnImpl.cpp
)

add_executable( gtest_NodeQnn gtest_NodeQnn.cpp 
        MockCLib.cpp
)

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

target_include_directories( QnnSdkLibForTest PUBLIC
    ${HEADERS_DIR}
    ${QNN_SAMPLEAPP_DIR}/src
    ${QNN_SAMPLEAPP_DIR}/src/WrapperUtils
    ${QNN_SDK_ROOT}/include/QNN
)
target_compile_definitions( QnnSdkLibForTest PRIVATE QCNODE_UNIT_TEST DISABLE_QC_LOG )
target_link_libraries( QnnSdkLibForTest QCNodeBase cdsprpc )

target_compile_definitions( gtest_NodeQnn PUBLIC QCNODE_UNIT_TEST )
target_link_libraries( gtest_NodeQnn gtest QnnSdkLibForTest QCNodeTestUtils BufferManager )
install(TARGETS gtest_NodeQnn DESTINATION bin)

