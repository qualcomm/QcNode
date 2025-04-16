

list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/include )
list( APPEND HEADERS_DIR ${PROJECT_SOURCE_DIR}/source/libs/OpenclIface )

set( OPENCLIFACE_HEADERS
    OpenclIface.hpp
)

set( TARGET_LIBRARIES OpenCL )


add_library(OpenclIface SHARED OpenclIface.cpp )
if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
target_compile_definitions(OpenclIface PUBLIC CL_TARGET_OPENCL_VERSION=200)
endif()
target_include_directories(OpenclIface PUBLIC ${HEADERS_DIR})
target_link_libraries(OpenclIface PUBLIC ${TARGET_LIBRARIES})
add_library(Ride::Hal::Libs::OpenclIface ALIAS OpenclIface)
install(TARGETS OpenclIface DESTINATION lib)

install( FILES ${OPENCLIFACE_HEADERS} DESTINATION include/QC/libs/OpenclIface )
