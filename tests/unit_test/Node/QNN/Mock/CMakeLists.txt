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


add_library( QnnInterfaceMock SHARED QnnInterfaceMock.cpp )
set_property( TARGET QnnInterfaceMock PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QnnInterfaceMock PROPERTY POSITION_INDEPENDENT_CODE ON )
target_include_directories( QnnInterfaceMock PUBLIC
    ${HEADERS_DIR}
    ${QNN_SAMPLEAPP_DIR}/src
    ${QNN_SAMPLEAPP_DIR}/src/WrapperUtils
    ${QNN_SDK_ROOT}/include/QNN
)
set_target_properties(QnnInterfaceMock PROPERTIES NO_SONAME TRUE)
install(TARGETS QnnInterfaceMock DESTINATION lib)

add_library( QnnSystemInterfaceMock SHARED QnnSystemInterfaceMock.cpp )
set_property( TARGET QnnSystemInterfaceMock PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QnnSystemInterfaceMock PROPERTY POSITION_INDEPENDENT_CODE ON )
target_include_directories( QnnSystemInterfaceMock PUBLIC
    ${HEADERS_DIR}
    ${QNN_SAMPLEAPP_DIR}/src
    ${QNN_SAMPLEAPP_DIR}/src/WrapperUtils
    ${QNN_SDK_ROOT}/include/QNN
)
set_target_properties(QnnSystemInterfaceMock PROPERTIES NO_SONAME TRUE)
install(TARGETS QnnSystemInterfaceMock DESTINATION lib)
