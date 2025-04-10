set( HEADERS_DIR ${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/source/libs/VideoCodec  )

set( VIDEOCODEC_HEADERS
    VidcDrvClient.hpp
    VidcCompBase.hpp
)

set( VIDEOCODEC_SOURCES
    VidcDrvClient.cpp
    VidcCompBase.cpp
)

set( TARGET_LIBRARIES RideHalComponentCommon )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
    list( APPEND TARGET_LIBRARIES vidc vidc_client mmosal )
endif()

add_library( RideHalVideoCodec SHARED ${VIDEOCODEC_SOURCES} )

if( "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux" )
target_include_directories( RideHalVideoCodec PUBLIC ${CMAKE_SYSROOT}/usr/include/mm-osal/include )
target_compile_definitions( RideHalVideoCodec PUBLIC _VIDC_LRH_LINUX_ )
endif()

set_property( TARGET RideHalVideoCodec PROPERTY LINKER_LANGUAGE CXX )
set_property( TARGET RideHalVideoCodec PROPERTY POSITION_INDEPENDENT_CODE ON )

target_include_directories( RideHalVideoCodec PUBLIC ${HEADERS_DIR} )

target_link_libraries( RideHalVideoCodec PUBLIC ${TARGET_LIBRARIES} )

install(TARGETS RideHalVideoCodec DESTINATION lib)

install( FILES ${VIDEOCODEC_HEADERS} DESTINATION include/ridehal/libs/VideoCodec )
