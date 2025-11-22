set( HEADERS_DIR 
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_PREFIX_PATH}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include )

set ( RESMON_SRC ResMonitor.cpp )

add_library( ResMonitor STATIC ${RESMON_SRC} )
target_include_directories( ResMonitor PUBLIC  ${HEADERS_DIR} )

set( TARGET_LIBRARIES QCNode )

set_property( TARGET ResMonitor PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET ResMonitor PROPERTY POSITION_INDEPENDENT_CODE ON )

target_link_libraries( ResMonitor PUBLIC compresmon )

