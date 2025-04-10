
if( DEFINED ENV{QNN_SDK_ROOT} )
    set( QNN_SDK_ROOT $ENV{QNN_SDK_ROOT} )
endif()

if ( NOT DEFINED QNN_SDK_ROOT )
    message( WARNING "QNN_SDK_ROOT is not defined" )
    return()
endif()

set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )

set( QNN_HEADERS
    ${HEADERS_DIR}/ridehal/component/QnnRuntime.hpp
)

if (EXISTS "${QNN_SDK_ROOT}/examples/QNN/SampleApp/SampleApp")
    set( QNN_SAMPLEAPP_DIR ${QNN_SDK_ROOT}/examples/QNN/SampleApp/SampleApp )
else()
    set( QNN_SAMPLEAPP_DIR ${QNN_SDK_ROOT}/examples/QNN/SampleApp )
endif()

set( QNN_SOURCES
    ${QNN_SAMPLEAPP_DIR}/src/Log/LogUtils.cpp
    ${QNN_SAMPLEAPP_DIR}/src/Log/Logger.cpp
    ${QNN_SAMPLEAPP_DIR}/src/PAL/src/linux/Directory.cpp
    ${QNN_SAMPLEAPP_DIR}/src/PAL/src/linux/Path.cpp
    ${QNN_SAMPLEAPP_DIR}/src/PAL/src/linux/DynamicLoading.cpp
    ${QNN_SAMPLEAPP_DIR}/src/PAL/src/linux/FileOp.cpp
    ${QNN_SAMPLEAPP_DIR}/src/PAL/src/common/StringOp.cpp
    ${QNN_SAMPLEAPP_DIR}/src/PAL/src/common/GetOpt.cpp
    ${QNN_SAMPLEAPP_DIR}/src/Utils/DataUtil.cpp
    ${QNN_SAMPLEAPP_DIR}/src/Utils/QnnSampleAppUtils.cpp
    ${QNN_SAMPLEAPP_DIR}/src/Utils/DynamicLoadUtil.cpp
    ${QNN_SAMPLEAPP_DIR}/src/Utils/IOTensor.cpp
    ${QNN_SAMPLEAPP_DIR}/src/WrapperUtils/QnnWrapperUtils.cpp
    QnnRuntime.cpp
)

add_library( RideHalQnnRuntime OBJECT ${QNN_HEADERS} ${QNN_SOURCES} )
# setting LINKER_LANGUAGE is necessary if QNN_SOURCES is empty
set_property( TARGET RideHalQnnRuntime PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalQnnRuntime PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalQnnRuntime PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${QNN_SAMPLEAPP_DIR}/src
    ${QNN_SAMPLEAPP_DIR}/src/Log
    ${QNN_SAMPLEAPP_DIR}/src/PAL/include
    ${QNN_SAMPLEAPP_DIR}/src/Utils
    ${QNN_SAMPLEAPP_DIR}/src/WrapperUtils
    ${QNN_SDK_ROOT}/include/QNN
)

target_compile_definitions( RideHalQnnRuntime PUBLIC QNN_API= )
set( TARGET_LIBRARIES RideHalComponentCommon stdc++ cdsprpc fastrpc )

target_link_libraries( RideHalQnnRuntime PUBLIC ${TARGET_LIBRARIES} )

install( FILES ${QNN_HEADERS} DESTINATION include/ridehal/component )
