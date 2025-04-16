set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/source/libs/VideoCodec  )

set( VIDEOCODEC_HEADERS
    VidcDrvClient.hpp
    VidcCompBase.hpp
)

set( VIDEOCODEC_SOURCES
    VidcDrvClient.cpp
    VidcCompBase.cpp
)

set( TARGET_LIBRARIES QCNodeComponentCommon )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES vidc vidc_client mmosal )
endif()

add_library( QCNodeVideoCodec SHARED ${VIDEOCODEC_SOURCES} )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
target_include_directories( QCNodeVideoCodec PUBLIC ${CMAKE_SYSROOT}/usr/include/mm-osal/include )
target_compile_definitions( QCNodeVideoCodec PUBLIC _VIDC_LRH_LINUX_ )
endif()

set_property( TARGET QCNodeVideoCodec PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET QCNodeVideoCodec PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( QCNodeVideoCodec PUBLIC ${HEADERS_DIR} )

target_link_libraries( QCNodeVideoCodec PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS QCNodeVideoCodec DESTINATION lib)

install( FILES ${VIDEOCODEC_HEADERS} DESTINATION include/qcnode/libs/VideoCodec )
